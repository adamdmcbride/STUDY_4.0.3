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
#include <dirent.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#define LOG_TAG "MLC"
#define LOG_CONTROLER_PID "service.mlc.pid"
#define EMMC_SD_CARD_DEV "/dev/block/mmcblk1p1"
#define EMMC_CARD_MOUNT_POINT "/mnt/sdcard"
#define EMMC_CARD_LOG_DIR "mnt/sdcard/log"
#define MODEM_TTY_PATH "/dev/ttyACM2"
#define TELINK_PROGRAM_PATH "/system/vendor/bin/telinklog"
#define TELINK_PROGRAM "telinklog"
#define ARRAY_LENGTH 256
#define DBG 1

static int writeP;
static int readP;

int strStartsWith(const char *line, const char *prefix)
{
   for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
      if (*line != *prefix) {
         return 0;
      }
   }

   return *prefix == '\0';
}

int initFileLogName(char *log_file_path)
{
   int log_file_dir = 0;
   int ret = -1;
   struct stat file_stat;

   if(NULL == log_file_path)
      return -1;

   if(stat(EMMC_CARD_LOG_DIR, &file_stat) == 0) 
   {
      if(S_ISDIR(file_stat.st_mode))  
      {
         if(DBG)
            LOGD("initFileLogName: /mnt/sdcard/log has existence!!");

         log_file_dir = 1;
      }
      else
      {
         if(unlink(EMMC_CARD_LOG_DIR) == 0)
         {
            umask(0);
            if(mkdir(EMMC_CARD_LOG_DIR, 0777) == 0)
            {
               log_file_dir = 1;
            }
         }
      }
   }
   else if(ENOENT == errno || ENOTDIR == errno)
   {
      if(mkdir(EMMC_CARD_LOG_DIR, 0777) == 0)
      {
         if(DBG)
            LOGD("initFileLogName: mkdir /mnt/sdcard/log successful!!");
         log_file_dir = 1;
      }
      else
      {
         LOGE("initFileLogName: mkdir /mnt/sdcard/log failure!!");
      }
   }

   if(log_file_dir == 1)
   {
      FILE *fd;
      int num = 0, max = 0;
      DIR  *dp;
      struct dirent *dirp;

      if((dp = opendir(EMMC_CARD_LOG_DIR)) == NULL)
      {
         LOGE("open dir /mnt/sdcard/log failure!!!use modem_log0 as log name.");
         sprintf( log_file_path, "%s/modem_log0", EMMC_CARD_LOG_DIR);
      }
      else
      {
         while((dirp = readdir(dp)) != NULL)
         {
            if((strcmp(dirp->d_name, ".") == 0) || 
                  (strcmp(dirp->d_name, "..") == 0) ||
               (!strStartsWith(dirp->d_name, "modem_log")))
            {
               continue;
            }

            if(DBG)
               LOGD("print the modem log file name. %s", dirp->d_name);

            sscanf(dirp->d_name, "modem_log%d", &num);
            if(num > max)
            {
               max = num;
            }
         }
         
         if(closedir(dp) < 0)
         {
            LOGE("close /mnt/sdcard/log failure in func initFileLogName!!!");
         }
         max++;

         sprintf( log_file_path, "%s/modem_log%d", EMMC_CARD_LOG_DIR, max);

      }

      LOGD("********* FILE_LOG_PATH =%s\n", log_file_path);
      return 0;
   }
   else
   {
      return -1;
   }

   return 0;
}

