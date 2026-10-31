#include "ossp.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

static void redirect_stdio(const char *log_file) {
    int fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}

int start_daemon(const AppConfig *cfg) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) return 0; /* parent returns immediately */

    if (setsid() < 0) _exit(1);

    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) _exit(1);
    if (pid > 0) _exit(0);

    umask(0);
    chdir("/");

    redirect_stdio(cfg->log_file);
    return run_watcher(cfg);
}

int stop_daemon(const AppConfig *cfg) {
    FILE *fp = fopen(cfg->pid_file, "r");
    if (!fp) return -1;
    int pid = 0;
    fscanf(fp, "%d", &pid);
    fclose(fp);
    if (pid <= 0) return -1;
    if (kill((pid_t)pid, SIGTERM) != 0) return -1;
    return 0;
}
