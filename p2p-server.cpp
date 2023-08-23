#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

// Socket Headers
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h" // Prototypes for functions used accross cpp files

/**
 * @returns random index 0 through peers_list_size - 1.
 * Used to randomly choose peer from passed peers list.
 */
unsigned int choose_random_peer(unsigned int peers_list_size)
{
    if (peers_list_size == 1)
    {
        return 0;
    }

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1, peers_list_size);

    return dist(rng) - 1;
}

/**
 * Reads file from fstream @param idx_fs and creates index of keywords and filenames in @param index.
 * Also adds searchable filenames to @param search_fnames
 * Each line in the index file must of the form: <fname>:<kw1>,<kw2>,<kw3>,...
 */
void create_f_kw_index(std::map<std::string, std::string> &kw_index, std::map<std::string, std::string> &f_index, std::fstream &idx_fs)
{
    std::string line;
    std::vector<std::string> fname_kws;
    std::vector<std::string> keywords;
    while (getline(idx_fs, line))
    {
        fname_kws = split_str(line, ':');
        f_index[fname_kws[0]] = fname_kws[1];
        keywords = split_str(fname_kws[1], ',');
        for (std::string kw : keywords)
        {
            kw_index[kw] = fname_kws[0];
        }
    }
}

/**
 * Prints system info from the @param srv_dtls object
 */
void print_peer_details(P2PServerDetails *srv_dtls)
{
    std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
    std::cout << "(print_peer_details) System C_i id: " << srv_dtls->c_id << '\n';
    std::cout << "(print_peer_details) System dc id: " << srv_dtls->dc_id << '\n';
    std::cout << "(print_peer_details) System shut_down status: " << srv_dtls->shut_down << '\n';
    if (srv_dtls->searchable_f_index.empty())
    {
        std::cout << "(print_peer_details) No files in F_i searchable filenames." << '\n';
    }
    else
    {
        std::cout << "(print_peer_details) System F_i filenames: ";
        for (auto fname_kws : srv_dtls->searchable_f_index)
        {
            std::cout << fname_kws.first << " ";
        }
        std::cout << '\n';
    }

    if (srv_dtls->searchable_kw_index.empty())
    {
        std::cout << "(print_peer_details) No file keywords in F_i searchable index." << '\n';
    }
    else
    {
        std::cout << "(print_peer_details) System F_i searchable index: ";
        for (auto iter : srv_dtls->searchable_kw_index)
        {
            std::cout << iter.first << "-" << iter.second << ' ';
        }
        std::cout << '\n';
    }

    if (srv_dtls->connected_peers.empty())
    {

        std::cout << "(print_peer_details) No peers connected." << '\n';
    }
    else
    {
        std::cout << "(print_peer_details) Connected Peers: ";
        for (PeerDetails *pr : srv_dtls->connected_peers)
        {
            if (pr)
            {
                std::lock_guard<std::mutex> pr_lock(pr->mtx);
                std::cout << 'C' << pr->c_id << "-" << pr->dc_id << ' ';
            }
        }
        std::cout << '\n';
    }
}

/**
 * Closes this P2P server fstream, clears search index files.
 * Marks this P2P server's shut_down flag as true.
 *
 * After srv_dtls->shut_down is marked true,
 * P2P listening socket will be closed in that thread.
 * All the P2P peer connections will also be terminated in their respective threads.
 */
void terminate_server(P2PServerDetails *srv_dtls)
{
    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);

        std::cout << "(terminate_server) Server shut down started." << '\n';

        srv_dtls->searchable_kw_index.clear();
        srv_dtls->searchable_f_index.clear();
        srv_dtls->shut_down = true;
        shutdown(srv_dtls->listening_sockfd, SHUT_RD);
        shutdown(srv_dtls->file_sharing_sockfd, SHUT_RD);
    }
}

/**
 * Used during shut_down to close connection with peers.
 * Shuts down socket & frees memory as well.
 */
