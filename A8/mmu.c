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
int terminated_processes=0;


int main(int argc, char *argv[]){

    int shmid1 = atoi(argv[3]);
    pagetable_t *SM1 = (pagetable_t *)shmat(shmid1, NULL, 0);
    int k = atoi(argv[5]);
    int m = atoi(argv[6]);
    int f = atoi(argv[7]);

    pt_t *base_pointer = (pt_t *)(SM1 + k);
    for(int i=0; i<k; i++) {SM1[i].PT = base_pointer + i*m;}


    int shmid2=atoi(argv[4]);
    int *SM2=(int *)shmat(shmid2,NULL,0);

    int msqid2=atoi(argv[1]);
    int msqid3=atoi(argv[2]);

    //open file result.txt and write to it
    FILE *fp=fopen("result.txt","w");
    if(fp==NULL){
        perror("fopen");
        exit(1);
    }




    while(1){
        if(terminated_processes>=k){
            //print total page faults and invalid references for each process
            for(int i=0;i<k;i++){
                printf("Process %d: Total Page Faults: %d, Total Invalid References: %d\n",i,SM1[i].total_PFs,SM1[i].total_invalid_refs);
                //write to file
                fprintf(fp,"Process %d: Total Page Faults: %d, Total Invalid References: %d\n",i,SM1[i].total_PFs,SM1[i].total_invalid_refs);
            }
            break;
        }

        msq3_t m3;
        if(msgrcv(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 1, 0) == -1){
            perror("msgrcv");
            exit(1);
        }
        count++;
        
        int page=m3.num;
        int pid=m3.pid;
        printf(" timestamp = %d, pid = %d, page = %d\n",count,pid,page);
        //write to file
        fprintf(fp," timestamp = %d, pid = %d, page = %d\n",count,pid,page);

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
           
            if(page == -9){
                // process has completed its execution
                // update free-frame list and release all allocated frames

                printf("-9 received\n");
                for(int i=0;i<SM1[index].m_i;i++){
                    if(SM1[index].PT[i].frame!=-1){
                        SM2[SM1[index].PT[i].frame]=1;
                        SM1[index].PT[i].frame=-1;
                        SM1[index].PT[i].valid_bit=0;
                        SM1[index].PT[i].lru_ctr=0;
                    }
                }
                
                // send Type II message to scheduler
                msq2_t m2;
                m2.type=2;
                m2.pid=pid;
                fflush(stdout);
                if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                     fflush(stdout);
                    perror("msgsnd");
                    exit(1);
                }
                printf("Sent message type %ld, pid %d", m2.type, m2.pid);
                fflush(stdout);
                terminated_processes++;

                
            }else if(page>= SM1[index].m_i){
                // invalid page reference
                printf("Invalid page reference- pid = %d, page no. = %d\n", pid, page);
                //write to file
                fprintf(fp,"Invalid page reference- pid = %d, page no. = %d\n", pid, page);
                for(int i=0;i<SM1[index].m_i;i++){
                    if(SM1[index].PT[i].frame!=-1){
                        SM2[SM1[index].PT[i].frame]=1;
                        SM1[index].PT[i].frame=-1;
                        SM1[index].PT[i].valid_bit=0;
                        SM1[index].PT[i].lru_ctr=0;
                    }
                }
                SM1[index].total_invalid_refs++;
                msq2_t m2;
                m2.type=2;
                m2.pid=pid;
                if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                    perror("msgsnd");
                    exit(1);
                }

                msq3_t m3;
                m3.type=2;
                m3.num=-2;
                m3.pid=pid;
                printf("Sending message: [%d] %d\n",m3.pid,m3.num);
                if(msgsnd(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 0) == -1){
                    perror("msgsnd");
                        exit(1);
                }
                terminated_processes++;
            }
            else {
                
              
                printf("index = %d, page = %d\n", index, page);
                // printf("SM1[index].PT[page].page is %d\n",SM1[index].PT[page].page);
                // printf("SM1[index].PT[page].frame is %d\n",SM1[index].PT[page].frame);
                
                // check if page is already in page table 
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
                    printf("Page fault sequence - pid= %d, page nu. = %d\n", pid, page);
                    //write to file
                    fprintf(fp,"Page fault sequence - pid= %d, page nu. = %d\n", pid, page);
                    sleep(2);
                    msq3_t m3;
                    m3.type=2;
                    m3.num=-1;
                    m3.pid=pid;
                    printf("Sending message: [%d] %d\n",m3.pid,m3.num);
                    if(msgsnd(msqid3, &m3, sizeof(msq3_t) - sizeof(long), 0) == -1){
                        perror("msgsnd");
                        sleep(5);
                        exit(1);
                    }
                    SM1[index].total_PFs++;

                    //print the PT table
                    // for(int i=0;i<SM1[index].m_i;i++){
                    //     printf("Page: %d, Frame: %d, Valid Bit: %d, LRU Counter: %d\n", SM1[index].PT[i].page, SM1[index].PT[i].frame, SM1[index].PT[i].valid_bit, SM1[index].PT[i].lru_ctr);
                    // }

                    // invoke PageFaultHandler
                    // if free frame is available, update page table and free-frame list
                    // else, do local page replacement

                    int free_frame=-1;
                    int ind=0;
                    while(1){
                        
                        printf("i= %d\n",ind);
                        printf("SM2[i] is %d\n",SM2[ind]);
                        if(SM2[ind]==-1) break;
                        if(SM2[ind]==1){
                            free_frame=ind;
                            break;
                        }
                        ind++;
                    }

                    // printf("free frame is %d\n",free_frame);
                    sleep(1);
                    if(free_frame!=-1){
                        // free frame is available
                        SM1[index].PT[page].frame=free_frame;
                        SM1[index].PT[page].valid_bit=1;
                        SM1[index].PT[page].lru_ctr=count;
                        SM2[free_frame]=0;
                        printf("Free frame found, allocated to free frame");
                    }
                    else{
                        // local page replacement
                        int min_lru=INT_MAX;
                        int victim_page=-1;
                        for(int i=0;i<SM1[index].m_i;i++){
                            if(SM1[index].PT[i].frame==-1) continue; 
                            if(SM1[index].PT[i].lru_ctr<min_lru){
                                min_lru=SM1[index].PT[i].lru_ctr;
                                victim_page=i;
                            }
                        }

                        if(victim_page!=-1){
                            printf("page to be replaced is %d\n",victim_page);
                           SM1[index].PT[page].frame=SM1[index].PT[victim_page].frame;

                            SM1[index].PT[victim_page].frame=-1;
                            SM1[index].PT[victim_page].valid_bit=0;
                            
                            SM1[index].PT[page].valid_bit=1;
                            SM1[index].PT[page].lru_ctr=count; 
                        } 
                            
                    }

                        //print the PT table
                    // for(int i=0;i<SM1[index].m_i;i++){
                    //     printf("Page: %d, Frame: %d, Valid Bit: %d, LRU Counter: %d\n", SM1[index].PT[i].page, SM1[index].PT[i].frame, SM1[index].PT[i].valid_bit, SM1[index].PT[i].lru_ctr);
                    // }


                    
                        // send Type I message to scheduler
                        msq2_t m2;
                        m2.type=1;
                        m2.pid=pid;
                        if(msgsnd(msqid2, &m2, sizeof(msq2_t) - sizeof(long), 0) == -1){
                            perror("msgsnd");
                            exit(1);
                        }
                        printf("Sent message type %ld, pid %d", m2.type, m2.pid);
        
                }               
            }
        }
    }

    return 0;
}