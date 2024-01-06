#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>

#define COUNTER 4
#define KEY_COUNTER 45281
#define BARBERS 5
#define CLIENTS 4



int main(int argc, char* argv[]) {
    // initialize counter
//    int nominals[COUNTER] = {1,2,5};
    int counter_id;
    counter_id = shmget(KEY_COUNTER, (COUNTER)*sizeof(int), IPC_CREAT|0600);
    if (counter_id == -1) {
        perror("Create counter shared memory error");
        exit(1);
    }
    volatile int* counter;
    counter = (int*)shmat(counter_id, NULL, 0);
    if (counter == NULL) {
        perror("Attach counter error");
        exit(1);
    }
    counter[0] = 1;
    counter[1] = 1;
    counter[2] = 1;
    counter[3] = 1+2+5;
    shmdt((void *)counter);

    for (int i = 10; i < (BARBERS+10); i++) {
        if (fork()==0){
            printf("Make barber\n");
            char id[10];
            sprintf(id, "%d", i);
            execlp("./fryzjer", "./fryzjer", id, NULL);
            printf("Make barber error\n");
        }
    }

    for (int i = 10; i < (CLIENTS+10); i++){
        if (fork()==0){
            printf("Make client\n");
            char id[10];
            sprintf(id, "%d", i);
            execlp("./klient", "./klient", id,NULL);
            printf("Make client error\n");
        }
    }

    wait(NULL);
    return 0;
}
