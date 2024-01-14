#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdlib.h>

#define COUNTER 4
#define KEY_COUNTER 45281
#define BARBERS 10
#define CLIENTS 5

#define BASE_ONES 0
#define BASE_TWOS 0
#define  BASE_FIVES 0


int main(int argc, char* argv[]) {
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
    counter[0] = BASE_ONES;
    counter[1] = BASE_TWOS;
    counter[2] = BASE_FIVES;
    counter[3] = (BASE_ONES*1)+(BASE_TWOS*2)+(BASE_FIVES*5);
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
