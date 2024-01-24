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

    if(pid>0)
        write(p[1], msg, MAX);
    else
    {
        sleep(1);
        write(p[1], "bruh", MAX);

        for(int i=0;i<2;i++)
        {
            read(p[0],buff, MAX);
            printf("%s", buff);
        }
    }
}