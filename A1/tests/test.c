#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int pid1, pid2, status; 

    if((pid1=fork())==0){
        // child 1 
        printf("Child 1: PID = %u, PPID = %u\n", getpid(), getppid());
        printf("pid1 = %d\n", pid1);
    }

    else{
        // parent 
        if((pid2=fork())==0){
            // child 2 
            printf("Child 2: PID = %u, PPID = %u\n", getpid(), getppid());
            printf("pid2 = %d\n", pid2);
            execlp("./hello", "./hello", NULL);
        }
        else{
            // parent 
            printf("Parent: PID = %u, PPID = %u\n", getpid(), getppid());
            printf("pid1 = %d, pid2 = %d\n", pid1, pid2);
        }
    }
    
    wait(&status);
    printf("PID = %u: Terminating...\n", getpid());
    exit(0);
}