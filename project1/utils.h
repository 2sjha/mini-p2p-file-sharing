#ifndef UTILS_H
#define UTILS_H

#include <iostream>

#define SRVR_PORT 5050
#define FSIZE 300
#define FMSGSIZE 102

std::string get_str(char *char_arr);
std::string get_str(char *char_arr)
{
    return std::string(char_arr);
}

char *get_char_arr(std::string str);
char *get_char_arr(std::string str)
{
    char *str_arr = (char *)calloc(str.length() + 1, sizeof(char));

    if (str_arr == nullptr)
    {
        std::cerr << "(get_char_arr) Cant allocate memory for str: " << str << '\n';
        exit(1);
    }

    int i = 0;
    while (i < str.length())
    {
        str_arr[i] = str[i];
        i++;
    }
    str_arr[i] = '\0';

    return str_arr;
}

int parse_int_char_arr(char *num);
int parse_int_char_arr(char *num)
{
    int i = -1;
    std::size_t pos{};
    try
    {
        i = std::stoi(num, &pos);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "(parse_int) Can't parse: " << num << " to int. Exception: " << ex.what() << '\n';
    }

    return i;
}

int parse_int_str(std::string num);
int parse_int_str(std::string num)
{
    char *num_arr = get_char_arr(num);
    int res = parse_int_char_arr(num_arr);
    free(num_arr);
    return res;
}

unsigned int len_server_f_msg(uint8_t *server_f_msg)
{
    unsigned int len = 0;
    if (!server_f_msg)
    {
        return len;
    }

    while (len < FMSGSIZE && *(server_f_msg + len) != '\0')
    {
        len++;
    }
    return len + 1;
}

#endif /* UTILS_H */