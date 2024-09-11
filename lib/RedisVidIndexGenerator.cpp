#include "RedisVidIndexGenerator.h"

#include "swss/logger.h"

#define MY_LOCK() \
if(m_protected) \
{ \
    if (!m_mutex->try_lock_for(std::chrono::minutes(2))) {\
            SWSS_LOG_ERROR("FATAL: Failed to lock the mutex within 2 minutes");\
        m_mutex->lock(); \
        SWSS_LOG_ERROR("Moraml Mutex continue after 2 minutes");\
    }\
} 

#define MY_UNLOCK() \
if(m_protected) \
{ \
    m_mutex->unlock(); \
}

using namespace sairedis;

RedisVidIndexGenerator::RedisVidIndexGenerator(
        _In_ std::shared_ptr<swss::DBConnector> dbConnector,
        _In_ const std::string& vidCounterName,
        _In_ std::shared_ptr<std::timed_mutex> t_mutex):
    m_dbConnector(dbConnector),
    m_vidCounterName(vidCounterName),
    m_mutex(t_mutex)
{
    SWSS_LOG_ENTER();

    if(t_mutex != nullptr)
    {
        m_protected = true;
    }
    else
    {
        m_protected = false;
    }
    // empty
}

uint64_t RedisVidIndexGenerator::increment()
{
    SWSS_LOG_ENTER();

    // this counter must be atomic since it can be independently accessed by
    // sairedis and syncd

    MY_LOCK();
    uint64_t connector = m_dbConnector->incr(m_vidCounterName); // "VIDCOUNTER"
    MY_UNLOCK();
    return connector;
}

void RedisVidIndexGenerator::reset()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_ERROR("not implemented");
}
