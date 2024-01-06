ipcrm --all

cc -Wall fryzjerzy_fryzjer.c fryzjerzy_semaphore_ops.h  fryzjerzy_semaphore_ops.c -o fryzjer
cc -Wall fryzjerzy_klient.c fryzjerzy_semaphore_ops.h  fryzjerzy_semaphore_ops.c  -o klient
cc -Wall fryzjerzy_main.c -o main


./main
