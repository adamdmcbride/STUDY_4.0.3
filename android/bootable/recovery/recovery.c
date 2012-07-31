/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/reboot.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "cutils/android_reboot.h"
#include "install.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"

#include "nufront/ns2816_config.h"

#ifdef NS115
#define POWER_CHECK    ///aahadd for check battery before install something 20110419
#define OTA 		///aah add for ota update,20110419, its OK now
#endif

#ifdef POWER_CHECK
///aah add for save the power limited of the motion in recovery
typedef struct pw_kind{
	int ota;
	int update;
}pw_kind_t;
static pw_kind_t pw_para;
static int pw_fd=-1;///for get real battery info
static int ac_fd=-1;///for ac check
#endif


enum card_type
{
	SDCARD=0,
	TFCARD
};


static const struct option OPTIONS[] = {
  { "send_intent", required_argument, NULL, 's' },
  { "update_package", required_argument, NULL, 'u' },
  { "wipe_data", no_argument, NULL, 'w' },
  { "wipe_cache", no_argument, NULL, 'c' },
  { "show_text", no_argument, NULL, 't' },
  { NULL, 0, NULL, 0 },
};

static const char *COMMAND_FILE = "/cache/recovery/command";
static const char *INTENT_FILE = "/cache/recovery/intent";
static const char *LOG_FILE = "/cache/recovery/log";
static const char *LAST_LOG_FILE = "/cache/recovery/last_log";
static const char *LAST_INSTALL_FILE = "/cache/recovery/last_install";
static const char *CACHE_ROOT = "/cache";
static const char *SDCARD_ROOT = "/sdcard";
static const char *TFCARD_ROOT = "/tfcard";
static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";
static const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
///aah add 2012-06-04 for ota to avoid 'space not enough'
//static const char *SIDELOAD_TEMP_DIR = "/tmp/sideload";
//static const char *SIDELOAD_TEMP_DIR1 = "/tfcard/sideload";
//static const char *SIDELOAD_TEMP_DIR2 = "/sdcard/sideload";
static char *SIDELOAD_TEMP_DIR;

///aah add 2012-04-11 for ota
#ifdef OTA
static const char *OTA_ZIP_DIR = "/sdcard/Download/update.zip";
#endif

#ifdef POWER_CHECK
static const char *PW_CONFIG_FILE = "/tmp/power/power_config.txt";
static const char *BATTERY_PATH = "/sys/class/power_supply/battery/capacity";
///aah add 2012-06-11 for check ac have plugged or not
static const char *BATTERY_AC = "/sys/class/power_supply/charger_ac/online";
int check_power_config(void);
#endif

extern UIParameters ui_parameters;    // from ui.c

/*
 * The recovery tool communicates with the main system through /cache files.
 *   /cache/recovery/command - INPUT - command line for tool, one arg per line
 *   /cache/recovery/log - OUTPUT - combined log file from recovery run(s)
 *   /cache/recovery/intent - OUTPUT - intent that was passed in
 *
 * The arguments which may be supplied in the recovery.command file:
 *   --send_intent=anystring - write the text out to recovery.intent
 *   --update_package=path - verify install an OTA package file
 *   --wipe_data - erase user data (and cache), then reboot
 *   --wipe_cache - wipe cache (but not user data), then reboot
 *   --set_encrypted_filesystem=on|off - enables / diasables encrypted fs
 *
 * After completing, we remove /cache/recovery/command and reboot.
 * Arguments may also be supplied in the bootloader control block (BCB).
 * These important scenarios must be safely restartable at any point:
 *
 * FACTORY RESET
 * 1. user selects "factory reset"
 * 2. main system writes "--wipe_data" to /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--wipe_data"
 *    -- after this, rebooting will restart the erase --
 * 5. erase_volume() reformats /data
 * 6. erase_volume() reformats /cache
 * 7. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 8. main() calls reboot() to boot main system
 *
 * OTA INSTALL
 * 1. main system downloads OTA package to /cache/some-filename.zip
 * 2. main system writes "--update_package=/cache/some-filename.zip"
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--update_package=..."
 *    -- after this, rebooting will attempt to reinstall the update --
 * 5. install_package() attempts to install the update
 *    NOTE: the package install must itself be restartable from any point
 * 6. finish_recovery() erases BCB
 *    -- after this, rebooting will (try to) restart the main system --
 * 7. ** if install failed **
 *    7a. prompt_and_wait() shows an error icon and waits for the user
 *    7b; the user reboots (pulling the battery, etc) into the main system
 * 8. main() calls maybe_install_firmware_update()
 *    ** if the update contained radio/hboot firmware **:
 *    8a. m_i_f_u() writes BCB with "boot-recovery" and "--wipe_cache"
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8b. m_i_f_u() writes firmware image into raw cache partition
 *    8c. m_i_f_u() writes BCB with "update-radio/hboot" and "--wipe_cache"
 *        -- after this, rebooting will attempt to reinstall firmware --
 *    8d. bootloader tries to flash firmware
 *    8e. bootloader writes BCB with "boot-recovery" (keeping "--wipe_cache")
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8f. erase_volume() reformats /cache
 *    8g. finish_recovery() erases BCB
 *        -- after this, rebooting will (try to) restart the main system --
 * 9. main() calls reboot() to boot main system
 */

static const int MAX_ARG_LENGTH = 4096;
static const int MAX_ARGS = 100;

// open a given path, mounting partitions as necessary
FILE*
fopen_path(const char *path, const char *mode) {
    if (ensure_path_mounted(path) != 0) {
        LOGE("Can't mount %s\n", path);
        return NULL;
    }

    // When writing, try to create the containing directory, if necessary.
    // Use generous permissions, the system (init.rc) will reset them.
    if (strchr("wa", mode[0])) dirCreateHierarchy(path, 0777, NULL, 1);

    FILE *fp = fopen(path, mode);
    return fp;
}

// close a file, log an error if the error indicator is set
static void
check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) LOGE("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

// command line args come from, in decreasing precedence:
//   - the actual command line
//   - the bootloader control block (one per line, after "recovery")
//   - the contents of COMMAND_FILE (one per line)
static void
get_args(int *argc, char ***argv) {
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    get_bootloader_message(&boot);  // this may fail, leaving a zeroed structure

    if (boot.command[0] != 0 && boot.command[0] != 255) {
        LOGI("Boot command: %.*s\n", sizeof(boot.command), boot.command);
    }

    if (boot.status[0] != 0 && boot.status[0] != 255) {
        LOGI("Boot status: %.*s\n", sizeof(boot.status), boot.status);
    }

    // --- if arguments weren't supplied, look in the bootloader control block
    if (*argc <= 1) {
        boot.recovery[sizeof(boot.recovery) - 1] = '\0';  // Ensure termination
        const char *arg = strtok(boot.recovery, "\n");
        if (arg != NULL && !strcmp(arg, "recovery")) {
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = strdup(arg);
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if ((arg = strtok(NULL, "\n")) == NULL) break;
                (*argv)[*argc] = strdup(arg);
            }
            LOGI("Got arguments from boot message\n");
        } else if (boot.recovery[0] != 0 && boot.recovery[0] != 255) {
            LOGE("Bad boot message\n\"%.20s\"\n", boot.recovery);
        }
    }

    // --- if that doesn't work, try the command file
    if (*argc <= 1) {
        FILE *fp = fopen_path(COMMAND_FILE, "r");
        if (fp != NULL) {
            char *argv0 = (*argv)[0];
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = argv0;  // use the same program name

            char buf[MAX_ARG_LENGTH];
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if (!fgets(buf, sizeof(buf), fp)) break;
                (*argv)[*argc] = strdup(strtok(buf, "\r\n"));  // Strip newline.
            }

            check_and_fclose(fp, COMMAND_FILE);
            LOGI("Got arguments from %s\n", COMMAND_FILE);
        }
    }

    // --> write the arguments we have back into the bootloader control block
    // always boot into recovery after this (until finish_recovery() is called)
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    int i;
    for (i = 1; i < *argc; ++i) {
        strlcat(boot.recovery, (*argv)[i], sizeof(boot.recovery));
        strlcat(boot.recovery, "\n", sizeof(boot.recovery));
    }
    set_bootloader_message(&boot);
}

