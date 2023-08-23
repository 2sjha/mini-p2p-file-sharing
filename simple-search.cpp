#include <iostream>
#include <string>
#include <map>

// Socket Headers
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h" // Prototypes for functions used accross cpp files

/**
 * Used to send messages to peers.
 * @param msg is sent on @param pr_dtls's scoket fd.
 */
void send_msg_to_peer(PeerDetails *pr_dtls, std::string msg)
{
    if (!pr_dtls || !pr_dtls->connected)
    {
        std::cerr << "(send_msg_to_peer) Can't send msg to peer. Peer is disconnected.\n";
        return;
    }

    int server_sock_send_count = send(pr_dtls->sock_fd, msg.c_str(), msg.length(), 0);
    if (server_sock_send_count < 0)
    {
        std::cerr << "(send_msg_to_peer) Socket send error." << '\n';
    }
    std::cout << "(send_msg_to_peer) " << msg << " message sent to peer " << pr_dtls->dc_id << ".\n";
}

/**
 * Forwards @param fwd_search_req_msg to all connected peers in @param srv_dtls except the peer @param pr_recvd
 */
void forward_search_req_messages(P2PServerDetails *srv_dtls, PeerDetails *pr_recvd, std::string search_id, std::string fwd_search_req_msg)
{
    std::vector<PeerDetails *> fwd_peers;
    for (PeerDetails *pr : srv_dtls->connected_peers)
    {
        if (pr != pr_recvd)
        {
            fwd_peers.push_back(pr);
        }
    }

    // File not found at this peer and noo peers to forward this message to.
    // So, send NO back to upstream pr_recvd peer
    if (fwd_peers.empty())
    {
        std::cout << "(forward_search_req_messages) No peers to forward search " << search_id << "." << '\n';
        std::string search_resp_msg = get_search_resp_msg(search_id, false, "");
        send_msg_to_peer(pr_recvd, search_resp_msg);
        return;
    }

    for (PeerDetails *pr : fwd_peers)
    {
        send_msg_to_peer(pr, fwd_search_req_msg);
    }
}

/**
 * Performs message parsing and performs action according.
 */
