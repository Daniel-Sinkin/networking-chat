#include "server.hpp"

using namespace ds_net;

int main() {
    const auto ai = get_ipv4_tcp_address_info(k_default_port);
    if (!ai) {
        std::cerr << "Failed to get address info\n";
        return 1;
    }
    ai->print();

    ServerSocket server{*ai};
    auto& log = Logger::get_logger("server");
    log.logln("Listening on port " + std::to_string(k_default_port));

    EventPoller poller{};
    poller.add(server.fd());
    while(true) {
        const auto events = poller.wait();
        for(auto& ev : events) {
            const auto fd = ev.data.fd;
            if(fd == server.fd()) {
                const auto new_fd = server.accept_connection();
                log.logln(std::format("Got new connection '{}'", new_fd));
                poller.add(new_fd);
            } else {
                auto& conn = server.get_connection(fd);
                if (const auto res = conn.receive_data(); res) {
                    std::string msg{reinterpret_cast<const char*>(res->data()), res->size()};
                    log.logln(std::format("Got data from connection '{}':\n{}", fd, msg));
                } else {
                    log.logln(std::format("Connection '{}' disconnected.", fd));
                }
            }
        }
    }
}