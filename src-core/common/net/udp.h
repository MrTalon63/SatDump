#pragma once

#include <string.h>
#include <stdexcept>

#if defined(_WIN32)
#include <stdio.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifndef _WIN32
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

namespace net
{
    class UDPClient
    {
    private:
        sockaddr_in sock_addr;
#if defined(_WIN32)
        WSADATA wsa;
#endif
        SOCKET sock = INVALID_SOCKET;

    public:
        UDPClient(char *address, int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
                throw std::runtime_error("Couldn't open UDP socket!");

#if defined(_WIN32)
            // Disable the WSAECONNRESET behavior on ICMP Port Unreachable
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
            BOOL bNewBehavior = FALSE;
            DWORD dwBytesReturned = 0;
            WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_port = htons(port);

#if defined(_WIN32)
            sock_addr.sin_addr.S_un.S_addr = inet_addr(address);
#else
            if (inet_aton(address, &sock_addr.sin_addr) == 0)
                throw std::runtime_error("Couldn't connect to UDP socket!");
#endif
        }

        UDPClient(const UDPClient&) = delete;
        UDPClient& operator=(const UDPClient&) = delete;

        ~UDPClient()
        {
            if (sock != INVALID_SOCKET)
            {
#if defined(_WIN32)
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            }
        }

        int send(uint8_t *data, int len)
        {
            int slen = sizeof(sockaddr);
            int r = sendto(sock, (char *)data, len, 0, (sockaddr *)&sock_addr, slen);
            return r;
        }

        int recv(uint8_t *data, int len)
        {
#if defined(_WIN32)
            int slen = sizeof(sockaddr);
#else
            socklen_t slen = sizeof(sockaddr);
#endif
            int r = recvfrom(sock, (char *)data, len, 0, (struct sockaddr *)&sock_addr, &slen);
            if (r == -1)
                throw std::runtime_error("Error receiving from UDP socket!");
            return r;
        }
    };

    class UDPServer
    {
    private:
        sockaddr_in sock_addr;
#if defined(_WIN32)
        WSADATA wsa;
#endif
        SOCKET sock = INVALID_SOCKET;

    public:
        UDPServer(int port)
        {
#if defined(_WIN32)
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
                throw std::runtime_error("Couldn't startup WSA socket!");
#endif

            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
                throw std::runtime_error("Couldn't open UDP socket!");

#if defined(_WIN32)
            // Disable the WSAECONNRESET behavior on ICMP Port Unreachable
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
            BOOL bNewBehavior = FALSE;
            DWORD dwBytesReturned = 0;
            WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

            memset((char *)&sock_addr, 0, sizeof(sock_addr));
            sock_addr.sin_family = AF_INET;
            sock_addr.sin_addr.s_addr = INADDR_ANY;
            sock_addr.sin_port = htons(port);

#if defined(_WIN32)
            sock_addr.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
            if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
                throw std::runtime_error("Couldn't connect to UDP socket!");
            int ttrue = 1;
            setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ttrue, sizeof(int));
        }

        UDPServer(const UDPServer&) = delete;
        UDPServer& operator=(const UDPServer&) = delete;

        ~UDPServer()
        {
            if (sock != INVALID_SOCKET)
            {
#if defined(_WIN32)
            closesocket(sock);
            WSACleanup();
#else
            shutdown(sock, SHUT_RDWR);
            close(sock);
#endif
            }
        }

        int send(uint8_t *data, int len)
        {
            int slen = sizeof(sockaddr);
            int r = sendto(sock, (char *)data, len, 0, (sockaddr *)&sock_addr, slen);
            return r;
        }

        int recv(uint8_t *data, int len)
        {
#if defined(_WIN32)
            int slen = sizeof(sockaddr);
#else
            socklen_t slen = sizeof(sockaddr);
#endif
            int r = recvfrom(sock, (char *)data, len, 0, (struct sockaddr *)&sock_addr, &slen);
            if (r == -1)
            {
#if defined(_WIN32)
                int err = WSAGetLastError();
                if (err != WSAEINTR && err != WSAEWOULDBLOCK && err != WSAECONNRESET)
#endif
                throw std::runtime_error("Error receiving from UDP socket!");
            }
            return r;
        }
    };
}
