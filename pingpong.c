#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <math.h>

#include "pingpong.h"
#include "queue.h"

//#define DEBUG

int current_id, aging;
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

void dispatcher_body () // dispatcher é uma tarefa
{
    int userTasks;
    task_t* next;

    userTasks = queue_size((queue_t*) task_ready_queue);

    t_current = &t_dispatcher;

    while ( userTasks > 0 )
    {
        next = d_prio_scheduler(); // scheduler é uma função

        if (next)
        {
            task_switch (next) ; // transfere controle para a tarefa "next"
        }

        userTasks = queue_size((queue_t*) task_ready_queue);
    }

    task_exit(1) ; // encerra a tarefa dispatcher
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

void pingpong_init ()
{
    current_id = 0;

    aging = -1;

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
    task_aux = t_current;

    t_current = task;

    swapcontext(&task_aux->context, &task->context);

    return 1;
}

void task_exit (int exit_code)
{
    #ifdef DEBUG
    printf ("task_exit: Iniciada\n") ;
    #endif

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
