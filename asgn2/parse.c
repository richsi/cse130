#include "functions.h"

void parse_request_line(Request *request, char *field, char *regex) {
    regex_t re;
    regmatch_t match;
    int rc;

    rc = regcomp(&re, regex, REG_EXTENDED);
    if (rc) {
        printf("PRQ regcomp failed\n");
        return;
    }
    rc = regexec(&re, (char *) request->buffer, 1, &match, 0);
    if (rc) {
        printf("PRQ regexec failed\n");
        return;
    }

    if (strcmp(field, METHOD) == 0) {
        request->method = malloc(match.rm_eo - match.rm_so);
        memcpy(request->method, request->buffer + match.rm_so, match.rm_eo - match.rm_so - 1);
        request->method[match.rm_eo - match.rm_so - 1] = '\0';

    } else if (strcmp(field, URI) == 0) {
        request->uri = malloc(match.rm_eo - match.rm_so);
        memcpy(request->uri, request->buffer + match.rm_so, match.rm_eo - match.rm_so - 1);
        request->uri[match.rm_eo - match.rm_so - 1] = '\0';

    } else if (strcmp(field, VERSION) == 0) {
        request->version = malloc(match.rm_eo - match.rm_so);
        memcpy(request->version, request->buffer + match.rm_so, match.rm_eo - match.rm_so - 2);
        request->version[match.rm_eo - match.rm_so - 2] = '\0';
    }

    request->buffer += match.rm_eo - match.rm_so;
    regfree(&re);
}

void get_header_field(Request *request) {
    request->header_field = request->buffer;
}

bool parse_header_field(Request *request) { //referenced from mitchells code
    regex_t re_key;
    regex_t re_value;
    regmatch_t match;
    int buffer_index = 0;
    int counter = 0;
    int rc, rv;
    request->head = NULL;

    rc = regcomp(&re_key, HEADER_NAME_REGEX, REG_EXTENDED);
    rv = regcomp(&re_value, HEADER_VALUE_REGEX, REG_EXTENDED);

    //printf("headerbuf:\n%s.\n", request->buffer);

    while (!rc && !rv) {
        rc = regexec(&re_key, (char *) request->buffer + buffer_index, 1, &match, 0);
        if (rc)
            break;

        int key_start = match.rm_so;
        int key_end = match.rm_eo;
        int key_length = key_end - key_start;

        char *key = strndup(request->buffer + buffer_index + key_start, key_length - 2);
        buffer_index += key_length;

        rv = regexec(&re_value, (char *) request->buffer + buffer_index, 1, &match, 0);
        if (rv)
            break;

        int value_start = match.rm_so;
        int value_end = match.rm_eo;
        int value_length = value_end - value_start;

        char *value = strndup(request->buffer + buffer_index + value_start, value_length - 2);
        buffer_index += value_length;

        //printf("value: %s\n", value);

        if (strcmp(key, "Content-Length") == 0) {
            request->length = atoi(value);
            counter++;
            insertNode(&request->head, key, value);
        } else {
            insertNode(&request->head, key, value);
        }

        if (counter > 1)
            return true;
    }

    //printList(request->head);

    regfree(&re_key);
    regfree(&re_value);
    return false;
}

void insertNode(struct Node **head, char *key, char *value) { // referenced from geeks4geeks
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
    struct Node *last = *head;

    new_node->key = key;
    new_node->value = value;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    while (last->next != NULL)
        last = last->next;

    last->next = new_node;
    return;
}

void printList(struct Node *node) {
    if (node == NULL)
        printf("null\n");
    while (node != NULL) {
        //printf("key: %s, value: %s\n", node->key, node->value);
        node = node->next;
    }
}
