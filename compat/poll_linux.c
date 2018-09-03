/*
 *  poll_linux.c
 *  author: Aleksei Kozadaev (2016)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#include "../imk.h"
#include "../log.h"

#define EVENT_SIZE      (sizeof(struct inotify_event))
#define BUF_LEN         (8 * (EVENT_SIZE + 16))

#define FILTERS         (IN_DELETE_SELF | IN_MOVE_SELF | IN_MODIFY | IN_ONESHOT)

static int g_ifd = -1;

ARRAY_FUNCS(fd, int)  // array handling functions

int set_watch(const char *path);
static void sig_handler(int);

int
fd_register(struct config *cfg, const char *path)
{
    int rv = 0;

    if (g_ifd == -1) {
        signal(SIGINT, sig_handler);
        signal(SIGTERM, sig_handler);

        if ((g_ifd = inotify_init()) == -1) {
            LOG_PERROR("inotify_init");
            exit(1);
        }

        array_fd_init(&cfg->fds);
    }

    struct stat st;
    if ((rv = stat(path, &st)) != 0) {
        return -1;
    }

    rv = set_watch(path);

    array_fd_append(&cfg->fds, rv);
    return rv;
}

int
fd_dispatch(const struct config *cfg)
{
    int len;
    char buf[BUF_LEN];
    time_t next = { 0 };

    for (;;) {
        if ((len = read(g_ifd, buf, BUF_LEN)) == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            LOG_PERROR("read");
            exit(1);
        }

        if (len == 0) {
            continue;
        }

        for (int i = 0; i < len;) {
            struct inotify_event *ev = (struct inotify_event *)&buf[i];

            /* sometimes IN_DELETE_SELF or IN_MOVE_SELF mean the file is being
             * processed by some program (eg. vim or gofmt), so if imk tries
             * to reattach to the file immediately it may not exist. So sleep
             * for a while before try to reattach to the file.*/
            if ((ev->mask & IN_DELETE_SELF) || (ev->mask & IN_MOVE_SELF)) {
                usleep(cfg->sleepDelete * 1000);
            }

            int idx;
            for (idx = 0; idx < cfg->fds.size; ++idx) {
                if (cfg->fds.data[idx] == ev->wd) {
                    break;
                }
            }

            if (idx == cfg->fds.size) {
                break;  /* not found */
            }

            LOG_INFO_VA("[====== %s (%u) =====]", cfg->files[idx], ev->wd);
            cfg->fds.data[idx] = set_watch(cfg->files[idx]);

            i += EVENT_SIZE + ev->len;
        }

        if (time(NULL) > next || cfg->onerun) {
            int rv = run_command(cfg->cmd);

            if (cfg->onerun) {
                exit(rv);
            }

            next = time(NULL) + cfg->threshold;
        }
    }

    return 0;
}

void
fd_close(struct config *cfg)
{
    if (g_ifd != -1) {
        close(g_ifd);
    }

    for (int i = 0; i < cfg->fds.size; ++i) {
        close(cfg->fds.data[i]);
    }

    array_fd_free(&cfg->fds);
}

int
set_watch(const char *path)
{
    int rv = inotify_add_watch(g_ifd, path, FILTERS);

    if (rv == -1) {
        LOG_PERROR("inotify_add_watch");
        exit(1);
    }

    return rv;
}

void
sig_handler(int sig)
{
    LOG_ERR("interrupted");
    exit(0);
}

