#include "ossp.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

static int make_version_path(const AppConfig *cfg, const char *file,
                             char *out, size_t cap) {
    char t[32];
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(t, sizeof(t), "%Y%m%d_%H%M%S", &tmv);
    snprintf(out, cap, "%s/%s_%s", cfg->versions, file, t);
    return 0;
}

static int backup_path(const AppConfig *cfg, const char *file,
                       char *out, size_t cap) {
    snprintf(out, cap, "%s/%s", cfg->backup, file);
    return 0;
}

int backup_create(const AppConfig *cfg, const char *file) {
    char src[PATH_MAX_LEN], dst[PATH_MAX_LEN];
    snprintf(src, sizeof(src), "%s/%s", cfg->source, file);
    backup_path(cfg, file, dst, sizeof(dst));

    write_json_event(cfg, "CREATE", file, "DETECTED", "inotify");
    write_log(cfg, "CREATE detected");

    struct stat st;
    if (stat(src, &st) != 0) return -1;
    write_json_event(cfg, "CREATE", file, "CHECKING", "inotify,stat()");
    write_log(cfg, "stat() metadata checked");

    write_json_event(cfg, "CREATE", file, "COPYING", "inotify,stat(),open(),read(),write()");
    if (copy_file(src, dst) != 0) return -1;

    write_json_event(cfg, "CREATE", file, "SYNCHRONIZED",
                     "inotify,stat(),open(),read(),write(),close()");
    write_log(cfg, "BACKUP COMPLETE");
    return 0;
}

int backup_modify(const AppConfig *cfg, const char *file) {
    char src[PATH_MAX_LEN], dst[PATH_MAX_LEN], ver[PATH_MAX_LEN];
    snprintf(src, sizeof(src), "%s/%s", cfg->source, file);
    backup_path(cfg, file, dst, sizeof(dst));

    write_json_event(cfg, "MODIFY", file, "DETECTED", "inotify");
    write_log(cfg, "MODIFY detected");

    struct stat st;
    if (stat(src, &st) != 0) return -1;
    write_json_event(cfg, "MODIFY", file, "CHECKING",
                     "inotify,stat()");

    if (access(dst, F_OK) == 0) {
        make_version_path(cfg, file, ver, sizeof(ver));
        if (copy_file(dst, ver) == 0) {
            write_json_event(cfg, "MODIFY", file, "VERSIONING",
                             "open(),read(),write(),close()");
            write_log(cfg, "previous backup version saved");
        }
    }

    write_json_event(cfg, "MODIFY", file, "COPYING",
                     "inotify,stat(),open(),read(),write()");
    if (copy_file(src, dst) != 0) return -1;

    write_json_event(cfg, "MODIFY", file, "SYNCHRONIZED",
                     "inotify,stat(),open(),read(),write(),close()");
    write_log(cfg, "BACKUP UPDATED");
    return 0;
}

int backup_delete(const AppConfig *cfg, const char *file) {
    char dst[PATH_MAX_LEN], ver[PATH_MAX_LEN];
    backup_path(cfg, file, dst, sizeof(dst));

    write_json_event(cfg, "DELETE", file, "DETECTED", "inotify");
    write_log(cfg, "DELETE detected");

    /* Preserve the last synchronized backup before removing the active copy. */
    if (access(dst, F_OK) == 0) {
        make_version_path(cfg, file, ver, sizeof(ver));
        strncat(ver, "_DELETED", sizeof(ver) - strlen(ver) - 1);
        write_json_event(cfg, "DELETE", file, "ARCHIVING",
                         "inotify,open(),read(),write(),close()");
        if (copy_file(dst, ver) != 0) {
            write_log(cfg, "failed to archive deleted file");
            write_json_event(cfg, "DELETE", file, "ARCHIVE_FAILED",
                             "inotify,open(),read(),write(),close()");
            return -1;
        }
        write_log(cfg, "deleted file archived to version history");
    }

    if (unlink(dst) == 0 || access(dst, F_OK) != 0) {
        write_json_event(cfg, "DELETE", file, "REMOVED",
                         "inotify,open(),read(),write(),close(),unlink()");
        write_log(cfg, "BACKUP REMOVED; deleted version preserved");
        return 0;
    }
    write_log(cfg, "failed to remove backup");
    return -1;
}
