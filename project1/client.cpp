#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <errno.h>
#include "utils.h"

typedef struct ServersDetails_st ServerDetails;
struct ServersDetails_st
{
    unsigned int c_id;
    bool connected = false;
    std::string hostname;
    unsigned int sock_fd;
    bool f_transfer_complete = false;
};

std::string get_server_addr(std::string arg)
{
    std::string delim = "=";
    std::string argparam;
    size_t argval_begin = arg.find(delim);

    if (argval_begin == std::string::npos)
    {
        return arg;
    }

    argparam = arg.substr(0, argval_begin);
    if (argparam == "--p1" || argparam == "--p2")
    {
        return arg.substr(++argval_begin);
    }
    else
    {
        std::cerr << "(get_server_addr) Invalid arg param: " << argparam << '\n';
        return arg;
    }
}

bool validate_server_addr(std::string p_addr)
{
    if (p_addr.length() != 4)
    {
        return false;
    }

    if (p_addr[0] != 'd' || p_addr[1] != 'c')
    {
        return false;
    }

    int dc_id = parse_int_str(p_addr.substr(2));
    if (dc_id < 1 || dc_id > 45)
    {
        return false;
    }

    return true;
}

bool validate_server_addr(std::string p1_addr, std::string p2_addr)
{
    bool p1_valid, p2_valid;

    p1_valid = validate_server_addr(p1_addr);
    if (!p1_valid)
    {
        std::cerr << "(validate_server_addr) Invalid P1 address: " << p1_addr << '\n';
    }

    p2_valid = validate_server_addr(p2_addr);
    if (!p2_valid)
    {
        std::cerr << "(validate_server_addr) Invalid P2 address: " << p2_addr << '\n';
    }

    if (!p1_valid || !p2_valid)
    {
        return false;
    }

    if (p1_addr == p2_addr)
    {
        std::cerr << "(validate_server_addr) P1 and P2 addresses: " << p1_addr << " must be different." << '\n';
        return false;
    }

    return true;
}

ServerDetails *connect_server(std::string serv_hostname, int c_id)
{
    serv_hostname += ".utdallas.edu";
    ServerDetails *srv_dtls = new ServerDetails;
    srv_dtls->hostname = serv_hostname;
    srv_dtls->c_id = c_id;

    // Resolve hostname -> IP address and connect
    int sfd, addrinfo_res;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char *hostname = get_char_arr(serv_hostname);
    char *port = get_char_arr(std::to_string(SRVR_PORT));

    /* Obtain address(es) matching host/port. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    addrinfo_res = getaddrinfo(hostname, port, &hints, &result);
    if (addrinfo_res != 0)
    {
        std::cerr << "(connect_server) getaddrinfo: " << gai_strerror(addrinfo_res) << '\n';
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            /* Success */
            srv_dtls->sock_fd = sfd;
            srv_dtls->connected = true;
            std::cout << "(connect_server) Socket connection established with " << serv_hostname << '\n';
            break;
        }

        close(sfd);
    }

    freeaddrinfo(result);
    free(hostname);
    free(port);

    /* No address succeeded */
    if (rp == nullptr)
    {
        srv_dtls->connected = false;
        std::cerr << "(connect_server) Socket connection error. Couldn't connect to " << serv_hostname << '\n';
    }

    return srv_dtls;
}

FILE *open_file(std::string fname)
{
    char *fname_arr = get_char_arr(fname);
    FILE *fp = std::fopen(fname_arr, "w");
    if (!fp)
    {
        std::cerr << "(open_file) Couldn't open file: " << fname << ". Exiting." << '\n';
        exit(1);
    }

    free(fname_arr);
    return fp;
}

void write_to_file(FILE *fp, uint8_t *f_contents)
{
    if (!fp)
    {
        std::cerr << "(write_to_file) Couldn't write to file. Invalid file pointer." << '\n';
        return;
    }

    int i = 0;
    while (i < FSIZE && fp)
    {
        putc(*f_contents, fp);
        f_contents++;
        i++;
    }

    std::cout << "(write_to_file) Written to file successfully." << '\n';
}

