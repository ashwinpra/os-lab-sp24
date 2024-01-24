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

    if(pid==0)
        printf("child exiting...\n");
    else
    {
        close(p[1]); // if this is not there, it will keep waiting
        read(p[0],buff, MAX);
    }
}