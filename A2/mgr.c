/*
    Operating Systems Laboratory Assignment 2
    Name: Ashwin Prasanth
    Roll Number: 21CS30009
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

struct process {
    int pid;
    int pgid;
    char status[20];
    char name[20];
};

struct process PT[11];

// utility function to create process struct
struct process createProcess(int pid, int pgid, char status[20], char name[20]) {
    struct process p;
    p.pid = pid;
    p.pgid = pgid;
    strcpy(p.status, status);
    strcpy(p.name, name);
    return p;
}

// utility function to check if PT is full
int isFull(struct process PT[11]) {
    return PT[10].pid != 0;
}

// utility global variables

int currChild = -1; // current child that is running, -1 if no child is running
int numSuspended = 0; // number of suspended children

// signal handler for parent
void parentSigHandler(int sig) {
    if (currChild == -1) {
        // if mgr is running normally (no children running), just print prompt
        printf("\nmgr> ");
        fflush(stdout);
        return;
    }

    else {
        // child is running, handle it's signal
        if (sig == SIGINT) {
            strcpy(PT[currChild].status,"TERMINATED");
            kill(PT[currChild].pid, SIGKILL);
        }
        else if (sig == SIGTSTP) {
            numSuspended++;
            strcpy(PT[currChild].status, "SUSPENDED");
            kill(PT[currChild].pid, SIGTSTP);
        }

        currChild = -1;
        printf("\n");
    }
}

int main() {

    // overriding default signal handlers
    signal(SIGINT, parentSigHandler);
    signal(SIGTSTP, parentSigHandler);

    srand((unsigned int)time(NULL));

    // initialising PT with current process and dummy empty values
    int parent_pid = getpid();
    PT[0] = createProcess(parent_pid, getpgid(parent_pid), "SELF", "mgr");
    for (int i = 1; i < 11; i++) {
        PT[i] = createProcess(0, 0, "EMPTY", "EMPTY");
    }

    char input; 
    
    while(1) {

        printf("mgr> ");
        scanf("%c", &input);
        fflush(stdin);
        
        if (input == 'p'){
            printf("NO\tPID\tPGID\tSTATUS\t\tNAME\n");
            for (int i = 0; i < 11; i++) {
                if (PT[i].pid != 0) {
                    if (strcmp(PT[i].status, "SELF" ) == 0)
                        printf("%d\t%d\t%d\t%s\t\t%s\n", i, PT[i].pid, PT[i].pgid, PT[i].status, PT[i].name);
                    else
                        printf("%d\t%d\t%d\t%s\t%s\n", i, PT[i].pid, PT[i].pgid, PT[i].status, PT[i].name);
                }
            }
        }

        else if (input == 'r'){
            
            // terminate if table is full
            if (isFull(PT)) {
                printf("Process table is full. Quitting...\n");
                exit(0);
            }

            int pid = fork();

            char letter = 'A' + rand() % 26;
            char job_name[20];
            sprintf(job_name, "job %c", letter);

            // find first empty slot in PT
            for (int i = 0; i < 11; i++) {
                if (PT[i].pid == 0) {
                    currChild = i;
                    break;
                }
            }

            setpgid(pid, pid); 

            if (pid == 0) {       
                execlp("./job", "./job", &letter, NULL);
            }

            else {
                PT[currChild] = createProcess(pid, getpgid(pid), "FINISHED", job_name);
                waitpid(pid, NULL, WUNTRACED);
                currChild = -1;
            }
        }

        else if (input == 'c') {

            // if no suspended jobs, continue
            if(numSuspended == 0) continue; 

            else{
                // print all suspended jobs, and take user input
                printf("Suspended jobs: ");
                for(int i = 0; i < 11; i++){
                    if(strcmp(PT[i].status, "SUSPENDED") == 0){
                        printf("%d ", i);
                    }
                }
                printf("(Pick one): ");
                int job;
                scanf("%d", &job);
                fflush(stdin);

                // checking if job number is valid, and continuing it
                if(strcmp(PT[job].status, "SUSPENDED") == 0) {
                    currChild = job;
                    kill(PT[job].pid, SIGCONT);
                    waitpid(PT[job].pid, NULL, 0);
                    strcpy(PT[job].status, "FINISHED");
                    numSuspended--;
                }

                else {
                    printf("Invalid job number\n");
                }
            }
        }

        else if (input == 'k') {

            // similar to 'c' command
            if(numSuspended == 0) continue; 

            else{
                printf("Suspended jobs: ");
                for(int i = 0; i < 11; i++){
                    if(strcmp(PT[i].status, "SUSPENDED") == 0){
                        printf("%d ", i);
                    }
                }
                printf("(Pick one): ");
                int job;
                scanf("%d", &job);
                fflush(stdin);

                // only difference - here we kill the process instead of continuing
                if(strcmp(PT[job].status, "SUSPENDED") == 0) {
                    currChild = job;
                    kill(PT[job].pid, SIGKILL);
                    strcpy(PT[job].status, "TERMINATED");
                    numSuspended--;
                }
                else {
                    printf("Invalid job number\n");
                }
            }
        }

        else if (input == 'h') {
            printf("Command : Action\n");
            printf("c\t: Continue a suspended job\n");
            printf("h\t: Print this help message\n");
            printf("k\t: Kill a suspended job\n");
            printf("p\t: Print the process table\n");
            printf("q\t: Quit\n");
            printf("r\t: Run a new job\n");
        }
            
        else if (input == 'q') {
            exit(0);
        }

        else {
            printf("Invalid command. Try again\n");
        }
    }
}
