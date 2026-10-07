#pragma once
#include<mutex>
#include<thread>
#include<deque>
#include<utility>

namespace counter6{
    struct UnsafeCounter{
        int value = 0;
        void Increment(int iters){
            for(int i = 1;i<=iters;i++){
                int tmp = value+1;
                std::this_thread::yield();
                value = tmp;
            }
        }
    };

    struct LockedCounter{
        int value = 0;
        std::mutex m_;
        void Increment(int iters){
            for(int i = 1;i<=iters;i++){
                m_.lock();
                int tmp = value + 1;
                value = tmp;
                m_.unlock();
            }
        }
    };

    struct ScopedCounter{
        int value;
        std::mutex m_;
        void Increment(int iters){
            for(int i = 1;i<=iters;i++){
                std::scoped_lock lk(m_);
                int tmp = value+1;
                value = tmp;
            }
        }
    };

    class Counter{
    public:
        void Increment(){
            std::scoped_lock lk(m_);
            ++val;
        }
        void Add(int n){
            std::scoped_lock lk(m_);
            val += n;
        }

        int Get() const{
            std::scoped_lock lk(m_);
            return val;
        }
        void Reset(){
            std::scoped_lock lk(m_);
            val = 0;
        }
    private:
        mutable std::mutex m_;
        int val = 0;
    };

    inline int RunCounter(Counter& c,int nthreads,int iters){
        auto nfunc = [iters,&c](){
            for(int i = 1;i<=iters;i++){
                c.Increment();
            }
        };
        std::deque<std::thread> dq;
        for(int i = 1;i<=nthreads;i++){
            std::thread t(nfunc);
            dq.push_back(std::move(t));
        }
        for(int i = 0;i<nthreads;i++){
            dq[i].join();
        }
        return c.Get();
    }

}