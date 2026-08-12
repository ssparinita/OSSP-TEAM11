#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid1, pid2, pid3;
    int status;

    printf("Parent PID : %d\n", getpid());

    pid1 = fork();

    if (pid1 == 0)
    {
        printf("\nChild 1\n");
        printf("PID  : %d\n", getpid());
        printf("PPID : %d\n", getppid());
        sleep(2);
        printf("Child 1 completed.\n");
        exit(0);
    }

    pid2 = fork();

    if (pid2 == 0)
    {
        printf("\nChild 2\n");
        printf("PID  : %d\n", getpid());
        printf("PPID : %d\n", getppid());
        sleep(3);
        printf("Child 2 completed.\n");
        exit(0);
    }

    pid3 = fork();

    if (pid3 == 0)
    {
        printf("\nChild 3\n");
        printf("PID  : %d\n", getpid());
        printf("PPID : %d\n", getppid());
        sleep(1);
        printf("Child 3 completed.\n");
        exit(0);
    }

    printf("\nParent waiting using wait()...\n");

    wait(&status);
    printf("One child process completed using wait().\n");

    printf("\nParent waiting for Child 2 using waitpid()...\n");

    waitpid(pid2, &status, 0);
    printf("Child 2 with PID %d completed using waitpid().\n", pid2);

    waitpid(pid3, &status, 0);
    printf("Child 3 with PID %d completed.\n", pid3);

    printf("\nAll child processes completed.\n");

    return 0;
}
