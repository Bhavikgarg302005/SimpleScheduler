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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MaxProcesses 250;

int ncpu;
int TSLICE;

sem_t scheduler;

typedef struct {
    char* cmd;
    int pid;
    time_t initial_time;
    time_t end_time;
    double completion_time;
    double waiting_time;
    int state;

}process;

typedef struct{
   process queue[250];
   int rear;
   int front;
} ProcessQueue;


ProcessQueue* scheduler_q;
ProcessQueue* terminated_q;

void enqueue(ProcessQueue* p,process p1){
    if(p->rear=250){
        printf("queue is full\n");
        exit(EXIT_FAILURE);
    }

    p->queue[p->rear]=p1;
    p->rear++;
}

void dequeu(ProcessQueue p){
    if(p.rear==0){
        printf("queue is empty\n");
        exit(EXIT_FAILURE);
    }

    p.front++;
}

void printQueue(ProcessQueue* p){
    for(int i=0;i<p->rear;i++){
        printf("command-%d was -%s\n",p->queue[i].pid,p->queue[i].cmd);
        printf("Starting time for process: %s\n", ctime(&p->queue[i].initial_time));
        printf("End time for process: %s\n", ctime(&p->queue[i].end_time));
        printf("Waiting time for process is %f\n",p->queue[i].waiting_time);
        printf("Completion Time for process is %f\n",p->queue[i].completion_time);
        printf("/n");
    }
}

//for handling the process that are completed
void handler(int signo){
    int status;
    pid_t pid;
    //checking for completed processes
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){
        for(int i=0;i<scheduler_q->rear;i++){
            if(scheduler_q->queue[i].pid==pid){
                //sent to terminated queue:
                enqueue(terminated_q,scheduler_q->queue[i]);

                //remove from scheduer queue:
                for (int j = i; j < scheduler_q->rear; j++) {
                    scheduler_q->queue[j] = scheduler_q->queue[j + 1];
                }
                scheduler_q->rear--;

            }
        }
    }
}


int main(int argc, char** argv){
    //Error check for ncpus and TSLICE
    if(argc<3 || argc>3){
       printf("Error in processing file");
       exit(EXIT_FAILURE);
    }
    ncpu=atoi(argv[1]);
    TSLICE=atoi(argv[2]);

    sem_init(&scheduler,0,0);
 
    scheduler_q->rear=0; 
    terminated_q->rear=0;

    int scheduler_child=fork();
    //scheduler process started:
    if(scheduler_child<0){
        printf("Error in fork\n");
        exit(EXIT_FAILURE);
    }
    if(scheduler_child==0){
        //for roundRobin
        int iter=0;
        while(1){
           if(iter>=scheduler_q->rear){
                iter=0;
           }
           int check_num=ncpu;
           while(iter<scheduler_q->rear && check_num>0){
            //finding ready for execution
            if(scheduler_q->queue[iter].state==1){
               //sent for running
               scheduler_q->queue[iter].state=0;
               check_num--;
            }
           }
           usleep(TSLICE * 1000);
           //shift the processes
           for(int i=0;i<scheduler_q->rear;i++){
            //senting running to ready
            if(scheduler_q->queue[i].state==0){

            }
           }


        }
       
    }
    //shellProcess started:
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
}

