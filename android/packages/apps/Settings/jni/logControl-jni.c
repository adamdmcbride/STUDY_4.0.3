#include "MLControler.h"
#include <jni.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#define TRUE 1
#define FALSE 0
#define BOOT_NO_LOG "/system/vendor/bin/run_no_modem_log.sh"
#define BOOT_HAVE_LOG "/system/vendor/bin/run_modem_log.sh"
#define REAL_BOOT_LOG "/data/log.sh"

static int open_boot_log(int flag)
{
   int ret = -1;

   struct stat file_stat;

   ret = unlink(REAL_BOOT_LOG);
   if(ret < 0)
   {
      LOGE("delete linker failure!!");
   }

   if(TRUE == flag)
   {
      ret = symlink(BOOT_HAVE_LOG, REAL_BOOT_LOG);
   }
   else if(FALSE == flag)
   {
      ret = symlink(BOOT_NO_LOG, REAL_BOOT_LOG);
   }
   if(ret < 0)
   {
      LOGE("found linker failure!!");
      return -1;
   }

   return 0;
}

jboolean Java_com_android_settings_LogSetting_openModemLog(JNIEnv* env, jobject thiz)
{
   int ret = -1;

   open_boot_log(TRUE);

   ret = do_open();
   if(ret < 0)
   {
      open_boot_log(FALSE);
      return (jboolean)FALSE;
   }

   return (jboolean)TRUE;
}

jboolean Java_com_android_settings_LogSetting_closeModemLog(JNIEnv* env, jobject thiz)
{
   int ret = -1;

   open_boot_log(FALSE);

   ret = do_close();
   if(ret < 0)
   {
      open_boot_log(TRUE);
      return (jboolean)FALSE;
   }

   return (jboolean)TRUE;
}

jboolean Java_com_android_settings_LogSetting_clearLogs(JNIEnv* env, jobject thiz)
{
   int ret = -1;

   //clear boot log setting
//   open_boot_log(FALSE);

   //close current telinklog program
//   do_close();

   ret = do_clearLogs();
   if(ret < 0)
   {
      LOGE("delete log failure!!!");
      return (jboolean)FALSE;
   }

   return (jboolean)TRUE;
}



