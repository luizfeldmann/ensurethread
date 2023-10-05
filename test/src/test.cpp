#include "ensurethread.hpp"
#include <gtest/gtest.h>

//! Tests if the thread check on the same thread returns true
TEST(check_thread, same)
{
    CEnsureThread et;

    EXPECT_TRUE(et.check_thread());
}

//! Tests if the thread check on the different thread returns false
TEST(check_thread, different)
{
    CEnsureThread et;

    std::thread([&et]() {
        EXPECT_FALSE(et.check_thread());
    }).join();
}

//! Tests if the 'wait' API calls the functions in the expected thread
TEST(wait, direct)
{
    // References the current thread
    CEnsureThread et;

    //! Utility to compare the threads
    struct STest
    {
        const std::thread::id m_id = std::this_thread::get_id();

        bool IsSame() // purposefully use non-const here
        {
            return m_id == std::this_thread::get_id();
        }
    } threadChecker;

    //! Used to wait while the other thread works
    bool bReady = false;

    std::thread other([&et, &bReady, &threadChecker]()
        {
            // Direct call from different thread should be false
            EXPECT_FALSE(threadChecker.IsSame());

            // Call via 'wait' should be true
            EXPECT_TRUE(et.wait(&threadChecker, &STest::IsSame));

            // Cause the polling loop to break
            bReady = true;
        });

    // Poll while the test runs
    while (!bReady)
        et.poll();

    // Free the other thread
    other.join();
}

// Tests the macro for automatic redirection
TEST(wait, macro)
{
    // References the current thread
    CEnsureThread et;

    //! Utility to compare the threads
    struct STest
    {
        const std::thread::id m_id = std::this_thread::get_id();

        bool IsSame() const
        {
            return m_id == std::this_thread::get_id();
        }

        int Work(CEnsureThread* et, int a, int b) const
        {
            // As the library to call this function again on the main thread
            ENSURE_THREAD(*et, &STest::Work, et, a, b);

            // This should be unreachable in the wrong thread
            EXPECT_TRUE(IsSame());

            // Check the return value propagates back
            return a + b;
        }
    } const threadChecker;

    // Sanity test the worker
    EXPECT_EQ(threadChecker.Work(&et, 1, 2), 3);

    //! Used to wait while the other thread works
    bool bReady = false;

    // Perform in some other thread
    std::thread other([&threadChecker, &et, &bReady]()
        {
            // Invoke some work in the non-main thread and check the result is valid
            EXPECT_EQ(threadChecker.Work(&et, 4, 5), 9);

            // Cause the polling loop to break
            bReady = true;
        });

    // Poll while the test runs
    while (!bReady)
        et.poll();

    // Free the other thread
    other.join();
}