static void
set_sdcard_update_bootloader_message() {
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    set_bootloader_message(&boot);
}

// How much of the temp log we have copied to the copy in cache.
static long tmplog_offset = 0;

static void
copy_log_file(const char* source, const char* destination, int append) {
    FILE *log = fopen_path(destination, append ? "a" : "w");
    if (log == NULL) {
        LOGE("Can't open %s\n", destination);
    } else {
        FILE *tmplog = fopen(source, "r");
        if (tmplog != NULL) {
            if (append) {
                fseek(tmplog, tmplog_offset, SEEK_SET);  // Since last write
            }
            char buf[4096];
            while (fgets(buf, sizeof(buf), tmplog)) fputs(buf, log);
            if (append) {
                tmplog_offset = ftell(tmplog);
            }
            check_and_fclose(tmplog, source);
        }
        check_and_fclose(log, destination);
    }
}


// clear the recovery command and prepare to boot a (hopefully working) system,
// copy our log file to cache as well (for the system to read), and
// record any intent we were asked to communicate back to the system.
// this function is idempotent: call it as many times as you like.
static void
finish_recovery(const char *send_intent) {
    // By this point, we're ready to return to the main system...
    if (send_intent != NULL) {
        FILE *fp = fopen_path(INTENT_FILE, "w");
        if (fp == NULL) {
            LOGE("Can't open %s\n", INTENT_FILE);
        } else {
            fputs(send_intent, fp);
            check_and_fclose(fp, INTENT_FILE);
        }
    }

    // Copy logs to cache so the system can find out what happened.
    copy_log_file(TEMPORARY_LOG_FILE, LOG_FILE, true);
    copy_log_file(TEMPORARY_LOG_FILE, LAST_LOG_FILE, false);
    copy_log_file(TEMPORARY_INSTALL_FILE, LAST_INSTALL_FILE, false);
    chmod(LOG_FILE, 0600);
    chown(LOG_FILE, 1000, 1000);   // system user
    chmod(LAST_LOG_FILE, 0640);
    chmod(LAST_INSTALL_FILE, 0644);

    // Reset to mormal system boot so recovery won't cycle indefinitely.
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    set_bootloader_message(&boot);

    // Remove the command file, so recovery won't repeat indefinitely.
    if (ensure_path_mounted(COMMAND_FILE) != 0 ||
        (unlink(COMMAND_FILE) && errno != ENOENT)) {
        LOGW("Can't unlink %s\n", COMMAND_FILE);
    }

    ensure_path_unmounted(CACHE_ROOT);
    sync();  // For good measure.
}

