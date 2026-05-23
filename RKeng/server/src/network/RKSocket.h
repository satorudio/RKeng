#pragma once
// RKSocket.h — минимальная обёртка над Winsock2 / POSIX UDP.
// Заменяет enet. Когда напишешь свой протокол — замени только этот файл.

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#  include <ws2tcpip.h>
   using RKSock = SOCKET;
   static constexpr RKSock RK_INVALID_SOCK = INVALID_SOCKET;
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <fcntl.h>
   using RKSock = int;
   static constexpr RKSock RK_INVALID_SOCK = -1;
#endif

#include <cstdint>
#include <cstring>

namespace RKeng::Net
{
    struct Addr
    {
        uint32_t ip   = 0;   // host byte order
        uint16_t port = 0;
    };

    // Инициализация сетевого стека (нужна только на Windows)
    inline bool Init()
    {
#ifdef _WIN32
        WSADATA wsa{};
        return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
#else
        return true;
#endif
    }

    inline void Shutdown()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    inline void Close(RKSock s)
    {
#ifdef _WIN32
        closesocket(s);
#else
        ::close(s);
#endif
    }

    // Создать UDP-сокет, привязать к порту. Возвращает RK_INVALID_SOCK при ошибке.
    inline RKSock CreateUDP(uint16_t port)
    {
        RKSock s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == RK_INVALID_SOCK) return s;

        // Неблокирующий режим
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(s, FIONBIO, &mode);
#else
        int flags = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port        = htons(port);
        if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        {
            Close(s);
            return RK_INVALID_SOCK;
        }
        return s;
    }

    // Отправить пакет. Возвращает байт отправлено или -1.
    inline int SendTo(RKSock s, const void* data, int size, uint32_t ip, uint16_t port)
    {
        sockaddr_in dst{};
        dst.sin_family      = AF_INET;
        dst.sin_addr.s_addr = htonl(ip);
        dst.sin_port        = htons(port);
        return static_cast<int>(
            ::sendto(s, reinterpret_cast<const char*>(data), size, 0,
                     reinterpret_cast<sockaddr*>(&dst), sizeof(dst)));
    }

    // Принять пакет. Возвращает байт получено, 0 если нет данных, -1 ошибка.
    inline int RecvFrom(RKSock s, void* buf, int maxSize, Addr& from)
    {
        sockaddr_in src{};
#ifdef _WIN32
        int srcLen = sizeof(src);
#else
        socklen_t srcLen = sizeof(src);
#endif
        int r = static_cast<int>(
            ::recvfrom(s, reinterpret_cast<char*>(buf), maxSize, 0,
                       reinterpret_cast<sockaddr*>(&src), &srcLen));
#ifdef _WIN32
        if (r == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) return 0;
            return -1;
        }
#else
        if (r < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
            return -1;
        }
#endif
        from.ip   = ntohl(src.sin_addr.s_addr);
        from.port = ntohs(src.sin_port);
        return r;
    }
}
