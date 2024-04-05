#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char const *argv[])
{
    int p = fork(); 
    int pgid;

    if (p == 0) {
        setpgid(0, 0);
        pgid = getpgid(0);
        printf("Child: pgid = %d\n", pgid);
        // sleep(10);
    } 

    else {
        pgid = getpgid(p);
        printf("Parent: pgid = %d\n", pgid);
        setpgid(0, 0);
        pgid = getpgid(p);
        printf("Parent: pgid = %d\n", pgid);
        wait(NULL);
    }
    return 0;
}
