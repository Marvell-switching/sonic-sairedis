#include "RedisSelectableChannel.h"

#include "swss/logger.h"
#include "syncd/Logger.h"

using namespace sairedis;

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

RedisSelectableChannel::RedisSelectableChannel(
        _In_ std::shared_ptr<swss::DBConnector> dbAsic,
        _In_ const std::string& asicStateTable,
        _In_ const std::string& getResponseTable,
        _In_ const std::string& tempPrefix,
        _In_ bool modifyRedis,
        _In_ std::shared_ptr<std::timed_mutex> t_mutex):
    m_dbAsic(dbAsic),
    m_tempPrefix(tempPrefix),
    m_modifyRedis(modifyRedis),
    m_mutex(t_mutex)
{
    SWSS_LOG_ENTER();

    m_asicState = std::make_shared<swss::ConsumerTable>(m_dbAsic.get(), asicStateTable);

    m_asicState->setModifyRedis(m_modifyRedis);

    /*
     * At the end we cant use producer consumer concept since if one process
     * will restart there may be something in the queue also "remove" from
     * response queue will also trigger another "response".
     */

    m_getResponse = std::make_shared<swss::ProducerTable>(m_dbAsic.get(), getResponseTable);

    SWSS_LOG_NOTICE("opened redis channel");
    if(t_mutex != NULL)
    {
        m_protected = true;
    }
    else
    {
        m_protected = false;
    }
}

bool RedisSelectableChannel::empty()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    bool ans = m_asicState->empty();
    MY_UNLOCK();
    return ans;
}

void RedisSelectableChannel::set(
        _In_ const std::string& key,
        _In_ const std::vector<swss::FieldValueTuple>& values,
        _In_ const std::string& op)
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    m_getResponse->set(key, values, op);
    MY_UNLOCK();
}

void RedisSelectableChannel::pop(
        _Out_ swss::KeyOpFieldsValuesTuple& kco,
        _In_ bool initViewMode)
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    if (initViewMode)
    {
        m_asicState->pop(kco, m_tempPrefix);
    }
    else
    {
        m_asicState->pop(kco);
    }
    MY_UNLOCK();
}

// Selectable overrides

int RedisSelectableChannel::getFd()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    int ans = m_asicState->getFd();
    MY_UNLOCK();
    return ans;
}

uint64_t RedisSelectableChannel::readData()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    uint64_t ans = m_asicState->readData();
    MY_UNLOCK();
    return ans;
}

bool RedisSelectableChannel::hasData()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    bool ans = m_asicState->hasData();
    MY_UNLOCK();
    return ans;
}

bool RedisSelectableChannel::hasCachedData()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    bool ans = m_asicState->hasCachedData();
    MY_UNLOCK();
    return ans;
}

bool RedisSelectableChannel::initializedWithData()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    bool ans = m_asicState->initializedWithData();
    MY_UNLOCK();
    return ans;
}

void RedisSelectableChannel::updateAfterRead()
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    m_asicState->updateAfterRead();
    MY_UNLOCK();
}

int RedisSelectableChannel::getPri() const
{
    SWSS_LOG_ENTER();
    MY_LOCK();
    auto ans = m_asicState->getPri();
    MY_UNLOCK();
    return ans;
}