static int
erase_volume(const char *volume) {
    ui_set_background(BACKGROUND_ICON_INSTALLING);
    ui_show_indeterminate_progress();
    ui_print("Formatting %s...\n", volume);

    ensure_path_unmounted(volume);

    if (strcmp(volume, "/cache") == 0) {
        // Any part of the log we'd copied to cache is now gone.
        // Reset the pointer so we copy from the beginning of the temp
        // log.
        tmplog_offset = 0;
    }

    return format_volume(volume);
}
#ifdef POWER_CHECK
int get_battery_info(void)
{
	char buf[64] = {0};
	int	 level = 0;
	//pw_fd = open(BATTERY_PATH, O_RDONLY);
	if(pw_fd<=0)
	{
		LOGE("get battery info:fd <0, [%s]\n", BATTERY_PATH);
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	lseek(pw_fd, 0, SEEK_SET);
	read(pw_fd, buf, sizeof(buf));
	level=atoi(buf);
	//ui_print("aahtest get battery capacity is [%d]!!!!!!!!!\n", level);
	if((level<=0) || (level>100))
	{
		LOGE("get battery info:battery value error, is [%d]!\n", level);
		return -1;
	}
	return level;
}

///0 ac not plugged, 1 ac plugged
int ac_check(void)
{
	char buf[16] = {0};
	int	 level = 0;
	//ac_fd = open(BATTERY_AC, O_RDONLY);
	if(ac_fd<=0)
	{
		LOGE("get battery info:fd <0, [%s]\n", BATTERY_AC);
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	lseek(ac_fd, 0, SEEK_SET);
	read(ac_fd, buf, sizeof(buf));
	level=atoi(buf);
	return level;
}
#endif

static char*
copy_sideloaded_package(const char* original_path) {
	
	struct stat st;

  if (ensure_path_mounted(original_path) != 0) {
    LOGE("Can't mount %s\n", original_path);
    return NULL;
  }

  if (stat("/tfcard/update.zip", &st) == 0) {
  LOGE("make sure your TF card has enough space,we suggest 1GB\n");
  SIDELOAD_TEMP_DIR ="/tfcard/sideload";
  }
  
  if (stat("/sdcard/update.zip", &st) == 0) {
  LOGE("make sure your SD card has enough space,we suggest 1GB\n");
  SIDELOAD_TEMP_DIR ="/sdcard/sideload";
  }


  if (ensure_path_mounted(SIDELOAD_TEMP_DIR) != 0) {
    LOGE("Can't mount %s\n", SIDELOAD_TEMP_DIR);
  //  return NULL;
  }
  if (mkdir(SIDELOAD_TEMP_DIR, 0700) != 0) {
    if (errno != EEXIST) {
      LOGE("Can't mkdir %s (%s)\n", SIDELOAD_TEMP_DIR, strerror(errno));
      return NULL;
    }
  }

  // verify that SIDELOAD_TEMP_DIR is exactly what we expect: a
  // directory, owned by root, readable and writable only by root.
//  struct stat st;
  if (stat(SIDELOAD_TEMP_DIR, &st) != 0) {
    LOGE("failed to stat %s (%s)\n", SIDELOAD_TEMP_DIR, strerror(errno));
    return NULL;
  }
  if (!S_ISDIR(st.st_mode)) {
    LOGE("%s isn't a directory\n", SIDELOAD_TEMP_DIR);
    return NULL;
  }
  if ((st.st_mode & 0777) != 0777) {
    LOGE("%s has perms %o\n", SIDELOAD_TEMP_DIR, st.st_mode);
    return NULL;
  }
  if (st.st_uid != 0) {
    LOGE("%s owned by %lu; not root\n", SIDELOAD_TEMP_DIR, st.st_uid);
    return NULL;
  }
  char copy_path[PATH_MAX];
  strcpy(copy_path, SIDELOAD_TEMP_DIR);
  strcat(copy_path, "/package.zip");

  char* buffer = malloc(BUFSIZ);
  if (buffer == NULL) {
    LOGE("Failed to allocate buffer\n");
    return NULL;
  }

  size_t read;
  FILE* fin = fopen(original_path, "rb");
  if (fin == NULL) {
    LOGE("Failed to open %s (%s)\n", original_path, strerror(errno));
    return NULL;
  }
  FILE* fout = fopen(copy_path, "wb");
  if (fout == NULL) {
    LOGE("Failed to open %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  while ((read = fread(buffer, 1, BUFSIZ, fin)) > 0) {
    if (fwrite(buffer, 1, read, fout) != read) {
      LOGE("Short write of %s (%s)\n", copy_path, strerror(errno));
      return NULL;
    }
  }

  free(buffer);

  if (fclose(fout) != 0) {
    LOGE("Failed to close %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  if (fclose(fin) != 0) {
    LOGE("Failed to close %s (%s)\n", original_path, strerror(errno));
    return NULL;
  }
  // "adb push" is happy to overwrite read-only files when it's
  // running as root, but we'll try anyway.
  if (chmod(copy_path, 0400) != 0) {
    LOGE("Failed to chmod %s (%s)\n", copy_path, strerror(errno));
    return NULL;
  }

  return strdup(copy_path);
}

static char**
prepend_title(const char** headers) {
    char* title[] = { "Android system recovery <"
                          EXPAND(RECOVERY_API_VERSION) "e>",
                      "",
                      NULL };

    // count the number of lines in our title, plus the
    // caller-provided headers.
    int count = 0;
    char** p;
    for (p = title; *p; ++p, ++count);
    for (p = headers; *p; ++p, ++count);

    char** new_headers = malloc((count+1) * sizeof(char*));
    char** h = new_headers;
    for (p = title; *p; ++p, ++h) *h = *p;
    for (p = headers; *p; ++p, ++h) *h = *p;
    *h = NULL;

    return new_headers;
}

static int
get_menu_selection(char** headers, char** items, int menu_only,
                   int initial_selection) {
    // throw away keys pressed previously, so user doesn't
    // accidentally trigger menu items.
    ui_clear_key_queue();

    ui_start_menu(headers, items, initial_selection);
    int selected = initial_selection;
    int chosen_item = -1;

    while (chosen_item < 0) {
        int key = ui_wait_key();
        int visible = ui_text_visible();

        if (key == -1) {   // ui_wait_key() timed out
            if (ui_text_ever_visible()) {
                continue;
            } else {
                LOGI("timed out waiting for key input; rebooting.\n");
                ui_end_menu();
                return ITEM_REBOOT;
            }
        }

        int action = device_handle_key(key, visible);

        if (action < 0) {
            switch (action) {
                case HIGHLIGHT_UP:
                    --selected;
                    selected = ui_menu_select(selected);
                    break;
                case HIGHLIGHT_DOWN:
                    ++selected;
                    selected = ui_menu_select(selected);
                    break;
                case SELECT_ITEM:
                    chosen_item = selected;
                    break;
                case NO_ACTION:
                    break;
            }
        } else if (!menu_only) {
            chosen_item = action;
        }
    }

    ui_end_menu();
    return chosen_item;
}

static int compare_string(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

static int
update_directory(const char* path, const char* unmount_when_done,
                 int* wipe_cache) {
    ensure_path_mounted(path);

    const char* MENU_HEADERS[] = { "Choose a package to install:",
                                   path,
                                   "",
                                   NULL };
    DIR* d;
    struct dirent* de;
    d = opendir(path);
    if (d == NULL) {
        LOGE("error opening %s: %s\n", path, strerror(errno));
        if (unmount_when_done != NULL) {
            ensure_path_unmounted(unmount_when_done);
        }
        return 0;
    }

    char** headers = prepend_title(MENU_HEADERS);

    int d_size = 0;
    int d_alloc = 10;
    char** dirs = malloc(d_alloc * sizeof(char*));
    int z_size = 1;
    int z_alloc = 10;
    char** zips = malloc(z_alloc * sizeof(char*));
    zips[0] = strdup("../");

    while ((de = readdir(d)) != NULL) {
        int name_len = strlen(de->d_name);

        if (de->d_type == DT_DIR) {
            // skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' &&
                de->d_name[1] == '.') continue;

            if (d_size >= d_alloc) {
                d_alloc *= 2;
                dirs = realloc(dirs, d_alloc * sizeof(char*));
            }
            dirs[d_size] = malloc(name_len + 2);
            strcpy(dirs[d_size], de->d_name);
            dirs[d_size][name_len] = '/';
            dirs[d_size][name_len+1] = '\0';
            ++d_size;
        } else if (de->d_type == DT_REG &&
                   name_len >= 4 &&
                   strncasecmp(de->d_name + (name_len-4), ".zip", 4) == 0) {
            if (z_size >= z_alloc) {
                z_alloc *= 2;
                zips = realloc(zips, z_alloc * sizeof(char*));
            }
            zips[z_size++] = strdup(de->d_name);
        }
    }
    closedir(d);

    qsort(dirs, d_size, sizeof(char*), compare_string);
    qsort(zips, z_size, sizeof(char*), compare_string);

    // append dirs to the zips list
    if (d_size + z_size + 1 > z_alloc) {
        z_alloc = d_size + z_size + 1;
        zips = realloc(zips, z_alloc * sizeof(char*));
    }
    memcpy(zips + z_size, dirs, d_size * sizeof(char*));
    free(dirs);
    z_size += d_size;
    zips[z_size] = NULL;

    int result;
    int chosen_item = 0;
    do {
        chosen_item = get_menu_selection(headers, zips, 1, chosen_item);

        char* item = zips[chosen_item];
        int item_len = strlen(item);
        if (chosen_item == 0) {          // item 0 is always "../"
            // go up but continue browsing (if the caller is update_directory)
            result = -1;
            break;
        } else if (item[item_len-1] == '/') {
            // recurse down into a subdirectory
            char new_path[PATH_MAX];
            strlcpy(new_path, path, PATH_MAX);
            strlcat(new_path, "/", PATH_MAX);
            strlcat(new_path, item, PATH_MAX);
            new_path[strlen(new_path)-1] = '\0';  // truncate the trailing '/'
            result = update_directory(new_path, unmount_when_done, wipe_cache);
            if (result >= 0) break;
        } else {
            // selected a zip file:  attempt to install it, and return
            // the status to the caller.
            char new_path[PATH_MAX];
            strlcpy(new_path, path, PATH_MAX);
            strlcat(new_path, "/", PATH_MAX);
            strlcat(new_path, item, PATH_MAX);

            ui_print("\n-- Install %s ...\n", path);
            set_sdcard_update_bootloader_message();
            char* copy = copy_sideloaded_package(new_path);
            if (unmount_when_done != NULL) {
                ensure_path_unmounted(unmount_when_done);
            }
            if (copy) {
#ifdef POWER_CHECK
				if(ac_check()!=0)
				{
					ui_print("Found ac has plugged in, keep it when upgrade!!!\n");
					sleep(3);	
				}
				else
				{
					ui_print("Not found ac has plugged in!!!\n");
					if(check_power_config())
					{
						ui_print("warning:check power_config failed!!!!!!!\n");
					}
					else
					{
						int ret=-1;
						ret=get_battery_info();
						if(ret<0)
						{
							ui_print("NOTE: this install may use [%d/100] battery, but get battery info failed\n", pw_para.update);
							ui_print("going to poweroff....");
							sleep(5);
							android_reboot(ANDROID_RB_POWEROFF, 0, 0);
						}
						else
						{
							if(ret < pw_para.update)
							{
								ui_print("NOTE: this install may use [%d/100] battery! Battery now is [%d]\n So please charge now!!!\n", pw_para.update, ret);
								ui_print("going to poweroff....");
								sleep(5);
								android_reboot(ANDROID_RB_POWEROFF, 0, 0);
							}
							else
							{
								ui_print("This installation may use [%d/100] battery \nBattery now is [%d], start install!\n ", pw_para.update, ret);
								sleep(3);
							}
						}
					}
				}
#endif
                result = install_package(copy, wipe_cache, TEMPORARY_INSTALL_FILE);
                free(copy);
            } else {
                result = INSTALL_ERROR;
            }
            break;
        }
    } while (true);

    int i;
    for (i = 0; i < z_size; ++i) free(zips[i]);
    free(zips);
    free(headers);

    if (unmount_when_done != NULL) {
        ensure_path_unmounted(unmount_when_done);
    }
    return result;
}

/*
	Added vendor code
*/
#define MAX_LINE 1024
#define PATH_SDCARD "/tmp/mntsd/nandroid/"
#define GO_BACK     "go back"


char* BACKUP_AND_RESTORE_MENU_ITEMS[] = { 
                       "- backup",
                       "- restore",
					   "- advance restore",
                       "- go back",
                       NULL };


/*
	you must free mem after calling this function
*/
char* match_string_line(const char* file, const char* pattern)
{
    char buf[MAX_LINE] = {0};  
    FILE *fp;                
    int len;                 

    if((fp = fopen(file,"r")) == NULL)
    {   
        perror("fail to read");
        return NULL;
    }   

        
    while(fgets(buf,MAX_LINE,fp) != NULL)
    {   
        len = strlen(buf);
        buf[len-1] = '\0';  /*去掉换行符*/
        if(strstr(buf, pattern) != NULL)
        {   
            char* ret = malloc(len * sizeof(char));
            memset(ret, 0x00, len * sizeof(char));
            memcpy(ret, buf, len * sizeof(char));
            return ret; 
        }   
        memset(buf, 0x00, sizeof(buf));
    }   
        
    return NULL;
}


static bool system_script(int status)
{
	if((-1 != status) && WIFEXITED(status))
		return true;

	return false;
}

static int get_image_name(char* name)
{
    if(name == NULL){
		LOGE("invalid input pointer, the pointer is null\n");
		return -1;
    }

    time_t now;
    struct tm *timenow;
    time(&now);
    timenow = localtime(&now);

    char demo[] = {"2011-10-21---14.12.20\n"};
    int size = strlen(demo);
    memset(name, 0x00, size);
    sprintf(&name[0], "%04d", timenow->tm_year+1900); 
    sprintf(&name[4], "%s", "-"); 
    sprintf(&name[5], "%02d", timenow->tm_mon+1); 
    sprintf(&name[7], "%s", "-"); 
    sprintf(&name[8], "%02d", timenow->tm_mday); 
    sprintf(&name[10], "%s", "---"); 
    sprintf(&name[13], "%02d", timenow->tm_hour); 
    sprintf(&name[15], "%s", "."); 
    sprintf(&name[16], "%02d", timenow->tm_min); 
    sprintf(&name[18], "%s", "."); 
    sprintf(&name[19], "%02d", timenow->tm_sec); 
    

    return 0; 
}

static int rename_image(const char *oldname)
{
    int ret = 0;
    char name[256] = {0};

    ret = get_image_name(name);
    	if(ret){
		return ret;
    }

    ret = system("/bin/make_mntsd.sh");
	if(ret){
		LOGE("mkdir /tmp/mntsd failed\n");
		return ret;
    }

    ret = mount("/dev/block/mmcblk0p1","/tmp/mntsd","vfat", MS_SYNCHRONOUS, 0);
	if(ret){
		LOGE("mount sd card failed\n");
			return ret;
    }

    char src[256] = {0};
    char dst[256] = {0};
    int pathlen = strlen(PATH_SDCARD);
    strcpy(src, PATH_SDCARD); 
    strcpy(&src[pathlen], oldname);
    strcpy(dst, PATH_SDCARD);
    strcpy(&dst[pathlen], name);

    ret = rename(src, dst);
    if(ret){
		LOGE("rename image file failed\n");
		umount("/tmp/mntsd");
		system("/bin/rm_mntsd.sh");
		return ret;
    }

    umount("/tmp/mntsd");
    system("/bin/rm_mntsd.sh");

    return 0;
}


static bool confirm_operation(char** headers)
{
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            title_headers = prepend_title((const char**)headers);
		}
        char* items[] = { " No",
                          " Yes",   // [1]
                          NULL };
		ui_reset_progress();
        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item == 1) {
			return true;
        }

		return false;
}

static unsigned int select_sdcard(char** headers)
{
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            title_headers = prepend_title((const char**)headers);
		}
        char* items[] = { " emmc sdcard",
                          " tf card",   // [1]
						  " go back",
                          NULL };
		ui_reset_progress();
        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
		switch(chosen_item){
			case 0:
				return SDCARD;
			case 1:
				return TFCARD;
		}

		return 2;
}


static void restore_image(const char* path, bool advance)
{

	int ret, status;
	const char* MENU_HEADERS[] = { "Choose a image to restore",
								path,
							   	"", 
							   	NULL };
	DIR* d;
	struct dirent* de; 
	d = opendir(path);
	if(d == NULL){
		if (errno == ENOENT){
            LOGE("device does not have restore image\n");
            return;
        }

		LOGE("error opening %s: %s\n", path, strerror(errno));
		return;
	}

	char** headers = prepend_title(MENU_HEADERS);
	int d_size = 0;
	int d_alloc = 10;
	char** items = malloc(d_alloc * sizeof(char*));
	while((de = readdir(d)) != NULL){
		int name_len = strlen(de->d_name);
		if(de->d_type == DT_DIR){
			// skip "." and ".." entries
            if (name_len == 1 && de->d_name[0] == '.') continue;
            if (name_len == 2 && de->d_name[0] == '.' &&
                de->d_name[1] == '.') continue;

			//add name to items
			if(d_size >= d_alloc) {
                d_alloc *= 2;
                items = realloc(items, d_alloc * sizeof(char*));
			}
			items[d_size] = malloc(name_len + 1);
            strcpy(items[d_size], de->d_name);
            items[d_size][name_len] = '\0';
            ++d_size;
		}
	}
	closedir(d);

	if(d_size == 0){
        free(items);
        LOGE("device does not have restore image\n");
        return;
    }
	

	char* ptrname = NULL;
	char** normal_item = NULL;
	int number = 3;
	if(!advance){
		//get the latest image name
    	int i;
    	ptrname = items[0];
    	for(i=1;i<d_size;i++){
        	if(strcmp(ptrname, items[i]) < 0)
            	ptrname = items[i];
    	}
		DEBUG_RECOVERY("latest image name is %s\n", ptrname);
	}

	//add go back menu
	if(advance){
		items[d_size] = malloc(strlen(GO_BACK)+1);
		strcpy(items[d_size], GO_BACK);
		items[d_size][strlen(GO_BACK)] = '\0';
		d_size++;
		if(d_size >= d_alloc) {
			d_alloc *= 2;
			items = realloc(items, d_alloc * sizeof(char*));
		}
		//items[d_size] = malloc(1);
		items[d_size] = NULL;
		qsort(items, d_size, sizeof(char*), compare_string);
	}else{
		normal_item = malloc(number * sizeof(char*));
		normal_item[0] = malloc(strlen(ptrname)+1);
		normal_item[1] = malloc(strlen(GO_BACK)+1);
		strcpy(normal_item[0], ptrname);
		normal_item[0][strlen(ptrname)] = '\0';
		strcpy(normal_item[1], GO_BACK);
		normal_item[1][strlen(GO_BACK)] = '\0';
		normal_item[2] = NULL;
		qsort(normal_item, number-1, sizeof(char*), compare_string);
	}
	

	int chosen_item = 0;
	do{
		ui_reset_progress();
		char* menu = NULL;
		if(advance){
			chosen_item = get_menu_selection(headers, items, 1, chosen_item);
        	menu = items[chosen_item];
		}else{
			chosen_item = get_menu_selection(headers, normal_item, 1, chosen_item);
        	menu = normal_item[chosen_item];
			
		}
        int item_len = strlen(menu);
		if(strcmp(GO_BACK, menu) == 0){
			break;
		}
		else{
			char* sub_headers[] = { "Confirm restore image?",
                                "restore image will cover current system",
                                "",  
                                NULL };
			if(confirm_operation((const char**)sub_headers)){
				char cmdline[100] = {0};		

				ui_print("start restore file system...\n");	
				//boot image
				ui_print("restore boot image...\n");
		#ifdef NS2816
				sprintf(cmdline, "%s %s 0", "/bin/ns2816/restore_boot.sh", menu);
		#else
			#ifdef NS115
				sprintf(cmdline, "%s %s 1", "/bin/ns2816/restore_boot.sh", menu);
			#endif
		#endif
				
				DEBUG_RECOVERY("restore  boot image cmdline is %s\n", cmdline);
				status = system(cmdline);
				if(!system_script(status)){
                    LOGE("system error, execute restore_boot.sh failed\n");
                    continue;
                }    
                ret = WEXITSTATUS(status);
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount boot partition failed\n");
							break;
						default:
							LOGE("restore boot.img failed\n");
							break;
					}
					continue;
				}

				//system image
				ui_print("restore system image...\n");
				memset(cmdline, 0x00, sizeof(cmdline));
		#ifdef NS2816
				sprintf(cmdline, "%s %s 0", "/bin/ns2816/restore_system.sh", menu);
		#else
			#ifdef NS115
				sprintf(cmdline, "%s %s 1", "/bin/ns2816/restore_system.sh", menu);
			#endif
		#endif
				
				DEBUG_RECOVERY("restore  boot system cmdline is %s\n", cmdline);
				status = system(cmdline);
				if(!system_script(status)){
                    LOGE("system error, execute restore_system.sh failed\n");
                    continue;
                }    
                ret = WEXITSTATUS(status);
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount system partition failed\n");
							break;
						default:
							LOGE("restore system.img failed\n");
							break;
					}
					continue;
				}

				//data image	
				ui_print("restore data image...\n");
				memset(cmdline, 0x00, sizeof(cmdline));
		#ifdef NS2816
				sprintf(cmdline, "%s %s 0", "/bin/ns2816/restore_data.sh", menu);
		#else
			#ifdef NS115
				sprintf(cmdline, "%s %s 1", "/bin/ns2816/restore_data.sh", menu);
			#endif
		#endif
				
				DEBUG_RECOVERY("restore data image cmdline is %s\n", cmdline);
				status = system(cmdline);
				if(!system_script(status)){
                    LOGE("system error, execute restore_data.sh failed\n");
                    continue;
                }    
                ret = WEXITSTATUS(status);
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount data partition failed\n");
							break;
						default:
							LOGE("restore data.img failed\n");
							break;
					}
					continue;
				}

				//cache image	
				ui_print("restore cache image...\n");
				memset(cmdline, 0x00, sizeof(cmdline));
		#ifdef NS2816
				sprintf(cmdline, "%s %s 0", "/bin/ns2816/restore_cache.sh", menu);
		#else
			#ifdef NS115
				sprintf(cmdline, "%s %s 1", "/bin/ns2816/restore_cache.sh", menu);
			#endif
		#endif
				
				DEBUG_RECOVERY("restore cache image cmdline is %s\n", cmdline);
				status = system(cmdline);
				if(!system_script(status)){
                    LOGE("system error, execute restore_cache.sh failed\n");
                    continue;
                }    
                ret = WEXITSTATUS(status);
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount cache partition failed\n");
							break;
						default:
							LOGE("restore cache.img failed\n");
							break;
					}
					continue;
				}

				
				//misc image	
				ui_print("restore misc image...\n");
				memset(cmdline, 0x00, sizeof(cmdline));
		#ifdef NS2816
				sprintf(cmdline, "%s %s 0", "/bin/ns2816/restore_misc.sh", menu);
		#else
			#ifdef NS115
				sprintf(cmdline, "%s %s 1", "/bin/ns2816/restore_misc.sh", menu);
			#endif
		#endif
				
				DEBUG_RECOVERY("restore misc image cmdline is %s\n", cmdline);
				status = system(cmdline);
				if(!system_script(status)){
                    LOGE("system error, execute restore_misc.sh failed\n");
                    continue;
                }    
                ret = WEXITSTATUS(status);
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount misc partition failed\n");
							break;
						default:
							LOGE("restore misc.img failed\n");
							break;
					}
					continue;
				}
				
				
				//restore end
				ui_print("restore image %s succeeded\n", menu);
			}
		}
	}while(true);

	int i;
    for (i = 0; i < d_size; ++i) free(items[i]);
    free(items);

	if(!advance){
    	for (i = 0; i < number; ++i) free(normal_item[i]);
    	free(normal_item);
	}
	
	return;
}

