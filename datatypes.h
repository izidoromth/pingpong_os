// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DAINF UTFPR
// Versão 1.0 -- Março de 2015
//
// Estruturas de dados internas do sistema operacional

#ifndef __DATATYPES__
#define __DATATYPES__

#include <ucontext.h>
#define STACKSIZE 32768

typedef enum task_states { ready, suspended, finished} task_state;
typedef enum owner_types { t_user, t_system} task_owner;

// Estrutura que define uma tarefa
typedef struct task_t
{
    struct task_t *prev, *next ;
    int id ;
    char* name;
    ucontext_t context;
    task_state state;
    int s_prio;
    int d_prio;
    task_owner owner;
    int quantum;
    unsigned int time_init;
    unsigned int time_process;
    unsigned int time_wakeup;
    int activations;
    int exit_code;
    struct task_t* suspended_tasks;
} task_t ;

// estrutura que define um semáforo
typedef struct semaphore_t
{
    int counter;
    struct task_t* task_queue;
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct barrier_t
{
    int counter;
    struct task_t* task_queue;
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif
