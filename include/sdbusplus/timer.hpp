#pragma once

#ifndef __ZEPHYR__
#include <systemd/sd-event.h>

#endif
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <memory>
#include <iostream>

namespace phosphor
{
#ifdef __ZEPHYR__
/** @class Timer
 *  @brief Manages starting watchdog timers and handling timeouts
 */
class Timer
{
  public:
    /** @brief Only need the default Timer */
    Timer() = delete;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    /** @brief Constructs timer object
     *         Uses the default io_context object
     *
     *  @param[in] userCallBack - optional function callback for timer
     *                            expirations
     */
    Timer(std::function<void()> userCallBack = nullptr) :
        ioContext(std::make_shared<boost::asio::io_context>()),
        workGuard(boost::asio::make_work_guard(*ioContext)),
        timer(*ioContext),
        expired(false),
        userCallBack(userCallBack)
    {
        // Initialize the timer
        initialize();
    }

    /** @brief Constructs timer object
     *
     *  @param[in] ioContext - asio::io_context pointer
     *  @param[in] userCallBack - optional function callback for timer
     *                            expirations
     */
    Timer(std::shared_ptr<boost::asio::io_context> ioContext, std::function<void()> userCallBack = nullptr) :
        ioContext(ioContext),
        workGuard(boost::asio::make_work_guard(*ioContext)),
        timer(*ioContext),
        expired(false),
        userCallBack(userCallBack)
    {
        // Initialize the timer
        initialize();
    }

    ~Timer()
    {
        stop();
        // ioContext->stop();
        // if (timerThread.joinable())
        // {
        //     timerThread.join();
        // }
    }

    inline bool isExpired() const
    {
        return expired;
    }

    inline bool isRunning() const
    {
        return !timer.expires_at().time_since_epoch().count() == 0;
    }

    /** @brief Starts the timer with specified expiration value.
     *  input is an offset from the current steady_clock
     */
    int start(std::chrono::microseconds usec, bool periodic = false)
    {
        // Disable the timer
        stop();
        expired = false;
        duration = usec;
        this->periodic = periodic;

        // Set the time
        timer.expires_after(usec);
        timer.async_wait([this](auto&& ec) {
            if (!ec)
            {
                timeoutHandler();
            }
        });

        return 0;
    }

    int stop()
    {
        timer.cancel();
        return 0;
    }

  private:
    /** @brief the asio::io_context structure */
    std::shared_ptr<boost::asio::io_context> ioContext;

    /** @brief Work guard to keep io_context running */
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;

    /** @brief asio::steady_timer */
    boost::asio::steady_timer timer;

    /** @brief Returns if the associated timer is expired
     *
     *  This is set to true when the timeoutHandler is called into
     */
    bool expired;

    /** @brief Optional function to call on timer expiration */
    std::function<void()> userCallBack;

    /** @brief timer duration */
    std::chrono::microseconds duration;

    /** @brief timer run type (oneshot or periodic) */
    bool periodic;

    /** @brief Thread to run the asio::io_context */
//    std::thread timerThread;

    /** @brief Initializes the timer object and starts the io_context thread
     *
     *  @error std::runtime exception thrown
     */
    void initialize()
    {
        if (!ioContext)
        {
            throw std::runtime_error("Timer has no io_context");
        }

        // timerThread = std::thread([this]() {
        //     ioContext->run();
        // });
    }

    /** @brief Callback function when timer goes off
     *
     *  On getting the signal, initiate the hard power off request
     *
     */
    void timeoutHandler()
    {
        expired = true;

        // Call optional user call back function if available
        if (userCallBack)
        {
            userCallBack();
        }

        if (periodic)
        {
            // Reset the timer for periodic operation
            timer.expires_after(duration);
            timer.async_wait([this](auto&& ec) {
                if (!ec)
                {
                    std::cout << "timeoutHandler" << std::endl;
                    timeoutHandler();
                }
            });
        }
        else
        {
            stop();
        }
    }
};

#else

/** @class Timer
 *  @brief Manages starting watchdog timers and handling timeouts
 */
class Timer
{
  public:
    /** @brief Only need the default Timer */
    Timer() = delete;
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    /** @brief Constructs timer object
     *         Uses the default sd_event object
     *
     *  @param[in] userCallBack - optional function callback for timer
     *                            expirations
     */
    Timer(std::function<void()> userCallBack = nullptr) :
        event(nullptr), eventSource(nullptr), expired(false),
        userCallBack(userCallBack)
    {
        // take a reference to the default event object
        sd_event_default(&event);
        // Initialize the timer
        initialize();
    }