static void backup_restore(){
    int ret = 0;
	int status = 0;
    const char* MENU_HEADERS[] = { "backup and restore your system",
                                   "",
				   					NULL };
    char** headers = prepend_title(MENU_HEADERS);
    int chosen_item = 0;
ITEM:
    do {
		ui_reset_progress();
		chosen_item = get_menu_selection(headers, BACKUP_AND_RESTORE_MENU_ITEMS, 1, chosen_item);
		switch(chosen_item){
			case ITEM_BR_BACKUP:
				ui_print("backup system needs several minutes, please wait...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_check.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_check.sh 1");
			#endif
		#endif
				if(!system_script(status)){
					LOGE("system error, execute backup_check.sh failed\n");
					goto ITEM;
				}
				ret = WEXITSTATUS(status); 
				if(ret){
					switch(ret){
						case 1:
							LOGE("mount sdcard failed\n");
							goto ITEM;
						case 2:
							LOGE("mount boot partition failed\n");
							goto ITEM;
						case 3:
							LOGE("mount system partition failed\n");
							goto ITEM;
						case 4:
							LOGE("mount data partition failed\n");
							goto ITEM;
						case 5:
							LOGE("mount cache partition failed\n");
							goto ITEM;
						case 6:
							LOGE("mount misc partition failed\n");
							goto ITEM;
						default:
							LOGE("sdcard dose not have enough space, sdcard needs %dM Bytes more space\n", ret);
							goto ITEM;
					}
				}
				
				//get sdcard free space
				char* sdmem = NULL;
				sdmem = match_string_line("/tmp/backup/backuplog", "free sdcard size is");
				if(sdmem){
						ui_print("%s\n", sdmem);
				}
				else{
					LOGE("get sdcard memory failed\n");
					free(sdmem);
					goto ITEM;
				}
				free(sdmem);
			
				//process boot.img
				ui_print("backup boot image...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_boot.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_boot.sh 1");
			#endif
		#endif
				
				if(!system_script(status)){
					LOGE("system error, execute backup_boot.sh failed\n");
					goto ITEM;
				}	
				ret = WEXITSTATUS(status);
				switch(ret)
				{
					case 1:
						LOGE("mount sdcard partition failed\n");	
						goto ITEM;
					case 2:
						LOGE("mount boot partition failed\n");	
						goto ITEM;
					case 3:
						LOGE("make boot.img failed\n");
						goto ITEM;
				} 
			
				//process system.img
				ui_print("backup system image...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_system.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_system.sh 1");
			#endif
		#endif
				
				if(!system_script(status)){
					LOGE("system error, execute backup_system.sh failed\n");
					goto ITEM;
				}	
				ret = WEXITSTATUS(status);
				switch(ret)
				{
					case 1:
						LOGE("mount sdcard partition failed\n");	
						goto ITEM;
					case 2:
						LOGE("mount system partition failed\n");	
						goto ITEM;
					case 3:
						LOGE("make system.img failed\n");
						goto ITEM;
				}

				 
				//process data.img
				ui_print("backup data image...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_data.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_data.sh 1");
			#endif
		#endif
				
				if(!system_script(status)){
					LOGE("data error, execute backup_data.sh failed\n");
					goto ITEM;
				}	
				ret = WEXITSTATUS(status);
				switch(ret)
				{
					case 1:
						LOGE("mount sdcard partition failed\n");	
						goto ITEM;
					case 2:
						LOGE("mount data partition failed\n");	
						goto ITEM;
					case 3:
						LOGE("make data.img failed\n");
						goto ITEM;
				}
								
				
				//process cache.img
				ui_print("backup cache image...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_cache.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_cache.sh 1");
			#endif
		#endif
				
				if(!system_script(status)){
					LOGE("data error, execute backup_cache.sh failed\n");
					goto ITEM;
				}	
				ret = WEXITSTATUS(status);
				switch(ret)
				{
					case 1:
						LOGE("mount sdcard partition failed\n");	
						goto ITEM;
					case 2:
						LOGE("mount cache partition failed\n");	
						goto ITEM;
					case 3:
						LOGE("make cache.img failed\n");
						goto ITEM;
				}

			
								
				//process misc.img
				ui_print("backup misc image...\n");
		#ifdef NS2816
				status = system("/bin/ns2816/backup_misc.sh 0");
		#else
			#ifdef NS115
				status = system("/bin/ns2816/backup_misc.sh 1");
			#endif
		#endif
				
				if(!system_script(status)){
					LOGE("data error, execute backup_misc.sh failed\n");
					goto ITEM;
				}	
				ret = WEXITSTATUS(status);
				switch(ret)
				{
					case 1:
						LOGE("mount sdcard partition failed\n");	
						goto ITEM;
					case 2:
						LOGE("mount misc partition failed\n");	
						goto ITEM;
					case 3:
						LOGE("make misc.img failed\n");
						goto ITEM;
				}
				
	
				char* imgname = NULL;
				imgname = match_string_line("/tmp/backup/backuplog", "image storage name is");
				if(imgname){
						ui_print("%s\n", imgname);
				}
				else{
					LOGE("get image name failed\n");
					free(imgname);
					goto ITEM;
				}
				free(imgname);
	
				ui_print("backup android image file succeed\n");
				break;
			case ITEM_BR_RESTORE:
#ifdef NS115
				ret=get_battery_info();
				if((ret>0) && (ret<=50))
				{
					ui_print("NOTE: Now the battery capacity is [%d]percent, may need to charge!\n", ret);
					sleep(2);///aahadd for keep a while for user can known the means
				}
#endif
		#ifdef NS2816
				ret = system("/bin/ns2816/restore_mount.sh 0");
		#else
			#ifdef NS115
				ret = system("/bin/ns2816/restore_mount.sh 1");
			#endif
		#endif
				//ret = system("/bin/ns2816/restore_mount.sh ");
                if(ret){
                    ret = system("/bin/ns2816/restore_umount.sh");
                    LOGE("mount sdcard failed\n");
                    break;
                }
                DEBUG_RECOVERY("mount sdcard end\n");
                restore_image("/tmp/restore/nandroid", false);
                ret = system("/bin/ns2816/restore_umount.sh");
                if(ret){
                    LOGE("umount /tmp/restore/nandroid failed\n");
                }

                break;

				break;
			case ITEM_ADVANCE_RESTORE:
#ifdef NS115
				ret=get_battery_info();
				if((ret>0) && (ret<=50))
				{
					ui_print("NOTE: Now the battery capacity is [%d]percent, may need to charge!\n", ret);
					sleep(1);///aahadd for keep a while for user can known the means
				}
#endif
			#ifdef NS2816
						ret = system("/bin/ns2816/restore_mount.sh 0");
			#else
				#ifdef NS115
						ret = system("/bin/ns2816/restore_mount.sh 1");
				#endif
			#endif
						//ret = system("/bin/ns2816/restore_mount.sh ");

                if(ret){
                    ret = system("/bin/ns2816/restore_umount.sh");
                    LOGE("mount sdcard failed\n");
                    break;
                }
                DEBUG_RECOVERY("mount sdcard end\n");
                restore_image("/tmp/restore/nandroid", true);
                ret = system("/bin/ns2816/restore_umount.sh");
                if(ret){
                    LOGE("umount /tmp/restore/nandroid failed\n");
                }

				break;
			case ITEM_BR_GOBACK:
				return;
		}
    } while (true);

    return; 
}


