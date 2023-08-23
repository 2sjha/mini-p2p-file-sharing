#include <iostream>
#include <vector>
#include "common.h"

/**
 * Prints Incorrect Usage message.
 *
 * Used whenever arguments processing fails.
 */
void print_incorrect_usage()
{
    std::cerr << "Incorrect Usage." << '\n';
    std::cerr << "For first Peer. Usage is ./p2p-server or ./p2p-server --id=<id>" << '\n';
    std::cerr << "For Next Peers. Usage is ./p2p-server --id=<id> --peers=<dcXY1>,<dcXY2>,<dcXY3>,..." << '\n';
}

/**
 * Validates a peer. Peer must be of the form dcXY.
 */
bool validate_peer(std::string parsed_peer)
{
    if (parsed_peer.length() != 4 || parsed_peer[0] != 'd' || parsed_peer[1] != 'c')
    {
        std::cerr << "Invalid peer: " << parsed_peer << '\n';
        return false;
    }

    int dc_id = parse_int_str(parsed_peer.substr(2));
    if (dc_id < 1 || dc_id > 45)
    {
        std::cerr << "Invalid peer dc_id: " << dc_id << ". Must be between 01 through 45." << '\n';
        return false;
    }

    return true;
}

/**
 * Validates c_id passed as argument. Puts parsed c_id into @param c_id. c_id arg must be an integer from 1 through 15.
 */
bool validate_args_c_id(char *arg, unsigned int &c_id)
{
    std::string arg1 = get_str(arg);

    if (arg1.length() > 7 || arg1.length() < 5 || arg1.substr(0, 5) != "--id=")
    {
        return false;
    }

    int parsed_id = parse_int_str(arg1.substr(5));
    if (parsed_id <= 0 || parsed_id > 15)
    {
        std::cerr << "Invalid id: " << parsed_id << ". Must be between 1 through 15." << '\n';
        return false;
    }

    c_id = parsed_id;
    return true;
}

/**
 * Validates peers list passed as argument. Puts parsed peers into @param peers.
 *
 * Uses @see validate_peer() to validate each individual peer.  @returns false, if any peer validation fails.
 */
bool validate_args_peers(char *arg, std::vector<std::string> &peers)
{
    std::string arg1 = get_str(arg);
    if (arg1.length() < 7 || arg1.substr(0, 8) != "--peers=")
    {
        return false;
    }

    std::vector<std::string> peers_list = split_str(arg1.substr(8), ',');

    for (std::string peer : peers_list)
    {
        if (!validate_peer(peer))
        {
            return false;
        }
        peers.push_back(peer);
    }

    return true;
}

/**
 * Validates arguments passed during init.
 * Uses @see validate_args_peers() and @see validate_args_c_id.
 */
bool validate_args(int argc, char *argv[], unsigned int &c_id, std::vector<std::string> &peers)
{
    if (argc == 1)
    {
        c_id = 1;
        return true;
    }
    else if (argc == 2)
    {
        return validate_args_c_id(argv[1], c_id);
    }
    else if (argc == 3)
    {
        return (validate_args_c_id(argv[1], c_id) || validate_args_c_id(argv[2], c_id)) && (validate_args_peers(argv[1], peers) || validate_args_peers(argv[2], peers));
    }
    else
    {
        return false;
    }
}

/**
 * Linear check if @param connected_peers contains @paramdc_id
 */
bool connected(std::vector<PeerDetails *> &connected_peers, std::string dc_id)
{
    for (PeerDetails *pr : connected_peers)
    {
        if (pr && pr->dc_id == dc_id)
        {
            return true;
        }
    }

    return false;
}