void disconnect_peer(PeerDetails *pr, std::string disconnect_msg)
{
    if (!pr)
    {
        std::cout << "(disconnect_peer) Peer already disconnected." << '\n';
        return;
    }

    if (pr->connected)
    {
        send_msg_to_peer(pr, disconnect_msg);
        pr->connected = false;
        shutdown(pr->sock_fd, SHUT_RDWR);
    }
    if (pr->listening_thread)
    {
        pr->listening_thread->join();
        delete pr->listening_thread;
    }
    delete pr;
}

/**
 * Called when shutting down the server to forcibly close connection with peers.
 * If the peer connection hasn't been closed until now, then send DISCONNECT, close connection and free peer memory.
 */
void disconnect_all_peers(P2PServerDetails *srv_dtls)
{
    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);

        PeerDetails *pr;
        if (srv_dtls->connected_peers.empty())
        {
            std::cout << "(disconnect_all_peers) No Peers to disconnect." << '\n';
        }
        else if (srv_dtls->connected_peers.size() == 1)
        {
            std::cout << "(disconnect_all_peers) Only 1 peer to disconnect." << '\n';
            pr = srv_dtls->connected_peers[0];
            if (pr)
            {
                disconnect_peer(pr, std::string(DISCONNECT_MSG));
                srv_dtls->connected_peers[0] = nullptr;
                srv_dtls->connected_peers.clear();
            }
        }
        else
        {
            // Randomly choose one of the neighbors
            unsigned int chosen_neighbor_idx = choose_random_peer(srv_dtls->connected_peers.size());
            pr = srv_dtls->connected_peers[chosen_neighbor_idx];

            // List of neighbors' dc_id other than the chosen neighbor
            std::vector<std::string> other_neighbors_list;
            for (int i = 0; i < srv_dtls->connected_peers.size(); ++i)
            {
                if (i != chosen_neighbor_idx)
                {
                    other_neighbors_list.push_back(srv_dtls->connected_peers[i]->dc_id);
                }
            }

            // Send DISCONNECT_ADDNBRS_dcXY,dcXY to chosen peer
            std::string disconnect_msg = std::string(DISCONNECT_ADDNBRS_MSG);
            disconnect_msg += join_str(other_neighbors_list, ',');
            disconnect_peer(pr, disconnect_msg);
            srv_dtls->connected_peers[chosen_neighbor_idx] = nullptr;
            srv_dtls->connected_peers.erase(srv_dtls->connected_peers.begin() + chosen_neighbor_idx);

            // Send DISCONNECT to Peers other than the chosen peer.
            for (int i = 0; i < srv_dtls->connected_peers.size(); ++i)
            {
                pr = srv_dtls->connected_peers[i];
                if (pr)
                {
                    disconnect_peer(pr, std::string(DISCONNECT_MSG));
                    srv_dtls->connected_peers[i] = nullptr;
                }
            }
            srv_dtls->connected_peers.clear();
        }
    }
}

/**
 * Initiates and sets up socket server to listen for incoming connections.
 * @returns socket file descriptor on which this P2P server can start accepting incoming P2P connections.
 */
void setup_p2p_listening_socket(P2PServerDetails *srv_dtls, bool file_server)
{
    // Server socket vars
    unsigned int server_sockfd = 0;
    struct sockaddr_in *server_addr = new sockaddr_in;
    socklen_t server_addrlen = sizeof(struct sockaddr_in);

    // Init socket server file descriptor
    int set_option = 1;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&set_option, sizeof(set_option));
    if (server_sockfd < 0)
    {
        std::cerr << "(setup_p2p_listening_socket) Socket creation error." << '\n';
        exit(1);
    }

    // Init server_addr instance
    memset(server_addr, '\0', server_addrlen);
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;

    if (file_server)
    {
        server_addr->sin_port = htons(FILE_SRVR_PORT);
    }
    else
    {
        server_addr->sin_port = htons(SRVR_PORT);
    }

    int server_sock_status = bind(server_sockfd, (struct sockaddr *)server_addr, server_addrlen);
    if (server_sock_status < 0)
    {
        std::cerr << "(setup_p2p_listening_socket) Socket connection error." << '\n';
        exit(1);
    }

    // Set up listening on the socket
    server_sock_status = listen(server_sockfd, MAX_CLIENTS);
    if (server_sock_status < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if (file_server)
    {
        srv_dtls->file_sharing_sockfd = server_sockfd;
        srv_dtls->file_sharing_server_addr = server_addr;
        std::cout << "(setup_p2p_listening_socket) Socker server for file sharing ready on port " << FILE_SRVR_PORT << "." << '\n';
    }
    else
    {
        srv_dtls->listening_sockfd = server_sockfd;
        srv_dtls->listening_server_addr = server_addr;
        std::cout << "(setup_p2p_listening_socket) Socker server ready on port " << SRVR_PORT << "." << '\n';
    }
}

