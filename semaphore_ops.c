#include "semaphore_ops.h"

static struct sembuf buf;

void increase(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Increasing semaphore error");
        exit(1);
    }
}



void reduce(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Reducing semaphore error");
        exit(1);
    }
}
