#include<atomic>
#include<cassert>

#define SPIN_LOCK_UNLOCK 0
#define SPIN_LOCK_WRITE_LOCK -1

using std::atomic;
using std::atomic_int;
using std::atomic_store_explicit;
using std::atomic_load_explicit;
using std::atomic_compare_exchange_weak_explicit;
using std::memory_order_relaxed;
using std::memory_order_acquire;
using std::memory_order_release;

#define spinlock_t atomic<int>
//typedef atomic_int spinlock_t; // why not work？

void rwlock_init(spinlock_t &l)
{
    atomic_store_explicit(&l, SPIN_LOCK_UNLOCK, memory_order_relaxed);
}

void read_lock(spinlock_t &l)
{
    int expected;
    int desired;

    while(true)
    {
        expected = atomic_load_explicit(&l, memory_order_relaxed);

        if(expected >= 0)
        {
            desired = 1 + expected;
            if(atomic_compare_exchange_weak_explicit(&l, &expected, desired, memory_order_relaxed, memory_order_relaxed))
                break; // success
        }
    }

    atomic_thread_fence(memory_order_acquire); // sync
}

void read_unlock(spinlock_t &l)
{
    int expected;
    int desired;

    while(true)
    {
        expected = atomic_load_explicit(&l, memory_order_relaxed);

        if(expected > 0)
        {
            desired = expected - 1;

            atomic_thread_fence(memory_order_release); // sync
            if(atomic_compare_exchange_weak_explicit(&l, &expected, desired, memory_order_relaxed, memory_order_relaxed))
                break; // success
        }
        else
        {
            assert(false);
        }
    }
}

void write_lock(spinlock_t &l)
{
    int expected;
    int desired;

    while(true)
    {
        expected = atomic_load_explicit(&l, memory_order_relaxed);

        if(expected == SPIN_LOCK_UNLOCK)
        {
            desired = SPIN_LOCK_WRITE_LOCK;
            if(atomic_compare_exchange_weak_explicit(&l, &expected, desired, memory_order_relaxed, memory_order_relaxed))
                break; // success
        }
    }

    atomic_thread_fence(memory_order_release); // sync
}

void write_unlock(spinlock_t &l)
{
    int expected;
    int desired;

    while(true)
    {
        expected = atomic_load_explicit(&l, memory_order_relaxed);

        if(expected == SPIN_LOCK_WRITE_LOCK)
        {
            desired = SPIN_LOCK_UNLOCK;

            atomic_thread_fence(memory_order_release); // sync
            if(atomic_compare_exchange_weak_explicit(&l, &expected, desired, memory_order_relaxed, memory_order_relaxed))
                break; // success
        }
        else
        {
            assert(false);
        }
    }
}

#ifdef TEST
//#include<thread>
#include<boost/thread.hpp>
#include<iostream>

spinlock_t g_lock;
long g_total;
const int count = 5000;

void add_job()
{
    for(int i = 0; i < count; ++i)
    {
        write_lock(g_lock);
        ++g_total;
        std::cout << "Thread ++ " << boost::this_thread::get_id() << std::endl;
        write_unlock(g_lock);
    }
}

void read_job()
{
    for(int i = 0; i < count; ++i)
    {
        read_lock(g_lock);
        std::cout << g_total << std::endl;;
        read_unlock(g_lock);
    }
}

int main()
{
    g_total = 0;
    rwlock_init(g_lock);


    boost::thread th1(add_job);
    boost::thread th2(add_job);

    boost::thread th3(read_job);
    boost::thread th4(read_job);
    boost::thread th5(read_job);

    th1.join();
    th2.join();
    th3.join();
    th4.join();
    th5.join();

    std::cout << "The total: " << g_total << std::endl;
}
#endif
