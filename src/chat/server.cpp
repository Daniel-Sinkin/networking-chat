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

    server.accept_connection();
    log.logln("Client connected!");
    while(true) {
        try{
            const auto s_res = server.receive_string();
            if(!s_res) break;
            Logger::get_logger("server").logln(std::format("Received Message from Client connection:\n{}", *s_res));
        } catch (...) {
            Logger::get_logger("server").logln("Failed to receive string, breaking.");
        }
    }
}