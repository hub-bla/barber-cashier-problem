#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include "semaphore_ops.h"

#define COUNTER 4

#define KEY_COUNTER 45281
#define KEY_CHAIRS 45282
#define KEY_WAITING_ROOM 45283
#define KEY_SPOTS 45284


#define DELAY 10000

struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};


void make_money(int *money, const int *nominals, int work_count);

bool can_sit(int spots_in_wr_mutex, volatile int* spots_in_wr);

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

    int wr_id, spots_in_wr_mutex, spots_in_wr_id;
    int c_id;
    volatile int* spots_in_wr;
    c_id = atoi(argv[1]);

    wr_id = msgget(KEY_WAITING_ROOM, IPC_CREAT|0600);
    if (wr_id == -1) {
        perror("Create waiting room error");
        exit(1);
    }


    spots_in_wr_mutex = semget(KEY_SPOTS, 1, IPC_CREAT|0600);
    if (spots_in_wr_mutex == -1) {
        perror("Create mutex for available spots in waiting room");
        exit(1);
    }



    spots_in_wr_id = shmget(KEY_SPOTS, sizeof(int), IPC_CREAT|0600);
    if (spots_in_wr_id == -1) {
        perror("Create number of available spots in waiting room");
        exit(1);
    }

    spots_in_wr = (int*)shmat(spots_in_wr_id, NULL, 0);
    if (spots_in_wr == NULL) {
        perror("Attach counter error");
        exit(1);
    }


    while (true){

        make_money(money, nominals,0);
        print_action("Money earned", c_id);

        if (can_sit(spots_in_wr_mutex, spots_in_wr) ==  false){
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


bool can_sit(int spots_in_wr_mutex, volatile int* spots_in_wr) {
    reduce(spots_in_wr_mutex, 0);
    if (*spots_in_wr == 0) {
        increase(spots_in_wr_mutex,0);
        return false;
    }
    // take a sit
    *spots_in_wr-=1;
    increase(spots_in_wr_mutex, 0);
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