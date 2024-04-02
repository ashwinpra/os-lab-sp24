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

typedef struct _msq3_t {
    long type;
    int num; // page or frame number
    int pid;
} msq3_t;

int main(int argc, char const *argv[])
{
    if (argc != 4) {
        printf("Invalid number of arguments\n");
        exit(1);
    }

    char *ref_str;
    strcpy(ref_str, argv[1]);
    int msqid1 = atoi(argv[2]);
    int msqid3 = atoi(argv[3]);

    printf("Process has started\n");

    key_t key = ftok("master.c", 110);
    int semid = semget(key, 1, IPC_CREAT | 0666);

    msq1_t msg1; 
    msg1.type = 1;
    msg1.pid = getpid();

    // send to ready queue 
    msgsnd(msqid1, (void *)&msg1, sizeof(msq1_t), 0);

    // wait for scheduler to schedule this process 
    P(semid);

    // send reference string to scheduler, one number at a time
    // do strtok on ref_str until NULL
    char *token = strtok(ref_str, " ");
    while (token != NULL) {
        msq3_t msg3;
        msg3.type = 1;
        msg3.num = atoi(token);
        msg3.pid = getpid();
        msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t), 0);

        // wait for mmu to allocate frame 
        msgrcv(msqid3, (void *)&msg3, sizeof(msq3_t), 0, 0);

        if(msg3.num == -1) {
            /*
                It gets –1 as frame number from MMU and saves the current element of the reference
                string for continuing its execution when it is scheduled next and goes into wait. MMU
                invokes PFH routine to handle the page fault. Please note that the current process is
                out of ready queue and scheduler enqueues it to the ready queue once page fault is
                resolved
            */
           printf("Process %d: Page fault, waiting to be loaded\n", getpid());
           //todo: wait for page fault handling
           continue;

        }

        else if(msg3.num == -2) {
            printf("Process %d: invalid page reference\n", getpid());
            exit(1);

        }

        token = strtok(NULL, " ");
    }

    printf("Process %d: Execution complete\n", getpid());
    msq3_t msg3;
    msg3.type = 1;
    msg3.num = -9; 
    msg3.pid = getpid();

    msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t), 0);
    
    printf("Process %d: Terminating\n", getpid());

    return 0;
}
