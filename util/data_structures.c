#include <stdlib.h>
#include <stdio.h>
#include "data_structures.h"
#include "so_scheduler.h"

// creeaza un nou thread node cu handler, prioritate, cuanta si event_id specifice
// (event_id este -1 la creearea unui thread care ajunge in Ready)
thread_node *create_thread_node(so_handler *func, char priority, char quantum, int event_id)
{
    thread_node *thnode = (thread_node *)malloc(sizeof(thread_node));
    
    if(thnode == NULL)
        return NULL;

    if(pthread_mutex_init(&(thnode->mutex), NULL))
        return INVALID_TID;
    
    thnode->func = func;
    thnode->priority = priority;
    thnode->quantum = quantum;
    thnode->event_id = event_id;
    thnode->next = NULL;

    return thnode;
}

// elibereaza memoria si distruge mutexul folosite de thread node ul dat ca parametru
thread_node *destroy_thread_node(thread_node *thnode)
{
    if(thnode == NULL)
        return NULL;
    
    pthread_mutex_destroy(&(thnode->mutex));
    free(thnode);

    return NULL;
}

// initializeaza structura de date de tip coada de prioritati
thread_priority_queue *qinit(char quantum)
{
    thread_priority_queue *thqueue = (thread_priority_queue *)malloc(sizeof(thread_priority_queue));

    if(thqueue == NULL)
        return NULL;

    thqueue->head = NULL;
    thqueue->quantum = quantum;

    return thqueue;
}

// adaugarea unui element in coada de prioritati, in functie de prioritatea acestuia
void qpush(thread_priority_queue *thqueue, thread_node *thnode)
{
    if(thqueue->head == NULL) {
        thqueue->head = thnode;
        return;
    }

    if(thnode->priority > thqueue->head->priority) {
        thnode->next = thqueue->head;
        thqueue->head = thnode;
        return;
    }

    thread_node *helper = thqueue->head;

    while(helper->next != NULL) {
        if(thnode->priority > helper->next->priority)
            break;

        helper = helper->next;
    }

    thnode->next = helper->next;
    helper->next = thnode;
}

// returneaza elementul cu prioritatea cea mai mare din coada (adica head)
thread_node *qpop(thread_priority_queue *thqueue)
{
    if(thqueue->head == NULL)
        return NULL;

    thread_node *thnode = thqueue->head;
    thqueue->head = thqueue->head->next;

    return thnode;
}

// returneaza prioritatea primului thread care asteapta in coada 
int qseek_prio(thread_priority_queue *thqueue)
{
    if(thqueue->head == NULL)
        return -1;

    return thqueue->head->priority;
}

// elbiereaza resursele folosite de coada de prioritati si eventualele threaduri
// ramase in aceasta
thread_priority_queue *qend(thread_priority_queue *thqueue)
{
    if(!thqueue)
        return NULL;

    thread_node *thnode;
    thread_node *helper = thqueue->head;

    while(helper) {
        thnode = helper;
        helper = helper->next;
        thnode = destroy_thread_node(thnode);
    }

    free(thqueue);
    return NULL;
}

// initializeaza lista de threaduri
thread_wait_list *linit(unsigned int io)
{
    thread_wait_list *thlist= (thread_wait_list *)malloc(sizeof(thread_wait_list));
    
    if(!thlist)
        return NULL;

    thlist->head = NULL;
    thlist->io = io;

    return thlist;
}

// adaugarea unui nou element in lista, la inceputul acesteia
void ladd(thread_wait_list *thlist, thread_node *thnode)
{
    thnode->next = thlist->head;
    thlist->head = thnode;
}

// eliminarea unui nod din lista in functie de un event_id specificat
thread_node *lremove(thread_wait_list *thlist, int event_id)
{
    thread_node *helper = thlist->head;

    if(helper == NULL)
        return NULL;

    if(helper->event_id == event_id) {
        thlist->head = thlist->head->next;
        return helper;
    }

    while(helper->next != NULL) {
        if(helper->next->event_id == event_id) {
            thread_node *rm = helper->next;
            helper->next = helper->next->next;
            return rm;
        }

        helper = helper->next;
    }

    return NULL;
}

// elibereaza resurele folosite de lista si de eventualele threaduri
// ramase in aceasta
thread_wait_list *lend(thread_wait_list *thlist)
{
    if(!thlist)
        return NULL;

    thread_node *thnode;
    thread_node *helper = thlist->head;

    while(helper) {
        thnode = helper;
        helper = helper->next;
        thnode = destroy_thread_node(thnode);
    }

    free(thlist);

    return NULL;
}