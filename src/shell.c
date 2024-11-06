// Project 2: Shell with FCFS and Non-preemptive SJF

// A simple FCFS/Non-Preemptive SJF shell that supports the following commands:
//      ver     - prints the shell version
//      exec    - executes a program with the given parameters
//      ps      - prints the living processes
//      kill    - kills a process with the given pid
//      help    - prints the help page
//      exit    - exits the shell

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <ctype.h>
#include "queue.h"

#define NO_CURR_PID 0

// Flag to indicate the scheduling type
// Default is SJF
int sched_type = SJF;

// PID of the current process running
pid_t curr_proc_pid = NO_CURR_PID;
// Current node we are working on
struct node *curr_proc_node = NULL;

// Flag to indicate if the shell is waiting for input/output
// Ensures we don't accept input while a process is running
// 0 = not waiting, 1 = waiting 
int io_occupied = 0;

// Flag to indicate if the shell is suspended
int fg_suspended = 0;

// Flag to indicate if the shell should continue running
int run = 1;

// Queue of processes (FCFS scheduling)
struct queue pid_list;


// Prints the help page and supported commands
void help() {
    printf("Manual Page\n\n");
    printf("This shell supports the following commands:\n");
    printf("\tver\n\texec\n\tps\n\tkill\n\thelp\n\texit\n");
    printf("For more details please type 'help <command>'\n");
}


// Prints the help page for a specific command
void helpcmd(char *cmd) {
    printf("Manual Page\n\n");

    if (strcmp(cmd , "ver") == 0) {
        printf("ver:\tShows details about the shell version\n");
    }
    else if (strcmp(cmd, "exec") == 0) {
        // Exec can execute any exectuable not just this one - but we will only use this one
        printf("exec p1(n1,qt1) p2(n2,qt2) ...:\nExecutes the programs p1, p2 ...\nEach program types a message for n times and it is given a time quantum of qt msec.\n");
        printf("If parameter (&) is given the program will be executed in the background\n");
    }
    else if (strcmp(cmd, "ps") == 0) {
        printf("ps:\tShows the living process with the given pid\n");
    }
    else if (strcmp(cmd, "kill") == 0) {
        printf("kill pid:\tEnds the process with the given pid\n");
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("help:\tYou should know this command by now\n");
    }
    else if (strcmp(cmd, "exit") == 0) {
        printf("exit:\tEnds the experience of working in the new shell\n");
    }
    else {
        printf("No such command. Type help to see a list of commands\n");
    }
}


// Prints the shell version
void ver() {
    // Make a string to determine if we are using FCFS or SJF
    char *sched_type_str;
    if (sched_type == SJF) { sched_type_str = "SJF"; }
    else { sched_type_str = "FCFS"; }

    printf("New Shell\n");
    printf("Details:\n");
    printf("\tScheduler: %s\n", sched_type_str);
    printf("\tProcessing limit: 1\n");
}


// Prints the LIVING processes, not the ones that are queued
// Meaning the current processes (SHELL and the only 1 running)
// Should never be more than 2 in current implementation
void ps() {
    printf("NEW SHELL presents the following living processes:\n");
    printf("\tPID\tNAME\n");

    // Include the process we are in, the shell itself
    // We don't have this on queue because we don't want to kill the shell
    // When doing dequeue it would pop right off the queue
    printf("\t%d\tNEW SHELL\n", getpid());

    struct node *curr_node = pid_list.head;
    while (curr_node != NULL) {
        // Only print processes that are "alive", i.e. have a pid
        if (curr_node->pid != NO_PID) {
           printf("\t%d\t%s\n", curr_node->pid, curr_node->name);
        }
        curr_node = curr_node->next;
    }
}


// Kills a process with the given pid
void mykill(int pid) {
    // If we are killing the shell, ask the user if they are sure
    if (getpid() == pid) {
        char answer;
        do {
            printf("You are about to kill the shell, are you sure? (y/n): ");
            scanf(" %c", &answer);
        } while (answer != 'y' && answer != 'n');

        // If they are sure, exit the shell
        if (answer == 'y') {
            run = 0;
            return;
        }
        if (answer == 'n') { return; }
    }

    // Don't let user kill below 0 - bad things happen
    // If we couldn't kill the process, print an error message
    // If it could, it will be handled in childdead signal handler
    if (pid <= 0 || kill(pid, SIGTERM) != 0) {
        printf("Unable to kill %d\n", pid);
        return;
    }
    printf("You have killed process %d\n", pid);
    // Remove the process from the queue
    delete(pid, &pid_list);

    // Wait for the process to die - once it does die 
    // We will resume from here once we get the signal
    pause();
}