void handle_peer_msg(P2PServerDetails *srv_dtls, PeerDetails *pr_dtls, std::string peer_msg)
{
    std::cout << "(handle_peer_msg) " << peer_msg << " message received from peer " << pr_dtls->dc_id << '\n';
    if (peer_msg == std::string(DISCONNECT_MSG))
    {
        {
            std::lock_guard<std::mutex> pr_lock(pr_dtls->mtx);
            pr_dtls->connected = false;
        }
    }
    else if (peer_msg.find(DISCONNECT_ADDNBRS_MSG) != std::string::npos)
    {
        {
            std::lock_guard<std::mutex> pr_lock(pr_dtls->mtx);
            pr_dtls->connected = false;
        }

        add_neighbors(srv_dtls, peer_msg.substr(strlen(DISCONNECT_ADDNBRS_MSG)));
    }
    else if (peer_msg.find(CONNECT_MSG) != std::string::npos)
    {
        std::vector<std::string> c_dc_id = split_str(peer_msg.substr(strlen(CONNECT_MSG)), '_');
        {
            std::lock_guard<std::mutex> pr_lock(pr_dtls->mtx);
            pr_dtls->c_id = parse_int_str(c_dc_id[0].substr(1));
            pr_dtls->dc_id = c_dc_id[1];
        }
    }
    else if (peer_msg.find(SEARCH_REQ_MSG) != std::string::npos)
    {
        // Search req message received will be like SEARCH-REQ_dcXY-1_2_F_a.txt or SEARCH-REQ_dcXY-1_2_KW_apple
        // Check if this peer has already received this message using search_id
        std::vector<std::string> msg_parts = split_str(peer_msg, '_');
        std::string search_id = msg_parts[1];

        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
            if (!srv_dtls->forwarded_searches.count(search_id))
            {
                int downstream_replies_needed = srv_dtls->connected_peers.size() - 1;
                srv_dtls->forwarded_searches[search_id] = std::make_tuple(0, downstream_replies_needed, pr_dtls, std::vector<std::string>());
            }
            else
            {
                // If this search Id is already being processed, then it means that this peer
                // must have received this search req message from some other peer before this req
                // Dont process this req, but send reply to this peer that it is already being processed
                // Because that peer will expect a certain number of replies back so that it can reply back to upstream
                std::string search_resp_msg = get_search_resp_msg(search_id, false, std::string(SEARCH_RESP_PROCESSED));
                send_msg_to_peer(pr_dtls, search_resp_msg);
                return;
            }

            unsigned int hop_count = parse_int_str(msg_parts[2]);
            bool keyword_search = (msg_parts[3] == std::string(SEARCH_KW));
            std::string search_word = msg_parts[4];
            std::string fname;

            // Check if this peer can satisfy this search req
            if (keyword_search)
            {
                // Keyword found so respond YES
                if (srv_dtls->searchable_kw_index.count(search_word))
                {
                    fname = srv_dtls->searchable_kw_index[search_word];
                    std::string search_resp_msg = get_search_resp_msg(search_id, true, join_str({srv_dtls->dc_id, fname}, '-'));
                    send_msg_to_peer(pr_dtls, search_resp_msg);
                    return;
                }
                else
                {
                    // decrease hop count since req message received was from senders perspective
                    // messages must be forwarded with decreased hopcount
                    hop_count--;

                    // Forward this to connected peers only if hop_count > 0.
                    // If hop count <= 0 means that this is the farthest peer this request must reach, thus this req msg must not be forwarded
                    // But if that peer doesnt have the file, then send reply NO without forwarding
                    if (hop_count > 0)
                    {
                        std::string fwd_search_req_msg = get_search_req_msg(search_id, hop_count, keyword_search, search_word);
                        forward_search_req_messages(srv_dtls, pr_dtls, search_id, fwd_search_req_msg);
                    }
                    else
                    {
                        std::string search_resp_msg = get_search_resp_msg(search_id, false, "");
                        send_msg_to_peer(pr_dtls, search_resp_msg);
                    }
                }
            }
            else
            {
                // Filename found so respond YES
                if (srv_dtls->searchable_f_index.count(search_word))
                {
                    fname = search_word;
                    std::string search_resp_msg = get_search_resp_msg(search_id, true, join_str({srv_dtls->dc_id, fname}, '-'));
                    send_msg_to_peer(pr_dtls, search_resp_msg);
                    return;
                }
                else
                {
                    // decrease hop count since req message received was from senders perspective
                    // messages must be forwarded with decreased hopcount
                    hop_count--;

                    // Forward this to connected peers only if hop_count > 0, which means that this is the farthest peer this request can go to.
                    // Reply NO if this peer doesnt have the file
                    if (hop_count > 0)
                    {
                        std::string fwd_search_req_msg = get_search_req_msg(search_id, hop_count, keyword_search, search_word);
                        forward_search_req_messages(srv_dtls, pr_dtls, search_id, fwd_search_req_msg);
                    }
                    else
                    {
                        std::string search_resp_msg = get_search_resp_msg(search_id, false, "");
                        send_msg_to_peer(pr_dtls, search_resp_msg);
                    }
                }
            }
        }
    }
    else if (peer_msg.find(SEARCH_RESP_MSG) != std::string::npos)
    {
        // Search response message received will be like
        // SEARCH-RESP_dcXY-1_NO or SEARCH-RESP_dcXY-1_NO_PROCESSED
        // or SEARCH-RESP_dcXY-1_YES_dcAB-fname or SEARCH-RESP_dcXY-1_YES_dcAB-fname,dcEF-fname,dcGH-fname
        std::vector<std::string> msg_parts = split_str(peer_msg, '_');
        std::string search_id = msg_parts[1];

        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);

            // To differentiate between response for Created searches and Forwarded searches.
            // We check the search_id in both these maps.
            if (srv_dtls->forwarded_searches.count(search_id))
            {
                // Increase replies received so far
                std::get<0>(srv_dtls->forwarded_searches[search_id])++;
                std::get<3>(srv_dtls->forwarded_searches[search_id]).push_back(peer_msg);

                // If received all downstream replies then filter YES peers and respond back to upstream
                if (std::get<0>(srv_dtls->forwarded_searches[search_id]) == std::get<1>(srv_dtls->forwarded_searches[search_id]))
                {
                    PeerDetails *upstream_pr = std::get<2>(srv_dtls->forwarded_searches[search_id]);
                    std::vector<std::string> downstream_responses = std::get<3>(srv_dtls->forwarded_searches[search_id]);
                    std::vector<std::string> downstream_response_parts;
                    std::vector<std::string> downstream_yes_responses;
                    bool file_found_downstream = false;
                    bool file_found = false;

                    // send consolidated message to upstream peer
                    for (std::string downstream_response : downstream_responses)
                    {
                        downstream_response_parts = split_str(downstream_response, '_');
                        file_found = (downstream_response_parts[2] == std::string(RESP_YES));
                        file_found_downstream |= file_found;
                        if (file_found)
                        {
                            downstream_yes_responses.push_back(downstream_response_parts[3]);
                        }
                    }

                    std::string upstream_resp_msg;
                    if (file_found_downstream)
                    {
                        std::string yes_responses = join_str(downstream_yes_responses, ',');
                        upstream_resp_msg = get_search_resp_msg(search_id, file_found_downstream, yes_responses);
                        send_msg_to_peer(upstream_pr, upstream_resp_msg);
                    }
                    else
                    {
                        upstream_resp_msg = get_search_resp_msg(search_id, file_found_downstream, "");
                        send_msg_to_peer(upstream_pr, upstream_resp_msg);
                    }
                }
            }
            else if (srv_dtls->created_searches.count(search_id))
            {
                // The simple_search thread would have marked a search FAILED if the timer expires.
                // So responses received after the timer expiry must be ignored.
                bool search_response_success = (msg_parts[2] == std::string(RESP_YES));
                if (srv_dtls->created_searches[search_id].first == SEARCH_ACTIVE)
                {
                    srv_dtls->created_searches[search_id].second.push_back(peer_msg);
                    if (search_response_success)
                    {
                        std::get<0>(srv_dtls->created_searches[search_id]) = SEARCH_SUCCESS;
                    }
                }
                else if (srv_dtls->created_searches[search_id].first == SEARCH_SUCCESS)
                {
                    srv_dtls->created_searches[search_id].second.push_back(peer_msg);
                }
                else
                {
                    std::cout << "(simple_search) Response received for " << search_id << " after timer expiry.\n";
                }
            }
            else
            {
                std::cerr << "(handle_peer_msg) Invalid response message from peer " << pr_dtls->dc_id << '\n';
            }
        }
    }
    else
    {
        std::cerr << "(handle_peer_msg) Invalid message from peer " << pr_dtls->dc_id << '\n';
    }
}

