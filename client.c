#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include "fryzjerzy_semaphore_ops.h"

#define COUNTER 4

#define KEY_COUNTER 45281
#define KEY_CHAIRS 45282
#define KEY_WAITING_ROOM 45283
#define KEY_CLIENTS 45284

// number of clients in the waiting room
#define NUM_CLIENTS 5


#define DELAY 10000

struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};


void make_money(int *money, const int *nominals, int work_count);

bool can_sit(int num_clients_mutex_id, volatile int* num_clients);

void inform_barber(int wr_id, int* money,  int c_id, int CLIENT_PRESENT);

void wait_for_change(int wr_id, int *money,  int c_id);

void print_action(char* text, int c_id);


int main(int argc, char* argv[]){
    char *clientPresentStr = getenv("CLIENT_PRESENT");
    int CLIENT_PRESENT = atoi(clientPresentStr);
    srand(time(NULL));
    // [1's, 2's, 5's, amount]
    int money[COUNTER] = {0,0,0,0};
    int nominals[COUNTER-1] = {1,2,5};

    int wr_id, num_clients_mutex_id, clients_id;
    int c_id;
    volatile int* num_clients;
    c_id = atoi(argv[1]);

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

        make_money(money, nominals,0);
        print_action("Money earned", c_id);

        if (can_sit(num_clients_mutex_id, num_clients) ==  false){
            print_action("Cannot sit", c_id);
            usleep(200);
            continue;
        }

        inform_barber(wr_id, money, c_id, CLIENT_PRESENT);
        print_action("Barber informed", c_id);

        wait_for_change(wr_id, money, c_id);
        print_action("Money received", c_id);
        int r = (rand()%DELAY)+1;
        usleep(r);
        sleep(1);
    }
    return 0;
}

void print_action(char* text, int c_id){
    printf("Client %d: %s\n", c_id, text);
}

void make_money(int *money, const int *nominals, int work_count){
    int work_time = rand() % 1000;
    int idx =  rand()%3;
    usleep(work_time);
    money[idx] += 1;
    money[COUNTER-1] += nominals[idx];

    // there is 50% chance to earn more money
    float prob = rand();
    if (work_count < 2) if (prob>0.5) make_money(money,  nominals, work_count+1);

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


void inform_barber(int wr_id, int* money, int c_id, int CLIENT_PRESENT){
    struct client arival;
    arival.mtype = CLIENT_PRESENT;
    for (int i = 0; i<COUNTER; i++) {
        arival.clients_money[i] = money[i];
        money[i] = 0;
    }

    arival.clients_id = c_id;

    if (msgsnd(wr_id, &arival, (sizeof(struct client) - sizeof(long)), 0) == -1){
        perror("Inform barber error");
        exit(1);
    }
}


void wait_for_change(int wr_id, int *money, int c_id){
    struct client received_money;

    if (msgrcv(wr_id,&received_money, (sizeof(struct client) - sizeof(long)), c_id, 0) == -1){
        perror("Receiving money error");
        exit(1);
    }

    for (int i = 0; i<COUNTER; i++){
        money[i] = received_money.clients_money[i];
    }

}