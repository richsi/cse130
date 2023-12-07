#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <regex.h>
#include <assert.h>
#include "asgn2_helper_funcs.h"
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_REQ_INPUT 2048
#define METHOD        "method"
#define URI           "uri"
#define VERSION       "version"
#define HEADER_NAME   "header_name"
#define HEADER_VALUE  "header_value"

#define VALID_HTTP_VERSION "HTTP/1.1"

#define METHOD_REGEX  "^([a-zA-Z]){1,8}( )"
#define URI_REGEX     "/(/?[a-zA-Z0-9_.-])*{2,64}( )"
#define VERSION_REGEX "(HTTP/[0-9].[0-9]\r\n)"

// #define HEADER_NAME_REGEX   "[a-zA-Z0-9.-]+: {1,128}"
#define HEADER_NAME_REGEX  "[ -~]+: "
#define HEADER_VALUE_REGEX "[^\r\n]+\r\n"

#define HEADERK_REGEX "([^a-zA-Z0-9.-]+){1,128}"
#define HEADERV_REGEX "[^ -~]{1,128}"

struct Node {
    char *key;
    char *value;
    struct Node *next;
};

typedef struct {
    char *buffer;
    size_t request_size;
    int socket;

    //REQUEST LINE
    char *method;
    char *uri;
    char *version;

    //HEADER FIELD
    char *header_field;
    struct Node *head;

    //MESSAGE BODY
    size_t content_size;
    size_t file_size;
    size_t length;

} Request;

void check_arg(int argc);
void validate_port(int port);
void validate_init(int init_listen);

bool process_client_request(Request *request);
void echo_client_msg(int accepted_socket);

void parse_request_line(Request *request, char *field, char *regex);

void get_header_field(Request *request);
bool parse_header_field(Request *request);
void insertNode(struct Node **head, char *key, char *value);
//void insertNode(struct Node** head, char* header);
void printList(struct Node *node);

bool validate_request(Request *request, bool mult);
void handle_contents_put(Request *request);
bool handle_contents_get(Request *request);
bool extra_content_length(struct Node *node);
bool is_invalid_header_field(Request *request);

bool check_method(Request *request);
void respond_get(Request *request);

void send_error_msg(int status_code, Request *request);
bool check_version(Request *request);
bool check_uri(Request *request);
void send_msg(int status_code, Request *request);
void finish_getting_contents(Request *request);
void free_struct(Request *request);
void initialize_null_struct(Request *request);

void empty_response(Request *request);

void handle_contents_put_two(Request *request);
