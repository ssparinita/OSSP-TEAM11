#include<stdio.h>
#include<unistd.h>
int main()
{
pid_t pid;
pid = fork();
if(pid == 0)
printf("Jai Babu"\n);
else
printf("Babu lake Babu Mahesh Babu\n");
return 0;
}
