#include <pthread.h>
#include "so_scheduler.h"

#ifndef PRIORITY_QUEUE
#define PRIORITY_QUEUE


typedef struct thread_node {
    pthread_t thread;
    struct thread_node *next;
    pthread_mutex_t mutex;
    so_handler *func;
    char quantum;
    char priority;
    int event_id;
} thread_node;

typedef struct thread_priority_queue {
    thread_node *head;
    char quantum;
} thread_priority_queue;

typedef struct thread_wait_list {
    thread_node *head;
    unsigned int io;
} thread_wait_list;

thread_node *create_thread_node(so_handler *func, char priority, char quantum, int event_id);

thread_node *destroy_thread_node(thread_node *thnode);

thread_priority_queue *qinit(char quantum);

void qpush(thread_priority_queue *thqueue, thread_node *thnode);

thread_node *qpop(thread_priority_queue *thqueue);

int qseek_prio(thread_priority_queue *thqueue);

thread_priority_queue *qend(thread_priority_queue *thqueue);

thread_wait_list *linit(unsigned int io);

void ladd(thread_wait_list *thlist, thread_node *thnode);

thread_node *lremove(thread_wait_list *thlist, int event_id);

thread_wait_list *lend(thread_wait_list *thlist);

#endif
