// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QString>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

class LearningStore;

// Writes to learning.db on a thread of its own, in order (a language
// stored whole is a second or so on a phone), with its own connection; the
// jobs get only copies of the data, never the keyboard's objects.
class StoreWriter
{
public:
    explicit StoreWriter(const QString &path);
    // Writes what is queued first.
    ~StoreWriter();
    StoreWriter(const StoreWriter &) = delete;
    StoreWriter &operator=(const StoreWriter &) = delete;

    void post(std::function<void(LearningStore &)> job);
    // Returns once everything posted is written (before reading the file).
    void wait();

private:
    void run();

    const QString m_path;
    std::mutex m_mutex;
    std::condition_variable m_wake;   // a job, or stop
    std::condition_variable m_idle;   // nothing queued or running
    std::deque<std::function<void(LearningStore &)>> m_jobs;
    bool m_busy = false;
    bool m_stop = false;
    std::thread m_thread;   // last: starts once the rest is ready
};
