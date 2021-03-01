#pragma once

#include "duktape.h"

#include <atomic>
#include <optional>
#include <stdint.h>
#include <thread>

namespace DNice {
    void registerUDPServerApi(duk_context* ctx);

    class UDPServer {
    public:
        UDPServer(uint16_t port);

        void start();

    private:
        uint16_t _port;
        bool _running;
        std::thread _thread;
    };
}
