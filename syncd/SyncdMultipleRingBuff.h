#pragma once

#include <functional>
#include <array>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>

#define MULIPLE_RING_BUFFER 1

namespace syncdMultipleRingBuff
{
        typedef std::map<std::string, std::string> EventMap;
        template<typename DataType, int RingSize>
        class RingBuffer{
        private:
#ifndef MULIPLE_RING_BUFFER
                static RingBuffer<DataType, RingSize>* instance;
#endif
                std::vector<DataType> buffer;
                int head = 0;
                int tail = 0;
                EventMap m_eventMap;
#ifndef MULIPLE_RING_BUFFER               
                protected:
                RingBuffer<DataType, RingSize>(): buffer(RingSize) {}
                ~RingBuffer<DataType, RingSize>() {
                        delete instance;
                }
#else
                public:
                RingBuffer<DataType, RingSize>(): buffer(RingSize) {}
                ~RingBuffer<DataType, RingSize>() {}                
#endif
        public:
 #ifndef MULIPLE_RING_BUFFER         
                RingBuffer<DataType, RingSize>(const RingBuffer<DataType, RingSize>&) = delete;
                RingBuffer<DataType, RingSize>(RingBuffer<DataType, RingSize>&&) = delete;
                RingBuffer<DataType, RingSize>& operator= (const RingBuffer<DataType, RingSize>&) = delete;
                RingBuffer<DataType, RingSize>& operator= (RingBuffer<DataType, RingSize>&&) = delete;
                static RingBuffer<DataType, RingSize>* Get();
#endif                
                bool Started = false;
                bool Idle = true;
                std::mutex mtx;
                std::condition_variable cv;
                bool IsFull();
                bool IsEmpty();
                bool push(DataType entry);
                bool pop(DataType& entry);
                DataType& HeadEntry();
                void addEvent(std::string* executor);
                void doTask();
                bool tasksPending();
                bool Serves(const std::string& tableName);
        };

#ifndef MULIPLE_RING_BUFFER  
        template<typename DataType, int RingSize>
        RingBuffer<DataType, RingSize>* RingBuffer<DataType, RingSize>::instance = nullptr;     
        RingBuffer<DataType, RingSize>* RingBuffer<DataType, RingSize>::Get()
        {
                if (instance == nullptr) {
                        instance = new RingBuffer<DataType, RingSize>();
                        SWSS_LOG_NOTICE("Syncd RingBuffer created at %p!", (void *)instance);
                }
                return instance;
        }
#endif

        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::IsFull()
        {
                return (tail + 1) % RingSize == head;
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
                        return false;
                }
                buffer[tail] = std::move(ringEntry);
                tail = (tail + 1) % RingSize;
                return true;
        }

        template<typename DataType, int RingSize>
        DataType& RingBuffer<DataType, RingSize>::HeadEntry() {
                return buffer[head];
        }

        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::pop(DataType& ringEntry)
        {
                if (IsEmpty())
                        return false;
                ringEntry = std::move(buffer[head]);
                head = (head + 1) % RingSize;
                return true;
        }

        template<typename DataType, int RingSize>
        void RingBuffer<DataType, RingSize>::addEvent(std::string* executor)
        {
        //     auto inserted = m_eventMap.emplace(std::piecewise_construct,
        //             std::forward_as_tuple(executor->getName()),
        //             std::forward_as_tuple(executor));
        //     // If there is duplication of executorName in m_eventMap, logic error
        //     if (!inserted.second)
        //     {
        //         SWSS_LOG_THROW("Duplicated executorName in m_eventMap: %s", executor->getName().c_str());
        //     }
        }
        template<typename DataType, int RingSize>
        void RingBuffer<DataType, RingSize>::doTask()
        {
        //     for (auto &it : m_eventMap) {
        //         it.second->drain();
        //     }
        }
        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::tasksPending()
        {
        //     for (auto &it : m_eventMap) {
        //         auto consumer = dynamic_cast<Consumer *>(it.second.get());
        //         if (consumer->m_toSync.size())
        //             return true;
        //     }
                return false;
        }
        template<typename DataType, int RingSize>
        bool RingBuffer<DataType, RingSize>::Serves(const std::string& tableName)
        {
        //     for (auto &it : m_eventMap) {
        //         if (it.first == tableName)
        //             return true;
        //     }
                return true;
        }
}