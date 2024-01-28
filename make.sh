#!/bin/bash

# Check if the number of arguments is less than 2
if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <CLIENTS> <BARBERS> <WAITING ROOM SPOTS>"
  exit 1
fi

export CLIENTS="$1"
export BARBERS="$2"
export SPOTS="$3"
# Calculate CLIENT_PRESENT as CLIENTS + 1 // one more so the clients has id from 1 to Clients
CLIENT_PRESENT=$((CLIENTS + 1))

export CLIENT_PRESENT

cc -Wall barber.c semaphore_ops.h  semaphore_ops.c -o barber
cc -Wall client.c semaphore_ops.h  semaphore_ops.c  -o client
cc -Wall main.c -o main

./main
