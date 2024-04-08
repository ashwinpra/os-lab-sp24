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
   
    if (argc != 5) {
        printf("Invalid number of arguments\n");
        exit(1);
    }   

    char ref_str[1000];
    strcpy(ref_str, argv[1]);
    int msqid1 = atoi(argv[2]);
    int msqid3 = atoi(argv[3]);
    int pid = atoi(argv[4]);

    key_t key = ftok("master.c", 110);
    int semid = semget(key, 1, IPC_CREAT | 0666);

    key_t key2 = ftok("master.c", 107); 

    msq1_t msg1; 
    msg1.type = 1;
    msg1.pid = pid;

    // send to ready queue 
    msgsnd(msqid1, (void *)&msg1, sizeof(msg1.pid), 0);

    // get the semaphore corresponding to the process
    int sem_sched = semget(key2+pid, 1, IPC_CREAT | 0666);

    // wait for scheduler to schedule this process 
    P(sem_sched);

    char* token = strtok(ref_str, ":");
    
    while (token != NULL) {
        msq3_t msg3;
        msg3.type = 1;
        msg3.num = atoi(token);
        msg3.pid = pid;

        msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 0);

        // wait for mmu to allocate frame 
        msgrcv(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 2, 0);

        if(msg3.num == -1) {
            printf("Process %d: Page fault, waiting to be loaded\n", pid);
            //wait for page fault handling
            P(sem_sched);
            printf("Loaded again\n");
            continue;
        }

        else if(msg3.num == -2) {
            printf("Process %d: invalid page reference\n", pid);
            exit(1);

        }

        token = strtok(NULL, ":");
    }

    printf("Process %d: Execution complete\n", pid);
    msq3_t msg3;
    msg3.type = 1;
    msg3.num = -9; 
    msg3.pid = pid;

    msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 0);

    printf("Process %d: Terminating\n", pid);

    return 0;
}
