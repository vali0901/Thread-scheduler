#include "so_scheduler.h"
#include "data_structures.h"
#include <stdlib.h>
// ready state
static thread_priority_queue *thqueue;

// waiting state
static thread_wait_list *thlist;

// running state
static thread_node *thrunning;

// mutex pentru scheduler, pentru a semnala cand poate face end
static pthread_mutex_t sched_mutex; 

// vector in care se retin toate pthread_t care apar pe parcursul executiei
// pentru a li se da join in so_end
static pthread_t threads[1024];

// numarul de threaduri manipulate de acest scheduler
static int nr_threads;


// functie ce schimba threadul curent daca este cazul
// verifica daca nu exista un thread curent sau daca
// threadaul curent si a terminat cuanta, sau in coada
// asteapta un thread cu prioritate mai mare; in caz
// afirmativ, un thread este scos din coada, iar curentul
// thread este pus in coada (daca nu este null); daca noul
// thread exista (nu este null), mutexul lui este deblocat
// pentru a-si incepe executia;
// altfel, inseamna ca nu mai exista niciun thread in coada,
// deci se poate incheia schedulerul (se deblocheaza mutexul
// schedulerului pentru a se putea apela so_end)
void reschedule(void)
{
	if(!thrunning) {
		thrunning = qpop(thqueue);
		goto ret;
	}

	if(thrunning->priority < qseek_prio(thqueue) 
		|| thrunning->quantum == 0) {
			thrunning->quantum = thqueue->quantum;
			qpush(thqueue, thrunning);
			thrunning = qpop(thqueue);
	}

ret:
	if(!thrunning) {
		pthread_mutex_unlock(&sched_mutex);
		return;
	}

	pthread_mutex_unlock(&(thrunning->mutex));
}

// functia ce va fi facuta de fiecare thread, contine un
// mecanism ce ajuta la blocarea executiei handlerului pana
// acesta ajunge in starea Running
// dupa incheierea handlerului, stim ca threadul si a incheiat
// practic executia, deci putem sa ii dezalocam structura, fiind
// necesar sa aducem alt thread in starea Running
void *work(void *thnode)
{
	pthread_mutex_lock(&(((thread_node *)thnode)->mutex));
	pthread_mutex_unlock(&(((thread_node *)thnode)->mutex));

	((thread_node *)thnode)->func(((thread_node *)thnode)->priority);
	
	thrunning = destroy_thread_node(thnode);

	reschedule();
}

DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
	// cuanta invalida
	if(time_quantum <= 0)
		return -1;

	// io invalid
	if(io < 0 || io > SO_MAX_NUM_EVENTS)
		return -1;

	// scheduler deja initializat
	if(thqueue || thlist)
		return -1;

	// alocarea propriu zisa a structurilor de date
	thqueue = qinit(time_quantum);

	if(!thqueue)
		return -1;
	
	thlist = linit(io);

	if(!thlist)
	   return -1;

	thrunning = NULL;
	nr_threads = 0;

	// initializare mutex si blocarea acestuia (pentru ca lockul din so_end
	// sa astepte deblocarea)
	pthread_mutex_init(&sched_mutex, NULL);
	pthread_mutex_lock(&sched_mutex);

	return 0;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	// verificare daca callerul este un thread manageriat de acest scheduler
	// sau este callerul din teste (primul fork)
	if(thrunning)
		pthread_mutex_lock(&(thrunning->mutex));

	// invalid handler, invalid priority
	if(func == NULL || priority < 0 || priority > SO_MAX_PRIO)
		return INVALID_TID;

	// crearea thread_node
	thread_node *thnode = create_thread_node(func, priority, thqueue->quantum, -1);

	// ii blocam executia
	pthread_mutex_lock(&(thnode->mutex));

	// crearea propriu zisa a threadului care va intra in work si va astepta
	// deblocarea mutexului
	int ret = pthread_create(&(thnode->thread), NULL, work, thnode); 

	if(ret)
		return INVALID_TID;

	// retinerea noului pthred_t
	pthread_t thread = thnode->thread;
	threads[nr_threads++] = thread;
	
	// adaugarea noului thread in coada
	qpush(thqueue, thnode);

	// daca callerul acestei functii este manageriat de acest scheduler
	// i se scade cuanta
	if(thrunning) 
		thrunning->quantum--;

	// apelarea reschedule 
	thread_node *last_thrunning = thrunning;
	reschedule();
	
	// cu ajutorul lui last_running blocam iesirea threadului curent din functie
	// pana la deblocarea mutexului de catre scheduler
	if(last_thrunning) {
		pthread_mutex_lock(&(last_thrunning->mutex));
		pthread_mutex_unlock(&(last_thrunning->mutex));
	}

	return thread;
}

DECL_PREFIX int so_wait(unsigned int io)
{
	// invalid io
	if(io < 0 || io >= thlist->io)
		return -1;

	pthread_mutex_lock(&(thrunning->mutex));

	thrunning->event_id = io;
	thrunning->quantum = 0;

	// adaugarea threadului in lista de threaduri in Waiting
	ladd(thlist, thrunning);

	thread_node *last_thrunning = thrunning;

	thrunning = NULL;

	// pornirea altui thread;
	reschedule();

	pthread_mutex_lock(&(last_thrunning->mutex));
	pthread_mutex_unlock(&(last_thrunning->mutex));

	return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
	// invalid io
	if(io < 0 || io >= thlist->io)
		return -1;

	pthread_mutex_lock(&(thrunning->mutex));

	int wokenthreads = 0;

	// trezirea efectiva a threadurilor semnalate de acest io si adaugarea
	// lor in coada
	thread_node *thnode = lremove(thlist, io);

	while(thnode != NULL) {
		thnode->event_id = -1;
		thnode->quantum = thqueue->quantum;
		thnode->next = NULL;

		qpush(thqueue, thnode);
		
		wokenthreads++;

		thnode = lremove(thlist, io);
	}

	thrunning->quantum--;

	thread_node *last_thrunning = thrunning;
	
	reschedule();

	pthread_mutex_lock(&(last_thrunning->mutex));
	pthread_mutex_unlock(&(last_thrunning->mutex));

	return wokenthreads;
}

DECL_PREFIX void so_exec(void)
{
	pthread_mutex_lock(&(thrunning->mutex));

	// thread manageriat de acest scheduler => scadem cuanta
	if(thrunning) 
		thrunning->quantum--;
	
	thread_node *last_thrunning = thrunning;
	reschedule();

	// acelasi sistem de blocare al executiei ca la fiecare functie
	pthread_mutex_lock(&(last_thrunning->mutex));
	pthread_mutex_unlock(&(last_thrunning->mutex));
}

// eliberare resurse folosite de scheduler
DECL_PREFIX void so_end(void)
{
	// asteptam deblocarea mutexului pentru a putea executa intructiunile
	if(thrunning) 
		pthread_mutex_lock(&sched_mutex);
	
	if(thqueue)
		thqueue = qend(thqueue);
	
	if(thlist)
		thlist = lend(thlist);
	
	// asteptarea finalizarii tuturor threadurilor si eliberarea memoriei
	// folosite de acestea
	for(int i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);

	pthread_mutex_destroy(&sched_mutex);
}