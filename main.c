#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stdlib.h>

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

#define BASE_ONES 0
#define BASE_TWOS 0
#define  BASE_FIVES 0



struct client {
    long mtype;
    int clients_money[4];
    int clients_id;
};

int main(int argc, char* argv[]) {
    char *clientsStr = getenv("CLIENTS");
    int CLIENTS = atoi(clientsStr);
    char *barbersStr = getenv("BARBERS");
    int BARBERS = atoi(barbersStr);

    int counter_id, wr_id, ch_id, counter_sems_id, num_clients_mutex_id, clients_id, barber_id, debts_id;

    int  waiting_barbers_id, waiting_barbers_mutex, waiting_barbers_queue_id;


    volatile int* counter;
    volatile int* debts;
    volatile int* waiting_barbers_num;
    volatile int* num_clients;

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

    waiting_barbers_id = shmget(NUM_WAITING_BARBERS_KEY, sizeof(int), IPC_CREAT|0600);
    if (waiting_barbers_id == -1) {
        perror("Create waiting barbers shared memory error");
        exit(1);
    }

    waiting_barbers_num = (int*)shmat(waiting_barbers_id, NULL, 0);
    if (waiting_barbers_num == NULL) {
        perror("Attach counter error");
        exit(1);
    }

    debts_id = shmget(KEY_DEBTS, (CLIENTS)*sizeof(int), IPC_CREAT|0600);
    if (debts_id == -1) {
        perror("Create debts shared memory error");
        exit(1);
    }

    debts = (int*)shmat(debts_id, NULL, 0);
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


    ch_id = semget(KEY_CHAIRS, 1, IPC_CREAT|0600);
    if (ch_id == -1) {
        perror("Create chairs shared memory error");
        exit(1);
    }

    if (semctl(ch_id, 0, SETVAL, (int)NUM_CHAIRS) == -1){
        perror("Set semaphore value to number of chairs error");
        exit(1);
    }


    waiting_barbers_mutex = semget(NUM_WAITING_BARBERS_KEY, 1, IPC_CREAT|0600);
    if (waiting_barbers_mutex == -1) {
        perror("Create mutex for number of reading number of waiting barbers error");
        exit(1);
    }

    if (semctl(waiting_barbers_mutex, 0, SETVAL, 1) == -1){
        perror("Set waiting_barbers_mutex mutex to 1 error");
        exit(1);
    }

    waiting_barbers_queue_id = semget(WAITING_BARBERS_QUEUE_KEY, 1, IPC_CREAT|0600);
    if (waiting_barbers_queue_id == -1) {
        perror("Create queue for waiting barbers");
        exit(1);
    }

    if (semctl(waiting_barbers_queue_id, 0, SETVAL, 0) == -1){
        perror("Set waiting_barbers_queue_id mutex to 1 error");
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

    *num_clients = 0;

    counter_sems_id = semget(KEY_COUNTER, 2, IPC_CREAT|0600);
    if (counter_sems_id == -1) {
        perror("Create counter semaphores error");
        exit(1);
    }

    if (semctl(counter_sems_id, 0, SETVAL, 1) == -1){
        perror("Set semaphore 1 for writing counter error");
        exit(1);
    }

    if (semctl(counter_sems_id, 1, SETVAL, 1) == -1){
        perror("Set semaphore 2 for reading counter error");
        exit(1);
    }

    // waiting room
    wr_id = msgget(KEY_WAITING_ROOM, IPC_CREAT|0600);
    if (wr_id == -1) {
        perror("Create waiting room error");
        exit(1);
    }



    struct client client_msg; // Create an instance of struct client

    while (msgrcv(wr_id, &client_msg, sizeof(client_msg), 0, IPC_NOWAIT) != -1) {
        // Message received and discarded
        printf("Received and discarded: %d\n", client_msg.clients_id);
    }

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
