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

#define PROB_ILLEGAL 0.1
#define T 250000

typedef struct _pagetable_t {
    int pid;
    int m_i; // number of required pages
    int f_i; // number of allocated frames
    int **PT; // Page Table
    int total_PFs;
    char *ref_str; // reference string
    // todo: illegal accesses?
} pagetable_t;


int main(){
    srand(time(0));

    int k, m, f;
    printf("Enter the number of processes: ");
    scanf("%d", &k);
    printf("Enter the Virtual Address Space size: ");
    scanf("%d", &m);
    printf("Enter the Physical Address Space size: ");
    scanf("%d", &f);

    // page table
    key_t key1 = ftok("master.c", 100);
    int shmid1 = shmget(key1, k * sizeof(pagetable_t), IPC_CREAT | 0666);
    pagetable_t *SM1 = (pagetable_t *)shmat(shmid1, NULL, 0);

    for(int i=0; i<k; i++){
        SM1[i].m_i = rand() % m + 1;
        SM1[i].f_i = 0;
        SM1[i].PT = (int **)malloc(m * sizeof(int *));
        for(int j=0; j<m; j++){
            SM1[i].PT[j] = (int *)malloc(3 * sizeof(int));
            // todo: check
            SM1[i].PT[j][0] = -1;
            SM1[i].PT[j][1] = 0;
            SM1[i].PT[j][2] = INT_MAX;
        }
        SM1[i].total_PFs = 0;

        // generate reference string : numbers separated by ":"
        // length between 2*m_i and 10*m_i
        int x = rand() % (8 *SM1[i].m_i + 1) + 2 * SM1[i].m_i;
        SM1[i].ref_str = (char *)malloc(4 * x * sizeof(char));
        for(int j=0; j<x; j++){
            if( (float)rand()/(float)RAND_MAX < PROB_ILLEGAL){
                if(j==0){
                    sprintf(SM1[i].ref_str, "%d", rand() % m);
                }
                else{
                    sprintf(SM1[i].ref_str, "%s:%d", SM1[i].ref_str, rand() % m);
                }
            }
            else{
                if(j==0){
                    sprintf(SM1[i].ref_str, "%d", rand() % m);
                }
                else{
                    sprintf(SM1[i].ref_str, "%s:%d", SM1[i].ref_str, rand() % m);
                }
            }
        }
    }

    // // print everything for checking
    // for(int i=0; i<k; i++){
    //     printf("Process %d\n", i);
    //     printf("m_i: %d\n", SM1[i].m_i);
    //     printf("f_i: %d\n", SM1[i].f_i);
    //     printf("PT: \n");
    //     for(int j=0; j<SM1[i].m_i; j++){
    //         printf("%d %d %d\n", SM1[i].PT[j][0], SM1[i].PT[j][1], SM1[i].PT[j][2]);
    //     }
    //     printf("total_PFs: %d\n", SM1[i].total_PFs);
    //     printf("ref_str: %s\n", SM1[i].ref_str);
    // }

    // free frames list
    key_t key2 = ftok("master.c", 101);
    int shmid2 = shmget(key2, f * sizeof(int), IPC_CREAT | 0666);
    int *SM2 = (int *)shmat(shmid2, NULL, 0);   

    // 1 means free, 0 means occupied
    for (int i=0; i<f; i++) {
        SM2[i] = 1;
    }

    // process to page number mapping
    key_t key3 = ftok("master.c", 102);
    int shmid3 = shmget(key3, k * sizeof(int), IPC_CREAT | 0666);
    int *SM3 = (int *)shmat(shmid3, NULL, 0);

    for (int i = 0; i < k; i++) {
        SM3[i] = 0;
    }

    // ready queue (message queue)
    key_t key4 = ftok("master.c", 103);
    int msqid1 = msgget(key4, IPC_CREAT | 0666);

    // message queue for communication between scheduler and MMU
    key_t key5 = ftok("master.c", 104);
    int msqid2 = msgget(key5, IPC_CREAT | 0666);

    // message queue for communication between processes and MMU
    key_t key6 = ftok("master.c", 105);
    int msqid3 = msgget(key6, IPC_CREAT | 0666);

    char SM1_str[10], SM2_str[10], SM3_str[10], msqid1_str[10], msqid2_str[10], msqid3_str[10];
    sprintf(SM1_str, "%d", shmid1);
    sprintf(SM2_str, "%d", shmid2);
    sprintf(SM3_str, "%d", shmid3);
    sprintf(msqid1_str, "%d", msqid1);
    sprintf(msqid2_str, "%d", msqid2);
    sprintf(msqid3_str, "%d", msqid3);


    // // create scheduler
    // if(fork()==0){
    //     execlp("xterm", "xterm", "-T", "Scheduler", "-e", "./scheduler", msqid1_str, msqid2_str, NULL);
    // }

    // // create MMU
    // if(fork()==0){
    //     execlp("xterm", "xterm", "-T", "MMU", "-e", "./mmu", msqid2_str, msqid3_str, SM1_str, SM2_str, NULL);
    // }

    // // make k processes
    // for(int i=0; i<k; i++){
    //     usleep(T);
    //     // generate reference string as string, numbers separated by ":"
    //     int pid = fork(); 
    //     if(pid!=0){
    //         SM1[i] = pid;
    //     }
    //     if(pid==0){
    //         execlp("xterm", "xterm", "-T", "Process", "-e", "./process", SM1[i].ref_str, msqid1_str, msqid3_str, NULL);
    //     }

    // }


}