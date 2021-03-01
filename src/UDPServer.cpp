#include "UDPServer.h"

#include "DNS.h"
#include "util.h"
#include "generated/udpServer.h"

#include <iostream>
#include <string>
#include <vector>

// IMPROVE: Build a POSIX version
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")

#define BUFLEN 512

namespace DNice {
    static std::vector<UDPServer> g_servers;

    UDPServer::UDPServer(uint16_t port):
        _port(port),
        _running(false) { }

    void UDPServer::start() {
        if (_running) {
            std::cerr << "Attempted to start already-running UDPServer." << std::endl;
            exit(-1);
        }

        _thread = std::thread([this]() {
            SOCKET s;
            struct sockaddr_in server, si_other;
            int slen , recv_len;
            uint8_t buf[BUFLEN];
            WSADATA wsa;

            slen = sizeof(si_other);

            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
                std::cerr << "Socket creation on port " << _port << "failed with error " << WSAGetLastError() << std::endl;
                exit(-1);
            }

            //Create a socket
            if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
                std::cerr << "Socket creation on port " << _port << "failed with error " << WSAGetLastError() << std::endl;
                exit(-1);
            }

            server.sin_family = AF_INET;
            server.sin_addr.s_addr = INADDR_ANY;
            server.sin_port = htons(_port);

            if (bind(s, (struct sockaddr*)(&server), sizeof(server)) == SOCKET_ERROR) {
                std::cerr << "Socket bind on port " << _port << "failed with error " << WSAGetLastError() << std::endl;
                exit(-1);
            }

            while(true) {
                memset(buf,'\0', BUFLEN);

                if ((recv_len = recvfrom(s, (char*)buf, BUFLEN, 0, (struct sockaddr*)(&si_other), &slen)) == SOCKET_ERROR) {
                    std::cerr << "Socket recieve on port " << _port << "failed with error " << WSAGetLastError() << std::endl;
                    exit(-1);
                }

                std::string parseError;
                Packet packet;
                parseDnsPacket(std::vector<uint8_t>(
                    std::begin(buf),
                    std::begin(buf) + recv_len
                ), packet, parseError);

                if (packet.questions.size() > 0 && !packet.questions[0].label.isPointer) {
                    std::cout << "looking up " << packet.questions[0].label.domainName << std::endl;
                } else {
                    std::cout << "huh?" << std::endl;
                }

                if (sendto(s, (char*)buf, recv_len, 0, (struct sockaddr*)(&si_other), slen) == SOCKET_ERROR) {
                    std::cerr << "Socket send on port " << _port << "failed with error " << WSAGetLastError() << std::endl;
                    exit(-1);
                }
            }

            closesocket(s);
            WSACleanup();
        });

        _running = true;
    }

    duk_ret_t udpserver_create_native(duk_context* ctx) {
        if (g_servers.size() >= MAX_SAFE_INT) {
            // IMPROVE: Central logging system
            std::cerr << "Out of js-safe addressible space for UDPServer." << std::endl;
            exit(-1);
        }

        const auto port = duk_get_int(ctx, -1);

        g_servers.push_back(UDPServer(port));

        duk_push_int(ctx, (duk_int_t)(g_servers.size() - 1));

        return 1;
    }

    duk_ret_t udpserver_start_native(duk_context* ctx) {
        const auto index = duk_get_int(ctx, -1);

        if (g_servers.size() <= index) {
            duk_push_string(ctx, ("No valid UDPServer at index " + std::to_string(index)).c_str());
        } else {
           g_servers[index].start();
            duk_push_undefined(ctx);
        }

        return 1;
    }

    void registerUDPServerApi(duk_context* ctx) {
        duk_push_c_function(ctx, udpserver_create_native, DUK_VARARGS);
        duk_put_global_string(ctx, "udpserver_create_native");

        duk_push_c_function(ctx, udpserver_start_native, DUK_VARARGS);
        duk_put_global_string(ctx, "udpserver_start_native");

        duk_eval_string(ctx, UDP_SERVER_JS);
    }
}
