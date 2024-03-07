#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/shm.h>
#include <time.h>

#ifdef VERBOSE
    int verbose = 1;
#else
    int verbose = 0;
#endif

#ifdef SLEEP 
    int slp = 1;
#else
    int slp = 0;
#endif

int main(int argc, char const *argv[])
{
    int n,t; 
    printf("n: ");
    scanf("%d", &n);

    printf("t: ");
    scanf("%d", &t);

    int shmid, status;
    int *M;
    shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);
    M = (int *) shmat(shmid, 0, 0);
    M[0] = 0; 

    srand(time(NULL));

    int cons_itemc[n+1];
    int cons_checksum[n+1];
    for(int i=0; i<=n; i++){
        cons_itemc[i] = 0;
        cons_checksum[i] = 0;
    }

    // fork n children (consumers)
    for(int i=1; i<=n; i++){ 
        if(fork()==0){
            // child i

            int itemc = 0; 
            int checksum = 0;

            printf("\t\t\t\tConsumer %d is alive\n", i);

            while(M[0] != -1) {
                if(M[0] == i){
                    if(verbose) printf("\t\t\t\tConsumer %d reads %d\n", i, M[1]);
                    itemc += 1;
                    checksum += M[1];
                    M[0] = 0;
                }
            }

            printf("\t\t\t\tConsumer %d has read %d items: Checksum = %d\n", i, itemc, checksum);

            exit(0);
        }
    }

    // Production loop
    printf("Producer is alive\n");
    for(int i=0; i<t; i++){
        int item = rand() % 900 + 100;
        int consumer = rand() % n + 1;

        if(verbose) printf("Producer produces %d for consumer %d\n", item, consumer);

        while(M[0]!= 0) {}

        M[0] = consumer;
        if(slp) usleep(100);
        M[1] = item;

        cons_itemc[consumer] += 1;
        cons_checksum[consumer] += M[1];
    }

    while (M[0] != 0){
        // wait for consumer to consume
    }
    M[0] = -1;
    wait(&status);
    shmdt(M);

    printf("Producer has produced %d items\n", t);
    for (int j=1; j<=n; j++){
        printf("%d items for consumer %d: Checksum = %d\n", cons_itemc[j], j, cons_checksum[j]);
    }

   return 0;
}

