#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>



#define SHM_NAME "/my_shared_memory" // Name for shared memory object

int ncpu;
int TSLICE;

typedef struct {
    char* cmd;
    int pid;
    struct timeval initial_time;
    struct timeval end_time;
    long long completion_time;
    long long waiting_time;
    int state;
} process;


typedef struct {
    process queue[1000];
    int rear;
    int front;
} ProcessQueue;

// Structure for shared data
typedef struct {
    sem_t scheduler;
    sem_t ready_q_sem;
    ProcessQueue* ready_q;
    ProcessQueue* terminated_q;
} SharedData;




void enqueue(ProcessQueue* p, process p1) {
    if (p->rear == 1000) {
        printf("Queue is full\n");
        exit(EXIT_FAILURE);
    }
    p->queue[p->rear] = p1;
    p->rear++;
}


void dequeu(ProcessQueue* p) {
    if (p->rear == 0) {
        printf("Queue is empty\n");
        exit(EXIT_FAILURE);
    }
    p->front++;
}


void SplitSubmit(char* cmd,char*y)
{
    int flag=0;
    int i=0;
    char * array[100];
    char * t=strtok(cmd," ");
    while(t!=NULL)
    {
        array[i]=t;
        i++;
        //printf("%s\n",array[i-1]);
        t=strtok(NULL," ");
    }
    if(strcmp(array[0], "submit") == 0)
    {
    
        //printf("%s\n",array[1]);
        int yy=0;
        while(array[1][yy]!='\0')
        {
            y[yy]=array[1][yy];
            yy++;

        }
        y[yy]='\0';
    }
}

void printEndQueue(ProcessQueue* p) {
    for (int i = p->front; i < p->rear; i++) {
        printf("Terminated Process with PID %d\n",p->queue[i].pid);
        printf("Command-%d was - %s\n", p->queue[i].pid, p->queue[i].cmd);
        printf("Waiting time for process is %lld\n", p->queue[i].waiting_time);
        printf("Completion Time for process is %lld\n", p->queue[i].completion_time);
        printf("\n");
    }
}

// For handling the processes that are completed
void handler(int sig, SharedData* data) {
    if(sig==SIGCHLD){
            printf("hi");
            int status;
            pid_t pid;
            while((pid == waitpid(-1,&status,WNOHANG))>0){
             for (int i = data->ready_q->front; i < data->ready_q->rear; i++) {
              if (data->ready_q->queue[i].pid == pid){
                data->ready_q->queue[i].state=-1;
                gettimeofday(&data->ready_q->queue[i].end_time,NULL);
                struct timeval elapsedTime;
                elapsedTime.tv_sec=data->ready_q->queue[i].end_time.tv_sec - data->ready_q->queue[i].initial_time.tv_sec;
                elapsedTime.tv_usec=data->ready_q->queue[i].end_time.tv_usec - data->ready_q->queue[i].initial_time.tv_usec;  
                long long elapsed = elapsedTime.tv_sec * 1000 + elapsedTime.tv_usec / 1000;
                data->ready_q->queue[i].completion_time += elapsed;
                data->ready_q->queue[i].waiting_time += elapsed;
                 // Send to terminated queue
                enqueue(data->terminated_q, data->ready_q->queue[i]);
                // Remove from ready queue
                for (int j = i; j < data->ready_q->rear - 1; j++) {
                    data->ready_q->queue[j] = data->ready_q->queue[j + 1];
                }
                data->ready_q->rear--;
                break; // Break after finding the process to avoid out-of-bounds access
            }
        }
    }
    }
    else{
        // Signal received, proceed with execution
    }
}


