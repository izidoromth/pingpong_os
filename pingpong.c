#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>

#include "pingpong.h"
#include "queue.h"

//#define DEBUG

struct sigaction action ;
struct itimerval timer;

int current_id, aging;
unsigned int time_ms;
task_t main_tcb, t_dispatcher;
task_t *t_current, *task_ready_queue = NULL, *task_suspended_queue = NULL;

task_t* scheduler()
{
    task_t* next;

    next = task_ready_queue;

    return next;
}

task_t* d_prio_scheduler()
{
    task_t *tmp = task_ready_queue;
    task_t *next = tmp;
    int high_d_prio = next->d_prio;

    do
    {
        if(tmp->d_prio <= high_d_prio)
        {
            high_d_prio = tmp->d_prio;
            next = tmp;
        }

        tmp = tmp->next;

    }while(tmp != task_ready_queue);

    task_aging(next);

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

void dispatcher_body ()
{
    int userTasks;
    task_t* next;

    userTasks = queue_size((queue_t*) task_ready_queue);

    t_current = &t_dispatcher;

    while ( userTasks > 0 )
    {
        next = d_prio_scheduler();

        if (next)
        {
            next->quantum = 20;

            task_resume(next);

            task_switch (next) ;
        }

        userTasks = queue_size((queue_t*) task_ready_queue);
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

    if(task->state == suspended)
        queue_remove((queue_t**) &task_suspended_queue, (queue_t*) task);

    queue_append((queue_t**) &task_ready_queue, (queue_t*) task);

    #ifdef DEBUG
    printf ("task_resume: Adição à fila de prontas concluída\n") ;
    #endif
}

/*void task_suspend (task_t *task, task_t **queue)
{
    #ifdef DEBUG
    printf ("task_suspend: Adição à fila de suspensas iniciada\n") ;
    #endif

    if(!task)
    {
        #ifdef DEBUG
        printf ("task_suspend: Tarefa não pode ser nula\n") ;
        #endif

        return;
    }

    if(task->state == suspended)
    {
        #ifdef DEBUG
        printf ("task_suspend: Tarefa já pertence a fila de suspensas\n") ;
        #endif

        return;
    }

    queue_remove((queue_t **) &task_ready_queue, (queue_t*) task);

    task->state = suspended;

    queue_append((queue_t **) &task_suspended_queue, (queue_t*) task);

    #ifdef DEBUG
    printf ("task_resume: Adição à fila de suspensas concluída\n") ;
    #endif
}*/

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

    set_timer_interrupt();

    task_create(&main_tcb,NULL,"Main");

    task_create(&t_dispatcher,dispatcher_body,"Dispatcher");

    t_current = &main_tcb;

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
    task->time_process = 0;
    task->activations = 0;

    if(task != &main_tcb && task != &t_dispatcher)
        queue_append((queue_t**) &task_ready_queue, (queue_t*) task);

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

    printf("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n",t_current->id, systime() - t_current->time_init, t_current->time_process, t_current->activations);

    if(exit_code == 0)
    {
        queue_remove((queue_t**) &task_ready_queue, (queue_t*) t_current);
        task_switch(&t_dispatcher);
    }

    if(exit_code == 1) task_switch(&main_tcb);

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

    if(t_current != &main_tcb)
    {
        queue_remove((queue_t**) &task_ready_queue, (queue_t*) t_current);

        queue_append((queue_t**) &task_ready_queue, (queue_t*) t_current);
    }

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
