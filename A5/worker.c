#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

int main(int argc, char const *argv[])
{   
    if (argc != 3) {
        printf("Usage: %s <n> <i>\n", argv[0]);
        return 0;
    }

    int n = atoi(argv[1]);
    int i = atoi(argv[2]);

    printf("Worker %d started\n", i);

    key_t key1 = ftok("graph.txt", 100);
    key_t key2 = ftok("graph.txt", 101);
    key_t key3 = ftok("graph.txt", 102);
    key_t key4 = ftok("graph.txt", 103);
    key_t key5 = ftok("graph.txt", 104);
    key_t key6 = ftok("graph.txt", 105);

    // access all the shared memory segments
    int shmid1 = shmget(key1, n * n * sizeof(int), 0777 | IPC_CREAT);
    int* A = (int *) shmat(shmid1, 0, 0);

    int shmid2 = shmget(key2, n * sizeof(int), 0777 | IPC_CREAT);
    int* T = (int *) shmat(shmid2, 0, 0);

    int shmid3 = shmget(key3, sizeof(int), 0777 | IPC_CREAT);
    int* idx = (int *) shmat(shmid3, 0, 0);

    // access all the semaphores
    int mtx = semget(key4, 1, 0777 | IPC_CREAT);
    int syncid = semget(key5, n, 0777 | IPC_CREAT);
    int ntfy = semget(key6, 1, 0777 | IPC_CREAT);

    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    // needs to wait for its predecessors to finish
    int indegree = 0;
    for (int j = 0; j < n; j++) {
        if (A[j*n+i] == 1) {
            indegree++;
        }
    }

    for (int j = 0; j < indegree; j++) {
        P(syncid);
    }

    // critical section
    P(mtx);
    T[*idx] = i;
    (*idx)++;
    V(mtx);

    // notify the boss
    V(ntfy);

    for(int j=0;j<n;j++){
        if(A[i*n+j] == 1){
            V(syncid);
        }
    }

    return 0;
}
