#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

int main ()
{
   int n, mypid, parpid;

   printf("Parent: n = "); scanf("%d", &n);

   /* Child creation */
   int f = fork(); 
   if (f) { /* Parent process */
      mypid = getpid();
      parpid = getppid();
        printf("Parent: forkval = %d\n", f);
      printf("Parent: PID = %u, PPID = %u...\n", mypid, parpid);
   } else {      /* Child process */
      mypid = getpid();
      parpid = getppid();
        printf("Child: forkval = %d\n", f);
      printf("Child : PID = %u, PPID = %u...\n", mypid, parpid);
      n = n * n;
   }

   printf("Process PID = %u: n = %d\n", mypid, n);

   exit(0);
}