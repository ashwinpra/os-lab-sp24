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

int main() {

    key_t key1 = ftok("graph.txt", 100);
    key_t key2 = ftok("graph.txt", 101);
    key_t key3 = ftok("graph.txt", 102);
    key_t key4 = ftok("graph.txt", 103);
    key_t key5 = ftok("graph.txt", 104);
    key_t key6 = ftok("graph.txt", 105);


    // open graph.txt and get adjacency graph
    FILE *fp = fopen("graph.txt", "r");
    int n;
    fscanf(fp, "%d", &n);
    int shmid1 = shmget(key1, n * n * sizeof(int), 0777 | IPC_CREAT);
    int* A; 
    A = (int *) shmat(shmid1, 0, 0);
    for(int i=0;i<n;i++){
        for (int j=0;j<n;j++){
            fscanf(fp, "%d", &A[i*n+j]);
        }
    }

    fclose(fp);

    // shared memory for topological sorting
    int shmid2 = shmget(key2, n * sizeof(int), 0777 | IPC_CREAT);
    int* T = (int *) shmat(shmid2, 0, 0);

    // shared memory for index in T 
    int shmid3 = shmget(key3, sizeof(int), 0777 | IPC_CREAT);
    int* idx = (int *) shmat(shmid3, 0, 0);
    *idx = 0;

    // semaphore mtx to block T and idx
    int mtx = semget(key4, 1, 0777 | IPC_CREAT);
    semctl(mtx, 0, SETVAL, 1);

    // semaphore to block workers until all predecessors are done
    int syncid = semget(key5, n, 0777 | IPC_CREAT);
    for (int i = 0; i < n; i++) {
        semctl(syncid, i, SETVAL, 0);
    }

    // semaphore ntfy for n workers to notify the boss
    int ntfy = semget(key6, 1, 0777 | IPC_CREAT);
    semctl(ntfy, 0, SETVAL, 0);

    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    for(int i=0;i<n;i++){
        for (int j=0;j<n;j++){
            printf("%d ",A[i*n+j]);
        }
        printf("\n");
    }
    printf("\n");

    // wait for n workers to finish
    for (int i = 0; i < n; i++) {
       P(ntfy);
    }

    // check for valid topological sort

    printf("Topological sorting of the vertices: ");
    for(int i = 0; i < n; i++) {
        printf("%d ", T[i]);
    }
    printf("\n");

    // check for valid topological sort
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (A[T[j]*n+T[i]] == 1 || T[i] == T[j]) {
                printf("Invalid topological sort\n");
                return 0;
            }
        }
    }

    // detach and remove shared memory
    shmdt(A);
    shmdt(T);
    shmdt(idx);
    shmctl(shmid1, IPC_RMID, 0);
    shmctl(shmid2, IPC_RMID, 0);
    shmctl(shmid3, IPC_RMID, 0);
    semctl(mtx, 0, IPC_RMID);
    semctl(syncid, 0, IPC_RMID);
    semctl(ntfy, 0, IPC_RMID);


    return 0;
}