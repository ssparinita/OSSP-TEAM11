#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 20

int main() {
    char input[200];
    char *args[MAX_ARGS];
    int count = 0;

    printf("Enter a command with arguments: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';

    char *token = strtok(input, " ");

    while (token != NULL && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " ");
    }

    args[count] = NULL;

    if (count == 0) {
        printf("No command entered.\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Child process created. PID: %d\n", getpid());
        printf("Executing command: %s\n", args[0]);

        execvp(args[0], args);

        perror("Execution failed");
        exit(1);
    }

    printf("Parent process. PID: %d\n", getpid());
    printf("Waiting for child process...\n");

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child process finished with status: %d\n",
               WEXITSTATUS(status));
    }

    return 0;
}