std::string read_msg_f_contents(uint8_t *server_f_msg, unsigned int len)
{
    std::string msg;
    if (!server_f_msg || len <= 0)
    {
        return msg;
    }

    int i = 0;
    while (i < len && *(server_f_msg + i) != '\0')
    {
        msg += (char)*(server_f_msg + i);
        i++;
    }

    return msg;
}

bool request_file_exists(ServerDetails *srv_dtls, std::string fname)
{
    std::string REQ_fname = "REQ_F_" + fname;
    std::string RESP_fyes = "RESP_F_YES";
    std::string RESP_fno = "RESP_F_NO";

    char *req_f_msg = get_char_arr(REQ_fname);

    int sock_send_count = send(srv_dtls->sock_fd, req_f_msg, strlen(req_f_msg), 0);
    if (sock_send_count < 0)
    {
        std::cerr << "(request_file_exists) Socket send error." << '\n';
        return false;
    }
    free(req_f_msg);
    std::cout << "(request_file_exists) " << REQ_fname << " message sent to server " << srv_dtls->c_id << "." << '\n';

    char resp_msg_buf[32];
    std::string resp_msg;
    while (srv_dtls->connected)
    {
        memset(resp_msg_buf, '\0', sizeof(resp_msg_buf));
        int sock_recv_count = read(srv_dtls->sock_fd, resp_msg_buf, sizeof(resp_msg_buf));
        if (sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(1000);
        }
        else if (sock_recv_count < 0)
        {
            std::cerr << "(request_file_exists) Socket receive error." << '\n';
            break;
        }
        else
        {
            resp_msg = get_str(resp_msg_buf);
            std::cout << "(request_file_exists) " << resp_msg << " message received from server " << srv_dtls->c_id << "." << '\n';

            if (resp_msg == RESP_fyes)
            {
                return true;
            }
            else if (resp_msg == RESP_fno)
            {
                return false;
            }
            else
            {
                std::cerr << "(request_file_exists) Unexpected message = " << resp_msg << " recevied." << '\n';
            }
        }
    }

    return false;
}