static void power_off()
{
	char* headers[] = { "Confirm power off device?",
                            "",
                            NULL };
    if(confirm_operation((const char**)headers)){
#if 0
        sync();
        reboot(RB_POWER_OFF);
#endif
		ui_print("start power off device\n");
		android_reboot(ANDROID_RB_POWEROFF, 0, 0);
    }
}


static void recovery_misc_signed()
{
    int ret, status=-1;
#ifdef NS2816
    status = system("/bin/ns2816/recovery_signed.sh 0");
#else
#ifdef NS115
	status = system("/bin/ns2816/recovery_signed.sh 1");
#endif
#endif

    if(!system_script(status)){
        LOGE("execute restore_system.sh failed\n");
        return;
    }
    ret = WEXITSTATUS(status);
    switch(ret)
    {
        case 1:
            LOGE("mount misc partition failed\n");
            return;
        case 2:
            LOGE("umount misc partition failed\n");
            return;
    }

    sync();
    reboot(RB_AUTOBOOT);

}

static void  reboot_recovery()
{
	char* headers[] = { "Confirm reboot recovery?",
                            "",
                            NULL };
    if(confirm_operation((const char**)headers)){
        recovery_misc_signed();
    }
}

#ifdef NS2816
static void update_ec()
{
	int state, ret;	

	ui_print("start updating ec, device will power off after updating ec\n ");
	state = system("/bin/ns2816/update_ec.sh");
	if(!system_script(state)){
		LOGE("system error, execute update_ec.sh failed\n");
		return;
	}
	ret = WEXITSTATUS(state);
	switch(ret)
	{
		case 1:
			LOGE("Can not mount tf card, please confirm whether the tf card has been inserted\n");
			break;
		case 2:
			LOGE("Can not find EC.BIN in tf card\n");
			break;
	}
}
#endif
static void update_xloader()
{
	
	int state, ret;	

	ui_print("start updating xloader, please wait...\n ");
#ifdef NS2816
	state = system("/bin/ns2816/update_xloader.sh 0");
#else
#ifdef NS115
///aah add here, may be need to add NS115-PAD and NS115-PONE after
state = system("/bin/ns2816/update_xloader.sh 1");
#endif
#endif

	if(!system_script(state)){
		LOGE("system error, execute update_xloader.sh failed\n");
		return;
	}
	ret = WEXITSTATUS(state);
	switch(ret)
	{
		case 1:
			LOGE("Can not mount tf card, please confirm whether the tf card has been inserted\n");
			return;
		case 2:
			LOGE("Can not find xload in tf card\n");
			return;
		///aah add 20120428
		case 3:
			LOGE("load xload from tf card failed\n");
			return;
		case 4:
			LOGE("error command to load xload\n");
			return;
	}
	ui_print("update xloader succeeded\n ");
}


