#include "functions.h"

int main(int argc, char **argv) {

    check_arg(argc);
    Listener_Socket socket;

    int port = atoi(argv[1]); // setting port number
    validate_port(port);
    signal(SIGPIPE, SIG_IGN); //code given by mitchell

    int init_listen = listener_init(&socket, port); //check int
    validate_init(init_listen);
    //Request* request = malloc(sizeof(Request));

    while (1) {
        Request *request = malloc(sizeof(Request));
        int accepted_socket = listener_accept(&socket); //check int
        request->socket = accepted_socket;

        initialize_null_struct(request);

        if (process_client_request(request)) {
            //printf("buf:\n%s\n", request->buffer);

            parse_request_line(request, METHOD, METHOD_REGEX);
            parse_request_line(request, URI, URI_REGEX);
            parse_request_line(request, VERSION, VERSION_REGEX);

            get_header_field(request);
            bool multiple_content_lengths = parse_header_field(request);

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                empty_response(request);
                if (strcmp(request->method, "GET\0") == 0) {
                    finish_getting_contents(request);
                } else if (strcmp(request->method, "PUT\0") == 0) {
                    check_uri(request);
                    handle_contents_put_two(request);
                }
                close(accepted_socket);
                continue;
            }

            // printf("\nmethod:\n%s.\n", request->method);

            // printf("\nversion:\n%s.\n", request->version);
            // printf("\nheaderfield:\n%s\n", request->header_field);

            if (validate_request(request, multiple_content_lengths)) {
                if (strcmp(request->method, "GET\0") == 0) {
                    if (handle_contents_get(request)) {
                        //printf("\nURI: %s.\n", request->uri);
                        respond_get(request);
                    } else {
                        send_error_msg(400, request);
                    }
                } else if (strcmp(request->method, "PUT\0") == 0) {
                    handle_contents_put(request);
                }
            } else
                finish_getting_contents(request);

        } else
            send_error_msg(400, request); // NO RNRN

        //initialize_null_struct(request);

        // free_struct(request);
        close(accepted_socket);
    }
    //echo_client_msg(accepted_socket); //echos client input
    return EXIT_SUCCESS;
}
