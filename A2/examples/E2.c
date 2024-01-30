#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

void abc (int sig) {
    printf("RIP child\n");
}

int main()
{ 
    int pid = fork();
    if( pid == 0 ) sleep(5);
    else
    {
        signal(SIGCHLD, abc);
        sleep(10);
        printf("Parent exited");
    }
}