// Kills all the processes in the queue (if wanted)and exits the shell
void myexit() {
    // No processes running, exit
    if (pid_list.head == NULL) { exit(EXIT_SUCCESS); }

    // If there are still processes running, ask the user if they want to kill them
    char answer;
    do {
        printf("There are still living processes. Do you want to kill them? (y/n): ");
        scanf(" %c", &answer);
        // print char for debug
        answer = tolower(answer);
    } while (answer != 'y' && answer != 'n');

    // We exiting no matter what
    run = 0;

    // User did not want to kill the processes, bad move but user is always right
    if (answer == 'n') { printf("Exiting without killing processes.\n"); }
    // If did want to kill, kill all that are still alive
    if (answer == 'y') { 
        while (pid_list.head != NULL) { 
            // If not a valid pid, dequeue and continue - don't kill a non-living process
            if (pid_list.head->pid == NO_PID) { 
                dequeue(&pid_list);
                continue; 
            }
            mykill(pid_list.head->pid); 
        } 
    }
}


// Assumes top of queue is the process to run
void runprocess() {
    // If the queue is empty, return
    if (pid_list.head == NULL) { return; }
    // If there already is a process running, return
    if (curr_proc_pid != NO_CURR_PID) { return; }

    // Set the current process to the head of the queue
    curr_proc_node = pid_list.head;

    // Enter new process, the head is the current process
    // As we can only store 1 process at a time (the head)

    // IN NEW PROCESS
    if ((curr_proc_node->pid = fork()) == 0) {
        // Silence the output if the process is in the background
        if (curr_proc_node->args[3] != NULL && strcmp(curr_proc_node->args[3], "&") == 0) {
            // Detach the process from the terminal
            setsid();
            
            // Run process in the background, i.e. no output/input in terminal
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }

        // Run the process/program based on the name
        execv(curr_proc_node->name, curr_proc_node->args);
        // Should never get here, if we do, something went wrong
        exit(-1);
    }

    // IN SHELL
    // Update the current process we are working on
    curr_proc_pid = curr_proc_node->pid;

    // Set a flag saying we are running in foreground
    io_occupied = (curr_proc_node->args[3] == NULL);

    // Let the user know the process is running in background
    if (!io_occupied) { 
        printf("Running process %s (PID: %d) in background!\n", curr_proc_node->name, curr_proc_pid); 
    }
}


// Enqques a process with the given parameters
// Assumes the input is in the form of exec p1(n1,qt1) p2(n2,qt2) ...
void exec(char *input) {
    // Keeps tracks of arguments
    int i;

    // Array of arguments, 10 can be any number, it's just a placeholder (should be more than 4)
    // args[0] = program name, args[1] = n, args[2] = qt, args[3] = bg
    char *args[10];
    // Temporary string to hold the input
    char *curr_arg;

    // Parse the input into the executable and its arguments
    // Format is p(n,qt,bg)
    //      Get the program name p, and the arguments n, qt, and bg which are comma seperated
    //      background is optional, if it is not given, the process will run in the foreground
    //      We don't need to check if the input is valid because we assume it is
    curr_arg = strtok(input, "(");
    args[0] = curr_arg;
    // Add the arguments to the array
    for (i = 1; (curr_arg = strtok(NULL, ",")) != NULL; i++) { args[i] = curr_arg; }
    // Remove the end parenthesis from the last argument, and make next one null
    args[i - 1] = strtok(args[i - 1], ")");
    // Last argument is null to indicate end of arguments
    args[i] = NULL;

    // Push the process to the queue
    enqueue(args[0], args, &pid_list);
}


