#include <random>
#include <thread>
#include <chrono>
#include <vector>
#include "client.hpp"
#include "net.hpp"

int main() {
    using namespace ds_net;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> base_dist(100, 999);
    const auto base = base_dist(gen) * 1000;

    std::vector<std::jthread> threads;
    for (int i = 0; i < 25; ++i) {
        threads.emplace_back([base, i] {
            std::mt19937 local_gen(std::random_device{}());
            std::uniform_int_distribution<int> sleep_dist(100, 700);
            const auto id = base + i + 1;

            ClientSocket client{*get_ipv4_tcp_connect_info("localhost", k_default_port)};
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dist(local_gen)));
                client.send_message(std::to_string(id));
            }
        });
    }
}