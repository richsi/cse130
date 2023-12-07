#include "queue.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
// pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

typedef struct node {
    void *buffer;
    struct node *next;
} Node;

typedef struct queue {
    int size;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    Node *head;
} queue_t;

queue_t *queue_new(int size) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    q->size = size;
    q->count = 0;
    q->head = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
    return q;
}

void queue_delete(queue_t **q) {
    if (q == NULL || *q == NULL)
        return;
    pthread_mutex_destroy(&(*q)->lock);
    pthread_cond_destroy(&(*q)->not_empty);
    pthread_cond_destroy(&(*q)->not_full);

    free(*q);
    *q = NULL;
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL || elem == NULL)
        return false;

    //printf("size: %i, counter: %i\n", q->size, q->count);

    pthread_mutex_lock(&q->lock);

    while (q->count == q->size) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    Node *new_node = (Node *) malloc(sizeof(Node *));
    Node *pointer = q->head;
    new_node->buffer = (void *) malloc(sizeof(void *));
    new_node->buffer = elem;

    if (q->head == NULL) { //if queue is empty
        q->head = new_node;
        pthread_cond_signal(&q->not_empty);
        pthread_mutex_unlock(&q->lock);

        return true;
    }

    while (pointer->next != NULL) {
        pointer = pointer->next;
    }

    pointer->next = new_node;
    q->count++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL || elem == NULL)
        return false;

    pthread_mutex_lock(&q->lock);

    while (q->count == q->size) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    Node *pointer = q->head;
    q->head = pointer->next;

    *elem = pointer->buffer;

    free(pointer);
    pointer = NULL;

    q->count--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return true;
}
