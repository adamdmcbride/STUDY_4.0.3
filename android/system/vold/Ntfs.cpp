/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Ntfs.h"
static char NTFS_3G_PATH[] = "/system/bin/ntfs-3g";
static char NTFS_FIX_PATH[] = "/system/bin/ntfsfix";
extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Ntfs::fix(const char *fsPath) {
  
    // no NTFS file system check is performed, always return true

    int rc = 0;
    const char *args[3];
    args[0] = NTFS_FIX_PATH;
    args[1] = fsPath;
    args[2] = NULL;

    rc = logwrap(2, args, 1);
    return rc;

}

int Ntfs::doMount(const char *fsPath, const char *mountPoint, bool createLost) {

    int rc = 0;
    const char *args[4];
    args[0] = NTFS_3G_PATH;
    args[1] = fsPath;
    args[2] = mountPoint;
    args[3] = NULL;

    rc = logwrap(3, args, 1);

    if (rc == 0 && createLost) {
        char *lost_path;
        asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
        if (access(lost_path, F_OK)) {
            /*
             * Create a LOST.DIR in the root so we have somewhere to put
             * lost cluster chains (fsck_msdos doesn't currently do this)
             */
            if (mkdir(lost_path, 0755)) {
                SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
            }
        }
        free(lost_path);
    }

    return rc;
}


int Ntfs::format(const char *fsPath, unsigned int numSectors) {
    
    SLOGE("Format ntfs filesystem not supported\n");
    errno = EIO;
    return -1;

}
