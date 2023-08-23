#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <set>

#define SRVR_PORT 5050
#define FILE_SRVR_PORT 5051
#define MAX_CLIENTS 15
#define MSG_SIZE 256
#define MAX_FILE_SIZE 1024 // max file size we can handle 1024B = 1KB

#define READ_MSG_SLEEP_TIME 1000                  // 1000 microseconds = 1 millisecond
#define MONITOR_CONNECTED_PEERS_SLEEP_TIME 100000 // 10^5 microseconds = 100 milliseconds

// P2P Connection messages
#define CONNECT_MSG "CONNECT_"
#define DISCONNECT_MSG "DISCONNECT"
#define DISCONNECT_ADDNBRS_MSG "DISCONNECT_ADDNBRS_"

// P2P Search messages
#define SEARCH_F "F"
#define SEARCH_KW "KW"
#define SEARCH_REQ_MSG "SEARCH-REQ"
#define SEARCH_RESP_MSG "SEARCH-RESP"

// P2P File messages
#define FILE_REQ_MSG "FILE-REQ"
#define FILE_RESP_MSG "FILE-RESP"
#define FILE_CONTENTS_REQ_MSG "FILE-CONTENTS-REQ"
#define FILE_CONTENTS_RESP_MSG "FILE-CONTENTS-RESP"

// Response messages
#define SEARCH_RESP_PROCESSED "PROCESSED"
#define RESP_YES "YES"
#define RESP_NO "NO"

#define SEARCH_ACTIVE 1
#define SEARCH_FAILED 2
#define SEARCH_SUCCESS 3

/**
 * Contains details of peers, this P2P server is connected to.
 */
typedef struct PeerDetails_st PeerDetails;
struct PeerDetails_st
{
    /**
     * Mutex to ensure proper synchronization accross thread accesses
     */
    std::mutex mtx;

    /**
     * c_id is the C_i id where this peer is running. Necessarily between 1 though 15.
     */
    unsigned int c_id;

    /**
     * dc_id is the dc server id where this peer is running.
     * dc_id is always of the form dcXY.
     */
    std::string dc_id;

    /**
     * Connected peer's socket connection file descriptor.
     */
    int sock_fd;

    /**
     * Thread pointer where the listening to this peer is happening
     */
    std::thread *listening_thread;

    /**
     * Denotes if this peer is connected.
     */
    bool connected = false;
};

/**
 * Contains details for this P2P server.
 */
typedef struct P2PServerDetails_st P2PServerDetails;
struct P2PServerDetails_st
{
    /**
     * Mutex to ensure proper synchronization accross thread accesses
     */
    std::mutex mtx;

    /**
     * Denotes if this P2P server is about to shut down.
     */
    bool shut_down = false;

    /**
     * Denotes C_i id in the P2P system. If this P2P server has c_id say C2, then it will have access to F2.
     */
    unsigned int c_id;

    /**
     * dc_id is the dc server id where this peer is running.
     * dc_id is always of the form dcXY.
     * Self dc_id is fetched using gethostname.
     */
    std::string dc_id;

    /**
     * Denotes the fstream object for the index file, which contains information
     * about other files it has access to, and their associated keywords.
     */
    std::fstream idx_fs;

    /**
     * Index created from Index file F_i.
     * Map of keyword -> File
     */
    std::map<std::string, std::string> searchable_kw_index;

    /**
     * Map of filenames which are shareable on the P2P system.
     * Map of file -> keywords(string from Fi) 
     */
    std::map<std::string, std::string> searchable_f_index;

    /**
     * Socket connection file descriptor on which this P2P server is listening for new incoming P2P connections.
     */
    unsigned int listening_sockfd;

    /**
     * Socket connection address info on which this P2P server is listening for new incoming P2P connections.
     */
    struct sockaddr_in *listening_server_addr;

    /**
     * Socket connection file descriptor on which this P2P server is listening for new incoming P2P file sharing connections.
     */
    unsigned int file_sharing_sockfd;

    /**
     * Socket connection address info on which this P2P server is listening for new incoming P2P file sharing connections.
     */
    struct sockaddr_in *file_sharing_server_addr;

    /**
     * List of Peers, this P2P server is connected to.
     * This is the adjacency list of each node in the P2P newtwork graph.
     */
    std::vector<PeerDetails *> connected_peers;

    /**
     * This Peer's search sequence counter to uniquely/sequentially identify searches initiated by this peer.
     */
    unsigned int search_counter = 0;

    /**
     * Map for details about created searches
     * Each search_id contains a tuple of <search_status, search_success, search_replies>
     */
    std::map<std::string, std::pair<int, std::vector<std::string>>> created_searches;

    /**
     * Map for details about forwarded searches
     * Each search_id contains a pair of <replies_received_so_far, replies_needed_from_downstream_peers, search_replies>
     */
    std::map<std::string, std::tuple<int, int, PeerDetails *, std::vector<std::string>>> forwarded_searches;
};

// Utils
std::string get_str(char *char_arr);
int parse_int_str(std::string num);
std::vector<std::string> split_str(std::string str, char delim);
std::string join_str(std::vector<std::string> str_list, char delim);
std::string get_dc_id();
std::string get_connect_msg(unsigned int c_id, std::string dc_id);
std::string get_search_id(std::string dc_id, unsigned int search_counter);
std::string get_search_req_msg(std::string search_id, unsigned int hop_count, bool keyword_search, std::string search_word);
std::string get_search_resp_msg(std::string search_id, bool resp_yes, std::string arg);

// Validations
void print_incorrect_usage();
bool validate_args(int argc, char *argv[], unsigned int &c_id, std::vector<std::string> &peers);
bool connected(std::vector<PeerDetails *> &connected_peers, std::string dc_id);

// P2P Server functions
void add_neighbors(P2PServerDetails *srv_dtls, std::string new_neighbors);
void connect_to_peer(PeerDetails* pr_dtls, unsigned int server_port);

// Simple Search
void simple_search(P2PServerDetails *srv_dtls, std::string search_word, bool keyword_search);
void send_msg_to_peer(PeerDetails *pr_dtls, std::string msg);
void listen_to_peer(P2PServerDetails *srv_dtls, PeerDetails *pr_dtls);

// File Sharing
void request_file_download(P2PServerDetails* srv_dtls, std::string peer_dc_id, std::string req_filename);
void accept_p2p_file_connections(P2PServerDetails *srv_dtls);

#endif /* COMMON_H */