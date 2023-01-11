#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>

typedef std::function<bool (int, std::string&)> process_message_t;

class TCPServer {
private:
    int fd;
    process_message_t process_message;

    std::vector<int> connections;
    std::vector<std::string> messages;
    std::vector<uint32_t> message_lengths;

    void set_flag(int flag);
    sockaddr_in create_address(uint16_t port);
public:
    TCPServer(std::function<bool (int, std::string)> _process_message);

    void start_listening(uint16_t port);
    void stop_listening();

    void accept_connections();
    void read_messages();

    void wait_for_event();
};

#endif // TCP_SERVER_H