bool process_file_contents(ServerDetails *srv_dtls, uint8_t *f_contents, uint8_t *resp_f_contents_msg)
{
    if (len_server_f_msg(resp_f_contents_msg) != FMSGSIZE)
    {
        return false;
    }

    int i;
    int j;
    std::string f_contents_start = "start";
    std::string f_contents_end = "end";
    uint8_t msg_seq_no = *resp_f_contents_msg;
    resp_f_contents_msg++;

    // Check if this is the first message in sequence of 3 messages and
    if (msg_seq_no == 1)
    {
        std::string msg_begin = read_msg_f_contents(resp_f_contents_msg, f_contents_start.length());
        if (msg_begin == f_contents_start)
        {
            i = 0;
            j = 0;
            while (i < FMSGSIZE && *(resp_f_contents_msg + i) != '\0')
            {
                *(f_contents + j) = *(resp_f_contents_msg + i);
                i++;
                j++;
            }
            return true;
        }

        return false;
    }
    else if (msg_seq_no == 2)
    {
        i = 0;
        j = FSIZE / 3;
        while (i < FMSGSIZE && *(resp_f_contents_msg + i) != '\0')
        {
            *(f_contents + j) = *(resp_f_contents_msg + i);
            i++;
            j++;
        }
        return true;
    }
    else if (msg_seq_no == 3)
    {
        std::string msg_end = read_msg_f_contents(resp_f_contents_msg + (FSIZE / 3) - f_contents_end.length(), f_contents_end.length());
        if (msg_end == f_contents_end)
        {
            i = 0;
            j = 2 * (FSIZE / 3);
            while (i < FMSGSIZE && *(resp_f_contents_msg + i) != '\0')
            {
                *(f_contents + j) = *(resp_f_contents_msg + i);
                i++;
                j++;
            }
            srv_dtls->f_transfer_complete = true;
            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

uint8_t *request_file_contents(ServerDetails *srv_dtls, std::string fname, FILE *fp)
{
    std::string REQ_fcontents = "REQ_F_" + fname + "_CONTENTS";
    char *req_f_msg = get_char_arr(REQ_fcontents);

    int sock_send_count = send(srv_dtls->sock_fd, req_f_msg, strlen(req_f_msg), 0);
    if (sock_send_count < 0)
    {
        std::cerr << "(request_file_contents) Socket send error." << '\n';
    }
    free(req_f_msg);
    std::cout << "(request_file_contents) " << REQ_fcontents << " message sent to server " << srv_dtls->c_id << "." << '\n';

    uint8_t *f_contents = (uint8_t *)calloc(FSIZE, sizeof(uint8_t));
    uint8_t resp_f_contents_msg[FMSGSIZE];

    while (srv_dtls->connected && !srv_dtls->f_transfer_complete)
    {
        memset(resp_f_contents_msg, '\0', sizeof(resp_f_contents_msg));
        int sock_recv_count = read(srv_dtls->sock_fd, resp_f_contents_msg, sizeof(resp_f_contents_msg));
        if (sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(1000);
        }
        else if (sock_recv_count < 0)
        {
            std::cerr << "(request_file_contents) Socket receive error." << '\n';
            break;
        }
        else
        {
            if (!process_file_contents(srv_dtls, f_contents, resp_f_contents_msg))
            {
                std::cerr << "(request_file_contents) Unexpected message recevied" << '\n';
            }
        }
    }

    return f_contents;
}

void terminate_client(ServerDetails *p1_dtls, ServerDetails *p2_dtls, FILE *fp, std::string fname)
{
    if (p1_dtls)
    {
        shutdown(p1_dtls->sock_fd, SHUT_RDWR);
        p1_dtls->connected = false;
        free(p1_dtls);
    }
    else
    {
        std::cerr << "(terminate_client) Couldn't close connection with P1." << '\n';
    }

    if (p2_dtls)
    {
        shutdown(p2_dtls->sock_fd, SHUT_RDWR);
        p2_dtls->connected = false;
        free(p2_dtls);
    }
    else
    {
        std::cerr << "(terminate_client) Couldn't close connection with P2." << '\n';
    }

    if (fp)
    {
        fclose(fp);
    }
    else
    {
        std::cerr << "(terminate_client) Couldn't close file " << fname << "." << '\n';
    }

    std::cout << "(terminate_client) Client shut down complete." << '\n';
}

int main(int argc, char *argv[])
{
    std::string p1_addr;
    std::string p2_addr;

    // Servers addresses not provided. so ask user for server addresse.
    if (argc == 1)
    {
        std::cout << "Usage: ./client --p1=<dc01-45> --p2=<dc01-45>" << '\n';
        std::cout << "Input <p1_addrr> <space> <p2_addr>" << '\n';
        std::cin >> p1_addr >> p2_addr;
    }
    // Server c_id provided as argument.
    else if (argc == 3)
    {
        p1_addr = get_server_addr(get_str(argv[1]));
        p2_addr = get_server_addr(get_str(argv[2]));
    }
    else
    {
        std::cerr << "Invalid usage. Usage: ./client --p1=<dc01-45> --p2=<dc01-45>" << '\n';
        return 1;
    }

    if (!validate_server_addr(p1_addr, p2_addr))
    {
        return 1;
    }

    ServerDetails *p1_dtls = connect_server(p1_addr, 1);
    if (!p1_dtls || !p1_dtls->connected)
    {
        exit(1);
    }

    ServerDetails *p2_dtls = connect_server(p2_addr, 2);
    if (!p2_dtls || !p2_dtls->connected)
    {
        exit(1);
    }

    std::string fname;
    std::cout << "Enter file name: ";
    std::cin >> fname;
    FILE *fp = nullptr;
    uint8_t *file_contents = nullptr;

    if (request_file_exists(p1_dtls, fname))
    {
        fp = open_file(fname);
        file_contents = request_file_contents(p1_dtls, fname, fp);
    }
    else if (request_file_exists(p2_dtls, fname))
    {
        fp = open_file(fname);
        file_contents = request_file_contents(p2_dtls, fname, fp);
    }
    else
    {
        std::cout << "File transfer failure." << '\n';
    }

    if (file_contents)
    {
        write_to_file(fp, file_contents);
        free(file_contents);
    }

    terminate_client(p1_dtls, p2_dtls, fp, fname);

    return 0;
}