int initSDCard()
{
   FILE *fd;
   int num = 0, max = 0;
   int has_mounted = 0;
   int ret = -1;
   int retry_tm = 10;
   char tmp[ARRAY_LENGTH], src_m[ARRAY_LENGTH], des_m[ARRAY_LENGTH];

#if USE_MOUNT_IN_PROGRAM
   fd = popen("mount", "r");
   if(NULL != fd)
   {
      while(fgets(tmp, sizeof(tmp), fd))
      {
         if(DBG)
            LOGD("mount result: %s\n", tmp);
         memset(des_m, 0, sizeof(des_m));
         sscanf(tmp, "%s %s ", src_m, des_m);
         if(0 == strcmp(des_m, EMMC_CARD_MOUNT_POINT))
         {
            has_mounted = 1;
            break;
         }
         memset(tmp, 0, sizeof(tmp));
      }

      pclose(fd);

      if(1 != has_mounted)
      {
         //    if((ret = mount( EMMC_SD_CARD_DEV, EMMC_CARD_MOUNT_POINT, "vfat", MS_MGC_VAL, "rw,dirsync,nosuid,nodev,noexec,relatime,uid=1000,gid=1015,fmask=0002,dmask=0002,allow_utime=0020,codepage=cp437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro")) == -1)

         if((ret = mount( EMMC_SD_CARD_DEV, EMMC_CARD_MOUNT_POINT, "vfat", MS_MGC_VAL, "")) == -1)
         {
            if(EBUSY != errno)
            {
               LOGE("mount EMMC SD error!!");
               return -1;
            }
         }
      }
   }
   else
   {
      LOGE("popen mount error!!");

      return -1;
   }

#else
   while((retry_tm-- > 0) && ((fd = popen("mount", "r")) != NULL))
   {
      while(fgets(tmp, sizeof(tmp), fd))
      {
         if(DBG)
            LOGD("mount result: %s\n", tmp);
         memset(des_m, 0, sizeof(des_m));
         sscanf(tmp, "%s %s ", src_m, des_m);
         if(0 == strcmp(des_m, EMMC_CARD_MOUNT_POINT))
         {
            has_mounted = 1;
            break;
         }
         memset(tmp, 0, sizeof(tmp));
      }

      pclose(fd);

      if(!has_mounted)
      {
         LOGD("The SD_card is not mounted, wait 1 second.");
         sleep(1);
      }
      else
      {
         break;
      }
   }

   if(!has_mounted)
   {
      LOGE("no SD card error!!");

      return -1;
   }
#endif

   if(DBG)
      LOGD("init EMMC SD card successful!!");

   return 0;
}

int waitTtyDev()
{
   struct stat file_stat;
   int retry_tm = 6;

   while(stat(MODEM_TTY_PATH, &file_stat) != 0)
   {
      if(retry_tm-- <= 0)
      {
         return -1;
      }
      LOGE("/dev/ttyACM2 is not existence. wait 1 second.");
      sleep(1);
   }

   return 0;
}

void write_val2father(int value)
{
   int ret = -1;

   do
   {
      ret = write(writeP, &value, sizeof(int));
   }while((ret < 0) && (errno == EINTR));
}

void do_real_work()
{
   int ret = -1;
   char path_buf[ARRAY_LENGTH];

   ret = initSDCard();
   if(ret < 0)
      goto error;

   ret = waitTtyDev();
   if(ret < 0)
   {
      goto error;
   }

   memset(path_buf, 0, sizeof(path_buf));
   ret = initFileLogName(path_buf);
   if(ret < 0)
   {
      goto error;
   }

   if(DBG)
      LOGD("modem log path %s", path_buf);

   write_val2father(0);
   ret = execl(TELINK_PROGRAM_PATH, TELINK_PROGRAM, MODEM_TTY_PATH, path_buf, "212", (char *)0);
   if(-1 == ret)
      LOGE("execl function execute error!!!");

   exit(-1);

error:
   write_val2father(-1);
   exit(0);
}

