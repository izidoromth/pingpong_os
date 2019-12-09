#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "pingpong.h"
#include "queue.h"

//#define DEBUG

struct sigaction action ;
struct itimerval timer;

int current_id, aging, sys_quantum;
unsigned int time_ms;
task_t main_tcb, t_dispatcher;
task_t *t_current, *task_ready_queue = NULL, *task_asleep_queue = NULL;

task_t* scheduler()
{
    task_t* next;

    next = task_ready_queue;

    return next;
}

void task_aging(task_t* next)
{
    task_t *tmp = task_ready_queue;

    do
    {
        if(tmp != next)
        {
            tmp->d_prio += aging;
        }
        else
        {
            tmp->d_prio = tmp->s_prio;
        }

        tmp = tmp->next;

    }while(tmp != task_ready_queue);
}

task_t* d_prio_scheduler()
{
    task_t *tmp = task_ready_queue;
    task_t *next = tmp;
    int high_d_prio;

    if(!next)
        return NULL;

    high_d_prio = next->d_prio;

    do
    {
        if(tmp->d_prio < high_d_prio)
        {
            high_d_prio = tmp->d_prio;
            next = tmp;
        }

        tmp = tmp->next;

    }while(tmp != task_ready_queue);

    task_aging(next);

    return next;
}

void wakeup_sleeping()
{
    task_t* slp_iterator, *slp_aux = NULL;
    int current_time;

    slp_iterator = task_asleep_queue;

    while(slp_iterator)
    {
        slp_aux = slp_iterator->next;

        current_time = systime();

        if(current_time >= slp_iterator->time_wakeup)
        {
            task_resume(slp_iterator);
        }

        if(slp_aux == task_asleep_queue || !task_asleep_queue)
            slp_iterator = NULL;
        else
            slp_iterator = slp_aux;
    }
}

void dispatcher_body ()
{
    int userTasks;
    task_t* next;

    userTasks = queue_size((queue_t*) task_ready_queue);

    while ( userTasks > 0 )
    {
        wakeup_sleeping();

        next = d_prio_scheduler();

        if (next)
        {
            next->quantum = sys_quantum;

            task_switch (next) ;
        }

        userTasks = queue_size((queue_t*) task_ready_queue) + queue_size((queue_t*) task_asleep_queue);
    }

    task_exit(1) ;
}

void task_resume (task_t *task)
{
    #ifdef DEBUG
    printf ("task_resume: Adição à fila de prontas iniciada\n") ;
    #endif

    if(!task)
    {
        #ifdef DEBUG
        printf ("task_resume: Tarefa não pode ser nula\n") ;
        #endif

        return;
    }

    if(task->state == ready)
        queue_remove((queue_t**) &task_ready_queue, (queue_t*) task);

    else if(task->state == suspended)
    {
        queue_remove((queue_t**) &t_current->suspended_tasks, (queue_t*) task);
        queue_remove((queue_t**) &task_asleep_queue, (queue_t*) task);
    }

    task->state = ready;

    queue_append((queue_t**) &task_ready_queue, (queue_t*) task);

    #ifdef DEBUG
    printf ("task_resume: Adição à fila de prontas concluída\n") ;
    #endif
}

void task_suspend (task_t *task, task_t **queue)
{
    #ifdef DEBUG
    printf ("task_suspend: Adição à fila de suspensas iniciada\n") ;
    #endif

    task_t* t_suspend = task;

    if(!t_suspend)
    {
        t_suspend = t_current;
    }

    if(t_suspend->state == ready)
        queue_remove((queue_t **) &task_ready_queue, (queue_t*) t_suspend);

    else if(t_suspend->state == suspended)
        queue_remove((queue_t **) &task_asleep_queue, (queue_t*) t_suspend);

    t_suspend->state = suspended;

    queue_append((queue_t **) queue, (queue_t*) t_suspend);

    #ifdef DEBUG
    printf ("task_resume: Adição à fila de suspensas concluída\n") ;
    #endif
}

unsigned int systime()
{
    return time_ms;
}

void tick_handler (int signum)
{
    time_ms++;

    if(t_current == NULL)
    {
        #ifdef DEBUG
        printf("tick_handler: Tarefa corrente nula!\n")
        #endif
        return;
    }

    t_current->time_process++;

    if(t_current->owner == t_system)
    {
        #ifdef DEBUG
        printf("tick_handler: Tarefa corrente é de sistema!\n")
        #endif
        return;
    }

    if(t_current->quantum == 1)
    {
        #ifdef DEBUG
        printf("tick_handler: Quantum zerado. Retornando ao dispatcher...\n")
        #endif

        task_resume(t_current);

        task_yield();
    }

    else
    {
        #ifdef DEBUG
        printf("tick_handler: Tick...\n")
        #endif
        t_current->quantum -= 1;
    }

    return;
}

