#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void queue_append (queue_t **queue, queue_t *elem)
{

    queue_t * aux_f, *aux_l;

    if(!queue)
    {
        printf("A fila não existe!\n");
        return;
    }

    if(!elem)
    {
        printf("O elemento é nulo!\n");
        return;
    }

    if((elem->next) || (elem->prev))
    {
        printf("O elemento já pertence a uma fila\n");
        return;
    }

    if(queue_size(*queue) == 0)
    {
        elem->next = elem;
        elem->prev = elem;
        *queue = elem;

        return;
    }

    aux_f = *queue;
    aux_l = *queue;

    do
    {
        aux_l = aux_l->next;
    }
    while(aux_l->next != aux_f);

    aux_l->next = elem;
    elem->prev = aux_l;
    elem->next = aux_f;
    aux_f->prev = elem;

    return;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
    queue_t* aux;

    if(!queue)
    {
        printf("A fila não existe!\n");
        return NULL;
    }

    if(!(*queue))
    {
        printf("A fila não deve estar vazia!\n");
        return NULL;
    }

    if(!elem)
    {
        printf("O elemento é nulo!\n");
        return NULL;
    }

    if(!((elem->next) && (elem->prev)))
    {
        printf("O elemento não pertence a uma fila\n");
        return NULL;
    }

    aux = *queue;

    if(*queue == elem)
    {
        *queue = (*queue)->next;

        if(queue_size(aux) == 1) *queue = NULL;
    }
    else
    {
        while(aux != elem && aux->next != *queue)
            aux = aux->next;
    }

    if(aux == elem)
    {
        aux->prev->next = aux->next;
        aux->next->prev = aux->prev;
    }
    else
    {
        printf("O elemento não pertence a esta fila\n");
        return NULL;
    }

    aux->next = NULL;
    aux->prev = NULL;

    return aux;
}

int queue_size (queue_t *queue)
{
    int count = 0;
    queue_t *aux;

    if(!queue)
        return 0;

    aux = queue;

    do
    {
        count++;
        aux = aux->next;
    }while(aux != queue);

    return count;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
    int i;

    printf("%s: [",name);
    queue_t* aux = queue;

    for(i=0;i<queue_size(queue);i++)
    {
      print_elem(aux);
      if (i != queue_size(queue) - 1)printf(" ");
      aux=aux->next;
    }

    printf("]\n");
}
