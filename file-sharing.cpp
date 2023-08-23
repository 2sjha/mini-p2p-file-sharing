#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

// Socket Headers
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h"

/**
 * Send message to peer but this supports uint8_t
 */
void send_uint8_msg_to_peer(PeerDetails *pr_dtls, uint8_t *msg, unsigned int msg_len)
{
    if (!pr_dtls || !pr_dtls->connected)
    {
        std::cerr << "(send_uint8_msg_to_peer) Can't send msg to peer. Peer is disconnected.\n";
        return;
    }

    int server_sock_send_count = send(pr_dtls->sock_fd, msg, msg_len, 0);
    if (server_sock_send_count < 0)
    {
        std::cerr << "(send_uint8_msg_to_peer) Socket send error." << '\n';
    }
    std::cout << "(send_uint8_msg_to_peer) message (seq no. " << (int)msg[0] << ", size " << msg_len << ") sent to peer.\n";
}

/**
 * Reads the file into a buffer and sends the file over on socket with sequence numbers attached on each message.
 * Sequence numbers attached in the beginning will not interfere with file contents, as our search files only contain english words.
 * Will only handle files less than MAX_FILE_SIZE. Cancels if file is bigger than that.
 */
void send_file(PeerDetails *pr_dtls, std::fstream &fs, std::string fname, unsigned int fparts)
{
    char c;
    unsigned int char_counter = 0;
    uint8_t file_contents_buf[fparts][MSG_SIZE + 1]; // similar to normal message but 1 extra byte to represent sequence number
    memset(file_contents_buf, 0, sizeof(file_contents_buf));

    int buf_i, buf_j;
    while (fs >> c)
    {
        buf_i = char_counter / MSG_SIZE;
        buf_j = char_counter % MSG_SIZE;
        char_counter++;
        if (buf_j == 0)
        {
            file_contents_buf[buf_i][buf_j] = (uint8_t)(buf_i + 1);
        }
        file_contents_buf[buf_i][buf_j + 1] = (uint8_t)c;
    }

    for (int i = 0; i <= buf_i; ++i)
    {
        send_uint8_msg_to_peer(pr_dtls, file_contents_buf[i], MSG_SIZE + 1);
    }

    std::cout << "(send_file) File " << fname << " sent successfully." << '\n';
}

/**
 * Handles Incoming file sharing requests
 */
void handle_file_req(P2PServerDetails *srv_dtls, PeerDetails *pr_dtls)
{
    char peer_msg_buf[MSG_SIZE];
    std::string peer_msg;
    std::fstream req_fs;
    std::string req_fname;
    unsigned int req_fparts = 0;
    std::string req_fkewords;

    while (pr_dtls && pr_dtls->connected)
    {
        memset(peer_msg_buf, 0, sizeof(peer_msg_buf));
        int server_sock_recv_count = read(pr_dtls->sock_fd, peer_msg_buf, sizeof(peer_msg_buf));
        if (server_sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(READ_MSG_SLEEP_TIME);
        }
        else if (server_sock_recv_count < 0)
        {
            std::cerr << "(handle_file_req) Socket receive error." << '\n';
            break;
        }
        else
        {
            peer_msg = get_str(peer_msg_buf);
            std::cout << "(handle_file_req) " << peer_msg << " message received from peer " << pr_dtls->dc_id << '\n';
            std::vector<std::string> peer_msg_parts = split_str(peer_msg, '_');
            std::string response_msg;

            if (peer_msg_parts[0] == std::string(FILE_REQ_MSG))
            {
                if (peer_msg_parts.size() != 2)
                {
                    response_msg = std::string(DISCONNECT_MSG);
                    send_msg_to_peer(pr_dtls, response_msg);
                    break;
                }

                req_fname = peer_msg_parts[1];
                req_fs.open(req_fname, std::fstream::in);
                if (req_fs.is_open())
                {
                    response_msg = join_str({std::string(FILE_RESP_MSG), std::string(RESP_YES)}, '_');
                    send_msg_to_peer(pr_dtls, response_msg);
                }
                else
                {
                    std::cerr << "(handle_file_req) Unable to read file " << req_fname << ". Cancelling.\n";
                    response_msg = join_str({std::string(FILE_RESP_MSG), std::string(RESP_NO)}, '_');
                    send_msg_to_peer(pr_dtls, response_msg);
                    break;
                }
            }
            else if (peer_msg_parts[0] == std::string(FILE_CONTENTS_REQ_MSG))
            {
                char c;
                int char_counter = 0;
                req_fs.unsetf(std::ios_base::skipws); // To not skip whitespaces

                // Check if file size > MAX_FILE_SIZE
                bool max_fsize = false;
                while (req_fs >> c)
                {
                    char_counter++;
                    if (char_counter > MAX_FILE_SIZE)
                    {
                        max_fsize = true;
                        std::cerr << "(send_file) File larger than max file size " << MAX_CLIENTS << "B. Cancelling.\n";
                        std::string response_msg = std::string(DISCONNECT_MSG);
                        send_msg_to_peer(pr_dtls, response_msg);
                        break;
                    }
                }
                if (max_fsize)
                {
                    break;
                }

                if (char_counter == MAX_FILE_SIZE)
                {
                    req_fparts = char_counter / MSG_SIZE;
                }
                else
                {
                    req_fparts = (char_counter / MSG_SIZE) + 1;
                }

                // Seek fstream back to the beginning so that fstream can be used to actually send the file.
                req_fs.clear();
                req_fs.seekg(0, std::ios::beg);

                {
                    std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
                    // Send expected number of parts in this file and the keywords in the file
                    response_msg = join_str({std::string(FILE_CONTENTS_RESP_MSG), std::to_string(req_fparts), srv_dtls->searchable_f_index[req_fname]}, '_');
                }
                send_msg_to_peer(pr_dtls, response_msg);

                // Sleep for some time before sending actual file contents so that the peer can read expected number of parts.
                usleep(req_fparts * READ_MSG_SLEEP_TIME);

                send_file(pr_dtls, req_fs, req_fname, req_fparts);
            }
            else if (peer_msg == std::string(DISCONNECT_MSG))
            {
                pr_dtls->connected = false;
                break;
            }
            else
            {
                std::cerr << "(handle_file_req) Invalid message.\n";
                break;
            }
        }
    }

    if (req_fs.is_open())
    {
        req_fs.close();
    }
    shutdown(pr_dtls->sock_fd, SHUT_RDWR);
    delete pr_dtls;
    std::cout << "(handle_file_req) File transfer complete.\n";
}

