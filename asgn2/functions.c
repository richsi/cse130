#include "functions.h"

void check_arg(int argc) {
    if (argc > 2) {
        fprintf(stderr, "Invalid Arguments\n");
        exit(1);
    }
}

void validate_port(int port) {
    if ((port < 1) || (port > 65535)) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }
}

void validate_init(int init_listen) {
    if (init_listen == -1) {
        fprintf(stderr, "Invalid Initialization\n");
        exit(1);
    }
}

bool process_client_request(Request *request) {
    int read_bytes;
    int written_bytes = 0;
    char buf[2048];
    // char* substring;

    while ((read_bytes = read_until(request->socket, buf + written_bytes, 1, "\r\n\r\n")) > 0) {

        if (strncmp(buf + written_bytes - 3, "\r\n\r", 3) == 0) {
            //printf("found rnrn\n");
            break;
        }
        written_bytes += 1;
    }

    if (strncmp(buf + written_bytes - 3, "\r\n\r", 3) != 0) {
        //printf("no rnrn\n");
        return false;
    }

    request->request_size = written_bytes;
    request->buffer = malloc(written_bytes + 1);
    memcpy(request->buffer, buf, written_bytes);
    request->buffer[written_bytes] = '\0';

    return true;
}

void echo_client_msg(int socket) {
    char buf[MAX_REQ_INPUT];
    int read_bytes, written_bytes;

    while ((read_bytes = read(socket, buf, MAX_REQ_INPUT)) > 0) {
        written_bytes = 0;
        if (read_bytes < 1)
            break;

        while (written_bytes < read_bytes) {
            written_bytes += write(0, buf + written_bytes, read_bytes - written_bytes);
        }
    }
}

bool is_invalid_header_field(Request *request) {
    regex_t re_key, re_value;
    regmatch_t match;
    int rc, rv;

    if ((strcmp(request->method, "GET\0") == 0) && (request->head == NULL)) {
        // printf("\r\n headerfield\n");
        return false;
    }

    struct Node *head = request->head;

    rc = regcomp(&re_key, HEADERK_REGEX, REG_EXTENDED);
    rv = regcomp(&re_value, HEADERV_REGEX, REG_EXTENDED);

    while (head != NULL) {
        //printf("\nkv:\n%s\n%s\n", head->key, head->value);
        rc = regexec(&re_key, (char *) head->key, 1, &match, 0);
        if (rc == 0)
            break;

        rv = regexec(&re_value, (char *) head->value, 1, &match, 0);
        if (rv == 0)
            break;

        head = head->next;
    }

    regfree(&re_key);
    regfree(&re_value);

    if (!rc || !rv) {
        return true;
    } else
        return false;

    return false;
}

void handle_contents_put(Request *request) {
    int read_bytes, written_bytes;
    char buf[2048];
    char *file = request->uri;
    file++;
    bool file_exists = false;

    if (access(file, F_OK) == 0)
        file_exists = true;

    if (strcmp(request->method, "PUT") == 0) {

        int fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, 0666);

        if (fd == -1)
            return;

        while ((read_bytes = read(request->socket, buf, 2048)) > 0) {
            written_bytes = 0;
            if (read_bytes < 1)
                break;

            while (written_bytes < read_bytes) {
                written_bytes += write(fd, buf + written_bytes, read_bytes - written_bytes);
            }
        }
        close(fd);
    }

    if (file_exists) {
        send_msg(200, request);
    } else
        send_msg(201, request);
}

void handle_contents_put_two(Request *request) {
    int read_bytes, written_bytes;
    char buf[2048];
    char *file = request->uri;
    file++;

    if (strcmp(request->method, "PUT") == 0) {

        int fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, 0666);

        if (fd == -1)
            return;

        while ((read_bytes = read(request->socket, buf, 2048)) > 0) {
            written_bytes = 0;
            if (read_bytes < 1)
                break;

            while (written_bytes < read_bytes) {
                written_bytes += write(fd, buf + written_bytes, read_bytes - written_bytes);
            }
        }
        close(fd);
    }
}

bool handle_contents_get(Request *request) {
    int read_bytes;
    char buf[2048];
    if (strcmp(request->method, "GET\0") == 0) {
        while ((read_bytes = read(request->socket, buf, 2048)) > 0) {
            return false;
        }
    }
    return true;
}

