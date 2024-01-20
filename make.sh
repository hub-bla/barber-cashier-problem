# Assuming CLIENTS is initially set to 2
export CLIENTS=5
export BARBERS=10

# Calculate CLIENT_PRESENT as CLIENTS + 1 // one more so the clients has id from 1 to Clients
CLIENT_PRESENT=$((CLIENTS + 1))

# Export CLIENT_PRESENT as an environment variable
export CLIENT_PRESENT


cc -Wall fryzjerzy_fryzjer.c fryzjerzy_semaphore_ops.h  fryzjerzy_semaphore_ops.c -o fryzjer
cc -Wall fryzjerzy_klient.c fryzjerzy_semaphore_ops.h  fryzjerzy_semaphore_ops.c  -o klient
cc -Wall fryzjerzy_main.c -o main


./main