    /** @brief Constructs timer object
     *
     *  @param[in] event - sd_event pointer
     *  @param[in] userCallBack - optional function callback for timer
     *                            expirations
     */
    Timer(sd_event* event, std::function<void()> userCallBack = nullptr) :
        event(event), eventSource(nullptr), expired(false),
        userCallBack(userCallBack)
    {
        if (!event)
        {
            // take a reference to the default event object
            sd_event_default(&event);
        }
        else
        {
            // take a reference to the event; the caller can
            // keep their ref or drop it without harm
            sd_event_ref(event);
        }
        // Initialize the timer
        initialize();
    }

    ~Timer()
    {
        // the *_unref functions do nothing if passed nullptr
        // so no need to check first
        eventSource = sd_event_source_unref(eventSource);
        event = sd_event_unref(event);
    }

    inline bool isExpired() const
    {
        return expired;
    }

    inline bool isRunning() const
    {
        int running = 0;
        if (sd_event_source_get_enabled(eventSource, &running) < 0)
        {
            return false;
        }
        return running != SD_EVENT_OFF;
    }

    /** @brief Starts the timer with specified expiration value.
     *  input is an offset from the current steady_clock
     */
    int start(std::chrono::microseconds usec, bool periodic = false)
    {
        // Disable the timer
        stop();
        expired = false;
        duration = usec;
        if (periodic)
        {
            // A periodic timer means that when the timer goes off,
            // it automatically rearms and starts running again
            runType = SD_EVENT_ON;
        }
        else
        {
            // A ONESHOT timer means that when the timer goes off,
            // it moves to disabled state.
            runType = SD_EVENT_ONESHOT;
        }

        // Get the current MONOTONIC time and add the delta
        auto expireTime = getTime() + usec;

        // Set the time
        int r = sd_event_source_set_time(eventSource, expireTime.count());
        if (r < 0)
        {
            throw std::runtime_error("Failure to set timer");
        }

        r = setEnabled(runType);
        if (r < 0)
        {
            throw std::runtime_error("Failure to start timer");
        }
        return r;
    }

    int stop()
    {
        return setEnabled(SD_EVENT_OFF);
    }

  private:
    /** @brief the sd_event structure */
    sd_event* event;

    /** @brief Source of events */
    sd_event_source* eventSource;

    /** @brief Returns if the associated timer is expired
     *
     *  This is set to true when the timeoutHandler is called into
     */
    bool expired;

    /** @brief Optional function to call on timer expiration */
    std::function<void()> userCallBack;

    /** @brief timer duration */
    std::chrono::microseconds duration;

    /** @brief timer run type (oneshot or periodic) */
    int runType;

    /** @brief Initializes the timer object with infinite
     *         expiration time and sets up the callback handler
     *
     *  @error std::runtime exception thrown
     */
    void initialize()
    {
        if (!event)
        {
            throw std::runtime_error("Timer has no event loop");
        }
        // This can not be called more than once.
        if (eventSource)
        {
            throw std::runtime_error("Timer already initialized");
        }

        // Add infinite expiration time
        auto r = sd_event_add_time(
            event, &eventSource,
            CLOCK_MONOTONIC, // Time base
            UINT64_MAX,      // Expire time - way long time
            0,               // Use default event accuracy
            [](sd_event_source* /*eventSource*/, uint64_t /*usec*/,
               void* userData) {
                auto timer = static_cast<Timer*>(userData);
                return timer->timeoutHandler();
            },     // Callback handler on timeout
            this); // User data
        if (r < 0)
        {
            throw std::runtime_error("Timer initialization failed");
        }

        // Disable the timer for now
        r = stop();
        if (r < 0)
        {
            throw std::runtime_error("Disabling the timer failed");
        }
    }

    /** @brief Enables / disables the timer */
    int setEnabled(int action)
    {
        return sd_event_source_set_enabled(eventSource, action);
    }

    /** @brief Callback function when timer goes off
     *
     *  On getting the signal, initiate the hard power off request
     *
     */
    int timeoutHandler()
    {
        if (runType == SD_EVENT_ON)
        {
            // set the event to the future
            auto expireTime = getTime() + duration;

            // Set the time
            int r = sd_event_source_set_time(eventSource, expireTime.count());
            if (r < 0)
            {
                throw std::runtime_error("Failure to set timer");
            }
        }
        expired = true;

        // Call optional user call back function if available
        if (userCallBack)
        {
            userCallBack();
        }

        int running;
        if (sd_event_source_get_enabled(eventSource, &running) < 0 ||
            running == SD_EVENT_ONESHOT)
        {
            stop();
        }
        return 0;
    }

    /** @brief Gets the current time from steady clock */
    static std::chrono::microseconds getTime()
    {
        using namespace std::chrono;
        auto usec = steady_clock::now().time_since_epoch();
        return duration_cast<microseconds>(usec);
    }
};

#endif

} // namespace phosphor