void send_msg(int status_code, Request *request) {
    if (status_code == 200) {
        char *buf = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 201) {
        char *buf = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n";
        write_all(request->socket, buf, strlen(buf));
    }
}

void respond_get(Request *request) {
    size_t passed_bytes = 0;
    char *file = request->uri;
    file++;

    struct stat st;

    int fd = open(file, O_RDONLY, 0666);
    if (fd == -1)
        return;

    int file_size = fstat(fd, &st);

    file_size = st.st_size;
    request->file_size = passed_bytes;

    char buf[50];
    snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", file_size);
    write_all(request->socket, buf, strlen(buf));
    passed_bytes = pass_bytes(fd, request->socket, file_size);

    close(fd);
}

bool check_method(Request *request) {
    if ((strcmp(request->method, "GET\0") != 0) && (strcmp(request->method, "PUT\0") != 0)) {
        return true;
    }
    return false;
}

void send_error_msg(int status_code, Request *request) {

    if (status_code == 400) {
        char *buf = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 403) {
        char *buf = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 404) {
        char *buf = "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 500) {
        char *buf = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal "
                    "Server Error\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 501) {
        char *buf = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
        write_all(request->socket, buf, strlen(buf));
    } else if (status_code == 505) {
        char *buf = "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not "
                    "Supported\n";
        write_all(request->socket, buf, strlen(buf));
    }
}

bool check_uri(Request *request) {
    char *file = request->uri;
    file++;

    if ((access(file, F_OK) != 0) && (strcmp(request->method, "GET\0") == 0)) {
        send_error_msg(404, request);
        return true;
    } else if (strcmp(request->method, "GET\0") == 0) {
        struct stat st;

        if (stat(file, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                send_error_msg(403, request);
                return true;
            }
        }

        int fd = open(file, O_RDONLY, 0666);
        int file_size = fstat(fd, &st);

        if (file_size < 0) {
            send_error_msg(500, request);
            close(fd);
            return true;
        }
    }
    return false;
}

bool check_version(Request *request) {
    char str[strlen(request->version) + 1];

    strcpy(str, request->version);
    // printf("func str: %s", str);

    char *version = strtok(str, "/");

    if (strcmp(version, "HTTP") != 0) {
        send_error_msg(400, request);
        return true;
    }

    version = strtok(NULL, "/");

    if (strcmp(version, "1.1") != 0) {
        send_error_msg(505, request);
        return true;
    }

    return false;
}

bool validate_request(Request *request, bool mult) {

    if ((request->method == NULL) || mult) {
        send_error_msg(400, request);
        return false;
    } else if (check_method(request)) {
        send_error_msg(501, request);
        return false;
    }

    if (request->uri == NULL) {
        send_error_msg(400, request);
        return false;
    } else if (check_uri(request)) {
        return false;
    }

    if (request->version == NULL) {
        send_error_msg(400, request);
        return false;
    } else if (check_version(request)) {
        return false;
    }

    // if (((strcmp(request->method, "PUT\0") == 0) && (request->head == NULL)) ||
    //     (is_invalid_header_field(request))){
    //     printf("invalid header\n");
    //     send_error_msg(400, request);
    //     return false;
    // }

    if (((strcmp(request->method, "PUT\0") == 0) && (request->head == NULL))
        || (is_invalid_header_field(request))) {
        printf("invalid header\n");
        send_error_msg(400, request);
        return false;
    }

    return true;
}

void finish_getting_contents(Request *request) {
    int read_bytes;
    char buf[2049];

    while ((read_bytes = read(request->socket, buf, 2048)) > 0) {
    }
}

void free_struct(Request *request) {

    free(request->buffer);
    request->buffer = NULL;
    free(request->method);
    request->method = NULL;
    free(request->uri);
    request->uri = NULL;
    free(request->header_field);
    request->header_field = NULL;
    free(request->version);
    request->version = NULL;
    //free(request);
}

void initialize_null_struct(Request *request) {
    request->buffer = NULL;
    request->method = NULL;
    request->uri = NULL;
    request->header_field = NULL;
    request->version = NULL;
}

void empty_response(Request *request) {

    char *buf = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\n";
    write_all(request->socket, buf, strlen(buf));
}