static void update_uboot()
{
	
	int state, ret;	

	ui_print("start updating uboot, please wait...\n ");
#ifdef NS2816
	state = system("/bin/ns2816/update_uboot.sh 0");
#else
#ifdef NS115
///aah add here, may be need to add NS115-PAD and NS115-PONE after
state = system("/bin/ns2816/update_uboot.sh 1");
#endif
#endif
	if(!system_script(state)){
		LOGE("system error, execute update_uboot.sh failed\n");
		return;
	}
	ret = WEXITSTATUS(state);
	switch(ret)
	{
		case 1:
			LOGE("Can not mount tf card, please confirm whether the tf card has been inserted\n");
			return;
		case 2:
			LOGE("Can not find u-boot.bin in tf card\n");
			return;
		///aah add 20120428
		case 3:
			LOGE("load u-boot.bin from tf card failed\n");
			return;
		case 4:
			LOGE("error command to load u-boot.bin\n");
			return;
	}
	ui_print("update u-boot succeeded\n ");
}
/*****************************vendor code end**************************************/

#ifdef NS115
///aahadd 20120519 
static void update_logo()
{
		int state, ret; 
	
		ui_print("start updating logo, please wait...\n ");
		state = system("/bin/ns2816/update_logo.sh");

		if(!system_script(state)){
			LOGE("system error, execute update_logo.sh failed\n");
			return;
		}
		ret = WEXITSTATUS(state);
		switch(ret)
		{
			case 1:
				LOGE("Can not mount tf card, please confirm whether the tf card has been inserted\n");
				return;
			case 2:
				LOGE("Can not find logo.bmp in tf card\n");
				return;
			case 3:
				LOGE("load logo.bmp from tf card failed\n");
				return;
		}
		ui_print("update logo succeeded\n ");
}


///aahadd 20120705

static int mount_sdcard(int on)
{
		int state, ret; 
	
		if(on)
		{
			ui_print("start mount sdcard, please wait...\n ");
			state = system("/bin/ns2816/mount_sdcard.sh 1");
		}
		else
		{
			ui_print("start umount sdcard, please wait...\n ");
			state = system("/bin/ns2816/mount_sdcard.sh 0");
		}

		if(!system_script(state)){
			ui_print("system error, execute mount_sdcard.sh failed\n");
			return 1;
		}
		ret = WEXITSTATUS(state);
		switch(ret)
		{
			case 1:
				if(on)
					ui_print("mount sdcard failed\n");
				else
					ui_print("umount sdcard failed\n");
				return 1;
		}
		if(on)
			ui_print("mount sdcard succeeded\n ");
		else
			ui_print("umount sdcard succeeded\n");
		return 0;
}

static int confirm_choose(char** headers)
{
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            title_headers = prepend_title((const char**)headers);
		}
        char* items[] = { " Close",
                          " Open",   // [1]
                          NULL };
		ui_reset_progress();
        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item == 1) {
			return 1;
        }

		return 0;
}

