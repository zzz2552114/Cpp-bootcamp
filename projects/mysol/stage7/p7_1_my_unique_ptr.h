#include<utility>


namespace myptr7{

    template <typename T>
    class MyUniquePtr{
        public:
        constexpr MyUniquePtr() noexcept = default;
        explicit MyUniquePtr(T* ptr) noexcept : inptr(ptr){
            ptr = nullptr;
        };

        ~MyUniquePtr(){
            delete inptr;
        }

        MyUniquePtr(const MyUniquePtr&) = delete;
        MyUniquePtr& operator=(const MyUniquePtr&) = delete;

        MyUniquePtr(MyUniquePtr&& other) noexcept: inptr(std::move(other.inptr)){
            other.inptr = nullptr;
        }

        MyUniquePtr& operator=(MyUniquePtr&& other) noexcept{
            if(this != &other){
                // 因为是移动赋值，先删除自己的！
                delete inptr;
                inptr = other.inptr;
                other.inptr = nullptr;
            }
            return *this;
        }

        T& operator*() const {
            return *inptr;
        }

        T* operator->() const{
            return inptr;
        }

        inline T* Get() const noexcept{
            return inptr;
        }

        explicit operator bool() const noexcept{
            if(inptr==nullptr) return 0;
            return 1;
        }

        T* Release() noexcept{
            T* tmp = inptr;
            inptr = nullptr;
            return tmp;
        }

        void Reset(T* p=nullptr) noexcept{
            if(p==Get()) return;
            delete inptr;
            inptr = p;
            p = nullptr;
        }

        void Swap(MyUniquePtr& other) noexcept{
            T* tmp = inptr;
            inptr = other.Get();
            other.inptr = tmp;
        }


        private:
        T* inptr = nullptr;
    };

    template <typename T,typename... Args>
    MyUniquePtr<T> MyMakeUnique(Args&&... args){
        MyUniquePtr<T> uptr(new T(std::forward<Args>(args)...));
        return uptr;
    }




}