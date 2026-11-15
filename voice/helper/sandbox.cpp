// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sandbox.h"

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstring>

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425   // the same on every architecture
#endif

#if defined(__aarch64__)
#define VOICE_AUDIT_ARCH AUDIT_ARCH_AARCH64
#elif defined(__x86_64__)
#define VOICE_AUDIT_ARCH AUDIT_ARCH_X86_64   // the PC test
#endif

namespace voice {

bool denyNetwork(std::string &error)
{
#ifndef VOICE_AUDIT_ARCH
    error = "unsupported architecture";
    return false;
#else
    // The socket's family is an int: the low half of args[0] on these
    // little-endian architectures.
    sock_filter filter[] = {
        /* 0 */ BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
        /* 1 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, VOICE_AUDIT_ARCH, 1, 0),
        /* 2 */ BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        /* 3 */ BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
        /* 4 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_io_uring_setup, 4, 0),
        /* 5 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_socket, 0, 2),
        /* 6 */ BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, args[0])),
        /* 7 */ BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AF_UNIX, 0, 1),
        /* 8 */ BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        /* 9 */ BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EACCES),
    };
    sock_fprog program = {static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0])), filter};
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        error = std::string("no_new_privs: ") + std::strerror(errno);
        return false;
    }
    // TSYNC: also the threads the libraries may have started already.
    if (syscall(__NR_seccomp, SECCOMP_SET_MODE_FILTER, SECCOMP_FILTER_FLAG_TSYNC, &program) != 0
            && prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) != 0) {
        error = std::string("seccomp: ") + std::strerror(errno);
        return false;
    }
    return true;
#endif
}

} // namespace voice
