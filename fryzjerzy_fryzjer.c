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
#define KEY_CHANGE_QUEUE 46283
#define KEY_DEBTS 45321
#define NUM_WAITING_BARBERS_KEY 46271
#define WAITING_BARBERS_QUEUE_KEY  47271


#define NUM_CHAIRS 1

// number of clients in the waiting room
#define NUM_CLIENTS 5


#define DELAY 10000

// waiting room spot
struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};

struct change_wait {
    long mtype;
};

int determine_price(const int* clients_money, const volatile int* counter, volatile int * debts, int clients_id,int barber_id);

void cut_hair(int price);
void put_in_counter(volatile int * counter, const int money_for_service[COUNTER], int counter_sem_id,
                    int waiting_barbers_mutex, int volatile *waiting_barbers_num, int waiting_barbers_queue_id);
bool can_change(int change_banknotes[COUNTER], int counter_sem_id, volatile int* counter, int *change);


void count_change(int change,  int clients_money[COUNTER],
                  volatile int* counter,
                  int counter_sem_id,
                  int barber_id, int waiting_barbers_mutex, volatile int* waiting_barbers_num,
                  int waiting_barbers_queue_id, const int BARBERS, int clients_id, volatile int* debts, const int CLIENTS);

void give_money_to_client(int clients_id,  const int change[COUNTER], int msgq_id);

void take_money_from_client(int clients_money[COUNTER], int paid_money[COUNTER], int price);

void print_action(char* text, int barber_id);

int nominals[COUNTER] = {1,2,5};

