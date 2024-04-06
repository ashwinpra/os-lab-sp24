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
    // printf("argc: %d\n", argc);
    // while(1);
    if (argc != 5) {
        printf("Invalid number of arguments\n");
        exit(1);
    }   

    char ref_str[1000];
    strcpy(ref_str, argv[1]);
    // printf("Received ref_string: %s\n", ref_str);
    int msqid1 = atoi(argv[2]);
    int msqid3 = atoi(argv[3]);
    int pid = atoi(argv[4]);

    printf("msqid1= %d, msqid3= %d, pid= %d\n", msqid1, msqid3, pid);

    printf("Process [%d] has started\n", pid);
    // while(1);

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
    printf("Waiting to be scheduled\n");
    P(sem_sched);
    printf("Done waiting\n");
    // sleep(5);

    char* token = strtok(ref_str, ":");
    
    while (token != NULL) {
        // sleep(5);
        printf("Sending page %d\n", atoi(token));
        msq3_t msg3;
        msg3.type = 1;
        msg3.num = atoi(token);
        msg3.pid = pid;

        printf("Sending [%d] %d", msg3.pid, msg3.num);
        msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 0);

        // wait for mmu to allocate frame 
        msgrcv(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 2, 0);
        printf("Received: [%d] %d\n",msg3.pid, msg3.num);
        // sleep(1);

        if(msg3.num == -1) {
            /*
                It gets –1 as frame number from MMU and saves the current element of the reference
                string for continuing its execution when it is scheduled next and goes into wait. MMU
                invokes PFH routine to handle the page fault. Please note that the current process is
                out of ready queue and scheduler enqueues it to the ready queue once page fault is
                resolved
            */
            printf("Process %d: Page fault, waiting to be loaded\n", pid);
            //todo: wait for page fault handling
            P(sem_sched);
            printf("Loaded again\n");
            continue;
        }

        else if(msg3.num == -2) {
            printf("Process %d: invalid page reference\n", pid);
            exit(1);

        }

        token = strtok(NULL, ":");
        printf("New token %s generated\n", token);
    }

    printf("Process %d: Execution complete\n", pid);
    msq3_t msg3;
    msg3.type = 1;
    msg3.num = -9; 
    msg3.pid = pid;

    printf("Sending [%d] %d", msg3.pid, msg3.num);
    msgsnd(msqid3, (void *)&msg3, sizeof(msq3_t) - sizeof(long), 0);

    //free the page table frames
    //free the frames
    
    printf("Process %d: Terminating\n", pid);

    sleep(10);

    return 0;
}
