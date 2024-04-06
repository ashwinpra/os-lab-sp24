#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <limits.h>

struct sembuf pop = {0, -1, 0};
struct sembuf vop = {0, 1, 0};

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

typedef struct _msq1_t {
    long type;
    int pid;
} msq1_t;


typedef struct _msq2_t {
    long type;
    int pid;
} msq2_t;

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        printf("Invalid number of arguments\n");
        exit(1);
    }

    int msqid1 = atoi(argv[1]);
    int msqid2 = atoi(argv[2]);

    printf("msqid1= %d, msqid2= %d\n", msqid1, msqid2);

    key_t key = ftok("master.c", 107);

    key_t key2 = ftok("master.c", 106);
    int sem_sched = semget(key2, 1, IPC_CREAT | 0666);

    // todo: check if empty and wait

    msq1_t msg1;
    msq2_t msg2;

    while (1) {
        // wait for process to come
        msgrcv(msqid1, (void *)&msg1, sizeof(int), 0, 0);
        int pid = msg1.pid;

        printf("Scheduler: Process %d has arrived\n", pid);

        //   msgrcv(msqid1, (void *)&msg1, sizeof(int), 0, 0);
        //  pid = msg1.pid;

        // printf("Scheduler: Process %d has arrived\n", pid);

        //   msgrcv(msqid1, (void *)&msg1, sizeof(int), 0, 0);
        //  pid = msg1.pid;

        // printf("Scheduler: Process %d has arrived\n", pid);

        // get the semaphore corresponding to the process
        int sem_proc = semget(key+pid, 1, IPC_CREAT | 0666);

        printf("Scheduler: Waiting for process %d to start\n", pid);

        // signal process to start
        printf("Signalling process %d\n", pid);
        V(sem_proc);

        // wait for process to finish
        printf("Waiting for message\n");
        msgrcv(msqid2, (void *)&msg2, sizeof(msq2_t) - sizeof(long), 0, 0);

        printf("Received message from process %d, type = %ld\n", msg2.pid, msg2.type);

        if(msg2.type == 1) {
            printf("Scheduler: Process %d has been re-added to ready queue\n", msg2.pid);
            msg1.pid = msg2.pid;
            msg1.type = 1;
            int val=msgsnd(msqid1, (void *)&msg1, sizeof(int), 0);
            printf("msgsnd returned %d\n", val);
        }

        else if(msg2.type == 2) {
            printf("Scheduler: Process %d has finished\n", msg2.pid);
        }

        // todo: check termination condition
    }

    V(sem_sched);

    return 0;
}
