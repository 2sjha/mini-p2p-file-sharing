#include <iostream>
#include <unistd.h>
#include "common.h"

#define SRVR_PORT 5050

/**
 * Converts C-style char array to C++ string.
 *
 * Used mostly to process arguments passed.
 */
std::string get_str(char *char_arr)
{
    return std::string(char_arr);
}

/**
 * Parse integer from @param[in] num string using stoi.
 */
int parse_int_str(std::string num)
{
    int i = -1;
    try
    {
        i = std::stoi(num);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "(parse_int) Can't parse: " << num << " to int. Exception: " << ex.what() << '\n';
    }

    return i;
}

/**
 * Splits the @param str into a list of token string with the delimiter @param delim,
 * and @returns a list of token strings.
 */
std::vector<std::string> split_str(std::string str, char delim)
{
    std::vector<std::string> tokens;
    std::string parsed_token;
    for (char c : str)
    {
        if (c == delim)
        {
            tokens.push_back(parsed_token);
            parsed_token.clear();
        }
        else
        {
            parsed_token.push_back(c);
        }
    }

    tokens.push_back(parsed_token);
    parsed_token.clear();

    return tokens;
}

/**
 * Joins the tokes in @param str_list into a strvwith the delimiter @param delim,
 * and @returns a concatenated string.
 */
std::string join_str(std::vector<std::string> str_list, char delim)
{
    std::string res = "";
    int i = 0;
    for (; i < str_list.size() - 1; ++i)
    {
        res += str_list[i];
        res.push_back(delim);
    }
    res += str_list[i];

    return res;
}

/**
 * Uses gethostname to fetch local hostname
 */
std::string get_dc_id()
{
    char hostname[256];
    std::string dc_id = "";

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        dc_id = get_str(hostname).substr(0, 4);
    }
    else
    {
        std::cerr << "(get_dc_id) Error fetching hostname." << std::endl;
    }

    return dc_id;
}

/**
 * Formats a connect message with @param c_id & @param dc_id
 */
std::string get_connect_msg(unsigned int c_id, std::string dc_id)
{
    std::string connect_msg = std::string(CONNECT_MSG);
    connect_msg.push_back('C');
    connect_msg += std::to_string(c_id);
    connect_msg.push_back('_');
    connect_msg += dc_id;

    return connect_msg;
}

/**
 * Creates a search_id from @param dc_id and @param search_counter
 * Search_id = dcXY-1
 */
std::string get_search_id(std::string dc_id, unsigned int search_counter)
{
    std::string search_id = dc_id;
    search_id.push_back('-');
    search_id += std::to_string(search_counter);

    return search_id;
}

/**
 * Formats a search message with @param dc_id, @param search_counter, @param hop_count, @param search_word
 * Search msg = SEARCH-REQ_dcXY-1_2_F_a.txt or SEARCH-REQ_dcXY_1_2_KW_apple
 */
std::string get_search_req_msg(std::string search_id, unsigned int hop_count, bool keyword_search, std::string search_word)
{
    std::vector<std::string> tokens;
    tokens.push_back(std::string(SEARCH_REQ_MSG));
    tokens.push_back(search_id);
    tokens.push_back(std::to_string(hop_count));
    if (keyword_search)
    {
        tokens.push_back(std::string(SEARCH_KW));
    }
    else
    {
        tokens.push_back(std::string(SEARCH_F));
    }
    tokens.push_back(search_word);

    return join_str(tokens, '_');
}

/**
 * Formats a search response message with @param search_id, @param peer_dc_id, @param resp_yes, @param arg
 * Search Response messages:
 * SEARCH-RESP_dcXY-1_NO                            when peer doesnt have the file but it cant forward the request or incase when its a consolidated response but no downstream peer has the file
 * SEARCH-RESP_dcXY-1_NO_PROCESSING                 when peer has already received the request from some other peer
 * SEARCH-RESP_dcXY-1_YES_dcAB-fname                when its a direct response if the peer has the file
 * SEARCH-RESP_dcXY-1_YES_dcAB-fname,dcEF-fname,..  when its a consolidated response to an upstream peer from a downstream peer who doesn't have the file
 */
std::string get_search_resp_msg(std::string search_id, bool resp_yes, std::string arg)
{
    std::vector<std::string> tokens;
    tokens.push_back(std::string(SEARCH_RESP_MSG));
    tokens.push_back(search_id);
    if (resp_yes)
    {
        tokens.push_back(std::string(RESP_YES));
    }
    else
    {
        tokens.push_back(std::string(RESP_NO));
    }

    if (!arg.empty())
    {
        tokens.push_back(arg);
    }

    return join_str(tokens, '_');
}