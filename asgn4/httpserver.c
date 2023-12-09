#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "queue.h"
#include "request.h"
#include "response.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/file.h>

#define DEAFULT_THREAD_COUNT 4
#define RID                  "Request-Id"
#define TEMP_FILENAME        ".temp_file"

void handle_connection(void);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void acquire_exclusive(int fd);
void acquire_shared(int fd);
int acquire_templock(void);
void release(int fd);

char *get_rid(conn_t *conn) {
    char *id = conn_get_header(conn, RID);
    if (id == NULL) {
        id = "0";
    }
    return id;
}

void audit_log(char *name, char *uri, char *id, int code) {
    fprintf(stderr, "%s,/%s,%d,%s\n", name, uri, code, id);
}

//Gloabls
queue_t *workq = NULL;

void usage(FILE *stream, char *exec) {
    fprintf(stream, "usage: %s [-t threads] <port>\n", exec);
}

void acquire_exclusive(int fd) {
    int rc = flock(fd, LOCK_EX);
    assert(rc == 0); //500 ERROR DONT USE ASSERT
    // debug("acquire_exclusive on %d", fd);
}

void acquire_shared(int fd) {
    int rc = flock(fd, LOCK_SH);
    assert(rc == 0);
    // debug("acquire_shared on %d", fd);
}

int acquire_templock(void) {
    int fd = open(TEMP_FILENAME, O_WRONLY); //CREATE TEMPFILE
    // debug("opened %d", fd);
    acquire_exclusive(fd);
    return fd;
}

void release(int fd) {
    // debug("release");
    flock(fd, LOCK_UN);
}

void init(int threads) {
    workq = queue_new(threads);
    creat(TEMP_FILENAME, 0600);
    assert(workq != NULL);
}

int main(int argc, char **argv) {
    int opt = 0;
    int threads = DEAFULT_THREAD_COUNT;
    pthread_t *threadids;

    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "t:h")) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'h': usage(stdout, argv[0]); return EXIT_SUCCESS;
        default: usage(stderr, argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong arguments: %s port_num", argv[0]);
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    size_t port
        = (size_t) strtoull(argv[optind], &endptr, 10); //ORDER OF ARGUMENTS SHOULD NOT MATTER
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    if (listener_init(&sock, port) < 0) {
        warnx("Cannot open listener sock: %s", argv[0]);
        return EXIT_FAILURE;
    }

    threadids = malloc(sizeof(pthread_t) * threads);
    init(threads);

    for (int i = 0; i < threads; ++i) {
        int rc = pthread_create(threadids + i, NULL, (void *(*) (void *) ) handle_connection, NULL);
        if (rc != 0) {
            warnx("Cannot create %d pthreads", threads);
            return EXIT_FAILURE;
        }
    }

    while (1) {
        uintptr_t connfd = listener_accept(&sock);
        queue_push(workq, (void *) connfd);
    }
    queue_delete(&workq);

    return EXIT_SUCCESS;
}

void handle_connection(void) {
    while (true) {
        uintptr_t connfd = 0;
        conn_t *conn = NULL;

        queue_pop(workq, (void **) &connfd);
        //debug("popped off %lu", connfd);
        conn = conn_new(connfd);

        const Response_t *res = conn_parse(conn);

        if (res != NULL) {
            conn_send_response(conn, res);
        } else {
            //debug("%s", conn_str(conn));
            const Request_t *req = conn_get_request(conn);

            if (req == &REQUEST_GET) {
                handle_get(conn);
            } else if (req == &REQUEST_PUT) {
                handle_put(conn);
            } else {
                handle_unsupported(conn);
            }
        }
        conn_delete(&conn);
        close(connfd);
    }
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    int status = 0;
    long num_bytes = 0;
    const Response_t *re = NULL;

    int lock = acquire_templock();
    int filefd = open(uri, O_RDONLY);
    int errnum = errno;

    if (filefd < 0) {
        if (errnum == EACCES) {
            status = 403;
            re = &RESPONSE_FORBIDDEN;
        } else if (errnum == ENOENT) {
            status = 404;
            re = &RESPONSE_NOT_FOUND;
        }
        goto out;
    } else {
        status = 200;
    }

    acquire_shared(filefd);
    release(lock);
    close(lock);

    struct stat st;
    num_bytes = fstat(filefd, &st);
    num_bytes = st.st_size;

    if (stat(uri, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            status = 403;
            re = &RESPONSE_FORBIDDEN;
        } else {
            conn_send_file(conn, filefd, num_bytes);
        }
    }

out:
    if (re != NULL) {
        conn_send_response(conn, re);
    }
    audit_log("GET", uri, get_rid(conn), status);
    if (filefd > 0) {
        release(filefd);
        close(filefd);
    } else {
        release(lock);
        close(lock);
    }
    return;
}

void handle_put(conn_t *conn) {
    // TODO: Implement PUT
    char *uri = conn_get_uri(conn);
    int status = 0;
    const Response_t *re = NULL;

    int lock = acquire_templock();
    bool exists = access(uri, F_OK) == 0;

    int filefd = open(uri, O_CREAT | O_WRONLY, 0600);
    int errnum = errno;

    struct stat st;

    if (stat(uri, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            status = 403;
            re = &RESPONSE_FORBIDDEN;
            goto out;
        }
    }

    if (filefd < 0) {
        if (errnum == EACCES) {
            status = 403;
            re = &RESPONSE_FORBIDDEN;
        } else {
            status = 500;
            re = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        goto out;
    }

    acquire_exclusive(filefd);
    release(lock);
    close(lock);

    int rc = ftruncate(filefd, 0);
    assert(rc == 0);
    conn_recv_file(conn, filefd);

    if (re == NULL) {
        if (exists) {
            status = 200;
            conn_send_response(conn, &RESPONSE_OK);
        } else {
            status = 201;
            conn_send_response(conn, &RESPONSE_CREATED);
        }
    }

out:

    if (re != NULL) {
        conn_send_response(conn, re);
    }

    audit_log("PUT", uri, get_rid(conn), status);
    if (filefd > 0) {
        release(filefd);
        close(filefd);
    } else {
        release(lock);
        close(lock);
    }
    return;
}

void handle_unsupported(conn_t *conn) {
    // debug("Unsupported request");
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
    return;
}