static void chooose_mount_sdcard(void)
{
	char* headers[] = { "Choose open of close?",
                            "",
                            NULL };
    if(confirm_choose((const char**)headers)){

		ui_print("start mount sdcard\n");
		mount_sdcard(1);
    }
	else
	{
		ui_print("start Umount sdcard\n");
		mount_sdcard(0);
	}
}


#endif


static void
wipe_data(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of all user data?",
                                "  THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title((const char**)headers);
        }

        char* items[] = { " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " Yes -- delete all user data",   // [7]
                          " No",
                          " No",
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 7) {
            return;
        }
    }

    ui_print("\n-- Wiping data...\n");
    device_wipe_data();
    erase_volume("/data");
    erase_volume("/cache");
    ui_print("Data wipe complete.\n");
}

static void
prompt_and_wait() {
	int state, ret;
    char** headers = prepend_title((const char**)MENU_HEADERS);

    for (;;) {
        finish_recovery(NULL);
        ui_reset_progress();

        int chosen_item = get_menu_selection(headers, MENU_ITEMS, 0, 0);

        // device-specific code may take some action here.  It may
        // return one of the core actions handled in the switch
        // statement below.
        chosen_item = device_perform_action(chosen_item);

        int status;
        int wipe_cache;
        switch (chosen_item) {
            case ITEM_REBOOT:
                return;

            case ITEM_WIPE_DATA:
                wipe_data(ui_text_visible());
                if (!ui_text_visible()) return;
                break;

            case ITEM_WIPE_CACHE:
                ui_print("\n-- Wiping cache...\n");
                erase_volume("/cache");
                ui_print("Cache wipe complete.\n");
                if (!ui_text_visible()) return;
                break;

            case ITEM_APPLY_SDCARD:
				{
					char* sdcard_headers[] = { 
										"please select storage type",
										"",  
										NULL };
					unsigned int ret = select_sdcard((const char**)sdcard_headers);
					switch(ret){
						case SDCARD:
							status = update_directory(SDCARD_ROOT, SDCARD_ROOT, &wipe_cache);
							break;
						case TFCARD:
							status = update_directory(TFCARD_ROOT, TFCARD_ROOT, &wipe_cache);
							break;
						default:
							continue;
					}

					if (status == INSTALL_SUCCESS && wipe_cache) {
						ui_print("\n-- Wiping cache (at package request)...\n");
						if (erase_volume("/cache")) {
							ui_print("Cache wipe failed.\n");
						} else {
							ui_print("Cache wipe complete.\n");
						}
					}
					if (status >= 0) {
						if (status != INSTALL_SUCCESS) {
							ui_set_background(BACKGROUND_ICON_ERROR);
							ui_print("Installation aborted.\n");
						} else if (!ui_text_visible()) {
							return;  // reboot if logs aren't visible
						} else {
							ui_print("\nInstall from sdcard complete.\n");
						}
					}
					break;
				}
            case ITEM_APPLY_CACHE:
                // Don't unmount cache at the end of this.
                status = update_directory(CACHE_ROOT, NULL, &wipe_cache);
                if (status == INSTALL_SUCCESS && wipe_cache) {
                    ui_print("\n-- Wiping cache (at package request)...\n");
                    if (erase_volume("/cache")) {
                        ui_print("Cache wipe failed.\n");
                    } else {
                        ui_print("Cache wipe complete.\n");
                    }
                }
                if (status >= 0) {
                    if (status != INSTALL_SUCCESS) {
                        ui_set_background(BACKGROUND_ICON_ERROR);
                        ui_print("Installation aborted.\n");
                    } else if (!ui_text_visible()) {
                        return;  // reboot if logs aren't visible
                    } else {
                        ui_print("\nInstall from cache complete.\n");
                    }
                }
                break;
			/*
				Added extend function by vendor, such as backup, restore, reboot recovery, update recovery from usb, power off, etc
			*/
			case ITEM_BACKUP_RESTORE:
                backup_restore();
                break;
            case ITEM_REBOOT_RECOVERY:
                reboot_recovery();
                break;
#ifdef NS2816
            case ITEM_UPDATE_RECOVERY:
				{
					char* sub_headers[] = { "Confirm update recovery?",
									"update recovery will cover current emmc recovery",
									"",
									NULL };
					if(!confirm_operation((const char**)sub_headers))
						break;
					ui_print("start updating emmc recovery, please eait...\n ");
					state = system("/bin/ns2816/update_recovery.sh");
					if(!system_script(state)){
						LOGE("system error, execute update_recovery.sh failed\n");
						continue;
					}
					ret = WEXITSTATUS(state);
					switch(ret){
						case 1:
							ui_print("recovey is boot from emmc. please boot recovery from usb storage if you want to update recovery\n");
							break;
						case 2:
							LOGE("mount usb storage failed\n");
							break;
						case 3:
							LOGE("recovery.img is not found in usb storage first partition\n");
							break;
						case 4:
							LOGE("mount emmc recovery partition failed\n");
							break;
						case 5:
							LOGE("mount recovery.img failed, is recovery a valid image file?\n");
							break;
					}

					ui_print("update emmc recovery completed\n");
					break;
				}
		
			case ITEM_UPDATE_EC:
				update_ec();	
				break;	
#endif
			case ITEM_UPDATE_XLOADER:
				update_xloader();	
				break;	
			case ITEM_UPDATE_UBOOT:
				update_uboot();	
				break;	
#ifdef NS115
			case ITEM_UPDATE_LOGO:
				update_logo();
				break;

			case ITEM_MOUNT_SDCARD:
				///mount sdcard as a usb disk
				chooose_mount_sdcard();
				break;
#endif
            case ITEM_POWER_OFF:
                power_off();
                break;


        }
    }
}

static void
print_property(const char *key, const char *name, void *cookie) {
    printf("%s=%s\n", key, name);
}

/*aahadd 2012-04-01 for 'OTA decide' in  boot config file*/
#ifdef OTA
static int check_ota_decision(void)
{
    int ret, status;
    status = system("/system/bin/sh /bin/ns2816/check_ota_decision.sh");
    if(!system_script(status)){
        LOGE("execute check_ota_decision.sh failed\n");
        return -3;
    }
    ret = WEXITSTATUS(status);
    //LOGE(" final check ota decision is [%d]\n",ret);
    return ret;
}
#endif
static int set_boot_default()
{
    int ret, status;
#ifdef NS2816
    status = system("/bin/ns2816/boot_set.sh 0");
#else
#ifdef NS115
	status = system("/bin/ns2816/boot_set.sh 1");
#endif
#endif

    if(!system_script(status)){
        LOGE("execute boot_set.sh failed\n");
        return -3;
    }
    ret = WEXITSTATUS(status);
    switch(ret)
    {
        case 1:
            LOGE("mount misc partition failed\n");
            return ret;
        case 2:
            LOGE("umount misc partition failed\n");
            return ret;
    }

    return 0;
}
#ifdef POWER_CHECK
/*
  * pos use to choose which position the config file saved
  * pos == 0(default, ota, sdcard/Download),; when ==1, update, do this before install;
  *no use now!!!
  */
static int find_check_config(void)
{
	char cmd[32] = {0};
    int ret, status;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "/bin/ns2816/find_pw_config.sh");
    status = system(cmd);
    if(!system_script(status)){
        LOGE("execute command[%s] failed\n", cmd);
        return -3;
    }
    ret = WEXITSTATUS(status);
    return ret;
}

