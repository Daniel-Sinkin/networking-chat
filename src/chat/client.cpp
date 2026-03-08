#include <iostream>

#include "client.hpp"
#include "net.hpp"

int main() {
    using namespace ds_net;
    ClientSocket client{*get_ipv4_tcp_connect_info("localhost", k_default_port)};
    while (true) {
        std::string s;
        std::cin >> s;
        client.send_message(s);
    }
}