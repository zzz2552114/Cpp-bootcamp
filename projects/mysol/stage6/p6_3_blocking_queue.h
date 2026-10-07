#include<mutex>
#include<stdexcept>
#include<deque>
#include<condition_variable>


namespace bq6{
    class ClosedError : public std::runtime_error{
    public:
        ClosedError() : runtime_error("BlockingQueue is closed"){}
    };



    template<typename T>
    class BlockingQueue{
    public:
        explicit BlockingQueue(size_t capacity):capacity_(capacity){}

        void Push(T value){
            std::unique_lock ulk(m_);
            if(closed_) throw ClosedError();
            if(q_.size()==capacity_){
                cv.wait(ulk,[this]{
                    return closed_ || q_.size()<capacity_;
                });
            }
            if (closed_)
                throw ClosedError();
            q_.emplace_back(std::move(value));
            cv.notify_all();
        }

        T Pop(){
            std::unique_lock ulk(m_);
            if(q_.empty()){
                cv.wait(ulk,[this]{
                    return closed_ || !q_.empty();
                });
            }
            if (q_.empty())
                throw ClosedError();
            T tmp = std::move(q_.front());
            q_.pop_front();
            cv.notify_all();
            return tmp;
        }

        void Close(){
            std::unique_lock ulk(m_);
            closed_ = true;
            cv.notify_all();
        }

        bool Closed() const{
            std::scoped_lock lk(m_);
            return closed_;
        }

        size_t Size() const{
            std::scoped_lock lk(m_);
            return q_.size();
        }

        size_t Capacity() const{
            std::scoped_lock lk(m_);
            return capacity_;
        }



    private:
        bool closed_ = false;
        std::deque<T> q_;
        size_t capacity_;
        mutable std::mutex m_;
        std::condition_variable cv;
    };
}

// 注意。这里也可以变成两个cv，一个提示empty一个提示full。