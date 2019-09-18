#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include "pingpong.h"
#include "queue.h"

//#define DEBUG

int current_id;
task_t main_tcb, t_dispatcher;
task_t *current_task, *task_ready_queue = NULL, *task_suspended_queue = NULL;

task_t* scheduler()
{
    task_t* next;

    next = queue_remove((queue_t**)&task_ready_queue,(queue_t*) task_ready_queue);

    return next;
}

dispatcher_body () // dispatcher é uma tarefa
{

    int userTasks;
    task_t* next;

    userTasks = queue_size((queue_t*) task_ready_queue);

    while ( userTasks > 0 )
    {
        next = scheduler(); // scheduler é uma função

        if (next)
        {
            task_switch (next) ; // transfere controle para a tarefa "next"
            // ações após retornar da tarefa "next", se houverem
        }
    }

    task_exit(0) ; // encerra a tarefa dispatcher
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

    queue_remove((queue_t **) &task_suspended_queue, (queue_t*) task);

    task->state = ready;

    queue_append((queue_t **) &task_ready_queue, (queue_t*) task);

    #ifdef DEBUG
    printf ("task_resume: Adição à fila de prontas concluída\n") ;
    #endif
}

void task_suspend (task_t *task, task_t **queue)
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
}

void pingpong_init ()
{
    current_id = 0;

    task_create(&main_tcb,NULL,"Main");

    current_task = &main_tcb;

    task_create(&t_dispatcher,dispatcher_body,"Dispatcher");

    task_switch(&t_dispatcher);

    setvbuf (stdout, 0, _IONBF, 0) ;
}

int task_create (task_t *task, void (*start_routine)(void *), void *arg)
{
    char *stack ;

    #ifdef DEBUG
    printf ("task_create: Criação de tarefa iniciada\n") ;
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
    task->state = ready;

    makecontext (&task->context, (void*)(*start_routine), 1, arg);

    task_resume(task);

    #ifdef DEBUG
    printf ("task_create: Tarefa criada com sucesso!\n") ;
    #endif

    return task->id;
}

int task_switch (task_t *task)
{
    task_t* aux_task;
    #ifdef DEBUG
    printf ("task_switch: Troca de contexto %d -> %d iniciada\n",task_id(),task->id) ;
    #endif

    if(!task)
    {
        #ifdef DEBUG
        printf ("task_switch: Task nula não pode trocar de contexto\n") ;
        #endif

        return -1;
    }

    aux_task = current_task;

    current_task = task;

    #ifdef DEBUG
    printf ("task_switch: Troca de contexto concluída\n") ;
    #endif

    swapcontext(&aux_task->context, &task->context);

    return 1;
}

void task_exit (int exit_code)
{
    task_switch(&main_tcb);
}

int task_id ()
{
    return current_task->id;
}

void task_yield ()
{
    queue_remove((queue_t**) task_ready_queue, (queue_t*) current_task);

    queue_append((queue_t**) task_ready_queue, (queue_t*) current_task);

    task_switch(&t_dispatcher);
}
