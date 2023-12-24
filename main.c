#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#define BARBERS 3
#define CLIENTS 4
int main(int argc, char* argv[]) {

    for (int i = 10; i < (BARBERS+10); i++) {
        if (fork()==0){
            printf("Make barber\n");
            execlp("./fryzjer", "./fryzjer", "", NULL);
            printf("Make barber error\n");
        }
    }

    for (int i = 10; i < (CLIENTS+10); i++){
        if (fork()==0){
            printf("Make client\n");
            execlp("./klient", "./klient","",NULL);
            printf("Make client error\n");
        }
    }

    wait(NULL);
    return 0;
}
