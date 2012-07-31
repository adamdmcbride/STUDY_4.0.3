#include <stdio.h>
#include <private/android_filesystem_config.h>
#include <linux/prctl.h>
#include <fcntl.h>
#include <utils/Log.h>
#include <dlfcn.h>
#include "MLControler.h"

/*
 * switchUser - Switches UID to system, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
   prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
   setuid(AID_SYSTEM);

   struct __user_cap_header_struct header;
   struct __user_cap_data_struct cap;
   header.version = _LINUX_CAPABILITY_VERSION;
   header.pid = 0;
   cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
   cap.inheritable = 0;
   capset(&header, &cap);
}

int main(int argc, char *argv[])
{
   int ret = -1;

#if USE_MOUNT_IN_PROGRAM
   ret = initSDCard();
   if(ret < 0)
   {
      LOGE("initSDCard failure!!");
      return -1;
   }
#endif

   waitTtyDev();

   switchUser();

   if(0 == do_open())
   {
      LOGD("Modem Log open successful when boot!!");
   }
   else
   {
      LOGE("Fail to open Modem Log when boot!!");
   }

   return 0;
}