void childdead(int signum){
    int dead_pid, status;

    // Collect the dead child's pid
    dead_pid = wait(&status);
    // Formatting
    printf("The child %d is dead\n", dead_pid);

    // Print out and error message if the process exited with a non-zero status
    // Helps debugging if process fails (for user not for shell)
    int exit_status;
    if ((exit_status = WEXITSTATUS(status)) != 0) {
        printf("An error occured in the executing process\n");
        printf("Code: Process %d exited with status %d\n", dead_pid, exit_status);
    }
    
    // Remove the process from the queue 
    // Not using dequeue because if we call kill, it will remove the process inproperly
    delete(dead_pid, &pid_list);
    // If the process that died was the current process, set the current process to 0
    // and its node to NULL
    // Will always happen in our case since we can only run 1 process at a time
    if (dead_pid == curr_proc_pid) { 
        curr_proc_pid = NO_CURR_PID; 
        curr_proc_node = NULL;
    }

    // Run next process if there is one, and if shell is supposed to run
    // If there isn't free up the io flag
    if (pid_list.head != NULL && run) { 
        printf("\n"); // Formatting
        runprocess(); 
    }
    else { io_occupied = 0; }
}


// Suspends all processes
void susp(int signum) {
    fg_suspended = 1;
    printf("\nAll processes supspended\n");
}


// Continues all processes
void cont(int signum) {
    fg_suspended = 0;
    printf("\nWaking all processes...\n");
    while (curr_proc_pid != NO_CURR_PID && fg_suspended != 1) { pause(); }
}


// Driver function
int main(int argc, char **argv) {
    // Last args determines what type of scheduling we want
    // Check if the user wants to use SJF scheduling
    if (argc == 2 && strcmp(argv[1], "FCFS") == 0) { sched_type = FCFS; }
    else { sched_type = SJF; }

    // Input buffer
    char input[15][30];
    // Number of arguments
    int arg_num, i;

    // Initialize the queue with the given scheduling type
    initqueue(&pid_list, sched_type);

    // Set up signal handlers
    signal(SIGCHLD, childdead);
    signal(SIGTSTP, susp);
    signal(SIGQUIT, cont);
    setpriority(PRIO_PROCESS, 0, -20);

    // Print the shell version
    printf("\n"); // Formatting
    ver();

    while (run){
        // If there is a process running and its in the foreground, pause the shell
        // We don't want any input until a signal is recieved. This can be any signal
        // but we are using SIGCHLD because it is the only one we are using
        // Bascially block untill the process is done
        while (io_occupied) { pause(); }

        printf("\n=>");
        for (arg_num = 0; (scanf("%s", input[arg_num])) == 1; arg_num++) {
            if (getchar() == '\n') { break; }
        }
        printf("\n");   // Formatting

        // In case we have a background then foreground process running
        // Clears it to ensure we can't run anything with process is going
        if (io_occupied) { continue; }

        // Shows current verison of shell
        if (strcmp(input[0], "ver") == 0 && arg_num == 0) { ver(); }
        // Prints the list of commands
        else if (strcmp(input[0], "help") == 0 && arg_num == 0) { help(); }
        // Prints the help for a specific command
        else if (strcmp(input[0], "help") == 0 && arg_num == 1) { helpcmd(input[arg_num]); }
        // Prints the LIVING processes (should only ever be 1 or 2)
        else if (strcmp(input[0], "ps") == 0 && arg_num == 0) { ps();}
        // Kills a process with the given pid
        else if (strcmp(input[0], "kill") == 0 && arg_num == 1) { mykill(atoi(input[1])); }
        // Executes a process with the given parameters
        else if (strcmp(input[0], "exec") == 0 && arg_num != 0) {
            // First we will check if the exec is valid,
            // If it is not, we will not execute that specific process
            // We will just print an error message and continue to next input
            for (i = 1; i <= arg_num; i++) { 
                // Check if the format is valid
                //      If it doesn't have a process name it is not valid
                //      If it doesn't have a start or end parenthesis, it is invalid
                // Will run the process if it is valid, otherwise print an error message
                if (input[i][0] == '(' || strchr(input[i], '(') == NULL || input[i][strlen(input[i]) - 1] != ')') {
                    printf("Invalid exec for arg %d. Type 'help exec' for help.\n\n", i);
                    continue;
                }

                // If valid exec, run the process
                else { exec(input[i]); }
            }

            // All processes have been added to the queue, run the first one
            // When the process finishes, the next one will take its place
            // No need to try to run each process individually - one call will suffice
            runprocess();
        }
        // Exits the shell
        else if (strcmp(input[0], "exit") == 0 && arg_num == 0) { myexit(); }
        // If the command is not recognized, print an error message
        else { printf("No such command. Check help for help.\n"); }
    }
    
    exit(EXIT_SUCCESS);
}