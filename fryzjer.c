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
#define KEY_CLIENTS 45284
#define NUM_CHAIRS 2
#define NUM_CLIENTS 1


#define CLIENT_PRESENT 1
#define NO_CLIENT 0

// waiting room spot
struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};

int cut_hair(const int* clients_money){

    //price
    int clients_amount = clients_money[COUNTER-1];
    int price = (rand() % clients_amount);
    printf("PRICE FOR CUT %d\n", price);
    int haircut_t = (int) (((float)price)*0.8);
    sleep(haircut_t);
    printf("Haircut done!\n");
    return price;
}


bool can_change(const int nominals[COUNTER-1], int counter_sem_id, volatile int* counter, int *change){
    printf("check if can change\n");
    reduce(counter_sem_id, 0);

    int change_banknotes[COUNTER];
    for (int i = 0; i < COUNTER; i++) change_banknotes[i] = 0;

    for (int i = COUNTER-2; i>=0; i--){
        while((counter[i] > 0) && ((*change - nominals[i]) >= 0)){
            printf("so far works %d nominal %d i %d\n", (*change - nominals[i]), nominals[i], i);
            *change -= nominals[i];
            counter[i] -= 1;
            change_banknotes[i] +=1;
            counter[COUNTER-1] -= nominals[i];
        }
    }

    // return money to the counter if the barber couldn't return full change
    increase(counter_sem_id, 0);

    if (*change != 0){
        for (int i = 0; i < (COUNTER-1); i++){
            counter[i] += change_banknotes[i];
            counter[COUNTER-1] += change_banknotes[i] * nominals[i];
        }
        return false;
    }

    return true;
}


//TODO: check what will happen if the counter will be allowed to used by other barber
// while the barber which freed the counter will start to drop money to counter concurrently
void count_change(int change_banknotes[COUNTER], int price,  int clients_money[COUNTER], volatile int* counter, int counter_sem_id, const int nominals[COUNTER-1]){
    printf("Count change problem %d, PRICE %d \n", clients_money[COUNTER-1], price);

    int change_amount = clients_money[COUNTER-1] - price;
    int temp_am = change_amount;

    // only one barber can perform active waiting
    if (counter[COUNTER-1] < clients_money[COUNTER-1]

    || (can_change(nominals, counter_sem_id, counter, &temp_am)==false)){
        // TODO: idk if this sem for reading is needed
        // for now i think in help so that not more than one semaphore is waiting for acquiring the counter
        reduce(counter_sem_id, 1);
        temp_am = change_amount;
        while (counter[COUNTER-1] < clients_money[COUNTER-1] ||
        (can_change(nominals, counter_sem_id, counter, &temp_am) ==  false)) {
            //spinning
            temp_am = change_amount;
        }
        increase(counter_sem_id, 1);

    }



    // add clients money to the counter
    for (int i = 0; i < (COUNTER-1); i++){
        counter[i] += clients_money[i];
        counter[COUNTER-1] += clients_money[i] * nominals[i];
    }

};






void give_money_to_client(int clients_id,  const int change[COUNTER], int msgq_id){

    struct client money_to_client;
    money_to_client.mtype = (long) clients_id;
    for (int i = 0; i < COUNTER; i++) money_to_client.clients_money[i] = change[i];

    if (msgsnd(msgq_id, &money_to_client, (sizeof(struct client) - sizeof(long)), 0) == -1){
        perror("Give money to client error");
        exit(1);
    }

}


int main(int argc, char* argv[]) {
    printf("Start barber\n");

    srand(time(NULL));

    int nominals[COUNTER] = {1,2,5};

    int counter_id, wr_id, ch_id, countner_sems_id, num_clients_mutex_id, clients_id;

    volatile int* counter;
    volatile int* num_clients;
    struct client wr_spot;

    // counter stores [num of 1's, num of 2's, num of 5's, amount]
    counter_id = shmget(KEY_COUNTER, (COUNTER)*sizeof(int), IPC_CREAT|0600);
    if (counter_id == -1) {
        perror("Create counter shared memory error");
        exit(1);
    }

    counter = (int*)shmat(counter_id, NULL, 0);
    if (counter == NULL) {
        perror("Attach counter error");
        exit(1);
    }

    for (int i = 0; i<(COUNTER-1); i++){
        counter[i] = 10;
        counter[COUNTER-1] += nominals[i]*counter[i];
    }

    ch_id = semget(KEY_CHAIRS, 1, IPC_CREAT|0600);
    if (ch_id == -1) {
        perror("Create chairs shared memory error");
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



    countner_sems_id = semget(KEY_COUNTER, 2, IPC_CREAT|0600);
    if (countner_sems_id == -1) {
        perror("Create counter semaphores error");
        exit(1);
    }




    if (semctl(ch_id, 0, SETVAL, (int)NUM_CHAIRS) == -1){
        perror("Set semaphore value to number of chairs error");
        exit(1);
    }

    if (semctl(countner_sems_id, 0, SETVAL, 1) == -1){
        perror("Set semaphore 1 for writing counter error");
        exit(1);
    }

    if (semctl(countner_sems_id, 1, SETVAL, 1) == -1){
        perror("Set semaphore 2 for reading counter error");
        exit(1);
    }

    // waiting room
    wr_id = msgget(KEY_WAITING_ROOM, IPC_CREAT|0600);
    if (wr_id == -1) {
        perror("Create waiting room error");
        exit(1);
    }


    // message queue

    while (true) {
        //if we get a msg from queue we need to already have a chair for client
        // because it will be removed from queue after receiving the message
        // pick client from waiting room
        printf("waiting for a client\n");

        if (msgrcv(wr_id,&wr_spot, (sizeof(struct client) - sizeof(long)), CLIENT_PRESENT, 0) == -1){
            perror("Checking waiting room error");
            exit(1);
        }

        reduce(num_clients_mutex_id, 0);
        *num_clients-=1;
        increase(num_clients_mutex_id,0);

        // find free chair
        reduce(ch_id, 0);

        // charge customer for a service
        // the money is in the received message

        // cut client's hair
        int price = cut_hair(wr_spot.clients_money);

        // free chair
        increase(ch_id, 0);
        // count change for a client and if it cannot then wait when it will be possible
        int change_banknotes[COUNTER];
        for (int i = 0; i < COUNTER; i++) change_banknotes[i] = 0;

        count_change(change_banknotes, price, wr_spot.clients_money, counter, countner_sems_id, nominals);
        printf("Change counted\n");
        // give change to a client
        give_money_to_client(wr_spot.clients_id, change_banknotes, wr_id);
        printf("Money gave to a client\n");
    }

    return 0;
}
