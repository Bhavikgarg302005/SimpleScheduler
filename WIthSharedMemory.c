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

#define SHM_NAME "/my_shared_memory" // Name for shared memory object
#define SIGSTOP 19
int ncpu;
int TSLICE;

typedef struct {
    char* cmd;
    int pid;
    time_t initial_time;
    time_t end_time;
    double completion_time;
    double waiting_time;
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
    ProcessQueue ready_q;
    ProcessQueue running_q;
    ProcessQueue terminated_q;
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

void printQueue(ProcessQueue* p) {
    for (int i = p->front; i < p->rear; i++) {
        printf("Command-%d was - %s\n", p->queue[i].pid, p->queue[i].cmd);
        printf("Starting time for process: %s", ctime(&p->queue[i].initial_time));
        printf("End time for process: %s", ctime(&p->queue[i].end_time));
        printf("Waiting time for process is %f\n", p->queue[i].waiting_time);
        printf("Completion Time for process is %f\n", p->queue[i].completion_time);
        printf("\n");
    }
}

// For handling the processes that are completed
void handler(int sig, SharedData* data) {
    int status;
    pid_t pid;
    // Checking for completed processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        sem_wait(&data->ready_q_sem);
        for (int i = data->ready_q.front; i < data->ready_q.rear; i++) {
            if (data->ready_q.queue[i].pid == pid) {
                // Send to terminated queue
                enqueue(&data->terminated_q, data->ready_q.queue[i]);
                // Remove from ready queue
                for (int j = i; j < data->ready_q.rear - 1; j++) {
                    data->ready_q.queue[j] = data->ready_q.queue[j + 1];
                }
                data->ready_q.rear--;
                break; // Break after finding the process to avoid out-of-bounds access
            }
        }
        sem_post(&data->ready_q_sem);
    }
}

int main(int argc, char** argv) {
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

    // Map the shared memory object
    SharedData* data = (SharedData*)mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize shared memory data
    sem_init(&data->scheduler, 1, 0);
    sem_init(&data->ready_q_sem, 1, 1);
    data->ready_q.rear = 0;
    data->ready_q.front = 0;
    data->terminated_q.rear = 0;
    data->terminated_q.front = 0;
    data->running_q.rear = 0;
    data->running_q.front = 0;

    signal(SIGCHLD, (void (*)(int))handler);

    int scheduler_child = fork();
    // Scheduler process started:
    if (scheduler_child < 0) {
        printf("Error in fork\n");
        exit(EXIT_FAILURE);
    }
    if (scheduler_child == 0) {
        // For round-robin scheduling
        while (1) {
            sem_wait(&data->ready_q_sem);
            int iter = data->ready_q.front;
            int check_num = ncpu;

            while (iter < data->ready_q.rear && check_num > 0) {
                // Finding ready for execution
                if (data->ready_q.queue[iter].state == 1) {
                    // Remove from ready_q
                    process p = data->ready_q.queue[iter];
                    dequeu(&data->ready_q);
                    // Send to running queue
                    enqueue(&data->running_q, p);
                    // Execute this process
                    kill(p.pid, SIGCONT);
                    p.state = 0; // Update state to running
                    check_num--;
                }
                iter++;
            }
            sem_post(&data->ready_q_sem);
            sleep(TSLICE);

            // Shift processes from running_queue to rear of ready_queue
            for(int i=0;i<data->ready_q.rear;i++){
             for(int i=data->running_q.front;i<data->running_q.rear;i++){
                enqueue(&(data->ready_q),data->running_q.queue[i]);
                //stop this proceess from execution
                kill(data->running_q.queue[i].pid, SIGSTOP);
                dequeu(&(data->running_q));
             }
           }
        }
    }

    // Shell process started:
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

        // freeing space 
        if(command){
            free(command);
        }
    }

    // Cleanup 
    munmap(data, sizeof(SharedData));  // Unmap shared memory
    shm_unlink(SHM_NAME);               // Remove shared memory object
    return 0;
}
