#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <errno.h>
#include "utils.h"

#define MAX_CLIENTS 1

typedef struct ServersDetails_st ServerDetails;
struct ServersDetails_st
{
    int c_id;
    bool shut_down = false;
    std::string fname;
    unsigned int sock_fd;
    unsigned int client_sock_fd;
    FILE *fp;
};

bool validate_cid(int c_id)
{
    if (c_id == 1 || c_id == 2)
    {
        return true;
    }
    else if (c_id == -1)
    {
        std::cerr << "(validate_cid) Invalid c_id. Exiting." << '\n';
    }
    else
    {
        std::cerr << "(validate_cid) Invalid c_id: " << c_id << ". Exiting." << '\n';
    }

    return false;
}

void server_socket_init(struct sockaddr_in &server_addr, unsigned int &server_sockfd,
                        char *server_ip, unsigned int server_port)
{
    int server_sock_status;

    // Init socket server file descriptor
    int set_option = 1;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&set_option, sizeof(set_option));
    if (server_sockfd < 0)
    {
        std::cerr << "(server_socket_init) Socket creation error." << '\n';
        exit(1);
    }

    // Init server_addr instance
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    server_sock_status = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (server_sock_status < 0)
    {
        std::cerr << "(server_socket_init) Socket connection error." << '\n';
        exit(1);
    }

    // Start listening on the socket
    server_sock_status = listen(server_sockfd, MAX_CLIENTS);
    if (server_sock_status < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "(server_socket_init) Socker server ready on port: " << server_port << "." <<'\n';
}

FILE *open_file(int c_id, std::string fname)
{
    char *fname_arr = get_char_arr(fname);
    FILE *fp = std::fopen(fname_arr, "r");
    if (!fp)
    {
        std::cerr << "(open_file) Couldn't open file: " << fname << ". Exiting." << '\n';
        exit(1);
    }
    free(fname_arr);
    return fp;
}

uint8_t *read_file(FILE *fp, unsigned int max_len)
{
    if (!fp)
    {
        std::cerr << "(read_file) Couldn't read file." << '\n';
    }

    uint8_t *f_contents = (uint8_t *)calloc(max_len + 1, sizeof(uint8_t));
    if (!f_contents)
    {
        std::cerr << "(read_file) Couldn't allocate memory to read file contents." << '\n';
        exit(1);
    }

    char c;
    int i = 0;
    while (fp && i < max_len)
    {
        c = getc(fp);
        *(f_contents + i) = c;
        i++;
    }
    *(f_contents + i) = '\0';

    return f_contents;
}

void terminate_server(ServerDetails *srv_dtls)
{
    srv_dtls->shut_down = true;
    fclose(srv_dtls->fp);
    shutdown(srv_dtls->sock_fd, SHUT_RDWR);
    std::cout << "Server " << srv_dtls->c_id << " shut down complete." << '\n';
}

uint8_t *copy_f_contents(uint8_t *f_contents, uint8_t msg_seq_no, unsigned int len)
{
    uint8_t *resp_f_msg = (uint8_t *)calloc(len + 2, sizeof(uint8_t));
    if (!resp_f_msg)
    {
        std::cerr << "(read_file) Couldn't allocate memory to create client response message." << '\n';
        exit(1);
    }

    *resp_f_msg = msg_seq_no;
    int i = 0;
    int j = (msg_seq_no - 1) * (FSIZE / 3);
    while (i < len)
    {
        *(resp_f_msg + i + 1) = *(f_contents + j);
        i++;
        j++;
    }

    *(resp_f_msg + i + 1) = '\0';

    return resp_f_msg;
}

void send_file_to_client(ServerDetails *srv_dtls)
{
    uint8_t *f_contents = read_file(srv_dtls->fp, FSIZE);

    for (int i = 1; i <= 3; ++i)
    {
        uint8_t *resp_f_msg = copy_f_contents(f_contents, i, (FSIZE / 3));

        int server_sock_send_count = send(srv_dtls->client_sock_fd, resp_f_msg, len_server_f_msg(resp_f_msg), 0);
        if (server_sock_send_count < 0)
        {
            std::cerr << "(send_file_to_client) Socket send error." << '\n';
        }

        free(resp_f_msg);
    }

    free(f_contents);
}

