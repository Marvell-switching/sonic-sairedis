#include "RedisNotificationProducer.h"

#include "sairediscommon.h"

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

using namespace syncd;

RedisNotificationProducer::RedisNotificationProducer(
        _In_ const std::string& dbName,
        _In_ std::shared_ptr<std::mutex> t_mutex) : m_mutex(t_mutex)
{
    SWSS_LOG_ENTER();

    m_db = std::make_shared<swss::DBConnector>(dbName, 0);

    m_notificationProducer = std::make_shared<swss::NotificationProducer>(m_db.get(), REDIS_TABLE_NOTIFICATIONS_PER_DB(dbName));

    if(t_mutex != nullptr)
    {
        m_protected = true;
    }
    else
    {
        m_protected = false;
    }
}

void RedisNotificationProducer::send(
        _In_ const std::string& op,
        _In_ const std::string& data,
        _In_ const std::vector<swss::FieldValueTuple>& values)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> vals = values;

    MY_LOCK();
    m_notificationProducer->send(op, data, vals);
    MY_UNLOCK();
}
