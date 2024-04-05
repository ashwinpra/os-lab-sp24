#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 100

int main()
{
    char *msg = "hello";
    char buf[MAX];
    int p[2];
    pipe(p);
    int pid=fork();

    if(pid==0) {
        sleep(5);
        printf("child exiting...\n");
    }
    else
    {
        // sleep(1);
        close(p[1]); 
        printf("Closed\n");
        int ret = read(p[0], buf, MAX);
        printf("ret = %d\n", ret);
    }
}