void send_msg_to_client(ServerDetails *srv_dtls, std::string server_msg)
{
    char *server_msg_buf = get_char_arr(server_msg);

    int server_sock_send_count = send(srv_dtls->client_sock_fd, server_msg_buf, strlen(server_msg_buf), 0);
    if (server_sock_send_count < 0)
    {
        std::cerr << "(send_msg_to_client) Socket send error." << '\n';
    }
    std::cout << "(send_msg_to_client) " << server_msg << " message sent to client." << '\n';
    free(server_msg_buf);
}

void handle_client_msg(std::string client_msg, ServerDetails *srv_dtls)
{
    std::string REQ_fname = "REQ_F_" + srv_dtls->fname;
    std::string REQ_fcontents = "REQ_F_" + srv_dtls->fname + "_CONTENTS";
    std::string RESP_fyes = "RESP_F_YES";
    std::string RESP_fno = "RESP_F_NO";

    std::cout << "(handle_client_msg) " << client_msg << " message received from client." << '\n';

    // If first REQ message received with correct file name, send YES RESP message
    if (client_msg == REQ_fname)
    {
        send_msg_to_client(srv_dtls, RESP_fyes);
    }
    // If file contents REQ message received, then send F1 in 3 messages with message_seq_numbers
    else if (client_msg == REQ_fcontents)
    {
        send_file_to_client(srv_dtls);
        std::cout << "File Transfer complete." << '\n';
        terminate_server(srv_dtls);
    }
    // Else send NO RESP messages
    else
    {
        send_msg_to_client(srv_dtls, RESP_fno);
    }
}

int main(int argc, char *argv[])
{
    int c_id = -1;

    // Server c_id not provided. so ask user for c_id.
    if (argc == 1)
    {
        std::cout << "Usage: ./server <c_id>" << '\n';
        std::cout << "c_id can only be 1 or 2." << '\n';
        std::cin >> c_id;
    }
    // Server c_id provided as argument.
    else if (argc == 2)
    {
        c_id = parse_int_char_arr(argv[1]);
    }
    else
    {
        std::cerr << "Invalid usage. Usage: ./server <c_id>" << '\n';
        return 1;
    }

    // Validate c_id = 1 or 2
    if (!validate_cid(c_id))
    {
        std::cerr << "c_id can only be 1 or 2." << '\n';
        return 1;
    }

    // Server socket vars
    unsigned int server_sockfd;
    struct sockaddr_in server_addr;
    socklen_t server_addrlen = sizeof(server_addr);
    // Socket open for connections from outside this server
    std::string server_hostname = "0.0.0.0";
    char *server_hostname_arr = get_char_arr(server_hostname);

    // Setup socket listening connection
    server_socket_init(server_addr, server_sockfd, server_hostname_arr, SRVR_PORT);
    free(server_hostname_arr);

    // Open files F1 or F2 respectively
    std::string server_fname = "f" + std::to_string(c_id) + ".dat";
    FILE *server_filepointer = open_file(c_id, server_fname);

    // Create ServerDetails instance which can be passed onto handle_terminate
    ServerDetails *srv_dtls = new ServerDetails;
    srv_dtls->fname = server_fname;
    srv_dtls->fp = server_filepointer;
    srv_dtls->sock_fd = server_sockfd;
    srv_dtls->c_id = c_id;

    // blocking Call to accept incoming socket connections.
    int server_new_socket = accept(server_sockfd, (struct sockaddr *)&server_addr, &server_addrlen);
    if (server_new_socket < 0)
    {
        perror("accept");
        exit(1);
    }
    else
    {
        std::cout << "Client connected." << '\n';
        srv_dtls->client_sock_fd = server_new_socket;
    }

    // Handle Client Socket connection
    char client_msg_buf[32];
    std::string client_msg;

    while (!srv_dtls->shut_down)
    {
        memset(client_msg_buf, 0, sizeof(client_msg_buf));
        int server_sock_recv_count = read(srv_dtls->client_sock_fd, client_msg_buf, sizeof(client_msg_buf));
        if (server_sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(1000);
        }
        else if (server_sock_recv_count < 0)
        {
            std::cerr << "Socket receive error." << '\n';
            break;
        }
        else
        {
            client_msg = get_str(client_msg_buf);
            handle_client_msg(client_msg, srv_dtls);
        }
    }

    return 0;
}