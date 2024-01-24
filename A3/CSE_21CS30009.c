#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
 
// first argument -> mode (C/S/E)
// second argument -> pfd[0] or pfd[1] for first pipe 
// third argument -> pfd1[0] or pfd1[1] for second pipe

int main(int argc, char *argv[]) {

    char* mode = "S";
    if(argc > 1) mode = argv[1];

    if(strcmp(mode,"S")==0){
        // 2 pipes, which will be switched when swaprole is called
        int pfd[2], pfd1[2]; 
        pipe(pfd);
        pipe(pfd1);

        printf("+++ CSE in supervisor mode: Started\n");
        printf("+++ CSE in supervisor mode: pfd = [%d %d]\n", pfd[0], pfd[1]);
        printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
        printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");

        int pid1 = fork();

        if(pid1 == 0){
            // command-input mode
            printf("Executing command-input mode\n");
            char str[2];
            sprintf(str, "%d", pfd[1]);
            char str1[2];
            sprintf(str1, "%d", pfd1[0]);
            execlp("xterm", "xterm", "-T", "Child1", "-e", "./CSE", "C", str, str1, NULL);
        }

        else {
            int pid2 = fork();
            if(pid2 == 0){
                // execute mode
                printf("Executing execute mode\n");
                char str[2];
                sprintf(str, "%d", pfd[0]);
                char str1[2];
                sprintf(str1, "%d", pfd1[1]);
                execlp("xterm", "xterm", "-T", "Child2", "-e", "./CSE", "E", str, str1, NULL);
                printf("+++ CSE in supervisor mode: Second child exited\n");
            }

            else {
                wait(NULL);
            }
        }
    }

    if(strcmp(mode,"C")==0){
        int pfd_1 = atoi(argv[2]); // write end of first pipe
        int pfd1_0 = atoi(argv[3]); // read end of second pipe

        // saving original stdin and stdout to restore
        int stdin_copy = dup(0);
        int stdout_copy = dup(1);

        // closing stdout and using write end of first pipe
        close(1); 
        dup(pfd_1);

        char command[100];
        int curr_role = 1; // variable to keep track of whether it is C or E

        while(1){
            if(curr_role){
                fputs("Enter command> ", stderr);
                fflush(stderr);
                fgets(command, 100, stdin);

                // remove newline
                for(int i=0; i<100; i++) {
                    if (command[i] == '\n') {
                        command[i] = '\0';
                        break;
                    }
                }

                printf("%s", command); // sending to E
                fflush(stdout);

                if(strcmp(command, "exit")==0) break;

                else if(strcmp(command, "swaprole")==0) {
                    // swapping curr_role, reopening stdout, closing stdin and using read end of second pipe instead
                    curr_role = 0;       
                    close(1);
                    dup(stdout_copy);
                    close(0);
                    dup(pfd1_0);
                    for(int i=0; i<100; i++) command[i] = '\0'; // clearing buffer
                }
            }
            else {
                // here, second pipe is used, and it is in E mode

                printf("Waiting for command> ");
                fflush(stdout);

                read(pfd1_0, command, 100); // reading from C and displaying it
                printf("%s\n", command);
                fflush(stdout);

                if(strcmp(command, "exit")==0) break;

                else if(strcmp(command, "swaprole")==0) {
                    // doing similar swapping of pipes and roles
                    curr_role = 1;
                    close(0);
                    dup(stdin_copy);
                    close(1);
                    dup(pfd_1);
                    for(int i=0; i<100; i++) command[i] = '\0'; // clearing buffer
                }

                else {
                    // it will execute the corresponding command
                    int pid = fork();
                    if(pid == 0){
                        // splitting multi-word arguments into an array to pass to execvp
                        char* args[100];
                        char* token = strtok(command, " ");
                        int i=0;
                        while(token != NULL){
                            args[i] = token;
                            token = strtok(NULL, " ");
                            i++;
                        }
                        args[i] = NULL;

                        // reopen stdin and stdout, then execute
                        close(0);
                        dup(stdin_copy);
                        close(1);
                        dup(stdout_copy);
                        execvp(args[0], args);
                    }
                    else {
                        wait(NULL);
                    }
                    for(int i=0; i<100; i++) command[i] = '\0'; // clearing buffer
                }
            }
        }
    }

    if(strcmp(mode,"E")==0){
        int pfd_0 = atoi(argv[2]); // read end of first pipe
        int pfd1_1 = atoi(argv[3]); // write end of second pipe

        // saving original stdin and stdout to restore
        int stdin_copy = dup(0);
        int stdout_copy = dup(1);

        char command[100];
        int curr_role = 0;

        // closing stdin and using read end of first pipe
        close(0);
        dup(pfd_0);

        while(1){
            // this is same as E mode done above (when swaprole is done in C), so not repeating comments
            if(curr_role == 0){
                printf("Waiting for command> ");
                fflush(stdout);

                read(pfd_0, command, 100); 

                printf("%s\n", command);
                fflush(stdout);

                if(strcmp(command, "exit")==0) break;

                else if(strcmp(command, "swaprole")==0) {
                    curr_role = 1;
                    close(0);
                    dup(stdin_copy);
                    close(1);
                    dup(pfd1_1);
                }

                else {
                    int pid = fork();
                    if(pid == 0){
                        char* args[100];
                        char* token = strtok(command, " ");
                        int i=0;
                        while(token != NULL){
                            args[i] = token;
                            token = strtok(NULL, " ");
                            i++;
                        }
                        args[i] = NULL;

                        close(0);
                        dup(stdin_copy);
                        close(1);
                        dup(stdout_copy);
                        execvp(args[0], args);
                    }
                    else {
                        wait(NULL);
                    }
                    for(int i=0; i<100; i++) command[i] = '\0';
                }
            }
            else {
                // it is in C mode now, with second pipe

                fputs("Enter command> ", stderr);
                fflush(stderr);
                fgets(command, 100, stdin);

                for(int i=0; i<100; i++) {
                    if (command[i] == '\n') {
                        command[i] = '\0';
                        break;
                    }
                }

                printf("%s", command);
                fflush(stdout);

                if(strcmp(command, "exit")==0) break;

                else if(strcmp(command, "swaprole")==0) {
                    curr_role = 0;       
                    close(1);
                    dup(stdout_copy);
                    close(0);
                    dup(pfd_0);
                    for(int i=0; i<100; i++) command[i] = '\0';
                }
            }
        }
    }
    return 0;
}
