// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// voice::denyNetwork(): no internet sockets afterwards, in any thread;
// AF_UNIX (PulseAudio) still works.

#include "sandbox.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <thread>

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok) {
        ++failures;
        std::printf("FAIL %s\n", what);
    }
}

bool opens(int family, int type)
{
    const int fd = socket(family, type, 0);
    if (fd >= 0)
        close(fd);
    return fd >= 0;
}

} // namespace

int main()
{
    check(opens(AF_INET, SOCK_STREAM), "AF_INET before");
    std::string error;
    check(voice::denyNetwork(error), error.c_str());
    check(!opens(AF_INET, SOCK_STREAM) && errno == EACCES, "AF_INET refused");
    check(!opens(AF_INET6, SOCK_DGRAM), "AF_INET6 refused");
    check(!opens(AF_NETLINK, SOCK_RAW), "AF_NETLINK refused");
    check(!opens(AF_PACKET, SOCK_RAW), "AF_PACKET refused");
    check(opens(AF_UNIX, SOCK_STREAM), "AF_UNIX allowed");
    bool threadRefused = false;
    std::thread([&threadRefused] { threadRefused = !opens(AF_INET, SOCK_DGRAM); }).join();
    check(threadRefused, "AF_INET refused in a new thread");
    std::printf(failures ? "%d failures\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
