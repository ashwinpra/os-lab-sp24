/*There can be following two cases:
 If the page is already in page table, MMU sends back the corresponding frame number to the
process.
 Else in case of page fault
 Sends –1 to the process as frame number
 Invoke the PageFaultHandler (PFH) to handle the page fault
 If a free frame is available, then update the page table, and also update the
corresponding free-frame list.
 If no free frame is available, then do local page replacement. Select victim
page using LRU, replace it, bring in a new frame, and update the page table.
 Intimate the scheduler by sending Type I message to enqueue the current process and
schedules the next process
 If MMU receives the page number –9 via message queue, then it infers that the process has
completed its execution and it updates the free-frame list and releases all allocated frames.
After this, MMU sends Type II message to the scheduler for scheduling the next process.
 If MMU receives –2, it prints “TRYING TO ACCESS INVALID PAGE REFERENCE” and
sends Type II message to the scheduler for scheduling the next process.
 Please note that MMU maintains a global timestamp (count), which is incremented by 1 after
every page reference.
6.1 Implementation
MMU is implemented in MMU.c. MMU will be executed via the Master process with four command
line arguments: page table (SM1), free frame list (SM2), MQ2 and MQ3 as mentioned in Matser
module. It will implement PFH as a function inside it and will call it whenever a page fault occurs. As
mentioned in scheduler section after resolving the page fault or in case of invalid page reference, it
sends corresponding notification messages (Type I or Type II) to the scheduler.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

struct sembuf pop = {0, -1, 0};
struct sembuf vop = {0, 1, 0};

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

#define PROB_ILLEGAL 0.1
#define T 250000

typedef struct _pt_t {
    int page;
    int frame;
    int lru_ctr;
    int valid_bit;
} pt_t;

typedef struct _pagetable_t {
    int pid;
    int m_i; // number of required pages
    // int f_i; // number of allocated frames
    pt_t *PT; // page table
    int total_PFs;
    int total_invalid_refs;
    char *ref_str; // reference string
} pagetable_t;

typedef struct _msq2_t {
    long type;
    int pid;
} msq2_t;

typedef struct _msq3_t {
    long type;
    int num; // page or frame number
    int pid;
} msq3_t;

int count=0;


int main(int argc, char *argv[]){

    int shmid1 = atoi(argv[3]);
    pagetable_t *SM1 = (pagetable_t *)shmat(shmid1, NULL, 0);
    int k = atoi(argv[5]);
    int m = atoi(argv[6]);
    int f = atoi(argv[7]);

    pt_t *base_pointer = (pt_t *)(SM1 + k);
    for(int i=0; i<k; i++) {SM1[i].PT = base_pointer + i*m;}

    // print everything for checking
    // for(int i=0; i<3; i++){
    //     printf("Process %d\n", i);
    //     printf("m_i: %d\n", SM1[i].m_i);
    //     printf("PT: \n");
    //     SM1[i].PT = base_pointer + i*m;
    //     sleep(5);
    //     for(int j=0; j<SM1[i].m_i; j++){
    //         printf("Page: %d, Frame: %d, Valid Bit: %d, LRU Counter: %d\n", SM1[i].PT[j].page, SM1[i].PT[j].frame, SM1[i].PT[j].valid_bit, SM1[i].PT[j].lru_ctr);
    //         sleep(3);
    //     }
    //     printf("total_PFs: %d\n", SM1[i].total_PFs);
    // }

    int shmid2=atoi(argv[4]);
    int *SM2=(int *)shmat(shmid2,NULL,0);

    int msqid2=atoi(argv[1]);
    int msqid3=atoi(argv[2]);

    printf("msqid2= %d, msqid3= %d\n", msqid2, msqid3);

    /*Master creates MMU and then MMU gets suspended. MMU wakes up after receiving the page
    number via message queue (MQ3) from process. It receives the page number from the process and
    checks in the page table for the corresponding process. There can be following two cases:
     If the page is already in page table, MMU sends back the corresponding frame number to the
    process.*/


    while(1){
        msq3_t m3;
        if(msgrcv(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 1, 0) == -1){
            perror("msgrcv");
            sleep(5);
            exit(1);
        }
        count++;
        
        int page=m3.num;
        int pid=m3.pid;
        printf("Received: [%d] %d\n", pid, page); 
        // while(1);
        // sleep(10);

        if(page==-2){
            printf("TRYING TO ACCESS INVALID PAGE REFERENCE\n");
            msq2_t m2;
            m2.type=2;
            m2.pid=pid;
            if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                perror("msgsnd");
                exit(1);
            }
            

        }else{
            int index=pid;
            // int total_entries=sizeof(SM1)/sizeof(SM1[0]);
            int total_entries=k;
            // printf("total entries: %d\n",total_entries);

            // for(int i=0;i<total_entries;i++){
            //     // printf("pid is %d\n",SM1[i].pid);
            //     if(SM1[i].pid==pid){
            //         index=i;
            //         break;
            //     }
            // }
            // printf("Found index: %d\n", index);
            // printf("index is %d\n",index);
            // printf("page is %d\n",page);
            // sleep(1);
            // while(1);

            if(page == -9){
                // process has completed its execution
                // update free-frame list and release all allocated frames
                for(int i=0;i<SM1[index].m_i;i++){
                    if(SM1[index].PT[i].frame!=-1){
                        SM2[SM1[index].PT[i].frame]=1;
                        SM1[index].PT[i].frame=-1;
                        SM1[index].PT[i].valid_bit=0;
                        SM1[index].PT[i].lru_ctr=INT_MAX;
                    }
                }

                // send Type II message to scheduler
                msq2_t m2;
                m2.type=2;
                m2.pid=pid;
                if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                    perror("msgsnd");
                    sleep(5);
                    exit(1);
                }

                
            }
            else {
                // check if page is already in page table
                // printf("here\n");
                // sleep(10);  
                printf("Reached this case\n");
                sleep(2);
                printf("index = %d, page = %d\n", index, page);
                sleep(2);
                printf("SM1[index].PT[page].page is %d\n",SM1[index].PT[page].page);
                sleep(3);
                printf("SM1[index].PT[page].frame is %d\n",SM1[index].PT[page].frame);
                sleep(3);
                
                if(SM1[index].PT[page].frame!=-1){
                    printf("Page is present\n");
                    // page is already in page table
                    // send frame number to process
                    msq3_t m3;
                    m3.type=2;
                    m3.num=SM1[index].PT[page].frame;
                    m3.pid=pid;
                    printf("Sending message: [%d] %d\n",m3.pid,m3.num);
                    if(msgsnd(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 0) == -1){
                        perror("msgsnd");
                        exit(1);
                    }
                    SM1[index].PT[page].lru_ctr=count;
                }
                
                else{
                    // page fault
                    // send -1 to process
                    printf("Page fault, gotta load\n");
                    msq3_t m3;
                    m3.type=2;
                    m3.num=-1;
                    m3.pid=pid;
                    printf("Sending message: [%d] %d\n",m3.pid,m3.num);
                    if(msgsnd(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 0) == -1){
                        perror("msgsnd");
                        exit(1);
                    }

                    // invoke PageFaultHandler
                    // if free frame is available, update page table and free-frame list
                    // else, do local page replacement

                    int free_frame=-1;
                    for(int i=0;i<sizeof(SM2)/sizeof(SM2[0]);i++){
                        if(SM2[i]==1){
                            free_frame=i;
                            break;
                        }
                    }

                    printf("free frame is %d\n",free_frame);
                    sleep(3);
                    if(free_frame!=-1){
                        // free frame is available
                        SM1[index].PT[page].frame=free_frame;
                        SM1[index].PT[page].valid_bit=1;
                        SM1[index].PT[page].lru_ctr=count;
                        SM2[free_frame]=0;
                    }
                    else{

                        // local page replacement
                        int min_lru=INT_MAX;
                        int victim_page=-1;
                        for(int i=0;i<SM1[index].m_i;i++){
                            if(SM1[index].PT[i].valid_bit!=1) continue; 
                            if(SM1[index].PT[i].lru_ctr<min_lru){
                                min_lru=SM1[index].PT[i].lru_ctr;
                                victim_page=i;
                            }
                        }

                        SM2[SM1[index].PT[victim_page].frame]=1;
                        SM1[index].PT[victim_page].frame=-1;
                        SM1[index].PT[victim_page].valid_bit=0;
                        SM1[index].PT[page].frame=SM1[index].PT[victim_page].frame;
                        SM1[index].PT[page].valid_bit=1;
                        SM1[index].PT[page].lru_ctr=count;     

                        // send Type I message to scheduler

                        msq2_t m2;
                        m2.type=1;
                        m2.pid=pid;
                        if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                            perror("msgsnd");
                            exit(1);
                        }
                    }
                }               
                // else, send -1 to process and invoke PageFaultHandler
                // send Type I message to scheduler
            }

        }
    }


    return 0;
}