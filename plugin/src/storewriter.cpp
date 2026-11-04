// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "storewriter.h"

#include "learningstore.h"

StoreWriter::StoreWriter(const QString &path)
    : m_path(path)
    , m_thread(&StoreWriter::run, this)
{
}

StoreWriter::~StoreWriter()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_wake.notify_one();
    m_thread.join();
}

void StoreWriter::post(std::function<void(LearningStore &)> job)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push_back(std::move(job));
    }
    m_wake.notify_one();
}

void StoreWriter::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle.wait(lock, [this] { return m_jobs.empty() && !m_busy; });
}

void StoreWriter::run()
{
    LearningStore store(m_path);
    std::unique_lock<std::mutex> lock(m_mutex);
    for (;;) {
        m_wake.wait(lock, [this] { return m_stop || !m_jobs.empty(); });
        if (m_jobs.empty())
            break;   // stopped, all written
        std::function<void(LearningStore &)> job = std::move(m_jobs.front());
        m_jobs.pop_front();
        m_busy = true;
        lock.unlock();
        job(store);
        lock.lock();
        m_busy = false;
        if (m_jobs.empty())
            m_idle.notify_all();
    }
}
