// Christian Jarani
// CS 1550
// Project 2: Syscalls

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <linux/prodcons.h>

//spin_lock(&sem_lock);
//spin_unlock(&sem_lock);

void cs1550_down(struct cs1550_sem *sem)    // Down wrapper function (implementation in sys.c)
{
    syscall(__NR_sys_cs1550_down, sem);
}
void cs1550_up(struct cs1550_sem *sem)      // Up wrapper function   (implementation in sys.c)
{
    syscall(__NR_sys_cs1550_up, sem);
}

int* init(int* base, int** current, int num_pancakes, int map)
{
    *current = *current + num_pancakes;             // Add defined number of pancakes to base pointer in mem
    if(*current > base + map)                         // If the number of pancakes we wish to hold is more than our buffer can handle
    {
        //perror(stderr, "Address out of range\n"); 
        exit(1);
    }
    return *current - num_pancakes;                 // Return base pointer
}

void consume(struct cs1550_sem* mutex, struct cs1550_sem* full, struct cs1550_sem* empty, struct pancake_queue* q, int index)
{
    while(1)
    {
        printf("\n");
        cs1550_down(full);              // Lock down full semaphore
        cs1550_down(mutex);             // Lock down mutex semaphore since we're entering a critical region
        sleep(1);                       // Take a quick nap
        if(q->num_pancakes > 0)         // If there are still pancakes in the queue
        {
            printf("Customer %d consumed Pancake%d", index, q->buf[q->head]);       // Will have to use write() instead of printf bc can't use stdlib
            q->head = (q->head + 1) % q->cap;                                       // Set new head of queue
            (q->num_pancakes)--;                                                    // Decrement number of pancakes of queue
        }
        cs1550_up(mutex);               // Release semaphore
        cs1550_up(empty);               // Release empty semaphore so we can update w/ new amount of pancakes (locked by producer)
    }
}

void produce(struct cs1550_sem* mutex, struct cs1550_sem* full, struct cs1550_sem* empty, struct pancake_queue* q, int index)
{
    while(1)
    {
        printf("\n");
        cs1550_down(empty);                 // Lock down empty semaphore
        cs1550_down(mutex);                 // Lock down mutex semaphore bc critical region yada, yada, yada
        sleep(1);                           // Take a quick nap
        if(q->num_pancakes < q->cap)        // If we can fit more pancakes into the queue
        {
            (q->most_recent)++;                                                 // Increment most_recent field to make room for another pancake
            q->buf[q->tail] = q->most_recent;                                   // Now add our pancake to the queue
            printf("Chef %d produced Pancake%d", index, q->most_recent);        // Will have to use write() instead of printf bc can't use stdlib
            q->tail = (q->tail + 1) % q->cap;                                   // Now we have to update our tail since we just added another pancake
            (q->num_pancakes)++;                                                // Increment number of pancakes in queue
        }
        cs1550_up(mutex);                   // Release semaphore
        cs1550_up(full);                    // Release full semaphore so we can update w/ new amount of pancakes (locked by consumer)
    }
}

void main(int argc, char *argv[])
{
    int consumers = atoi(argv[1]);
    int producers = atoi(argv[2]);
    int buf_len   = atoi(argv[3]);

    void* map = (void *) mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    if(map == (void *) -1) 
    {
        //perror(stderr, "!!! ERROR - MMAP FAILURE !!!\n");
        exit(1);
    }

    int *base = map;
    int *current = map;

    // Sets pointers to our shared data (so we can properly manage mutual exclusion)
    struct cs1550_sem* mutex = (struct cs1550_sem*)     init(base, &current, sizeof(struct cs1550_sem),    MAP_SIZE);  // Semaphore which manages access to down/up functions
    struct cs1550_sem* full  = (struct cs1550_sem*)     init(base, &current, sizeof(struct cs1550_sem),    MAP_SIZE);  // Semaphore which represents number of pancakes ACQUIRED
    struct cs1550_sem* empty = (struct cs1550_sem*)     init(base, &current, sizeof(struct cs1550_sem),    MAP_SIZE);  // Semaphore which represents number of pancakes AVAILABLE
    struct pancake_queue* q  = (struct pancake_queue*)  init(base, &current, sizeof(struct pancake_queue), MAP_SIZE);  // Queue  for our pancakes
    int*   buf               = (int*)                   init(base, &current, sizeof(int) * buf_len,        MAP_SIZE);  // Buffer for queue
    
    mutex->want_pancake =  1;
    full->want_pancake  =  0;
    empty->want_pancake =  buf_len;

    q->buf              =  buf;      // Set to starting memory location of our buffer
    q->cap              =  buf_len;  // Set to MAX pancakes our buffer can hold
    q->num_pancakes     =  0;        // Set to current number of pancakes in our queue (none)
    q->head             =  0;        // Initialize head
    q->tail             =  0;        // Initialize tail
    q->most_recent      = -1;        // Initialize most recently added task to queue

    int i;
    if(consumers > 0 && producers > 0) {        // If users wants to make producers & consumers
        if(fork() == 0) {                           // If you are the consumer (first fork makes original consumer)
            for(i = 0; i < consumers - 1; i++)          // Keep forking for desired # of consumers
                if(fork() == 0) break;                      // Break if you're the consumer that was just forked
            consume(mutex, full, empty, q, i);      // Consume now that you're alive
        }
        else {                                  // Else you are the producer
            for(i = 0; i < producers - 1; i++)      // Keep forking for desired # of producers
                if(fork() == 0) break;                  // Break if you're the producer that was just forked
            produce(mutex, full, empty, q, i);      // Produce now that you're alive
        }
    }
}