void set_timer_interrupt()
{
  action.sa_handler = tick_handler ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }

  timer.it_value.tv_usec = 1 ;
  timer.it_value.tv_sec  = 0 ;
  timer.it_interval.tv_usec = 1000 ;
  timer.it_interval.tv_sec  = 0 ;

  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }
}

void pingpong_init ()
{
    current_id = 0;

    aging = -1;

    time_ms = -1;

    sys_quantum = 20;

    set_timer_interrupt();

    task_create(&main_tcb,NULL,"Main");

    task_create(&t_dispatcher,dispatcher_body,"Dispatcher");

    t_current = &main_tcb;

    task_yield();

    setvbuf (stdout, 0, _IONBF, 0) ;
}

int task_create (task_t *task, void (*start_routine)(void *), void *arg)
{
    char *stack ;

    #ifdef DEBUG
    printf ("task_create: Criação da tarefa %s iniciada\n",(char*) arg) ;
    #endif

    if(!task)
    {
        #ifdef DEBUG
        printf ("task_create: Task não pode ser nula\n") ;
        #endif

        return -1;
    }

    getcontext (&task->context);

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
       task->context.uc_stack.ss_sp = stack ;
       task->context.uc_stack.ss_size = STACKSIZE;
       task->context.uc_stack.ss_flags = 0;
       task->context.uc_link = 0;
    }
    else
    {
       perror ("task_create: Erro na criação de pilha da tarefa: ");
       return -1;
    }

    task->id = current_id; current_id++;
    task->name = arg;
    task->next = NULL;
    task->prev = NULL;
    task->d_prio = 0;
    task->owner = task->id == 1 ? t_system : t_user;
    task->time_init = systime();
    task->quantum = sys_quantum;
    task->time_process = 0;
    task->activations = 0;
    task->state = ready;
    task->suspended_tasks = NULL;

    if(task != &t_dispatcher)
        task_resume(task);

    makecontext (&task->context, (void*)(*start_routine), 1, arg);

    #ifdef DEBUG
    printf ("task_create: Tarefa criada com sucesso!\n") ;
    #endif

    return task->id;
}

int task_switch (task_t *task)
{
    task_t* task_aux;

    #ifdef DEBUG
    printf ("task_switch: Troca de contexto de %s -> %s iniciada\n",t_current->name,task->name) ;
    #endif

    if(!task)
    {
        #ifdef DEBUG
        printf ("task_switch: Task nula não pode trocar de contexto\n") ;
        #endif

        return -1;
    }

    #ifdef DEBUG
    printf ("task_switch: Troca de contexto concluída\n") ;
    #endif

    if(t_current->activations == 0) t_current->time_init = systime();

    task_aux = t_current;

    t_current = task;

    t_current->activations++;

    swapcontext(&task_aux->context, &task->context);

    return 1;
}

void task_exit (int exit_code)
{
    #ifdef DEBUG
    printf ("task_exit: Iniciada\n") ;
    #endif

    #ifdef PROCTIMEDEBUG
    printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",t_current->id, systime() - t_current->time_init, t_current->time_process, t_current->activations);
    #endif

    t_current->exit_code = exit_code;

    t_current->state = finished;

    queue_remove((queue_t**) &task_ready_queue, (queue_t*) t_current);

    task_t *t_resume = t_current->suspended_tasks;

    while(t_resume)
    {
        task_resume(t_resume);
        t_resume = t_current->suspended_tasks;
    }

    task_switch(&t_dispatcher);

    #ifdef DEBUG
    printf ("task_yield: Concluída\n") ;
    #endif
}

int task_id ()
{
    return t_current->id;
}

void task_yield ()
{
    #ifdef DEBUG
    printf ("task_yield: Iniciada\n") ;
    #endif

    task_switch(&t_dispatcher);

    #ifdef DEBUG
    printf ("task_yield: Concluída\n") ;
    #endif
}

int task_getprio (task_t *task)
{
    #ifdef DEBUG
    printf ("task_getprio: Realizada\n") ;
    #endif

    if(!task)
        return t_current->s_prio;

    return task->s_prio;
}

