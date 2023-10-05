#ifndef _ENSURETHREAD_H_
#define _ENSURETHREAD_H_

#include <future>
#include <thread>
#include <functional>
#include <deque>

//! @brief Utility to ensure a function executes in the desired thread
class CEnsureThread
{
private:
    //! The ID of the desired thread
    const std::thread::id m_id;

    //! Locks the queue
    std::mutex m_mxQueue;

    //! Queue of completion executors
    std::deque<std::packaged_task<void()>> m_dqExecutors;

    template<typename R, typename...Args>
    R wait_internal(Args&&...args)
    {
        std::packaged_task<R()> task(std::forward<Args>(args)...);

        // Thread safe add executor to queue
        {
            std::lock_guard<std::mutex> lock(m_mxQueue);
            m_dqExecutors.emplace_back([&task]() { task(); });
        }

        // Wait for result and return
        return task.get_future().get();
    }

    //! Non copy-constructible
    CEnsureThread(const CEnsureThread&) = delete;

    //! Non copy-assignable
    CEnsureThread& operator=(const CEnsureThread&) = delete;

public:
    //! Creates the ensurer based on the current thread
    inline CEnsureThread()
        : m_id(std::this_thread::get_id())
    {

    }

    //! @brief Gets the ID of the desired thread
    inline std::thread::id get_id() const
    {
        return m_id;
    }

    //! Checks if the current thread is the same as the desired one
    //! @return True if current and desired match
    inline bool check_thread() const
    {
        return get_id() == std::this_thread::get_id();
    }

    //! @brief Waits for the function to complete on another thread
    //! @tparam T The class type owning the function
    //! @tparam R The return type of the function
    //! @tparam ...Args All the remaining arguments of the function
    //! @param obj The instance of the object to execute the function
    //! @param method The member method
    //! @param ...args All the arguments to capture
    //! @return The value return by the other thread
    template <typename T, typename R, typename...Args>
    R wait(T* obj, R(T::* method)(Args...), Args&...args)
    {
        return wait_internal<R>(std::bind(method, obj, args...));
    }

    template <typename T, typename R, typename...Args>
    R wait(const T* obj, R(T::* method)(Args...) const, Args&...args)
    {
        return wait_internal<R>(std::bind(method, obj, args...));
    }

    //! @brief Polls one executor from the queue
    //! @return True if executed (not empty)
    inline bool poll_one()
    {
        bool bPoll = false;

        if (!m_dqExecutors.empty())
        {
            bPoll = true;

            // Remove from queue
            m_mxQueue.lock();
            auto fnExecutor = std::move(m_dqExecutors.front());
            m_dqExecutors.pop_front();
            m_mxQueue.unlock();

            // Execute
            fnExecutor();
        }

        return bPoll;
    }

    //! @brief Polls all executors from the queue
    inline void poll()
    {
        while (poll_one());
    }
};

//! Calls this function again in the desired thread
#define ENSURE_THREAD(THREAD, METHOD, ...) \
    {if (!(THREAD).check_thread()) \
        return (THREAD).wait(this, METHOD,  __VA_ARGS__);} 

#endif