#include "ossp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

static void usage(const char *p) {
    printf("Usage:\n");
    printf("  %s <source> <backup>              Start daemon\n", p);
    printf("  %s <source> <backup> --foreground Run in foreground\n", p);
    printf("  %s <source> <backup> --stop       Stop daemon\n", p);
}

static void absolute_path(const char *in, char *out, size_t cap) {
    if (realpath(in, out) == NULL) snprintf(out, cap, "%s", in);
}

int main(int argc, char **argv) {
    if (argc < 3) { usage(argv[0]); return 1; }

    AppConfig cfg = {0};
    absolute_path(argv[1], cfg.source, sizeof(cfg.source));
    absolute_path(argv[2], cfg.backup, sizeof(cfg.backup));

    /* Project-local runtime files. */
    snprintf(cfg.versions, sizeof(cfg.versions), "%s/../versions", cfg.backup);
    snprintf(cfg.log_file, sizeof(cfg.log_file), "%s/../logs/backup.log", cfg.backup);
    snprintf(cfg.events_file, sizeof(cfg.events_file), "%s/../logs/events.jsonl", cfg.backup);
    snprintf(cfg.pid_file, sizeof(cfg.pid_file), "%s/../logs/backup.pid", cfg.backup);

    ensure_dir(cfg.backup);
    ensure_dir(cfg.versions);
    {
        char logdir[PATH_MAX_LEN];
        snprintf(logdir, sizeof(logdir), "%s/../logs", cfg.backup);
        ensure_dir(logdir);
    }

    if (argc >= 4 && strcmp(argv[3], "--stop") == 0) {
        if (stop_daemon(&cfg) == 0) printf("Backup daemon stop requested.\n");
        else printf("No running daemon found.\n");
        return 0;
    }

    if (argc >= 4 && strcmp(argv[3], "--foreground") == 0) {
        printf("Running backup monitor in foreground.\n");
        printf("Source: %s\nBackup: %s\n", cfg.source, cfg.backup);
        fflush(stdout);
        return run_watcher(&cfg);
    }

    printf("Starting Linux backup daemon...\n");
    if (start_daemon(&cfg) != 0) {
        perror("start_daemon");
        return 1;
    }
    printf("Daemon started. PID file: %s\n", cfg.pid_file);
    return 0;
}
