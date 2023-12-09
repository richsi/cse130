#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <getopt.h>

typedef struct node {
    int ref_bit;
    struct node *next;
    char *element;
} Node;

typedef struct {
    Node *items; // head
    Node *unique_list;
    int size_u;
    int clock_ptr;
    int size;
    char *policy;
    int miss_sum;
} Cache;

Cache *cache_new(char *policy, int size) {
    Cache *cache = (Cache *) malloc(sizeof(Cache));
    cache->items = NULL;
    cache->unique_list = NULL;
    cache->policy = policy;
    cache->size = size;
    cache->miss_sum = 0;
    cache->size_u = 0;
    cache->clock_ptr = 0;
    return cache;
}

bool list_contains(Node *items, char *element) {
    if (items == NULL || element == NULL) {
        // printf("list_contains NULL\n");
        return false;
    }
    Node *temp = items;
    while (temp != NULL) {
        if (strcmp(temp->element, element) == 0) {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

bool list_is_full(Cache *cache) {
    int size = cache->size;
    if (size == 0)
        return false;

    Node *temp = cache->items;
    int count = 0;

    while (temp != NULL) {
        ++count;
        temp = temp->next;
    }

    // printf("count: %i\n", count);

    if (count == size) {
        return true;
    } else
        return false;
}

Node *new_node(char *element) {
    Node *new_node = (Node *) malloc(sizeof(Node));
    new_node->element = element;
    return new_node;
}

void list_remove_front(Cache *cache) {
    Node *temp = cache->items;
    cache->items = cache->items->next;

    // if (cache->evicted == NULL){
    //     cache->evicted = temp;
    // } else {
    //     Node* last_node = cache->evicted;
    //     while(last_node->next != NULL){
    //         last_node = last_node->next;
    //     }
    //     last_node->next = temp;
    // }

    temp->next = NULL;
    temp = NULL;
    free(temp);
}

void list_push_back(Cache *cache, Node *new) {
    if (cache->items == NULL) {
        cache->items = new;
        // printf("item->element: %s", cache->items->element);
    } else {
        Node *last_node = cache->items;
        while (last_node->next != NULL) {
            last_node = last_node->next;
        }
        last_node->next = new;
    }
}

void list_move_end(Cache *cache, char *element) {
    Node *prev = NULL;
    Node *curr = cache->items;

    while (curr != NULL && strcmp(curr->element, element) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL)
        return;
    if (prev != NULL) {
        prev->next = curr->next;
    } else {
        cache->items = curr->next;
    };

    Node *tail = cache->items;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = curr;
    curr->next = NULL;
}

void log_element(Cache *cache, char *element) {
    Node *new = new_node(element);

    if (cache->unique_list == NULL) {
        cache->unique_list = new;
        cache->size_u++;
        return;
    }

    Node *temp = cache->unique_list;
    while (temp != NULL) {
        if (strcmp(temp->element, element) == 0)
            return;
        temp = temp->next;
    }

    //element is unique
    new->next = cache->unique_list;
    cache->unique_list = new;
    cache->size_u++;
}

int insert_fifo(Cache *cache, char *element) {
    assert(cache != NULL);
    assert(element != NULL);

    log_element(cache, element);

    if (list_contains(cache->items, element)) {
        return 1;
    }

    if (list_is_full(cache)) {
        list_remove_front(cache);
    }

    Node *new = new_node(element);
    list_push_back(cache, new);
    cache->miss_sum++;
    return 0;
}

void cache_print(Cache *cache) {
    Node *temp = cache->items;
    if (temp == NULL) {
        printf("NULL\n");
    }

    while (temp != NULL) {
        printf("el: %s,%i\n", temp->element, temp->ref_bit);
        temp = temp->next;
    }
}

int insert_lru(Cache *cache, char *element) {
    assert(cache != NULL);
    assert(element != NULL);

    log_element(cache, element);

    if (list_contains(cache->items, element)) {
        list_move_end(cache, element);
        // cache_print(cache);
        return 1;
    }

    if (list_is_full(cache)) {
        list_remove_front(cache);
    }

    Node *new = new_node(element);
    list_push_back(cache, new);
    cache->miss_sum++;
    // cache_print(cache);
    return 0;
}

Node *list_get(Node *items, char *element) {
    Node *temp = items;
    while (temp != NULL) {
        if (strcmp(temp->element, element) == 0)
            break;
        temp = temp->next;
    }
    return temp;
}

Node *list_get_index(Node *item, int clock_ptr) {
    int count = clock_ptr;
    Node *temp = item;

    while (temp != NULL) {
        if (count == 0)
            break;
        temp = temp->next;
        count--;
    }
    return temp;
}

void list_overwrite_index(Node *item, int clock_ptr, Node *new) {
    Node *temp = item;
    Node *slow = NULL;
    int count = clock_ptr;

    if (clock_ptr == 0) {
        item->element = new->element;
        return;
    }

    while (temp != NULL && count != 0) {
        slow = temp;
        temp = temp->next;
        count--;
    }

    slow->next = new;
    new->next = temp->next;
    temp->next = NULL;
}

int insert_clock(Cache *cache, char *element) {
    assert(cache != NULL);
    assert(element != NULL);

    log_element(cache, element);

    if (list_contains(cache->items, element)) {
        Node *curr = list_get(cache->items, element);
        curr->ref_bit = 1;
        return 1;
    }

    if (list_is_full(cache)) {
        Node *curr = list_get_index(cache->items, cache->clock_ptr);

        while (curr->ref_bit == 1) {
            curr->ref_bit = 0;
            cache->clock_ptr = (cache->clock_ptr + 1) % cache->size;
            curr = list_get_index(cache->items, cache->clock_ptr);
        }

        Node *new = new_node(element);
        list_overwrite_index(cache->items, cache->clock_ptr, new);
        cache->clock_ptr = (cache->clock_ptr + 1) % cache->size;
    } else {
        Node *new = new_node(element);
        list_push_back(cache, new);
    }

    cache->miss_sum++;
    return 0;
}

void cache_insert(Cache *cache, char *item) {
    assert(cache != NULL);
    assert(item != NULL);

    if (strcmp(cache->policy, "F") == 0) {
        if (insert_fifo(cache, item)) {
            printf("HIT\n");
        } else
            printf("MISS\n");
    } else if (strcmp(cache->policy, "L") == 0) {
        if (insert_lru(cache, item)) {
            printf("HIT\n");
        } else
            printf("MISS\n");
    } else if (strcmp(cache->policy, "C") == 0) {
        if (insert_clock(cache, item)) {
            printf("HIT\n");
            // cache_print(cache);
        } else {
            printf("MISS\n");
            // cache_print(cache);
        }
    }
}

int main(int argc, char **argv) {
    //./cacher [-N size] <policy>
    int opt;
    int cache_size = -1;
    char *policy = "F";

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Invalid number of arguments\n");
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "N:FLC")) != -1) {
        switch (opt) {
        case 'N':
            cache_size = strtol(optarg, NULL, 10);
            if (cache_size <= 0) {
                fprintf(stderr, "bad cache size");
                exit(EXIT_FAILURE);
            }
            break;
        case 'F': policy = "F"; break;
        case 'L': policy = "L"; break;
        case 'C': policy = "C"; break;
        default: fprintf(stderr, "Invalid option\n"); exit(EXIT_FAILURE);
        }
    }

    if ((strcmp(policy, "F") != 0 && strcmp(policy, "L") != 0 && strcmp(policy, "C") != 0)
        || cache_size == -1) {
        fprintf(stderr, "Invalid policy. Policy must be -F, -L, or -C.\n");
        exit(EXIT_FAILURE);
    }

    Cache *cache = cache_new(policy, cache_size);

    char *item = (char *) malloc(sizeof(char) * 3);
    while (fgets(item, 1024, stdin) != NULL) {
        // printf("item: %s", item);
        cache_insert(cache, item);
        item = (char *) malloc(sizeof(char) * cache_size);
    }

    int ca = cache->miss_sum - cache->size_u;

    printf("%i %i\n", cache->size_u, ca);

    return 0;
}
