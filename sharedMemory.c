#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_QUEUE_SIZE 150
#define MAX_ITEM_SIZE 150

typedef struct Process {
    char cmd[MAX_ITEM_SIZE];
    pid_t pid;
    int priority;
    struct timeval initial_time;
    struct timeval end_time;
    long long completion_time;
    long long waiting_time;
    int state;
} Process;
typedef struct sharedArray
{
    pid_t pid[1000];
    char* programs[1000];
    int  waiting[1000];
    int  completion[1000];
    sem_t mutex;
    int it;
} sharedArray;

typedef struct PriorityQueue {
    Process processes[MAX_QUEUE_SIZE];
    int rear;
    int num_process;
    sem_t mutex;
    sem_t empty;
    sem_t full;
    int priority;
} PriorityQueue;

//for making it happend in sharedMemory using key
int create_priority_queue(key_t key, PriorityQueue **queue) {
    int shmid = shmget(key, sizeof(PriorityQueue), IPC_CREAT | 0666);
    //error check
    if (shmid == -1) {
        perror("shmget");
        return -1;
    }

    *queue = (PriorityQueue *)shmat(shmid, NULL, 0);
    //error check
    if (*queue == (PriorityQueue *)(-1)) {
        perror("shmat");
        return -1;
    }
    //made semaphores for ensuring no race condition
    sem_init(&(*queue)->mutex, 1, 1);
    sem_init(&(*queue)->empty, 1, MAX_QUEUE_SIZE);
    sem_init(&(*queue)->full, 1, 0);

    (*queue)->num_process = 0;
    (*queue)->rear = 0;
    return 0;
}
//this create is for SharedArray which is required in Scheduler to hold completion time and waiting time
int create(key_t key,sharedArray **arr){
        int shmid = shmget(key, sizeof(sharedArray), IPC_CREAT | 0666);
        if (shmid == -1) {
        perror("shmget");
        return -1;
        }
        *arr = (sharedArray *)shmat(shmid, NULL, 0);
        if (*arr == (sharedArray *)(-1)) {
         perror("shmat");
         return -1;
        }
        sem_init(&(*arr)->mutex, 1, 1);
        (*arr)->it=0;
        return 0;
}
//Add in queue
void enqueue(PriorityQueue* queue, const Process* process) {
    sem_wait(&queue->empty);
    sem_wait(&queue->mutex);
    queue->processes[queue->rear] = *process;
    queue->rear++;
    queue->num_process++;
    
    sem_post(&queue->mutex);
    sem_post(&queue->full);
}

int dequeue(PriorityQueue* queue, Process* process) {
    if (queue->rear == -1) {
        return 0;  // Queue is empty
    }
    
    sem_wait(&queue->full);
    sem_wait(&queue->mutex);

    *process = queue->processes[0];
    for (int i = 0; i < queue->rear; i++) {
        queue->processes[i] = queue->processes[i + 1];
    }
    queue->num_process--;
    queue->rear--;

    sem_post(&queue->mutex);
    sem_post(&queue->empty);

    return 1;  // Successfully dequeued
}
int get_num_process(PriorityQueue* queue){
    sem_wait(&queue->mutex);
    int ret = queue->num_process;
    sem_post(&queue->mutex);

    return ret;
}
int is_empty(PriorityQueue *queue) {
    sem_wait(&queue->mutex);
    int empty = (queue->rear == -1);
    sem_post(&queue->mutex);

    return empty;
}

int is_full(PriorityQueue *queue) {
    sem_wait(&queue->mutex);
    int full = (queue->rear == MAX_QUEUE_SIZE - 1);
    sem_post(&queue->mutex);

    return full;
}