/**
 * Runs on a different thread to check for peers that have sent DISCONNECT message.
 * The peers' listening threads should have exitedgracefully by now, since their connected is false.
 * Once in a while, check for disconnected threads and perform clean up.
 */
void monitor_connected_peers(P2PServerDetails *srv_dtls)
{
    while (srv_dtls && !srv_dtls->shut_down)
    {
        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
            PeerDetails *pr_dtls;
            for (int i = 0; i < srv_dtls->connected_peers.size(); ++i)
            {
                // No need to acquire pr_dtls lock
                // As we're only modifying disconnected peers & they're not being accessed by any thread anyway
                pr_dtls = srv_dtls->connected_peers[i];
                if (pr_dtls && !pr_dtls->connected)
                {
                    if (pr_dtls->listening_thread)
                    {
                        pr_dtls->listening_thread->join();
                        delete pr_dtls->listening_thread;
                    }
                    delete pr_dtls;
                    srv_dtls->connected_peers[i] = nullptr;
                }
            }

            // Remove all nullptrs from the vector
            srv_dtls->connected_peers.erase(std::remove_if(srv_dtls->connected_peers.begin(), srv_dtls->connected_peers.end(), [](PeerDetails *ptr)
                                                           { return ptr == nullptr; }),
                                            srv_dtls->connected_peers.end());
        } // mutex will be released here when lock goes out of scope

        // Sleep, then do same thing again
        usleep(MONITOR_CONNECTED_PEERS_SLEEP_TIME);
    }
}

/**
 * Reolves hostname from @param pr_dtls->dc_id and establishes a connection with that P2P server.
 * If connection successful, Keep listening on that socket until this P2P server shuts down or that P2P peer disconnects.
 * If connection unsuccessful, Prints error message.
 */
void connect_to_peer(PeerDetails *pr_dtls, unsigned int server_port)
{
    std::string pr_hostname = pr_dtls->dc_id;
    pr_hostname += ".utdallas.edu";

    // Resolve hostname -> IP address and connect
    int sfd, addrinfo_res;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    /* Obtain address(es) matching host/port. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    addrinfo_res = getaddrinfo(pr_hostname.c_str(), std::to_string(server_port).c_str(), &hints, &result);

    if (addrinfo_res != 0)
    {
        std::cerr << "(connect_to_peer) getaddrinfo: " << gai_strerror(addrinfo_res) << '\n';
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
            pr_dtls->sock_fd = sfd;
            pr_dtls->connected = true;
            std::cout << "(connect_to_peer) Socket connection established with " << pr_hostname << " on port " << server_port << '\n';
            break;
        }

        close(sfd);
    }

    freeaddrinfo(result);

    /* No address succeeded */
    if (rp == nullptr || !pr_dtls->connected)
    {
        std::cerr << "(connect_to_peer) Socket connection error. Couldn't connect to " << pr_hostname << '\n';
        return;
    }
}

/**
 * Called when a peer disconnects.
 * The disconnecting peer sends a list of neighbors to be added when that peer connection is dropped,
 * so that this peer can use the disconnecting peer's neighbors, thus avoiding network partioning.
 */