int check_power_config(void)
{
	char buf[128] = {0};
	FILE *FP=NULL;
	char *p=NULL;
	
	if(find_check_config())
	{
		LOGE("cannot found power config\n");
		return -1;
	}
	//ui_print("found power config\n");
	
	memset(&pw_para, 0, sizeof(struct pw_kind));
	FP = fopen(PW_CONFIG_FILE, "r");
	if(FP == NULL)
	{
		ui_print("cannot open power config\n");
		return -1;
	}
	while(1)
	{
		if(feof(FP))
		{
			ui_print("read the power config end\n");
			break;
		}
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), FP);
		p = strstr(buf, "ota");
		if(p) pw_para.ota = atoi(buf+4);
		p = strstr(buf, "update");
		if(p) pw_para.update= atoi(buf+7);
	}
	if(((pw_para.ota<=0)&&(pw_para.ota>100)) || ((pw_para.update<=0)&&(pw_para.update>100)))
	//if((!pw_para.ota) || (!pw_para.update))
	{
		ui_print("the power config content is something error ota[%d]? update[%d]?!!!\n", pw_para.ota, pw_para.update);
		if(FP) fclose(FP);
		return -1;
	}
	//ui_print("found power limited, ota is [%d], update is [%d]\n", pw_para.ota, pw_para.update);
	if(FP) fclose(FP);
	return 0;
}
#endif

int
main(int argc, char **argv) {
    time_t start = time(NULL);

    // If these fail, there's not really anywhere to complain...
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);
    printf("Starting recovery on %s", ctime(&start));

    device_ui_init(&ui_parameters);
    ui_init();
    ui_set_background(BACKGROUND_ICON_INSTALLING);
    load_volume_table();
    get_args(&argc, &argv);

    int previous_runs = 0;
    const char *send_intent = NULL;
    const char *update_package = NULL;
    int wipe_data = 0, wipe_cache = 0;

    int arg;
    while ((arg = getopt_long(argc, argv, "", OPTIONS, NULL)) != -1) {
        switch (arg) {
        case 'p': previous_runs = atoi(optarg); break;
        case 's': send_intent = optarg; break;
        case 'u': update_package = optarg; break;
        case 'w': wipe_data = wipe_cache = 1; break;
        case 'c': wipe_cache = 1; break;
        case 't': ui_show_text(1); break;
        case '?':
            LOGE("Invalid command argument\n");
            continue;
        }
    }
	
    device_recovery_start();
	
#ifdef POWER_CHECK
	ui_show_text(1);///aahadd for print the notice!
	pw_fd = open(BATTERY_PATH, O_RDONLY);
	if(pw_fd<=0)
	{
		ui_print("warning:get battery info:open [%s]failed\n", BATTERY_PATH);
	}

	ac_fd = open(BATTERY_AC, O_RDONLY);
	if(ac_fd<=0)
	{
		ui_print("warning:get battery info:open [%s]failed\n", BATTERY_AC);
	}
#endif

	///aahadd 2012-04-11 , ota==0 means no need to update,else do ota, default no need
#ifdef OTA
	update_package=NULL;///to avoid other update from udisk or sdcard
	int status = INSTALL_SUCCESS;
	char cmd[128]={0};
	int ret=0;
	int ota=check_ota_decision();
	//ui_print("OTA decision is %s !\n",ota ? "yes":"no");
	if(ota)
	{
		ui_print("start OTA update....\n");
#ifdef POWER_CHECK

		if(ac_check()!=0)
		{
			ui_print("Found ac has plugged in, keep it when updating!!!\n");
			sleep(3);	
		}
		else
		{
			ui_print("Not found ac has plugged in!!!\n");
			if(check_power_config())
			{
				ui_print("Warning:check power limited failed, please confirm AC charged when installing!\n");
				sleep(5);///aahadd for keep a while for user can known the means
			}
			else
			{
				ret=get_battery_info();
				if(ret<0)
				{
					ui_print("NOTE: get battery info failed, this installation may use [%d/100] battery!\n", pw_para.ota);
					ui_print("going to poweroff....");
					sleep(5);///aahadd for keep a while for user can known the means
					android_reboot(ANDROID_RB_POWEROFF, 0, 0);
				}
				else
				{
					if(ret < pw_para.ota)
					{
						ui_print("NOTE: this install may use [%d/100] battery! Battery now is [%d]\n So please charge now!!!\n", pw_para.ota, ret);
						ui_print("going to poweroff....");
						sleep(5);///aahadd for keep a while for user can known the means					
						android_reboot(ANDROID_RB_POWEROFF, 0, 0);
					}
					else
					{
						ui_print("This installation may use [%d/100] battery \nBattery now is [%d], start install!\n ", pw_para.ota, ret);
						sleep(3);
					}
				}	
			}
		}
#endif
			memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "%s/package.zip", SIDELOAD_TEMP_DIR);
	    	char *copy=(char*)&cmd[0];
			status = install_package(copy, &wipe_cache, TEMPORARY_INSTALL_FILE);
			ui_print("OTA update end, status: %d \n", status);
	}
	else
	{
		ui_print("NO need OTA update!\n");
		ensure_path_mounted(SDCARD_ROOT);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "rm -rf %s", OTA_ZIP_DIR);
		system(cmd);
		ensure_path_unmounted(SDCARD_ROOT);
	}
#endif
#ifdef POWER_CHECK
	ui_show_text(0);
#endif
	///aahadd 2012-04-11 end

    printf("Command:");
    for (arg = 0; arg < argc; arg++) {
        printf(" \"%s\"", argv[arg]);
    }
    printf("\n");

    if (update_package) {
        // For backwards compatibility on the cache partition only, if
        // we're given an old 'root' path "CACHE:foo", change it to
        // "/cache/foo".
        if (strncmp(update_package, "CACHE:", 6) == 0) {
            int len = strlen(update_package) + 10;
            char* modified_path = malloc(len);
            strlcpy(modified_path, "/cache/", len);
            strlcat(modified_path, update_package+6, len);
            printf("(replacing path \"%s\" with \"%s\")\n",
                   update_package, modified_path);
            update_package = modified_path;
        }
    }
    printf("\n");

    property_list(print_property, NULL);
    printf("\n");
#ifndef OTA
    int status = INSTALL_SUCCESS;
#endif
    if (update_package != NULL) {
        status = install_package(update_package, &wipe_cache, TEMPORARY_INSTALL_FILE);
        if (status == INSTALL_SUCCESS && wipe_cache) {
            if (erase_volume("/cache")) {
                LOGE("Cache wipe (requested by package) failed.");
            }
        }
        if (status != INSTALL_SUCCESS) ui_print("Installation aborted.\n");
    } else if (wipe_data) {
        if (device_wipe_data()) status = INSTALL_ERROR;
        if (erase_volume("/data")) status = INSTALL_ERROR;
        if (wipe_cache && erase_volume("/cache")) status = INSTALL_ERROR;
        if (status != INSTALL_SUCCESS) ui_print("Data wipe failed.\n");
    } else if (wipe_cache) {
        if (wipe_cache && erase_volume("/cache")) status = INSTALL_ERROR;
        if (status != INSTALL_SUCCESS) ui_print("Cache wipe failed.\n");
    } else {
        status = INSTALL_ERROR;  // No command specified
    }
#ifdef OTA
	if(ota) 
	{
		ui_print("OTA update ok, to set boot default!\n");
		copy_log_file(TEMPORARY_LOG_FILE, LAST_LOG_FILE, false);
    	chmod(LAST_LOG_FILE, 0640);
#ifdef POWER_CHECK
		if(pw_fd) close(pw_fd);
		if(ac_fd) close(ac_fd);
#endif
	}
#endif

	if(set_boot_default()){
        LOGE("set boot mode failed\n");
    }
    //if (status != INSTALL_SUCCESS) ui_set_background(BACKGROUND_ICON_ERROR);
    ui_set_background(BACKGROUND_ICON_INSTALLING);
#ifdef NS115
ui_show_text(1);
#endif
    if (status != INSTALL_SUCCESS || ui_text_visible()) {
        prompt_and_wait();
    }
#ifdef POWER_CHECK
	if(pw_fd)close(pw_fd);
	if(ac_fd) close(ac_fd);
#endif

    // Otherwise, get ready to boot the main system...
    finish_recovery(send_intent);
    ui_print("Rebooting, please wait...\n");
    //android_reboot(ANDROID_RB_RESTART2, 0, 0);
	system("/system/bin/reboot");
	reboot(RB_AUTOBOOT);
    return EXIT_SUCCESS;
}
