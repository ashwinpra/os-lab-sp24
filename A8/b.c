#include <stdio.h> 
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>

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

    // receive message
    msgrcv(msqid, (void *)&msg1, sizeof(msq1_t) - sizeof(long), 0, 0);

    printf("Received: %d\n", msg1.pid);

    return 0;
}
