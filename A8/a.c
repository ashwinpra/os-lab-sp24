#include <stdio.h> 
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _msq1_t {
    long type;
    int pid;
} msq1_t;

int main(int argc, char const *argv[])
{
    // make message queue 
    key_t key = ftok("master.c", 200);
    int msqid = msgget(key, IPC_CREAT | 0666);

    msq1_t msg1;
    msg1.type = 1;
    msg1.pid = 12; 

    // send message
    msgsnd(msqid, (void *)&msg1, sizeof(msq1_t) - sizeof(long), 0);
    
    printf("Sent message\n");

    sleep(10);


    return 0;
}
