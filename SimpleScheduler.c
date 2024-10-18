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

#define MaxProcesses 250;

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

}process;

typedef struct{
   process queue[250];
   int rear;
   int front;
} ProcessQueue;


ProcessQueue running_q;
ProcessQueue terminated_q;

void enqueue(ProcessQueue p,process p1){
    if(p.rear=249){
        printf("queue is full\n");
        exit(EXIT_FAILURE);
    }

    p.queue[p.rear]=p1;
    p.rear++;
}

void dequeu(ProcessQueue p){
    if(p.rear==0){
        printf("queue is empty\n");
        exit(EXIT_FAILURE);
    }

    p.front++;
}

void printQueue(ProcessQueue p){
    for(int i=0;i<p.rear;i++){
        printf("command-%d was -%s\n",p.queue[i].pid,p.queue[i].cmd);
        printf("Starting time for process: %s\n", ctime(&p.queue[i].initial_time));
        printf("End time for process: %s\n", ctime(&p.queue[i].end_time));
        printf("Waiting time for process is %f\n",p.queue[i].waiting_time);
        printf("Completion Time for process is %f\n",p.queue[i].completion_time);
        printf("/n");
    }
}