/**
 * Runs on a different thread, start listening for incoming P2P file connections only.
 */
void accept_p2p_file_connections(P2PServerDetails *srv_dtls)
{
    socklen_t server_addrlen = sizeof(struct sockaddr_in);
    while (!srv_dtls->shut_down)
    {
        // blocking Call to accept incoming socket connections.
        int peer_sockfd = accept(srv_dtls->file_sharing_sockfd, (struct sockaddr *)srv_dtls->file_sharing_server_addr, &server_addrlen);
        if (peer_sockfd < 0)
        {
            break;
        }
        else
        {
            std::cout << "(accept_p2p_file_connections) New peer connected for file sharing." << '\n';
            PeerDetails *pr_dtls = new PeerDetails();
            pr_dtls->sock_fd = peer_sockfd;
            pr_dtls->connected = true;
            std::thread *peer_file_sharing_thread = new std::thread(handle_file_req, srv_dtls, pr_dtls);
        }
    }

    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
        close(srv_dtls->file_sharing_sockfd);
        delete srv_dtls->file_sharing_server_addr;
        std::cout << "(accept_p2p_file_connections) Closed socket for incoming P2P file connections." << '\n';
    }
}

/**
 * Used to request a file @param req_file from @param peer_dc_id
 */
void request_file_download(P2PServerDetails *srv_dtls, std::string peer_dc_id, std::string req_filename)
{
    PeerDetails *pr_dtls = new PeerDetails();
    pr_dtls->dc_id = peer_dc_id;
    connect_to_peer(pr_dtls, FILE_SRVR_PORT);

    std::string file_req_msg_str = join_str({std::string(FILE_REQ_MSG), req_filename}, '_');
    send_msg_to_peer(pr_dtls, file_req_msg_str);

    char peer_msg_buf[MSG_SIZE];
    std::string peer_msg;
    std::vector<std::string> peer_msg_parts;
    uint8_t req_fparts = 0;
    std::string req_fkeywords;

    while (pr_dtls && pr_dtls->connected)
    {
        memset(peer_msg_buf, 0, sizeof(peer_msg_buf));
        int server_sock_recv_count = read(pr_dtls->sock_fd, peer_msg_buf, sizeof(peer_msg_buf));
        if (server_sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(READ_MSG_SLEEP_TIME);
        }
        else if (server_sock_recv_count < 0)
        {
            std::cerr << "(request_file_download) Socket receive error." << '\n';
            break;
        }
        else
        {
            peer_msg = get_str(peer_msg_buf);
            peer_msg_parts = split_str(peer_msg, '_');
            std::cout << "(request_file_download) " << peer_msg << " message received from peer " << pr_dtls->dc_id << '\n';
            if (peer_msg == std::string(DISCONNECT_MSG))
            {
                pr_dtls->connected = false;
                close(pr_dtls->sock_fd);
                delete pr_dtls;
                return;
            }
            else if (peer_msg_parts[0] == std::string(FILE_RESP_MSG) && peer_msg_parts.size() == 2 && peer_msg_parts[1] == std::string(RESP_NO))
            {
                std::cerr << "(request_file_download) File tranfer failure." << '\n';
                pr_dtls->connected = false;
                close(pr_dtls->sock_fd);
                delete pr_dtls;
                return;
            }
            else if (peer_msg_parts[0] == std::string(FILE_RESP_MSG) && peer_msg_parts.size() == 2 && peer_msg_parts[1] == std::string(RESP_YES))
            {
                std::cout << "(request_file_download) File found at peer " << pr_dtls->dc_id << ". Requesting file download.\n";

                // Request file contents
                std::string f_contents_req_msg = join_str({std::string(FILE_CONTENTS_REQ_MSG), req_filename}, '_');
                send_msg_to_peer(pr_dtls, f_contents_req_msg);
            }
            else if (peer_msg_parts[0] == std::string(FILE_CONTENTS_RESP_MSG) && peer_msg_parts.size() == 3)
            {
                req_fparts = parse_int_str(peer_msg_parts[1]);
                req_fkeywords = peer_msg_parts[2];
                break;
            }
            else
            {
                std::cerr << "(request_file_download) Invalid message.\n";
                pr_dtls->connected = false;
                close(pr_dtls->sock_fd);
                delete pr_dtls;
                return;
            }
        }
    }

    std::fstream req_fs;
    req_fs.open(req_filename, std::fstream::out);
    if (!req_fs.is_open())
    {
        std::cerr << "(request_file_download) Couldn't write to file " << req_filename << ". Cancelling.\n";
        send_msg_to_peer(pr_dtls, std::string(DISCONNECT_MSG));
        pr_dtls->connected = false;
        close(pr_dtls->sock_fd);
        delete pr_dtls;
        return;
    }

    // Now receive file contents
    uint8_t file_msg_buf[MSG_SIZE + 1];
    uint8_t file_contents_buf[req_fparts][MSG_SIZE]; // similar to normal message but 1 extra byte to represent sequence number
    memset(file_contents_buf, 0, sizeof(file_contents_buf));

    while (pr_dtls && pr_dtls->connected)
    {
        memset(file_msg_buf, 0, sizeof(file_msg_buf));
        int server_sock_recv_count = read(pr_dtls->sock_fd, file_msg_buf, sizeof(file_msg_buf));
        if (server_sock_recv_count == 0)
        {
            // Sleep and check again later for messages
            usleep(READ_MSG_SLEEP_TIME);
        }
        else if (server_sock_recv_count < 0)
        {
            std::cerr << "(request_file_download) Socket receive error." << '\n';
            break;
        }
        else
        {
            uint8_t seq_no = file_msg_buf[0];
            int i = 1;
            if (seq_no > 0 && seq_no <= req_fparts)
            {
                while (i <= MSG_SIZE)
                {
                    if (file_msg_buf[i] == 0)
                    {
                        break;
                    }
                    else
                    {
                        file_contents_buf[seq_no - 1][i - 1] = file_msg_buf[i];
                        i++;
                    }
                }
            }
            else
            {
                std::cerr << "(request_file_download) Invalid message." << '\n';
                break;
            }

            // Check if all seq_nos have been filled
            bool all_parts_found = true;
            for (int i = 0; i < req_fparts; ++i)
            {
                all_parts_found &= (file_contents_buf[i][0] != 0);
                if (!all_parts_found)
                {
                    break;
                }
            }
            if (all_parts_found)
            {
                break;
            }
        }
    }

    for (int i = 0; i < req_fparts; ++i)
    {
        for (int j = 0; j < MSG_SIZE; ++j)
        {
            if (file_contents_buf[i][j] == 0)
            {
                break;
            }
            else
            {
                req_fs.write((char *)&file_contents_buf[i][j], 1);
            }
        }
    }
    if (req_fs.is_open())
    {
        req_fs.close();
    }

    // Cleanup after file request complete
    std::cout << "(request_file_download) File " << req_filename << " downloaded successsfully from " << pr_dtls->dc_id << '\n';
    std::string index_update = join_str({req_filename, req_fkeywords}, ':');
    {
        std::lock_guard<std::mutex> src_lock(srv_dtls->mtx);
        srv_dtls->searchable_f_index[req_filename] = req_fkeywords;
        std::vector<std::string> keywords = split_str(req_fkeywords, ',');
        for (std::string kw : keywords)
        {
            srv_dtls->searchable_kw_index[kw] = req_filename;
        }

        std::string idx_fname = "f" + std::to_string(srv_dtls->c_id) + ".dat";
        srv_dtls->idx_fs.open(idx_fname, std::fstream::out | std::fstream::app);
        if (srv_dtls->idx_fs.is_open())
        {
            srv_dtls->idx_fs << '\n';
            srv_dtls->idx_fs << index_update;
            srv_dtls->idx_fs.close();
            std::cout << "(request_file_download) Index file " << idx_fname << " updated successsfully with index " << index_update << '\n';
        }
        else
        {
            std::cout << "(request_file_download) Index file update failed. \n";
        }
    }

    shutdown(pr_dtls->sock_fd, SHUT_RDWR);
    delete pr_dtls;
}