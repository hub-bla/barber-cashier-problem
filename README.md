# Sleeping Barber-Cashiers
The project that was made for the System and Concurrent Programming class at Poznan University of Technology.

## Solution Overview


This solution addresses the Sleeping Barber-Cashiers problem using Inter-Process Communication (IPC) objects. The key IPC objects used are:

1. **Counter (Cash Register) - Shared Memory:**
   - A 4-element list representing the number of banknotes for different denominations and the total cash in the register.
   - One semaphore is employed for reading/writing to the cash register.

2. **Waiting Room (WR) - Message Queue:**
   - Clients use the message queue to send their money and identifiers.
   - The barber utilizes it to notify the client about the completion of the service and return the remaining money.

3. **Number of spots in waiting room (spots_in_wr) - Shared Memory:**
   - Tracks the number of people in the waiting room.
   - Synchronized access using a binary semaphore (`spots_in_wr_mutex`).

4. **Chairs (ch) - Semaphore:**
   - Manages the number of available chairs in the waiting room.
   - The barber waits if no chair is available.

5. **Waiting Barbers (waiting_barbers_num) - Shared Memory:**
   - Helps avoid deadlocks by signaling if all working barbers are waiting for change.
   - If all barbers are waiting then the last barber coming to waiting queue makes a debt that the salon is obligated to reduce every time the client that could get change come.
     Thus, the barber that would normally wait for new money in the counter can work again. The debt that the salon has for every client is track with shared memery array where every index
     corresponds to the client's ids. This means it doesn't require any locks beacuse every barber can handle one client at the time so there are no data races to the same place in the array.
   - Access protected by a binary semaphore (`waiting_barber_mutex`).

6. **Waiting Barbers Queue (waiting_barbers_queue_id) - Semaphore:**
   - Barbers waiting for change use this semaphore.
   - A barber not waiting for change notifies one waiting barber about the new cash value.

## Usage
 - Run the following command `./make.sh <NUMBER OF CLIENTS> <NUMBER OF BARBERS> <WAITING ROOM SPOTS>`

