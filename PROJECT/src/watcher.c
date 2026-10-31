#include "ossp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

/* backup functions */
int backup_create(const AppConfig *, const char *);
int backup_modify(const AppConfig *, const char *);
int backup_delete(const AppConfig *, const char *);

static volatile sig_atomic_t running = 1;
static void handle_stop(int sig) { (void)sig; running = 0; }

int run_watcher(const AppConfig *cfg) {
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        perror("inotify_init1");
        return 1;
    }

    int wd = inotify_add_watch(fd, cfg->source,
        IN_CREATE | IN_CLOSE_WRITE | IN_MODIFY | IN_DELETE | IN_MOVED_TO);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return 1;
    }

    FILE *pf = fopen(cfg->pid_file, "w");
    if (pf) { fprintf(pf, "%d\n", getpid()); fclose(pf); }

    signal(SIGTERM, handle_stop);
    signal(SIGINT, handle_stop);

    write_log(cfg, "monitor started • inotify watch active");
    write_json_event(cfg, "SYSTEM", "", "MONITORING", "inotify_init(),inotify_add_watch()");

    char buffer[16384] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (running) {
        ssize_t len = read(fd, buffer, sizeof(buffer));
        if (len < 0) {
            if (errno == EAGAIN || errno == EINTR) { usleep(100000); continue; }
            break;
        }
        if (len == 0) continue;

        for (char *ptr = buffer; ptr < buffer + len; ) {
            struct inotify_event *ev = (struct inotify_event *)ptr;
            if (ev->len > 0 && ev->name[0] != '\0') {
                char full[PATH_MAX_LEN];
                snprintf(full, sizeof(full), "%s/%s", cfg->source, ev->name);

                if (ev->mask & IN_ISDIR) {
                    ptr += sizeof(struct inotify_event) + ev->len;
                    continue;
                }

                if (ev->mask & IN_CREATE) {
                    if (is_regular_file(full)) backup_create(cfg, ev->name);
                } else if (ev->mask & IN_CLOSE_WRITE) {
                    if (is_regular_file(full)) backup_modify(cfg, ev->name);
                } else if (ev->mask & IN_MODIFY) {
                    /* CLOSE_WRITE is used as the main modification trigger. */
                } else if (ev->mask & IN_DELETE) {
                    backup_delete(cfg, ev->name);
                } else if (ev->mask & IN_MOVED_TO) {
                    if (is_regular_file(full)) backup_create(cfg, ev->name);
                }
            }
            ptr += sizeof(struct inotify_event) + ev->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    unlink(cfg->pid_file);
    write_log(cfg, "monitor stopped");
    write_json_event(cfg, "SYSTEM", "", "STOPPED", "inotify_rm_watch()");
    return 0;
}
