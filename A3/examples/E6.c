#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
    FILE* fp; 
    int status; 
    char path[100];

    fp = popen("ls -l", "r");

    if (fp == NULL){
        printf("Failed to run command\n");
        exit(1);
    }
    while (fgets(path, 100, fp) != NULL) printf("%s woo\n", path);

    status = pclose(fp);

    if (status == -1){
        printf("Error in closing the pipe\n");
        exit(1);
    }

    return 0;
}