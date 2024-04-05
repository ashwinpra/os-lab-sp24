#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int i, n; 
    scanf("%d", &n);

    for(i=0; i<n; i++){
        int ret = fork(); 
        if(ret==0) printf("child %d\n", i);
        else printf("parent %d\n", i);
    }

    // printf("Hello!\n");

    exit(0);
}
