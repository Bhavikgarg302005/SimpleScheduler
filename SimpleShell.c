#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h> 
#include <ctype.h> 
#include <readline/readline.h>
#include <readline/history.h>
#include "sharedMemory.c"


#define MAX_COMMANDS 100
#define MAX_ARGS 100
int ncpu;
int TSLICE;

//for storing history 
typedef struct {
    char* cmd;
    int pid;
    time_t initial_time;
    int backgrnd;
    double duration;
} history;

//cntr for tracking number of commands executed
int cntr = 0;

//creating history array
history hst[250];


//fxn to print history
void histry() {
    printf("Shell completed\n");
}

//for scheduler
void submit(char* input, PriorityQueue* queue) {
    char* dup_input = malloc(strlen(input) + 1); strcpy(dup_input, input);
    char* token = strtok(dup_input, " ");

    token = strtok(NULL, " ");
    char executable[256];
    strcpy(executable, token);

    token = strtok(NULL, " ");
    int priority = (token != NULL) ? atoi(token) : 1;

    Process process;
    strcpy(process.cmd, executable);
    process.priority = priority;
    process.pid = -2;

    if (!is_full(queue)){
        enqueue(queue, &process);
    }

    free(dup_input);
    return;
}

//required to remove '&' in background commands
void remove_character(char *str, char char_to_remove) {
    int i, j = 0;
    int len = strlen(str);
    char result[len + 1]; 

    for (i = 0; i < len; i++) {
        if (str[i] != char_to_remove) {
            result[j++] = str[i];
        }
    }
    result[j] = '\0'; 
    strcpy(str, result);
}

// forking the chid and running the command 
int create_and_run(char *command) {
    //for calculating duration of command execuation
    clock_t start_time = clock();
    //forking
    int child_st = fork();
    //intitial time
    time_t initial_time;
    time(&initial_time);
    //Error check
    if (child_st < 0) {
        perror("Fork failed");
        exit(0);
    }
    //checking if this is background process or not
    int background=0;
    if (strchr(command, '&')) {
        background = 1;
        remove_character(command, '&');
    }
    
    if (child_st == 0) {
        char *argv[1024];
        //tokenising the command
        char *temp = strtok(command, " ");
        //Error check
        if (temp == NULL) {
            printf("Error in tokenising string\n");
            exit(EXIT_FAILURE);
        }
        //making a char* array for storing the commands
        int pos = 0;
        while (temp != NULL) {
            argv[pos++] = temp;
            temp = strtok(NULL, " ");
        }
        argv[pos] = NULL;
        //Exec call and error check
        if (execvp(argv[0], argv) == -1) {
            printf("Exec error\n");
            exit(EXIT_FAILURE);
        }
        exit(1);
    } else {
        int status;
        //If not background then waiting for child to terminate
        if(!background){
          if (waitpid(child_st, &status, 0) == -1) {
              printf("Wait failed\n");
              exit(EXIT_FAILURE);
            }
        }
        //calculate end time for commmand execution
        clock_t end_time = clock();
        //copying the command to store in history
        hst[cntr].cmd = malloc(strlen(command) + 1);
        //Error check
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
        if(background){
            hst[cntr].backgrnd=1;
        }
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = child_st;
        hst[cntr].initial_time = initial_time;
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
    }
    return 1;
}

//launching create and run fxn and also checking teh commands that cannot be handled by exec such as cd,history,exit 
int launch(char *command,PriorityQueue* queue) {
    //checking wheather cd command
    if(strstr(command,"submit")!=NULL){
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        submit(command, queue);  
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
         //coping the command for storing in history
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = getpid();
        hst[cntr].initial_time = initial_time;
        clock_t end_time = clock();
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
        return 1;

    }
    if (strncmp(command, "cd", 2) == 0) {
        //for storing history
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        char *dir = strtok(command + 3, " ");
        //Error check
        if (dir == NULL) {
            dir = getenv("HOME");
        }
        //error checking
        if (chdir(dir) != 0) {
            printf("cd failed\n");
            exit(EXIT_FAILURE);
        }
        //for storing history
        hst[cntr].cmd = malloc(strlen(command) + 1);
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
         //coping the command for storing in history
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = getpid();
        hst[cntr].initial_time = initial_time;
        clock_t end_time = clock();
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
        return 1;
    } 
    //checking wheather it is custom history command
    if (strcmp(command, "history") == 0) { 
        //for storing history
        clock_t start_time = clock();
        time_t initial_time;
        time(&initial_time);
        hst[cntr].cmd = malloc(strlen(command) + 1);
        //Error check
        if (hst[cntr].cmd == NULL) {
            printf("Error in allocating size\n");
            exit(EXIT_FAILURE);
        }
        //coping the command for storing in history
        strcpy(hst[cntr].cmd, command);
        hst[cntr].pid = getpid();
        hst[cntr].initial_time = initial_time;
        clock_t end_time = clock();
        hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
        cntr++;
        histry();
        
        return 1;
    } else if (strcmp(command, "exit") == 0) { 
        histry();
        return 0;
    }
    return create_and_run(command);
}
//for handling crtl+c signal and also sigchild such that background child does not get (zombie)
void my_handler(int signum) {
    if(signum==SIGINT){
       printf("Exited Successfully \n");
       histry();
       exit(0);
    }
    if(signum==SIGCHLD){
        //to prevent child from becoming (zombie)
        while((waitpid(-1, NULL, WNOHANG) > 0));
    }
}

//for proper slicing
void slice(char *s, char *t, int st, int en) {
    if (st <= en) {
        int o = 0;
        for (int i = st; i <= en; ++i) {
            t[o] = s[i];
            o++;
        }
        t[o] = '\0';
    }
}

