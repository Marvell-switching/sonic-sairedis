#include "RedisVidIndexGenerator.h"

#include "swss/logger.h"

#define MY_LOCK() \
if(m_protected) \
{ \
    LogToModuleFile("1", "before MY_LOCK()"); \
    m_mutex->lock(); \
    LogToModuleFile("1", "after MY_LOCK()"); \
} 

#define MY_UNLOCK() \
if(m_protected) \
{ \
    LogToModuleFile("1", "before MY_UNLOCK()"); \
    m_mutex->unlock(); \
    LogToModuleFile("1", "after MY_UNLOCK()"); \
}

using namespace sairedis;

RedisVidIndexGenerator::RedisVidIndexGenerator(
        _In_ std::shared_ptr<swss::DBConnector> dbConnector,
        _In_ const std::string& vidCounterName,
        _In_ std::shared_ptr<std::mutex> t_mutex):
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
