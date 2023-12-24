cc -Wall fryzjer.c semaphore_ops.h  semaphore_ops.c -o fryzjer
cc -Wall klient.c semaphore_ops.h  semaphore_ops.c  -o klient
cc -Wall main.c -o main


./main
