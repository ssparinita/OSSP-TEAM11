#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Child process started.\n");
        printf("Child PID: %d\n", getpid());

        printf("Child is working...\n");
        sleep(3);

        printf("Child process completed.\n");

        exit(42);
    }

    printf("Parent process started.\n");
    printf("Parent PID: %d\n", getpid());
    printf("Waiting for child using waitpid()...\n");

    int status;

    pid_t result = waitpid(pid, &status, 0);

    if (result == -1) {
        perror("waitpid failed");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("Child terminated normally.\n");
        printf("Child exit status: %d\n", WEXITSTATUS(status));
    }

    printf("Parent process completed.\n");

    return 0;
}