int do_open()
{
   int retry_time = 10;
   int pipe_arr[2], ret = -1;
   pid_t pid;
   struct sigaction newact,oldact;
   newact.sa_handler = SIG_IGN;
   sigemptyset(&newact.sa_mask);
   newact.sa_flags = 0;
   sigaction(SIGCHLD,&newact,&oldact);

   ret = pipe(pipe_arr);
   if(ret < 0)
   {
      LOGE("pipe error!!!");
      return -1;
   }

   readP = pipe_arr[0];
   writeP = pipe_arr[1];

   do
   {
      pid = fork();
      if(pid < 0)
      {
         LOGE("Setting jni fork error.");
         continue;
      }
      if(pid > 0)
      {
         char pid_numbuf[16];
         int tmp;

         close(writeP);
         writeP = -1;
         while(read(readP, &tmp, sizeof(int)) < 0)
         {
            if(errno == EINTR)
            {
               continue;
            }
            else
            {
               goto error;
            }
         }

         if(tmp == -1)
         {
            goto error;
         }

         memset(pid_numbuf, 0, sizeof(pid_numbuf));
         snprintf(pid_numbuf, sizeof(pid_numbuf), "%d", pid);
         pid_numbuf[15] = '\0';
         ret = property_set(LOG_CONTROLER_PID, pid_numbuf);
         if(ret < 0)
         {
            LOGE("set LOG_CONTROLER_PID failure!");
            goto error;
         }

         return 0;
      }
      if(pid == 0)
      {
         close(readP);
         readP = -1;
         do_real_work();
      }
   }while(retry_time-- > 0);

error:
   if(pid > 0)
   {
      ret = kill(pid, SIGKILL);
      if(ret < 0)
      {
            LOGE("1. kill process of the MLC failed !!");
      }
      else
      {
            LOGD("1. kill process of the MLC successful !!!");
      }
   }

   return -1;
}

int do_close()
{
   int pid = -1;
   int ret = -1;
   char pid_numbuf[16];
   char *stopstr;

   memset(pid_numbuf, 0, sizeof(pid_numbuf));
   ret = property_get(LOG_CONTROLER_PID, pid_numbuf, "null");
   if(ret < 0)  
   {
      LOGE("execute get_property LOG_CONTROLER_PID failure!");
      goto error;
   }
  
   ret = property_set(LOG_CONTROLER_PID, "null");
   if(ret < 0)
   {
      LOGE("set property LOG_CONTROLER_PID to null failure!");
   }

   if(0 == strcmp(pid_numbuf, "null"))
   {
      LOGE("get LOG_CONTROLER_PID = %s is error!!", pid_numbuf);
      goto error;
   }

   pid = strtol(pid_numbuf, &stopstr, 10);
   if(stopstr == pid_numbuf)
   {
      LOGE("the format of pid error, strtol error!!");
      goto error;
   }

   if(pid > 0)
   {
      ret = kill(pid, SIGKILL);
      if(ret < 0)
      {
            LOGE("kill process of the MLC failed !!");
            goto error;
      }
      else
      {
            LOGD("kill process of the MLC successful !!!");
      }
   }

   return 0;

error:

   ret = property_set(LOG_CONTROLER_PID, "null");
   if(ret < 0)
   {
      LOGE("set LOG_CONTROLER_PID to null failure!");
   }

   return -1;
}

int do_clearLogs()
{
   int ret = -1;
   DIR  *dp;
   struct dirent *dirp;
   char buf[ARRAY_LENGTH];
   struct stat file_stat;

   if(stat(EMMC_CARD_LOG_DIR, &file_stat) != 0)
   {
      if(ENOENT == errno || ENOTDIR == errno)
      {
         LOGD("the file has be deleted.");
         return 0;
      }
   }

   if((dp = opendir(EMMC_CARD_LOG_DIR)) == NULL)
   {
      LOGE("open dir /mnt/sdcard/log failure!!!");
      return -1;
   }

   memset(buf, 0, sizeof(buf));
   while((dirp = readdir(dp)) != NULL)
   {
      if(strcmp(dirp->d_name, ".") == 0 || 
         strcmp(dirp->d_name, "..") == 0)
      {
         continue;
      }

      if(DBG)
         LOGD("print the file in log dir. %s", dirp->d_name);

      sprintf(buf, "%s/%s", EMMC_CARD_LOG_DIR, dirp->d_name);

      ret = remove(buf);
      if(ret < 0)
      {
         LOGE("delete %s failure!!", buf);
      }
   }

   if(closedir(dp) < 0)
   {
      LOGE("close /mnt/sdcard/log failure!!!");
      return -1;
   }

   ret = remove(EMMC_CARD_LOG_DIR);
   if(ret < 0)
   {
      LOGE("delete dir /mnt/sdcard/log failure!!!");
      return -1;
   }
   
   return 0;
}


