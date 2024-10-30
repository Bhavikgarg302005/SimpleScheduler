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
#include <sys/ipc.h>
#include "sharedMemory.c"

int NCPU;
int TSLICE;
PriorityQueue* queue;
sharedArray* arr;


pid_t pids[256];

int it = 0;
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




volatile sig_atomic_t terminate = 0;

char* process_name[256];
void hstry_print(int signum) {
    (void) signum;
    terminate = 1;
    sem_wait(&arr->mutex);
    for (int i = 0; i < it; i++) {
        if (kill(pids[i], 0) == 0) {
            arr->completion[i]+=TSLICE;
            kill(pids[i], SIGCONT);
        }
        int status;
        waitpid(pids[i], &status, 0);
    }                                                                                       
    sem_post(&arr->mutex);
    printf("Scheduler history: \n");
    sem_wait(&arr->mutex);
    for (int i = 0; i < it; i++) {      
        printf("%d----->",i+1);                                                                                                                                
        printf("PID: %d\nName: %s\nCompletion Time: %d\nWaiting Time: %d\n",pids[i], arr->programs[i], arr->completion[i],arr->waiting[i]);
    }
    sem_post(&arr->mutex);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
    shmdt(queue);
    shmdt(arr);

    printf("Scheduler exiting...\n");
    exit(0);
}


void scheduler(PriorityQueue* queue) {
    Process P[NCPU];
    int num = get_num_process(queue);
    int cycle;
    if(NCPU>num){
        cycle=num;
    }
    else{
        cycle=NCPU;
    }

    while (cycle > 0) {
            if(NCPU>num){
              cycle=num;
            }
            else{
               cycle=NCPU;
            }
        for (int i = 0; i < cycle; i++) {
            dequeue(queue, &P[i]);
            
            if (P[i].pid == -2) {
                P[i].pid = fork();
                if (P[i].pid == -1) {
                    perror("fork");
                    return;
                }
                char* args[2];
                args[0] = P[i].cmd;
                args[1] = NULL;
        
                if (P[i].pid == 0) {
                    execv(args[0], args);
                    perror("Error");
                    exit(EXIT_FAILURE);
                } else {
                    sem_wait(&arr->mutex);
                    pids[it] = P[i].pid;    
                    arr->programs[it]=malloc(strlen(P[i].cmd)+1);
                    strcpy(arr->programs[it], P[i].cmd);
                    it++;
                    sem_post(&arr->mutex);
                }
            }
            sem_wait(&arr->mutex);
            arr->completion[i]+=TSLICE;
            sem_post(&arr->mutex);
            if (kill(P[i].pid, SIGCONT) == -1) {
               sem_wait(&arr->mutex);
               arr->completion[i]-=TSLICE;
               sem_post(&arr->mutex);
               perror("SIGCONT");
               exit(EXIT_FAILURE);

            }
        }
        
        usleep(TSLICE*1000);
        
        for (int i = 0; i < cycle; i++) {
            int z;
            int x=waitpid(P[i].pid,&z,WNOHANG);
            if(x==0){
                if (kill(P[i].pid, 0) == 0) {
                 enqueue(queue, &P[i]);
                 sem_wait(&arr->mutex);
                 arr->waiting[i]+=TSLICE;
                 sem_post(&arr->mutex);
                 if (kill(P[i].pid, SIGTSTP) == -1) {
                    sem_wait(&arr->mutex);
                    arr->waiting[i]-=TSLICE;
                    sem_post(&arr->mutex);
                    perror("SIGTSTP");
                    exit(EXIT_FAILURE);
                }
             }
            }
            else if(x==-1){
                perror("Error");
            }
        }
        num -= cycle;
    }
}
int main(int argc, char** argv){
    key_t key = ftok("keyfile", 'R');
    int shmid = shmget(key, sizeof(int), 0666);
    int shmid2= shmget(key,sizeof(int),0666);
    int x=create_priority_queue(key,&queue);
    int y=create(key,&arr);
    pid_t shell_pid = atoi(argv[2]);
    NCPU = atoi(argv[3]);
    TSLICE = atoi(argv[4]);

    pid_t pid = getpid();
    
    queue = shmat(shmid, NULL, 0);
    arr= shmat(shmid2,NULL,0);
    if (queue == (PriorityQueue*)(-1)) {
        perror("shmat122");
        return 1;
    }
    if(arr == (sharedArray*)(-1)){
        printf("error in shared Array");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, hstry_print) == SIG_ERR) {
        perror("Error in signal handler");
        exit(EXIT_FAILURE);
    }
    
    while (!terminate){
        scheduler(queue);
    }
    return 0;
}
