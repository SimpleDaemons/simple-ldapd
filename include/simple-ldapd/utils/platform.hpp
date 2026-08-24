/**
 * @file platform.hpp
 * @brief Platform types and LDAP constants
 * @author SimpleDaemons
 * @copyright 2026 SimpleDaemons
 * @license Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#ifndef SIMPLE_LDAPD_WINDOWS
#define SIMPLE_LDAPD_WINDOWS
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#if defined(__APPLE__)
#define SIMPLE_LDAPD_MACOS
#elif defined(__FreeBSD__)
#define SIMPLE_LDAPD_FREEBSD
#elif defined(__linux__)
#define SIMPLE_LDAPD_LINUX
#elif defined(__OpenBSD__) || defined(__NetBSD__)
#define SIMPLE_LDAPD_BSD
#else
#error "Unsupported platform"
#endif
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace simple_ldapd {

#ifdef SIMPLE_LDAPD_WINDOWS
using socket_t = SOCKET;
#define SOCKET_ERROR_CODE WSAGetLastError()
#define CLOSE_SOCKET closesocket
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
using socket_t = int;
#define SOCKET_ERROR_CODE errno
#define CLOSE_SOCKET ::close
#define INVALID_SOCKET_VALUE (-1)
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

using port_t = uint16_t;

inline constexpr port_t kLdapDefaultPort = 389;
inline constexpr port_t kLdapsDefaultPort = 636;
inline constexpr port_t kLdapDevelopmentPort = 3389;

std::string platformName();

}  // namespace simple_ldapd
