#pragma once

#include "OidIndexGenerator.h"

#include "swss/dbconnector.h"
#include "swss/sal.h"

#include <memory>
#include "syncd/Logger.h"
#include <mutex>

namespace sairedis
{
    class RedisVidIndexGenerator:
        public OidIndexGenerator
    {
        public:

            RedisVidIndexGenerator(
                    _In_ std::shared_ptr<swss::DBConnector> dbConnector,
                    _In_ const std::string& vidCounterName,
                    _In_ std::shared_ptr<std::mutex> t_mutex = nullptr);

            virtual ~RedisVidIndexGenerator() = default;

        public:

            virtual uint64_t increment() override;

            virtual void reset() override;

        private:

            std::shared_ptr<swss::DBConnector> m_dbConnector;

            std::string m_vidCounterName;

            std::shared_ptr<std::mutex> m_mutex;
    };
}
