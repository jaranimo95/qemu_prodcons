
#ifndef PRODCONS_H_INCLUDED
#define PRODCONS_H_INCLUDED

#define MAP_SIZE 0x0000FFFF

#include <asm/mman.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <linux/kernel.h>

struct cs1550_sem_node
{
  struct task_struct* customer;
  struct cs1550_sem_node* next;
};

struct cs1550_sem
{
   int want_pancake;              // # of locks available (+ values represent open spots, - values represent request queue)
   struct cs1550_sem_node* head;  // Pointer to head of our pancake queue
};

struct pancake_queue
{
    int* buf;           // Buffer to hold our pancakes in
    int  cap;           // MAX pancakes we can produce
    int  num_pancakes;  // Current number of pancakes in queue
    int  head;          // Head of queue
    int  tail;          // Tail of queue
    int  most_recent;   // Last pancake added to queue
};

void cs1550_down(struct cs1550_sem *sem);  // Down wrapper function (implementation in sys.c)
void cs1550_up(struct cs1550_sem *sem);  // Up wrapper function   (implementation in sys.c)

#endif