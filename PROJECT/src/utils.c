#include "ossp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

void timestamp_now(char *buf, size_t size) {
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(buf, size, "%H:%M:%S", &tmv);
}

int ensure_dir(const char *path) {
    if (!path || !*path) return -1;
    char tmp[PATH_MAX_LEN];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (len > 1 && tmp[len-1] == '/') tmp[len-1] = '\0';

    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) == -1 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) == -1 && errno != EEXIST) return -1;
    return 0;
}

int ensure_parent_dir(const char *path) {
    char tmp[PATH_MAX_LEN];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *slash = strrchr(tmp, '/');
    if (!slash) return 0;
    if (slash == tmp) {
        tmp[1] = '\0';
    } else {
        *slash = '\0';
    }
    return ensure_dir(tmp);
}

int is_regular_file(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    if (ensure_parent_dir(dst) != 0) { close(in); return -1; }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }

    char buf[65536];
    ssize_t n;
    int rc = 0;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w < 0) { rc = -1; break; }
            off += w;
        }
        if (rc != 0) break;
    }
    if (n < 0) rc = -1;
    if (close(in) != 0) rc = -1;
    if (close(out) != 0) rc = -1;
    return rc;
}

static void json_escape(const char *src, char *dst, size_t cap) {
    size_t j = 0;
    for (size_t i = 0; src && src[i] && j + 2 < cap; ++i) {
        char c = src[i];
        if (c == '"' || c == '\\') {
            dst[j++] = '\\'; dst[j++] = c;
        } else if (c == '\n') {
            dst[j++] = '\\'; dst[j++] = 'n';
        } else if (c == '\r') {
            dst[j++] = '\\'; dst[j++] = 'r';
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

int write_log(const AppConfig *cfg, const char *message) {
    FILE *fp = fopen(cfg->log_file, "a");
    if (!fp) return -1;
    char t[32];
    timestamp_now(t, sizeof(t));
    fprintf(fp, "[%s] %s\n", t, message);
    fclose(fp);
    return 0;
}

int write_json_event(const AppConfig *cfg, const char *type, const char *file,
                     const char *stage, const char *syscalls) {
    FILE *fp = fopen(cfg->events_file, "a");
    if (!fp) return -1;

    char et[32], ef[NAME_MAX_LEN*2], es[256], ec[1024];
    timestamp_now(et, sizeof(et));
    json_escape(file, ef, sizeof(ef));
    json_escape(stage, es, sizeof(es));
    json_escape(syscalls, ec, sizeof(ec));

    fprintf(fp,
        "{\"timestamp\":\"%s\",\"type\":\"%s\",\"file\":\"%s\","
        "\"stage\":\"%s\",\"syscalls\":\"%s\"}\n",
        et, type, ef, es, ec);
    fflush(fp);
    fclose(fp);
    return 0;
}
