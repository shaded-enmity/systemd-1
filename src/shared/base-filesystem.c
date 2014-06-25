/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2014 Kay Sievers

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "base-filesystem.h"
#include "log.h"
#include "macro.h"
#include "util.h"
#include "label.h"
#include "mkdir.h"

typedef struct BaseFilesystem {
        const char *dir;
        mode_t mode;
        const char *target;
} BaseFilesystem;

static const BaseFilesystem table[] = {
        { "bin",      0, "usr/bin"   },
        { "lib",      0, "usr/lib"   },
        { "lib64",    0, "usr/lib64" },
        { "root",  0755, NULL },
        { "sbin",     0, "usr/sbin"  },
};

int base_filesystem_create(const char *root) {
        _cleanup_close_ int fd = -1;
        unsigned i;
        int r;

        fd = open(root, O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if (fd < 0)
                return -errno;

        for (i = 0; i < ELEMENTSOF(table); i ++) {
                if (table[i].target) {
                        /* check if target exists */
                        if (faccessat(fd, table[i].target, F_OK, AT_SYMLINK_NOFOLLOW) < 0)
                                continue;

                        r = symlinkat(table[i].target, fd, table[i].dir);
                        if (r < 0 && errno != EEXIST) {
                                log_error("Failed to create symlink at %s/%s: %m", root, table[i].dir);
                                return -errno;
                        }
                        continue;
                }

                RUN_WITH_UMASK(0000)
                        r = mkdirat(fd, table[i].dir, table[i].mode);
                if (r < 0 && errno != EEXIST) {
                        log_error("Failed to create directory at %s/%s: %m", root, table[i].dir);
                        return -errno;
                }
        }

        return 0;
}