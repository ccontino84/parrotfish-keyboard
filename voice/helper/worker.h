// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "recognizer.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace voice {

// Decodes jobs in order on one thread, once the model is loaded.
class Worker
{
public:
    explicit Worker(Recognizer &recognizer)
        : m_recognizer(recognizer)
        , m_thread([this] { run(); })
    {
    }

    ~Worker()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_quit = true;
        }
        m_cond.notify_all();
        m_thread.join();
    }

    void setReady()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ready = true;
        }
        m_cond.notify_all();
    }

    // `session` tells jobs of a cancelled recording apart.
    void add(int session, std::function<void(Recognizer &)> job)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_jobs.push_back({session, std::move(job)});
        }
        m_cond.notify_all();
    }

    void drop(int session)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_dropped = session;
        m_jobs.erase(std::remove_if(m_jobs.begin(), m_jobs.end(),
                                    [session](const Job &j) { return j.session <= session; }),
                     m_jobs.end());
    }

    bool dropped(int session)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return session <= m_dropped;
    }

private:
    struct Job {
        int session;
        std::function<void(Recognizer &)> run;
    };

    void run()
    {
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond.wait(lock, [this] { return m_quit || (m_ready && !m_jobs.empty()); });
                if (m_quit)
                    return;
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
                if (job.session <= m_dropped)
                    continue;
            }
            job.run(m_recognizer);
        }
    }

    Recognizer &m_recognizer;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::deque<Job> m_jobs;
    bool m_ready = false;
    bool m_quit = false;
    int m_dropped = 0;
    std::thread m_thread;
};

// Commands from stdin, read on their own thread.
class Commands
{
public:
    Commands()
        : m_thread([this] { run(); })
    {
        m_thread.detach();
    }

    // The next command, or "" after `timeout` seconds; "quit" at end of input.
    std::string next(double timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait_for(lock, std::chrono::duration<double>(timeout), [this] { return !m_lines.empty(); });
        if (m_lines.empty())
            return std::string();
        std::string line = m_lines.front();
        m_lines.pop_front();
        return line;
    }

    bool pending()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_lines.empty();
    }

private:
    void run()
    {
        std::string line;
        while (std::getline(std::cin, line))
            push(line);
        push("quit");
    }

    void push(const std::string &line)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lines.push_back(line);
        }
        m_cond.notify_all();
    }

    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::deque<std::string> m_lines;
    std::thread m_thread;
};

} // namespace voice
