#pragma once

#include "duktape.h"

#include <stdint.h>

namespace DNice {
    void registerUDPServerApi(duk_context* ctx);

    class UDPServer {
    public:
        UDPServer(uint16_t port);

    private:
        uint16_t _port;
    };
}
