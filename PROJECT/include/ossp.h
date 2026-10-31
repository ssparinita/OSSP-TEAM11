#ifndef OSSP_H
#define OSSP_H

#include <stddef.h>
#include <sys/types.h>

#define PATH_MAX_LEN 4096
#define NAME_MAX_LEN 256
#define EVENT_MAX_LEN 8192

typedef struct {
    char source[PATH_MAX_LEN];
    char backup[PATH_MAX_LEN];
    char versions[PATH_MAX_LEN];
    char log_file[PATH_MAX_LEN];
    char events_file[PATH_MAX_LEN];
    char pid_file[PATH_MAX_LEN];
} AppConfig;

void timestamp_now(char *buf, size_t size);
int ensure_dir(const char *path);
int ensure_parent_dir(const char *path);
int copy_file(const char *src, const char *dst);
int write_json_event(const AppConfig *cfg, const char *type, const char *file,
                     const char *stage, const char *syscalls);
int write_log(const AppConfig *cfg, const char *message);
int is_regular_file(const char *path);

int start_daemon(const AppConfig *cfg);
int stop_daemon(const AppConfig *cfg);
int run_watcher(const AppConfig *cfg);

#endif
