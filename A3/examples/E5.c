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
    char buff[MAX];
    int p[2];
    pipe(p);
    int pid=fork();

    if(pid==0) {
        printf("child exiting...\n");
    }
    else
    {
        sleep(1); // child will exit before parent writes, so there is no reader
        close(p[0]); 
        int ret = write(p[1], msg, MAX);
        printf("ret = %d\n", ret);
    }
}