int main(int argc, char** argv) {
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = handler;
    //sigaction(SIGINT, &sig, NULL);
    sigaction(SIGCHLD, &sig, NULL);
    sigaction(SIGUSR1,&sig,NULL);

    // Error check for ncpus and TSLICE
    if (argc != 3) {
        printf("Error in processing file\n");
        exit(EXIT_FAILURE);
    }
    ncpu = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

     // Create shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Configure the size of the shared memory object
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory obj
    SharedData* data = (SharedData*)mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    data->ready_q = malloc(sizeof(ProcessQueue)+1);
    data->terminated_q = malloc(sizeof(ProcessQueue)+1);

    // Initialize shared memory data
    sem_init(&data->scheduler, 0, 0);
    sem_init(&data->ready_q_sem, 0, 1);
    data->ready_q->rear = 0;
    data->ready_q->front = 0;
    data->terminated_q->rear = 0;
    data->terminated_q->front = 0;

    if (!data->ready_q || !data->terminated_q) {
     perror("Memory allocation failed");
     exit(EXIT_FAILURE);
    }
    int scheduler_child = fork();
    // Scheduler process started:
    if (scheduler_child < 0) {
        printf("Error in fork\n");
        exit(EXIT_FAILURE);
    }
    if (scheduler_child == 0) {

        struct timespec ts;
        ts.tv_sec=0;
        ts.tv_nsec=TSLICE * 1000000;
        // For round-robin scheduling
        sem_wait(&data->scheduler);
        while(1){
            int iter = data->ready_q->front;
            int check_num = ncpu;
            // printf("%d",data->ready_q->rear);
            while (iter < data->ready_q->rear && check_num > 0) {
                // Finding ready for execution
                if (data->ready_q->queue[iter].state == 1) {
                    printf("entered the loop\n");
                    // Remove from ready_q
                    process p = data->ready_q->queue[iter];
                     kill(p.pid, SIGUSR1);
                    // Execute this process
                    gettimeofday(&data->ready_q->queue[iter].initial_time,NULL);
                    p.state = 0; // Update state to running
                    kill(p.pid, SIGCONT);
                    check_num--;
                }
                iter++;
            }
            //sem_post(&data->ready_q_sem);
            usleep(TSLICE*1000);

            for(int i=data->ready_q->front ;i<data->ready_q->rear;i++){
                if(data->ready_q->queue[i].state==0){
                   kill(data->ready_q->queue[i].pid,SIGSTOP);
                   gettimeofday(&data->ready_q->queue[i].end_time,NULL);
                   struct timeval elapsedTime;                
                   elapsedTime.tv_sec=data->ready_q->queue[i].end_time.tv_sec - data->ready_q->queue[i].initial_time.tv_sec;
                   elapsedTime.tv_usec=data->ready_q->queue[i].end_time.tv_usec - data->ready_q->queue[i].initial_time.tv_usec;  
                   long long elapsed = elapsedTime.tv_sec * 1000 + elapsedTime.tv_usec / 1000;
                   data->ready_q->queue[i].completion_time += elapsed;
                   data->ready_q->queue[i].waiting_time += elapsed;
                   dequeu(data->ready_q);
                   enqueue(data->ready_q,data->ready_q->queue[i]);
                   data->ready_q->queue[i].state=1;
                }
            }
           
        }
    }

    // Shell process started: printf("Process with PID %d finished execution. Execution Time: %lld ms\n", pid, shell_queue.processes[i].total_execution_time);
    //things left->remove submit from user-command then give to execlp
    //making// sem_wait(&data->ready_q_sem); sure when will we print all user command list  
    int status;
    while(1){
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
        //removing new line character and replacing it with null character
        int len = strlen(command);
        if (len > 0 && command[len - 1] == '\n') {
            command[len - 1] = '\0';
        }
        if(strcmp(command, "exit") == 0){
            printf("%d",data->terminated_q->rear);
            printEndQueue(data->terminated_q);
            exit(EXIT_FAILURE);
        }
        char* a="submit";
        if(strstr(command,a) == NULL){
             system(command);
        }

        pid_t child_p=fork();
       /*doubt in this part*/
        if(child_p==0){
                // signal(SIGUSR1,handler); // Register signal handler
                // pause();
                char* args[3]; // Array to hold command arguments
                args[0]=malloc(strlen(command)+1);
                SplitSubmit(command, args[0]);
                args[1]=NULL;
                execvp(args[0], args);
                perror("Execution failed");
                exit(1);
        }
        else{
            process new_process;
            new_process.pid = child_p;
            new_process.cmd=malloc(strlen(command)+1);
            strcpy(new_process.cmd, command);
            new_process.state = 1;
            new_process.completion_time = 0;
            gettimeofday(&new_process.initial_time, NULL);
            new_process.waiting_time = TSLICE;
            enqueue(data->ready_q,new_process);
            sem_post(&data->scheduler);
        }

        // freeing space 
        if(command){
            free(command);
        }
    }
    while(data->ready_q->rear>=0){
        int status;
        pid_t pid=wait(&status);
        if(pid==-1){
            break;
        }

        for(int i=0;i<=data->ready_q->rear;i++){
            if(data->ready_q->queue[i].pid==pid){
                 printf("Process with PID %d finished execution. Execution Time: %lld ms\n", pid, data->ready_q->queue[i].completion_time);
                 data->ready_q->queue[i].state=-1;
                 gettimeofday(&data->ready_q->queue[i].end_time,NULL);
                  struct timeval elapsedTime;
                  elapsedTime.tv_sec = data->ready_q->queue[i].end_time.tv_sec - data->ready_q->queue[i].initial_time.tv_sec;
                  elapsedTime.tv_usec = data->ready_q->queue[i].end_time.tv_usec - data->ready_q->queue[i].initial_time.tv_usec;
                  long long elapsed = elapsedTime.tv_sec * 1000 + elapsedTime.tv_usec / 1000;
                  data->ready_q->queue[i].completion_time += elapsed;
                   
            }
        }
    } 
    printEndQueue(data->terminated_q);
    exit(0);

    // Cleanup 
    munmap(data, sizeof(SharedData));  // Unmap shared memory
    shm_unlink(SHM_NAME);               // Remove shared memory object
    return 0;
}