void task_setprio (task_t *task, int prio)
{
    #ifdef DEBUG
    printf ("task_setprio: Realizada\n") ;
    #endif

    if(!task)
    {
        t_current->s_prio = prio;
        t_current->d_prio = prio;
    }
    else if (prio > 20)
    {
        task->s_prio = 20;
        task->d_prio = 20;
    }
    else if (prio < -20)
    {
        task->s_prio = -20;
        task->d_prio = -20;
    }
    else
    {
        task->s_prio = prio;
        task->d_prio = prio;
    }
}

int task_join (task_t *task)
{
    #ifdef DEBUG
    printf ("task_join: Iniciada\n") ;
    #endif

    if(!task || task->state == finished)
    {
        #ifdef DEBUG
        printf ("task_join: Tarefa não existe ou finalizada\n") ;
        #endif

        return -1;
    }

    task_suspend(t_current,&task->suspended_tasks);

    task_yield();

    #ifdef DEBUG
    printf ("task_join: Concluída\n") ;
    #endif

    return task->exit_code;
}

void task_sleep (int t)
{
    #ifdef DEBUG
    printf ("task_sleep: Iniciada\n") ;
    #endif

    t_current->time_wakeup = systime() + t*1000;

    task_suspend(t_current, &task_asleep_queue);

    task_yield();

    #ifdef DEBUG
    printf ("task_sleep: Concluída\n") ;
    #endif
}


//---------------------------------------------------------------------------
//-------------------IMPLEMENTAÇÃO FUNÇÕES DE SEMÁFORO-----------------------

int sem_create (semaphore_t *s, int value)
{
    if(!s)
    {
        #ifdef DEBUG
        printf ("sem_create: Ponteiro para semáforo nulo\n") ;
        #endif

        return -1;
    }

    s->counter = value;
    s->task_queue = NULL;

    #ifdef DEBUG
    printf ("sem_create: Semáforo criado com sucesso!\n") ;
    #endif

    return 0;
}

