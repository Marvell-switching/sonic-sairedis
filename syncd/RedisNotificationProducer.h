#pragma once

#include "NotificationProducerBase.h"

#include "swss/dbconnector.h"
#include "swss/notificationproducer.h"

namespace syncd
{
    class RedisNotificationProducer:
        public NotificationProducerBase
    {
        public:

            RedisNotificationProducer(
                    _In_ const std::string& dbName,
                    _In_ std::shared_ptr<std::mutex> t_mutex = nullptr);

            virtual ~RedisNotificationProducer() = default;

        public:

            virtual void send(
                    _In_ const std::string& op,
                    _In_ const std::string& data,
                    _In_ const std::vector<swss::FieldValueTuple>& values) override;

        private:

            std::shared_ptr<swss::DBConnector> m_db;

            std::shared_ptr<swss::NotificationProducer> m_notificationProducer;
            
            bool m_protected;

            std::shared_ptr<std::mutex> m_mutex;
    };
}
