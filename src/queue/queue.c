#include "queue.h"


void queue_init(Queue* q)
{
    q->head = NULL;
}


void queue_push(Queue* q, void* value, size_t value_size)
{
    QElement* element = (QElement *) malloc(sizeof(QElement));
    element->value = value;
    element->value_size = value_size;
    element->next = NULL;

    QElement* head = q->head;
    if(head == NULL) {q->head = element; return;}

    while(head->next != NULL) head = head->next;
    head->next = element;
}


bool queue_empty(Queue* q)
{
    return q->head == NULL;
}


void queue_pop(Queue* q, void** value, size_t* value_size)
{
    void* res_value = NULL;
    size_t res_size = -1;

    if(q->head != NULL)
    {
        QElement* head = q->head;
        q->head = q->head->next;
        res_value = head->value;
        res_size = head->value_size;
        free(head);
    }
    
    if(value != NULL) *value = res_value;
    if(value_size != NULL) *value_size = res_size;
}


void queue_delete(Queue* q)
{
    while(!queue_empty(q)) queue_pop(q, NULL, NULL);
}