void pipeC(char *i) {
    //cheking wheather piped command is to be deal in background or not
    int background=0;
    if (strchr(i, '&')) {
        background = 1;
    
       remove_character(i, '&');
    }
    char *cmdd = malloc(strlen(i)+1);
    if(cmdd==NULL){
        printf("Error in malloc\n");
        exit(EXIT_FAILURE);
    }
    strcpy(cmdd,i);
    char *cm = strtok(i, "|");
    if (cm == NULL) {
        printf("Error in allocating space");
        exit(EXIT_FAILURE);
    }
    //for calculating duration of command execution
    time_t start_time=clock();
    
    char *commands[MAX_COMMANDS];
    int num_commands = 0;
   // spliting the input string into individual commands
    while (cm != NULL && num_commands < MAX_COMMANDS) {
        commands[num_commands] = (char*)malloc(strlen(cm) + 1);
        if (commands[num_commands] == NULL) {
            printf("Error in allocating command space");
            exit(EXIT_FAILURE);
        }
        slice(cm,commands[num_commands],0,strlen(cm)-1);
        cm = strtok(NULL, "|");
        num_commands++;
    }

    int pipe_fd[2 * (num_commands - 1)]; // Create pipe file descriptors

    // Create pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipe_fd + i * 2) == -1) {
            printf("Pipe creation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    // Fork and execute each command
    for (int i = 0; i < num_commands; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            if (i > 0) { // If not the first command, get input from the previous pipe
                dup2(pipe_fd[(i - 1) * 2], 0); // Read end of the previous pipe
            }
            if (i < num_commands - 1) { // If not the last command, output to the next pipe
                dup2(pipe_fd[i * 2 + 1], 1); // Write end of the current pipe
            }
            // Close all pipe fds in the child
            for (int j = 0; j < 2 * (num_commands - 1); j++) {
                close(pipe_fd[j]);
            }
             // Tokenize the command into arguments
            char *args[MAX_ARGS];
            char *cmd1 = commands[i];
            cm= strtok(cmd1, " ");
            int arg_count = 0;

            while (cm != NULL) {
                args[arg_count++] = cm;
                cm= strtok(NULL, " ");
            }
            args[arg_count] = NULL; // Null-terminate the arguments
            // Execute the command
            if (execvp(args[0], args) == -1) {
                printf("Execvp error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    // Parent process closes all pipe ends
    for (int i = 0; i < 2 * (num_commands - 1); i++) {
        close(pipe_fd[i]);
    }
    //If not background process than wait for child process to execute:
    int status;
    if(!background){
     for (int i = 0; i < num_commands-1; i++) {
          if (wait(NULL) == -1) {
              printf("Wait failed\n");
              exit(EXIT_FAILURE);
            }
     }
    }
    //calculating end time for command execuation
    clock_t end_time = clock();
    hst[cntr].cmd = malloc(strlen(cmdd) + 1);
    if (hst[cntr].cmd == NULL) {
        printf("Error in allocating size\n");
        exit(EXIT_FAILURE);
    }
    strcpy(hst[cntr].cmd, cmdd);
    hst[cntr].pid = pid;
    if(background){
        hst[cntr].backgrnd=1;
    }
    hst[cntr].initial_time = time(NULL);
    hst[cntr].duration = ((double)(end_time - start_time) / CLOCKS_PER_SEC);
    cntr++; 
     //free malloc space
    for (int j = 0; j < num_commands; j++) {
        free(commands[j]);
    }
    free(cm);
}

//removing new line character
int pipe_call(char i[]) {
    i[strcspn(i, "\n")] = 0;
    pipeC(i);
    return 1;
}

int main(int argc, char** argv){
    key_t key = ftok("keyfile", 'R');
    int shmid = shmget(key, sizeof(int), 0666);

    PriorityQueue* queue;
    if (create_priority_queue(key, &queue) == -1){
        perror("shared memory segment error");
        exit(EXIT_FAILURE);
    }

    char shmid_str[32];
    //sprintf(shmid_str, "%d", shmid);
    char shell_pid_str[32];
    //sprintf(shell_pid_str, "%d", getpid());
    char* args[] = {"scheduler", shmid_str, shell_pid_str, argv[1], argv[2], NULL};

    int scheduler_pid = fork();
    if (scheduler_pid == -1){
        perror("scheduler fork");
        return 0;
    }
    if (scheduler_pid == 0){
        execv("./scheduler", args);
        perror("execv");
        exit(EXIT_FAILURE);
    }
    //Signal handling
    int status;
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig)); 
    sig.sa_handler = my_handler;
    //Error check in handling signal
    if (sigaction(SIGINT, &sig, NULL) == -1) {
        printf("sigaction error\n");
        exit(EXIT_FAILURE);
    }
    //Error check in handling signal
    if(sigaction(SIGCHLD,&sig,NULL)== -1){
        printf("sigaction error\n");
        exit(EXIT_FAILURE);
    }
    //Main do-while loop
    do{
        char* command;
        command=malloc(500);
        //Error check for malloc
        if(command==NULL){
            printf("Allocation not done\n");
            exit(EXIT_FAILURE);
        }
        //reading user-input
        command=readline("gaur-garg-iiitd.ac:");
        //if not entered anything again ask for it
        if(command[0] == '\0'){
            continue;
        }
        //checking wheather a piped command or not
        int bool = 0;
        int len = strlen(command);
        for (int i = 0; i < len; i++) {
            if (command[i] == '|') {
                status = pipe_call(command);
                bool = 1;
                break;
            }
        }
        //removing new line character and replacing it with null character
        if (bool == 0) {
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }
            status = launch(command,queue);
        }
        // freeing space 
        if(command){
            free(command);
        }
        
    }while (status);
    
    printf("Completed shell process\n");
    return 0;
}