/**
 * This thread is spawned for each peer this server is connected to.
 * The peer details is passed in @param pr_dtls when the thread is spawned for this thread.
 * If a message is read on the peer's socket, then uses @see handle_peer_msg to msg handling.
 */
void listen_to_peer(P2PServerDetails *srv_dtls, PeerDetails *pr_dtls)
{
    char peer_msg_buf[MSG_SIZE];
    std::string peer_msg;

    while (!srv_dtls->shut_down && pr_dtls && pr_dtls->connected)
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
            std::cerr << "(listen_to_peer) Socket receive error." << '\n';
            break;
        }
        else
        {
            peer_msg = get_str(peer_msg_buf);
            handle_peer_msg(srv_dtls, pr_dtls, peer_msg);
        }
    }

    std::cout << "(listen_to_peer) Stopped listening to peer " << pr_dtls->dc_id << ".\n";
    close(pr_dtls->sock_fd);
}

/**
 * Called when the user on this peer initiates a search for a file or keyword.
 */
void simple_search(P2PServerDetails *srv_dtls, std::string search_word, bool keyword_search)
{
    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
        srv_dtls->search_counter++;

        // Check is File/keyword already on this server.
        if (keyword_search)
        {
            if (srv_dtls->searchable_kw_index.count(search_word))
            {
                std::cout << "(simple_search) File " << srv_dtls->searchable_kw_index[search_word]
                          << " already present for keyword " << search_word << " on this P2P peer.\n";

                // Handle already existing filename. lol.
                // Generate random file if not present already.

                return;
            }
        }
        else
        {
            if (srv_dtls->searchable_f_index.count(search_word))
            {
                std::cout << "(simple_search) File " << search_word << " already present on this P2P peer.\n";

                // Handle already existing filename. lol.
                // Generate random file if not present already.

                return;
            }
        }
    }

    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
        if (srv_dtls->connected_peers.empty())
        {
            std::cout << "(simple_search) No peer connected. Search failed.\n";
            return;
        }
    }

    unsigned int hop_count = 1;
    std::string search_msg;
    std::string search_id;
    while (hop_count <= 16)
    {
        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
            if (keyword_search)
            {
                std::cout << "(simple_search) Search " << srv_dtls->search_counter << " initiated on peer "
                          << srv_dtls->dc_id << " for keyword " << search_word << " with hop count " << hop_count << '\n';
            }
            else
            {
                std::cout << "(simple_search) Search " << srv_dtls->search_counter << " initiated on peer "
                          << srv_dtls->dc_id << " for file " << search_word << " with hop count " << hop_count << '\n';
            }

            search_id = get_search_id(srv_dtls->dc_id, srv_dtls->search_counter);
            srv_dtls->created_searches[search_id] = std::make_pair(SEARCH_ACTIVE, std::vector<std::string>());
            search_msg = get_search_req_msg(search_id, hop_count, keyword_search, search_word);

            for (PeerDetails *pr : srv_dtls->connected_peers)
            {
                send_msg_to_peer(pr, search_msg);
            }
        }

        // this thread sleeps for hop_count seconds
        // responses will be received on the listening thread
        sleep(hop_count);

        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
            // after this thread wakes up it checks if the listening thread has marked this search_id complete or failed
            if (std::get<0>(srv_dtls->created_searches[search_id]) == SEARCH_SUCCESS)
            {
                break;
            }
            else if (std::get<0>(srv_dtls->created_searches[search_id]) == SEARCH_ACTIVE)
            {
                std::get<0>(srv_dtls->created_searches[search_id]) = SEARCH_FAILED;
                std::cout << "(simple_search) Search " << srv_dtls->search_counter << " timed out." << '\n';

                hop_count *= 2;
                srv_dtls->search_counter++;
            }
        }
    }

    // Seach complete
    // Present download options if success
    if (std::get<0>(srv_dtls->created_searches[search_id]) == SEARCH_SUCCESS)
    {
        std::cout << "(simple_search) Search completed successfully.\n";
        std::vector<std::string> downstream_yes_responses;

        // Read all responses received for this search-id and show options which dc servers said yes. 
        for (std::string downstream_response : srv_dtls->created_searches[search_id].second)
        {
            std::vector<std::string> downstream_response_parts = split_str(downstream_response, '_');
            if (downstream_response_parts.size() == 4)
            {
                bool file_found = (downstream_response_parts[2] == std::string(RESP_YES));
                if (file_found)
                {
                    std::vector<std::string> responses = split_str(downstream_response_parts[3], ',');
                    for (std::string res : responses)
                    {
                        downstream_yes_responses.push_back(res);
                    }
                }
            }
        }

        if (keyword_search)
        {
            std::cout << "(simple_search) Responses for keyword " << search_word << " after search completion: " << '\n';
        }
        else
        {
            std::cout << "(simple_search) Responses for filename " << search_word << " after search completion: " << '\n';
        }

        for (int i = 1; i <= downstream_yes_responses.size(); ++i)
        {
            std::cout << i << " " << downstream_yes_responses[i - 1] << '\n';
        }

        std::string file_option_str;
        int file_option;
        for (int i = 0; i < 3; ++i)
        {
            std::cout << "Choose which peer & file to download from " << '\n';
            std::cin >> file_option_str;
            file_option = parse_int_str(file_option_str);
            if (file_option < 1 || file_option > downstream_yes_responses.size())
            {
                std::cout << "Invalid choice. Try again." << '\n';
            }
            else
            {
                std::vector<std::string> dc_id_fname = split_str(downstream_yes_responses[file_option - 1], '-');
                request_file_download(srv_dtls, dc_id_fname[0], dc_id_fname[1]);
                break;
            }
        }
    }
    else if (std::get<0>(srv_dtls->created_searches[search_id]) == SEARCH_FAILED || std::get<0>(srv_dtls->created_searches[search_id]) == SEARCH_ACTIVE)
    {
        if (keyword_search)
        {
            std::cout << "(simple_search) Search failed for keyword " << search_word << '\n';
        }
        else
        {
            std::cout << "(simple_search) Search failed for filename " << search_word << '\n';
        }
    }
}