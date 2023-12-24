#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "semaphore_ops.h"

#define COUNTER 4
#define KEY_COUNTER 45281
#define KEY_CHAIRS 45282
#define KEY_WAITING_ROOM 45283
#define NUM_CHAIRS 2
#define KEY_CLIENTS 45284
#define NUM_CLIENTS 1
//through main we will give client ids
#define CLIENT_PRESENT 1

struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};


void make_money(int *money, const int *nominals, int work_count){
    printf("%d\n", work_count);
    int idx =  rand()%3;
    printf("idx: %d\n", idx);
    money[idx] += 1;
    money[COUNTER-1] += nominals[idx];

    printf("1's: %d 2's: %d 5's: %d SUM: %d\n", money[0],money[1],money[2],money[3]);
    // there is 50% chance to earn more money
    if (work_count < 10) if (rand()>0.5) make_money(money,  nominals, work_count+1);

}


bool can_sit(int num_clients_mutex_id, volatile int* num_clients) {
    reduce(num_clients_mutex_id, 0);
    if (*num_clients == NUM_CLIENTS) {
        increase(num_clients_mutex_id,0);
        return false;
    }
    // take a sit
    *num_clients+=1;
    increase(num_clients_mutex_id, 0);
    return true;
}


void inform_barber(int wr_id, int* money, int c_id){
    struct client arival;
    arival.mtype = CLIENT_PRESENT;
    for (int i = 0; i<COUNTER; i++) {
        arival.clients_money[i] = money[i];
        printf("MONEY SEND TO BARBER %d\n", arival.clients_money[i]);
        money[i] = 0;
    }
    printf("SUM %d\n", arival.clients_money[COUNTER-1]);
    arival.clients_id = c_id;

    if (msgsnd(wr_id, &arival, (sizeof(struct client) - sizeof(long)), 0) == -1){
        perror("Inform barber error");
        exit(1);
    }


}


void wait_for_change(int wr_id, int *money, int c_id){
    struct client recieved_money;

    if (msgrcv(wr_id,&recieved_money, (sizeof(struct client) - sizeof(long)), c_id, 0) == -1){
        perror("Receiving money error");
        exit(1);
    }

    for (int i = 0; i<COUNTER; i++){
        money[i] = recieved_money.clients_money[i];
    }

}

int main(int argc, char* argv[]){
    srand(time(NULL));
    // [1's, 2's, 5's, amount]
    int money[COUNTER] = {0,0,0,0};
    int nominals[COUNTER-1] = {1,2,5};

    int wr_id, num_clients_mutex_id, clients_id, c_id;
    volatile int* num_clients;

    c_id = 10;
    printf("CLIENT ID: %d, Made some money", c_id);

    wr_id = msgget(KEY_WAITING_ROOM, IPC_CREAT|0600);
    if (wr_id == -1) {
        perror("Create waiting room error");
        exit(1);
    }


    num_clients_mutex_id = semget(KEY_CLIENTS, 1, IPC_CREAT|0600);
    if (num_clients_mutex_id == -1) {
        perror("Create mutex for number of clients in waiting room error");
        exit(1);
    }

    if (semctl(num_clients_mutex_id, 0, SETVAL, 1) == -1){
        perror("Set num_client mutex to 1 error");
        exit(1);
    }

    clients_id = shmget(KEY_CLIENTS, sizeof(int), IPC_CREAT|0600);
    if (clients_id == -1) {
        perror("Create number of clients shared memory error");
        exit(1);
    }

    num_clients = (int*)shmat(clients_id, NULL, 0);
    if (num_clients == NULL) {
        perror("Attach counter error");
        exit(1);
    }

    while (true){
        printf("Start client\n");

        //make money

        make_money(money, nominals,0);

        // go to barber shop
        if (can_sit(num_clients_mutex_id, num_clients) ==  false){
            printf("Cannot sit\n");
            continue;
        }

        inform_barber(wr_id, money, c_id);
        printf("Barber informed\n");

        wait_for_change(wr_id, money, c_id);
        printf("Money received\n");

    }
    // how message queue will work
    // client sends that is waiting in the waiting room
    // then it will wait for the barber will receive that
    // barber will pick a chair, do the haircut, and while that client will be waiting for the barber
    // to give him the change through message with the unique client id that was sent earlier with money

    return 0;

}