void add_neighbors(P2PServerDetails *srv_dtls, std::string new_neighbors)
{
    std::vector<std::string> neighbors_list = split_str(new_neighbors, ',');

    for (std::string neighbor : neighbors_list)
    {
        {
            std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
            // Check if this neighbor sent from disconnecting peer is already a neighbor to this peer
            if (connected(srv_dtls->connected_peers, neighbor))
            {
                std::cout << "(add_neighbors) Already connected to peer " << neighbor << ".\n";
                continue;
            }
        }

        PeerDetails *pr_dtls = new PeerDetails();
        pr_dtls->dc_id = neighbor;

        connect_to_peer(pr_dtls, SRVR_PORT);
        if (pr_dtls->connected)
        {
            // Send First Message after Connect
            send_msg_to_peer(pr_dtls, get_connect_msg(srv_dtls->c_id, srv_dtls->dc_id));
            {
                std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
                srv_dtls->connected_peers.push_back(pr_dtls);
            }

            std::thread *peer_listening_thread = new std::thread(listen_to_peer, srv_dtls, pr_dtls);
            pr_dtls->listening_thread = peer_listening_thread;
        }
        else
        {
            delete pr_dtls;
        }
    }
}

/**
 * Runs on a different thread, start listening for incoming P2P peer connections.
 * If a peer connection is accepted, its peer_sockfd is pushed to connected peers.
 * which can be used to recv & send for p2p messaging.
 */
void accept_p2p_incoming_connections(P2PServerDetails *srv_dtls)
{
    socklen_t server_addrlen = sizeof(struct sockaddr_in);
    while (!srv_dtls->shut_down)
    {
        // blocking Call to accept incoming socket connections.
        int peer_sockfd = accept(srv_dtls->listening_sockfd, (struct sockaddr *)srv_dtls->listening_server_addr, &server_addrlen);
        if (peer_sockfd < 0)
        {
            break;
        }
        else
        {
            std::cout << "(accept_p2p_incoming_connections) New peer connected." << '\n';
            PeerDetails *pr_dtls = new PeerDetails();
            pr_dtls->connected = true;
            pr_dtls->sock_fd = peer_sockfd;
            {
                std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
                srv_dtls->connected_peers.push_back(pr_dtls);
            }

            std::thread *peer_listening_thread = new std::thread(listen_to_peer, srv_dtls, pr_dtls);
            {
                std::lock_guard<std::mutex> pr_lock(pr_dtls->mtx);
                pr_dtls->listening_thread = peer_listening_thread;
            }
        }
    }

    {
        std::lock_guard<std::mutex> srv_lock(srv_dtls->mtx);
        close(srv_dtls->listening_sockfd);
        delete srv_dtls->listening_server_addr;
        std::cout << "(accept_p2p_incoming_connections) Closed socket for incoming P2P connections." << '\n';
    }
}

