#pragma once

#include <functional>
#include <array>
#include <mutex>
#include <condition_variable>
#include <map>

#include "swss/logger.h"

namespace syncdMultipleRingBuff
{
        template<typename DataType, int RingSize>
        class RingBuffer {
        private:
                std::vector<DataType> buffer;
                int head = 0;
                int tail = 0;
        public:
                RingBuffer(): buffer(RingSize) {}
                ~RingBuffer() {}
                std::mutex mtx_push;
                std::mutex mtx_pop;
                std::condition_variable cv_empty;
                std::condition_variable cv_full;
                bool Started = false;
                bool IsFull();
                bool IsEmpty();
                bool push(DataType entry);
                bool pop(DataType& entry);
        };

        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::IsFull()
        {
                bool ret = (tail + 1) % RingSize == head;
                if ( ret )
                        SWSS_LOG_NOTICE("IsFull: %d", ret);
                return ret;
        }
        
        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::IsEmpty()
        {
                return tail == head;
        }

        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::push(DataType ringEntry)
        {
                if (IsFull()){
                        SWSS_LOG_NOTICE("Buffer is full");
                        return false;
                }
                buffer[tail] = std::move(ringEntry);
                tail = (tail + 1) % RingSize;
                return true;
        }

        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::pop(DataType& ringEntry)
        {
                if (IsEmpty()){
                        SWSS_LOG_NOTICE("Buffer is empty");
                        return false;
                }
                ringEntry = std::move(buffer[head]);
                head = (head + 1) % RingSize;
                return true;
        }

        using AnyTask = std::function<void()>;
        #define ORCH_RING_SIZE 1024
        typedef RingBuffer<AnyTask, ORCH_RING_SIZE> SyncdRing;

}