//TODO: make client ids from 1? ----DONE----
//TODO: make sure theres no possibility of deadlock,
// if the last barber that is not waiting needs wait for change it sets that change as a debt of local ----DONE----
// TODO: clean up to functions so main is tidy
int main(int argc, char* argv[]) {

    srand(time(NULL));
    char *clientPresentStr = getenv("CLIENT_PRESENT");
    const int CLIENT_PRESENT = atoi(clientPresentStr);
    char *clientsStr = getenv("CLIENTS");
    const int CLIENTS = atoi(clientsStr);
    char *barbersStr = getenv("BARBERS");
    const int BARBERS = atoi(barbersStr);

    int counter_id, wr_id, ch_id, counter_sems_id, num_clients_mutex_id, clients_id, barber_id, debts_id;

    int  waiting_barbers_id, waiting_barbers_mutex, waiting_barbers_queue_id;


    barber_id = atoi(argv[1]);
    volatile int* counter;
    volatile int* debts;
    volatile int* waiting_barbers_num;
    volatile int* num_clients;
    struct client wr_spot;


    // counter stores [num of 1's, num of 2's, num of 5's, amount]
    counter_id = shmget(KEY_COUNTER, (COUNTER)*sizeof(int), IPC_CREAT|0600);
    if (counter_id == -1) {
        perror("Create counter shared memory error");
        exit(1);
    }

    waiting_barbers_id = shmget(NUM_WAITING_BARBERS_KEY, sizeof(int), IPC_CREAT|0600);
    if (waiting_barbers_id == -1) {
        perror("Create waiting barbers shared memory error");
        exit(1);
    }

    debts_id = shmget(KEY_DEBTS, (CLIENTS)*sizeof(int), IPC_CREAT|0600);
    if (debts_id == -1) {
        perror("Create debts shared memory error");
        exit(1);
    }

    counter = (int*)shmat(counter_id, NULL, 0);
    if (counter == NULL) {
        perror("Attach counter error");
        exit(1);
    }

    waiting_barbers_num = (int*)shmat(waiting_barbers_id, NULL, 0);
    if (waiting_barbers_num == NULL) {
        perror("Attach counter error");
        exit(1);
    }


    debts = (int*)shmat(debts_id, NULL, 0);
    if (debts == NULL) {
        perror("Attach debts error");
        exit(1);
    }

    ch_id = semget(KEY_CHAIRS, 1, IPC_CREAT|0600);
    if (ch_id == -1) {
        perror("Create chairs shared memory error");
        exit(1);
    }


    waiting_barbers_mutex = semget(NUM_WAITING_BARBERS_KEY, 1, IPC_CREAT|0600);
    if (waiting_barbers_mutex == -1) {
        perror("Create mutex for number of reading number of waiting barbers error");
        exit(1);
    }


    waiting_barbers_queue_id = semget(WAITING_BARBERS_QUEUE_KEY, 1, IPC_CREAT|0600);
    if (waiting_barbers_queue_id == -1) {
        perror("Create queue for waiting barbers");
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


    counter_sems_id = semget(KEY_COUNTER, 2, IPC_CREAT|0600);
    if (counter_sems_id == -1) {
        perror("Create counter semaphores error");
        exit(1);
    }


    // waiting room
    wr_id = msgget(KEY_WAITING_ROOM, IPC_CREAT|0600);
    if (wr_id == -1) {
        perror("Create waiting room error");
        exit(1);
    }


    while (true) {

        print_action("Waiting for a client", barber_id);
        //pick client
        if (msgrcv(wr_id,&wr_spot, (sizeof(struct client) - sizeof(long)), CLIENT_PRESENT, 0) == -1){
            perror("Checking waiting room error");
            exit(1);
        }
        print_action("Picked client", barber_id);

        reduce(num_clients_mutex_id, 0);
        *num_clients-=1;
        increase(num_clients_mutex_id,0);
        // find free chair
        reduce(ch_id, 0);


        int price = determine_price(wr_spot.clients_money, counter, debts, wr_spot.clients_id, barber_id);
        // pick needed enough money from client
        int money_for_service[COUNTER];
        for (int i = 0; i< COUNTER; i++) money_for_service[i] = 0;
        printf("Barber %d Check clients money before %d %d %d %d\n", barber_id,wr_spot.clients_money[0],wr_spot.clients_money[1],wr_spot.clients_money[2],wr_spot.clients_money[3]);
        printf("Barber %d Price %d\n", barber_id,price);
        take_money_from_client(wr_spot.clients_money, money_for_service, price);

        printf("Barber %d Check clients money after %d %d %d %d\n", barber_id, wr_spot.clients_money[0],wr_spot.clients_money[1],wr_spot.clients_money[2],wr_spot.clients_money[3]);
        put_in_counter(counter, money_for_service, counter_sems_id,  waiting_barbers_mutex, waiting_barbers_num, waiting_barbers_queue_id);
        cut_hair(price);

        // free chair
        increase(ch_id, 0);

        int change_amount = money_for_service[COUNTER-1] - price;
        // check if the barber needs to count change
        if (change_amount == 0){
            wr_spot.mtype = wr_spot.clients_id;
            if (msgsnd(wr_id, &wr_spot, (sizeof(struct client) - sizeof(long)), 0) == -1){
                perror("Thank customer for appointment error");
                exit(1);
            }
            print_action("Customer paid exact price", barber_id);
            continue;
        }

        // count change for a client and if it cannot then wait when it will be possible

        count_change(change_amount, wr_spot.clients_money, counter,
                     counter_sems_id, barber_id, waiting_barbers_mutex,
                     waiting_barbers_num, waiting_barbers_queue_id, BARBERS, wr_spot.clients_id, debts, CLIENTS);
        print_action("Change counted", barber_id);

        give_money_to_client(wr_spot.clients_id, wr_spot.clients_money, wr_id);
        print_action("Money gave to a client", barber_id);
        int r = (rand()%DELAY)+1;
        usleep(r);
        sleep(1);
    }

    return 0;
}



void take_money_from_client(int clients_money[COUNTER], int paid_money[COUNTER], int price){

    int temp_price = price;
    int end_picks[COUNTER-1] = {0,0,0};
    int distances[COUNTER-1] = {1, 1 ,1};

    for (int i =COUNTER-2; i>=0; i--){
        while ((clients_money[i]>0)&&((temp_price-nominals[i])>=0)){
            temp_price -= nominals[i];
            clients_money[i] -= 1;
            paid_money[i] +=1;
            clients_money[COUNTER-1] -= nominals[i];
            paid_money[COUNTER-1] += nominals[i];
        }
    }

    if (temp_price == 0) return;

    int min_idx = COUNTER-2;

    for (int i =COUNTER-2; i>=0; i--) {
        int tt_price = temp_price;
        int temp_nominal_amount = clients_money[i];
        while ((temp_nominal_amount>0)&&(tt_price>=0)){
            tt_price -= nominals[i];
            end_picks[i] += 1;
            temp_nominal_amount -= 1;
        }
        distances[i] = tt_price;
    }

    for (int i =COUNTER-2; i>=0; i--){
        if (distances[i] < 0 && distances[min_idx] < distances[i]) min_idx = i;
        else if ((distances[i] < 0) && (distances[min_idx] > 0))  min_idx =i;
    }

    clients_money[min_idx] -= end_picks[min_idx];
    clients_money[COUNTER-1] -= (nominals[min_idx]* end_picks[min_idx]);

    // now we know how much and what should we choose to be the closest to the price

    paid_money[min_idx] += end_picks[min_idx];

    paid_money[COUNTER-1] += (nominals[min_idx]* end_picks[min_idx]);

}

void put_in_counter(volatile int * counter, const int money_for_service[COUNTER], int counter_sem_id, int waiting_barbers_mutex, int volatile* waiting_barbers_num, int waiting_barbers_queue_id){
    reduce(counter_sem_id, 0);
    for (int i = 0; i<COUNTER; i++) counter[i] += money_for_service[i];
    increase(counter_sem_id, 0);

    // check if any barbers is waiting for new money in counter
    reduce(waiting_barbers_mutex, 0);
    if (*waiting_barbers_num > 0){
        increase(waiting_barbers_queue_id, 0);
    }
    increase(waiting_barbers_mutex, 0);
}

void print_action(char* text, int barber_id){
    printf("Barber %d: %s\n", barber_id, text);
}


int determine_price(const int* clients_money, const volatile int* counter, volatile int * debts, int clients_id, int barber_id){
    int clients_amount = clients_money[COUNTER-1];
    int price =  ((rand()%clients_amount)+1);
    if (price>clients_amount) price-=1;
    // barbershop's debt to a client
    int client_debt = debts[clients_id-1];
    if (client_debt>0){
        print_action("Reducing debt", barber_id);
        if(price>client_debt){
            price -= client_debt;
            debts[clients_id-1] = 0;
        }else{
            debts[clients_id-1] -= price;
            price = 0;
        }
    }
    return price;
}

void cut_hair(int price){
    int haircut_t = (int) (((float)price)*0.3);
    usleep(haircut_t*10);

}

bool can_change(int change_banknotes[COUNTER], int counter_sem_id, volatile int* counter, int *change){

    reduce(counter_sem_id, 0);

    printf("Counter %d %d %d %d\n", counter[0],counter[1],counter[2],counter[3]);
    for (int i = COUNTER-2; i>=0; i--){
        while((counter[i] > 0) && ((*change - nominals[i]) >= 0)){
            *change -= nominals[i];
            counter[i] -= 1;
            change_banknotes[i] +=1;
            counter[COUNTER-1] -= nominals[i];
        }
    }

    // return money to the counter if the barber couldn't return full change

    if (*change != 0){
        for (int i = 0; i < (COUNTER-1); i++){
            counter[i] += change_banknotes[i];
            counter[COUNTER-1] += change_banknotes[i] * nominals[i];
            change_banknotes[i] = 0;
        }
        change_banknotes[COUNTER-1] = 0;
        increase(counter_sem_id, 0);
        return false;
    }
    increase(counter_sem_id, 0);

    return true;
}



void count_change(int change,  int clients_money[COUNTER],
                  volatile int* counter,
                  int counter_sem_id,
                  int barber_id, int waiting_barbers_mutex,
                  volatile int* waiting_barbers_num, int waiting_barbers_queue_id, const int BARBERS
                  , int clients_id, volatile int* debts, const int CLIENTS){

    int change_banknotes[COUNTER];
    for (int i = 0; i < COUNTER; i++) change_banknotes[i]=0;

    int temp_change = change;

    if (counter[COUNTER-1] < change
        || (can_change(change_banknotes, counter_sem_id, counter, &temp_change)==false)){

        // check if all barbers are waiting
        reduce(waiting_barbers_mutex, 0);
        *waiting_barbers_num+=1;
        if (BARBERS>CLIENTS){
            if (*waiting_barbers_num == CLIENTS){
                print_action("All barbers are waiting, make debt", barber_id);
                debts[clients_id-1] = change;
                *waiting_barbers_num-=1;
                increase(waiting_barbers_mutex, 0);
                return;
            }
        }else {
            if (*waiting_barbers_num == BARBERS) {
                print_action("All barbers are waiting, make debt", barber_id);
                debts[clients_id - 1] = change;
                *waiting_barbers_num -= 1;
                increase(waiting_barbers_mutex, 0);
                return;
            }
        }
        increase(waiting_barbers_mutex, 0);

        temp_change = change;
        while (counter[COUNTER-1] < change ||
               (can_change(change_banknotes, counter_sem_id, counter, &temp_change) ==  false)) {

            print_action("Wait for another barber to tell about new money", barber_id);
            //wait for new money in counter
            reduce(waiting_barbers_queue_id,0);



            temp_change = change;
            printf("Change amount %d\n", temp_change);
            printf("Counter %d %d %d %d\n", counter[0],counter[1],counter[2],counter[3]);
            printf("Clients money %d %d %d %d\n", clients_money[0],clients_money[1],clients_money[2],clients_money[3]);
            sleep(1);

            temp_change = change;

        }
        print_action("Got money that was waiting for", barber_id);
        reduce(waiting_barbers_mutex, 0);
        *waiting_barbers_num-=1;
        increase(waiting_barbers_mutex, 0);

    }

    // add change to clients money
    for (int i = 0; i < (COUNTER-1); i++){
        clients_money[i] += change_banknotes[i];
        clients_money[COUNTER-1] += change_banknotes[i] * nominals[i];
    }

}

void give_money_to_client(int clients_id,  const int clients_money[COUNTER], int msgq_id){

    struct client money_to_client;
    money_to_client.mtype = (long) clients_id;
    for (int i = 0; i < COUNTER; i++) money_to_client.clients_money[i] = clients_money[i];

    if (msgsnd(msgq_id, &money_to_client, (sizeof(struct client) - sizeof(long)), 0) == -1){
        perror("Give money to client error");
        exit(1);
    }

}