int main(int argc, char *argv[])
{

    bool first_peer = false;
    std::vector<std::string> peers;
    unsigned int c_id;
    if (argc == 1 || argc == 2)
    {
        if (!validate_args(argc, argv, c_id, peers))
        {
            print_incorrect_usage();
            exit(1);
        }

        std::cout << "Are you sure this is the first peer in the p2p system?" << '\n';
        std::cout << "Press Y to continue or anything else to quit." << '\t';
        char y;
        std::cin >> y;
        if (y = 'y' || y == 'Y')
        {
            first_peer = true;
        }
        else
        {
            std::cout << "Exiting." << '\n';
            exit(0);
        }
    }
    else if (argc == 3)
    {
        if (!validate_args(argc, argv, c_id, peers))
        {
            print_incorrect_usage();
            exit(1);
        }
    }
    else
    {
        print_incorrect_usage();
    }

    P2PServerDetails *srv_dtls = new P2PServerDetails();
    srv_dtls->c_id = c_id;
    srv_dtls->dc_id = get_dc_id();

    // Open F_i index file
    std::string idx_fname = "f" + std::to_string(c_id) + ".dat";
    srv_dtls->idx_fs.open(idx_fname, std::fstream::in);
    if (!srv_dtls->idx_fs.is_open())
    {
        std::cerr << "(open_file) Couldn't open file: " << idx_fname << ". Exiting." << '\n';
        exit(1);
    }
    create_f_kw_index(srv_dtls->searchable_kw_index, srv_dtls->searchable_f_index, srv_dtls->idx_fs);
    // Close idx_fs after creating index
    if (srv_dtls->idx_fs.is_open())
    {
        srv_dtls->idx_fs.close();
    }

    // Setup Socket servers on 2 different ports,
    // 1 for P2P connections and Simple Search
    // 2nd only for file sharing.
    setup_p2p_listening_socket(srv_dtls, false);
    setup_p2p_listening_socket(srv_dtls, true);

    // Set up Listening socket on another thread
    // This function handles incoming p2p connections for simple search.
    std::thread p2p_connections_thread(accept_p2p_incoming_connections, srv_dtls);

    // Set up File sharing socket on another thread
    // Handles file sharing requests
    std::thread p2p_file_sharing_thread(accept_p2p_file_connections, srv_dtls);

    // Set up a daemon which periodically checks which peers have disconnected themselves
    // Does cleanup for disconnected peerss
    std::thread connected_peers_monitoring_thread(monitor_connected_peers, srv_dtls);

    // If this is not the first peer, then connect to this peer and add to adjacency list
    if (!first_peer)
    {
        // Randomly choose one of the peers
        unsigned int rng_peer_idx = choose_random_peer(peers.size());

        PeerDetails *pr_dtls = new PeerDetails();
        pr_dtls->dc_id = peers[rng_peer_idx];

        connect_to_peer(pr_dtls, SRVR_PORT);
        if (pr_dtls->connected)
        {
            // Send First Message after Connect: CONNECT_Ci_dcXY
            send_msg_to_peer(pr_dtls, get_connect_msg(srv_dtls->c_id, srv_dtls->dc_id));
            std::thread *peer_listening_thread = new std::thread(listen_to_peer, srv_dtls, pr_dtls);
            pr_dtls->listening_thread = peer_listening_thread;
            srv_dtls->connected_peers.push_back(pr_dtls);
        }
        else
        {
            std::cerr << "Couldn't establish connection with first peer " << pr_dtls->dc_id << ". Exiting. \n";
            exit(1);
        }
    }

    unsigned int cmd;
    std::string search_word;
    std::string cmd_input;
    while (!srv_dtls->shut_down)
    {
        std::cout << "Press 0 to shut down this server." << '\n';
        std::cout << "Press 1 to search for a file using FILENAME in the P2P system." << '\n';
        std::cout << "Press 2 to search for a file using KEYWORD in the P2P system." << '\n';
        std::cout << "Press 3 for P2P peer details." << '\n';

        std::cin >> cmd_input;
        cmd = parse_int_str(cmd_input);
        if (cmd == 0)
        {
            terminate_server(srv_dtls);
        }
        else if (cmd == 1)
        {
            std::cout << "Enter FILENAME to search." << '\n';
            std::cin >> search_word;
            simple_search(srv_dtls, search_word, false);
            std::cout << '\n';
        }
        else if (cmd == 2)
        {
            std::cout << "Enter KEYWORD to search." << '\n';
            std::cin >> search_word;
            simple_search(srv_dtls, search_word, true);
            std::cout << '\n';
        }
        else if (cmd == 3)
        {
            print_peer_details(srv_dtls);
            std::cout << '\n';
        }
        else
        {
            std::cerr << "Invalid command." << '\n';
        }
    }

    connected_peers_monitoring_thread.join();
    p2p_connections_thread.join();
    p2p_file_sharing_thread.join();
    disconnect_all_peers(srv_dtls);
    std::cout << "Shut Down complete." << '\n';
    return 0;
}