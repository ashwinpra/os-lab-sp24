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

int terminated_processes = 0;

int main(int argc, char const *argv[])
{
    if (argc != 4) {
        printf("Invalid number of arguments\n");
        exit(1);
    }

    int msqid1 = atoi(argv[1]);
    int msqid2 = atoi(argv[2]);
    int k = atoi(argv[3]);

    key_t key = ftok("master.c", 107);

    key_t key2 = ftok("master.c", 106);
    int sem_sched = semget(key2, 1, IPC_CREAT | 0666);

    msq1_t msg1;
    msq2_t msg2;

    while (1) {
        // wait for process to come
        msgrcv(msqid1, (void *)&msg1, sizeof(int), 0, 0);
        int pid = msg1.pid;

        printf("Scheduler: Process %d has arrived\n", pid);

        // get the semaphore corresponding to the process
        int sem_proc = semget(key+pid, 1, IPC_CREAT | 0666);

        // signal process to start
        V(sem_proc);

        // wait for process to finish
        msgrcv(msqid2, (void *)&msg2, sizeof(msq2_t) - sizeof(long), 0, 0);

        if(msg2.type == 1) {
            printf("Message Type 1\n");
            printf("Page fault handled for process %d\n", msg2.pid);
            printf("Scheduler: Process %d has been re-added to ready queue\n", msg2.pid);
            msg1.pid = msg2.pid;
            msg1.type = 1;
            msgsnd(msqid1, (void *)&msg1, sizeof(int), 0);
        }

        else if(msg2.type == 2) {
            printf("Message Type 2\n");
            printf("Scheduler: Process %d has terminated\n", msg2.pid);
            terminated_processes++;
        }

        if(terminated_processes >= k) break;
    }

    V(sem_sched);

    return 0;
}
