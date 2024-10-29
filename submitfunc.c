#include<stdio.h>
#include<stdlib.h>
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

//y will store the result so bhavik pass a y to it
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
int main()
{
    char i[1024];
    printf("Enter command: ");
    if (fgets(i, sizeof(i), stdin) == NULL) {
        printf("Error in fgets");
        return 1;
    }
    int len =strlen(i);
    i[len-1]='\0';
    char*y=(char*)malloc(100);
    SplitSubmit(i,y);
    if(y!=NULL)
    {
        printf("%s",y);
    }
    
    return 0;
}
