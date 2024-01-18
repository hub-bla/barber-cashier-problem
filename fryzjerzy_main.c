#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <stdlib.h>

#define COUNTER 4

#define KEY_COUNTER 45281
#define NUM_WAITING_BARBERS_KEY 46271
#define KEY_DEBTS 45321

#define BASE_ONES 0
#define BASE_TWOS 0
#define  BASE_FIVES 0


int main(int argc, char* argv[]) {
    char *clientsStr = getenv("CLIENTS");
    int CLIENTS = atoi(clientsStr);
    char *barbersStr = getenv("BARBERS");
    int BARBERS = atoi(barbersStr);

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
    int  waiting_barbers_id;
    waiting_barbers_id = shmget(NUM_WAITING_BARBERS_KEY, sizeof(int), IPC_CREAT|0600);
    if (waiting_barbers_id == -1) {
        perror("Create waiting barbers shared memory error");
        exit(1);
    }

    volatile int* waiting_barbers_num;
    waiting_barbers_num = (int*)shmat(waiting_barbers_id, NULL, 0);
    if (waiting_barbers_num == NULL) {
        perror("Attach counter error");
        exit(1);
    }

    int debts_id = shmget(KEY_DEBTS, (CLIENTS)*sizeof(int), IPC_CREAT|0600);
    if (debts_id == -1) {
        perror("Create debts shared memory error");
        exit(1);
    }

    volatile int *debts = (int*)shmat(debts_id, NULL, 0);
    if (debts == NULL) {
        perror("Attach debts error");
        exit(1);
    }

    for (int i=0; i<CLIENTS; i++) debts[i] = 0;

    *waiting_barbers_num = 0;


    counter[0] = BASE_ONES;
    counter[1] = BASE_TWOS;
    counter[2] = BASE_FIVES;
    counter[3] = (BASE_ONES*1)+(BASE_TWOS*2)+(BASE_FIVES*5);
    shmdt((void *)counter);

    for (int i = 1; i <= BARBERS; i++) {
        if (fork()==0){
            printf("Make barber\n");
            char id[10];
            sprintf(id, "%d", i);
            execlp("./fryzjer", "./fryzjer", id, NULL);
            printf("Make barber error\n");
        }
    }

    for (int i = 1; i <= (CLIENTS); i++){
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