int sem_down (semaphore_t *s)
{
    if(!s)
    {
        #ifdef DEBUG
        printf ("sem_down: Ponteiro para semáforo nulo\n") ;
        #endif

        return -1;
    }

    s->counter--;

    if(s->counter < 0)
    {
        task_suspend(t_current, &s->task_queue);

        task_yield();

        if(!s->task_queue)
        {
            #ifdef DEBUG
            printf ("sem_down:O semafóro foi destruído!\n") ;
            #endif

            return -1;
        }
    }

    #ifdef DEBUG
    printf ("sem_down: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

int sem_up(semaphore_t* s)
{
    task_t* task_wake;

    if(!s)
    {
        #ifdef DEBUG
        printf ("sem_up: Ponteiro para semáforo nulo\n") ;
        #endif

        return -1;
    }

    s->counter++;

    if(s->counter <= 0)
    {
        task_wake = s->task_queue;

        queue_remove((queue_t**) &s->task_queue, (queue_t*) task_wake);

        if(!task_wake)
        {
            #ifdef DEBUG
            printf ("sem_up: Tarefa para acordar nula\n") ;
            #endif

            return -1;
        }

        task_resume(task_wake);
    }

    #ifdef DEBUG
    printf ("sem_up: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

int sem_destroy (semaphore_t *s)
{
    task_t* wake_itr;

    if(!s)
    {
        #ifdef DEBUG
        printf ("sem_detroy: Ponteiro para semáforo nulo\n") ;
        #endif

        return -1;
    }

    if(!s->task_queue)
    {
        #ifdef DEBUG
        printf ("sem_detroy: Nenhuma tarefa para acordar. Operação finalizada!\n") ;
        #endif

        s = NULL;

        return 0;
    }

    wake_itr = s->task_queue;

    while(wake_itr)
    {
        queue_remove((queue_t**) &s->task_queue, (queue_t*) wake_itr);

        task_resume(wake_itr);

        wake_itr = s->task_queue;
    }

    s = NULL;

    #ifdef DEBUG
    printf ("sem_destroy: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

//---------------------------------------------------------------------------
//-------------------IMPLEMENTAÇÃO FUNÇÕES DE BARREIRA-----------------------

int barrier_create (barrier_t *b, int N)
{
    if(!b)
    {
        #ifdef DEBUG
        printf ("barrier_create: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    if(N <= 0)
    {
        #ifdef DEBUG
        printf ("barrier_join: Tamanho da barreira deve ser maior que 0\n") ;
        #endif

        return -1;
    }

    b->counter = N;
    b->task_queue = NULL;

    #ifdef DEBUG
    printf ("barrier_create: Barreira criada com sucesso!\n") ;
    #endif

    return 0;
}

int barrier_join (barrier_t *b)
{
    task_t* wake_itr;

    if(!b)
    {
        #ifdef DEBUG
        printf ("barrier_join: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    if(queue_size((queue_t*) b->task_queue) < b->counter - 1)
    {
        task_suspend(t_current, &b->task_queue);

        task_yield();

        if(!b->task_queue)
        {
            #ifdef DEBUG
            printf ("barrier_join: A barreira foi destruída!\n") ;
            #endif

            return -1;
        }
    }
    else
    {
        if(!b->task_queue)
        {
            #ifdef DEBUG
            printf ("barrier_join: Barreira inexistente!\n") ;
            #endif

            return -1;
        }

        wake_itr = b->task_queue;

        while(wake_itr)
        {
            queue_remove((queue_t**) &b->task_queue, (queue_t*) wake_itr);

            task_resume(wake_itr);

            wake_itr = b->task_queue;
        }
    }

    #ifdef DEBUG
    printf ("barrier_join: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

int barrier_destroy (barrier_t *b)
{
    task_t* wake_itr;

    if(!b)
    {
        #ifdef DEBUG
        printf ("barrier_destroy: Ponteiro para barreira nulo\n") ;
        #endif

        return -1;
    }

    if(!b->task_queue)
    {
        #ifdef DEBUG
        printf ("barrier_destroy: Nenhuma tarefa para acordar. Operação finalizada!\n") ;
        #endif

        return 0;
    }

    wake_itr = b->task_queue;

    while(wake_itr)
    {
        queue_remove((queue_t**) &b->task_queue, (queue_t*) wake_itr);

        task_resume(wake_itr);

        wake_itr = b->task_queue;
    }

    free(b);

    #ifdef DEBUG
    printf ("barrier_destroy: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

int mqueue_create (mqueue_t *queue, int max, int size)
{
    if(!queue)
    {
        #ifdef DEBUG
        printf ("mqueue_create: Ponteiro para fila de mensagens não pode nulo\n") ;
        #endif

        return -1;
    }

    if(max <= 0)
    {
        #ifdef DEBUG
        printf ("mqueue_create: Número de mensagens deve ser maior que 0\n") ;
        #endif

        return -1;
    }

    if(size <= 0)
    {
        #ifdef DEBUG
        printf ("mqueue_create: Tamanho das mensagens deve ser maior que 0\n") ;
        #endif

        return -1;
    }

    queue->max_msg = max;
    queue->msg_size = size;
    queue->buffer = malloc(queue->max_msg*queue->msg_size);
    queue->num_msgs = 0;

    sem_create(&queue->buffer_sem, 1);
    sem_create(&queue->available_sem, queue->max_msg);
    sem_create(&queue->items_sem, 0);

    #ifdef DEBUG
    printf ("mqueue_create: Operação realizada com sucesso!\n") ;
    #endif

    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg)
{
    if(!queue)
    {
        #ifdef DEBUG
        printf ("mqueue_send: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    sem_down(&queue->available_sem);
    sem_down(&queue->buffer_sem);

    void* new_msg = queue->buffer + (queue->num_msgs*queue->msg_size);

    memcpy(new_msg, msg, queue->msg_size);
    queue->num_msgs++;

    sem_up(&queue->buffer_sem);
    sem_up(&queue->items_sem);

    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg)
{
    if(!queue)
    {
        #ifdef DEBUG
        printf ("mqueue_recv: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    void *itr1, *itr2;

    sem_down(&queue->items_sem);
    sem_down(&queue->buffer_sem);

    memcpy(msg, queue->buffer, queue->msg_size);

    itr1 = queue->buffer;
    itr2 = queue->buffer + queue->msg_size;

    while(itr2 < (queue->buffer + queue->num_msgs*queue->msg_size))
    {
        memcpy(itr1, itr2, queue->msg_size);
        itr1 += queue->msg_size;
        itr2 += queue->msg_size;
    }

    queue->num_msgs--;

    sem_up(&queue->buffer_sem);
    sem_up(&queue->available_sem);

    return 0;
}

int mqueue_destroy (mqueue_t *queue)
{
    if(!queue)
    {
        #ifdef DEBUG
        printf ("mqueue_destroy: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    queue->buffer = NULL;
    sem_destroy(&queue->available_sem);
    sem_destroy(&queue->buffer_sem);
    sem_destroy(&queue->items_sem);

    return 0;
}

int mqueue_msgs (mqueue_t *queue)
{
    if(!queue)
    {
        #ifdef DEBUG
        printf ("mqueue_msgs: Ponteiro para barreira não pode nulo\n") ;
        #endif

        return -1;
    }

    return queue->num_msgs;
}
