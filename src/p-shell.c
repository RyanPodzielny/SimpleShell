// And example of a process, this could be anything in terms a the program
// In fact we can run literally any executable on the system
// Here we simply just print the arguments passed to the program
// For a select amount of time per print

// All background and foreground logic handleed in shell.c

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

void cont(int sig_num) {

}

void stop(int sig_num) {
    pause();
}

int main(int argc, char **argv) {
    int i, num, sltime;

    signal(SIGQUIT, cont);
    signal(SIGTSTP, stop);

    num = atoi(argv[1]);
    sltime = 1000 * atoi(argv[2]);
    
    for (i = 1; i <= num; i++){
        printf("This is program %s and it prints for the %d time of %d...\n", argv[0], i, num);
        usleep(sltime);
    }

    exit(EXIT_SUCCESS);
}
