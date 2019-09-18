#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#include "pingpong.h"

int current_id;
task_t main_tcb;
task_t *current_task;

void pingpong_init ()
{
    current_id = 0;

    task_create(&main_tcb,NULL,"Main");

    current_task = &main_tcb;

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

    getcontext (&task->tcontext);

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
       task->tcontext.uc_stack.ss_sp = stack ;
       task->tcontext.uc_stack.ss_size = STACKSIZE;
       task->tcontext.uc_stack.ss_flags = 0;
       task->tcontext.uc_link = 0;
    }
    else
    {
       perror ("task_create: Erro na criação de pilha da tarefa: ");
       return -1;
    }

    task->tid = current_id; current_id++;
    task->tname = arg;

    makecontext (&task->tcontext, (void*)(*start_routine), 1, arg);

    #ifdef DEBUG
    printf ("task_create: Tarefa criada com sucesso!\n") ;
    #endif

    return task->tid;
}

int task_switch (task_t *task)
{
    task_t* aux_task;
    #ifdef DEBUG
    printf ("task_switch: Troca de contexto %d -> %d iniciada\n",task_id(),task->tid) ;
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

    swapcontext(&aux_task->tcontext, &task->tcontext);

    return 1;
}

void task_exit (int exit_code)
{
    task_switch(&main_tcb);
}

int task_id ()
{
    return current_task->tid;
}
