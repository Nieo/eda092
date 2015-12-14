/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */

struct semaphore channel, mutex;
struct semaphore sender_normal;
//struct semaphore sender_prio;
struct semaphore receiver_normal;
//struct semaphore receiver_prio;

static int direction;
static int wait_for_channel;
static int in_channel;
static int wait_send;
static int wait_rec;

static unsigned int thread_nbr;


/* initializes semaphores */ 
void init_bus(void){ 
 
    random_init((unsigned int)123456789); 

    sema_init(&mutex, 1);
    sema_init(&channel, 3);
    sema_init(&sender_normal, 0);
    sema_init(&receiver_normal, 0);

    direction = -1;
    in_channel = 0;
    wait_for_channel = 0;
    wait_send = 0;
    wait_rec = 0;

    thread_nbr = 0;
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
    unsigned int i;

    for (i=0; i<num_priority_send; i++) {
        thread_create ("send_prio", PRI_MAX, senderPriorityTask, 0);
    }

    for (i=0; i<num_priority_receive; i++) {
        thread_create ("receive_prio", PRI_MAX, receiverPriorityTask, 0);
    }

    for (i=0; i<num_tasks_send; i++) {
        thread_create ("send_normal", PRI_DEFAULT, senderTask, 0);
    }

    for (i=0; i<num_task_receive; i++) {
        thread_create ("receive_normal", PRI_DEFAULT, receiverTask, 0);
    }
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        //msg("Create sender task");
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        //msg("Create receiver task");
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
    sema_down(&mutex);
    if (direction == -1 || in_channel == 0) {
        direction = task.direction;
    }

    sema_up(&mutex);

    if (direction == task.direction) {
        wait_for_channel++;
        sema_down(&channel);
        wait_for_channel--;
        in_channel++;
    } else {
        if (task.direction == SENDER) {
            wait_send++;
            sema_down(&sender_normal);
            wait_send--;
        } else {
            wait_rec++;
            sema_down(&receiver_normal);
            wait_rec--;
        }
        
        getSlot(task);
    }
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
    //msg("Started transfer");
    //Add some random sleep

    int64_t sleeptime = (int64_t)random_ulong();
    timer_sleep(sleeptime%100);
    //msg("Finished transfer");
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
    in_channel--;
    sema_up(&channel);

    //if (wait_for_channel == 0) {
    if ((direction == SENDER && wait_send > 0) || (direction == RECEIVER && wait_send == 0 && wait_for_channel == 0)) {
        direction = SENDER;
        sema_up(&sender_normal);
    } else if((direction == RECEIVER && wait_rec > 0) || (direction == SENDER && wait_rec == 0 && wait_for_channel == 0)){
        direction = RECEIVER;
        sema_up(&receiver_normal);
    }
    //}

    
}
