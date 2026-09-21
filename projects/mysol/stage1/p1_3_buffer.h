class Buffer{
    public:
        inline static int allocs = 0,frees = 0;
        explicit Buffer(size_t n) : size_(n),valid(true) {
            ++allocs;
            buff = new int[n];
            for(int i = 0;i<n;++i) buff[i] = 0;
        }
        ~Buffer(){
            if(buff){
                delete[] buff;
            }
            ++frees;
            size_ = 0;
            valid = false;
        }

        Buffer(Buffer&& other) noexcept: 
            size_(other.size_),
            buff (other.buff) ,
            valid(true)
            {
                other.size_ = 0;
                other.buff = nullptr;
                other.valid = false;
            }

        Buffer& operator=(Buffer&& other) noexcept{
            if(this!=&other){
                size_ = other.size_;
                buff = other.buff;
                valid = true;
                other.size_ = 0;
                other.buff = nullptr;
                other.valid = false;
            }
            return *this;
        }


        Buffer(const Buffer &other) = delete;
        Buffer& operator=(const Buffer &other) = delete;
        


        size_t Size() const noexcept
        {
            return size_;
        }
        bool Owns() const noexcept
        {
            if(valid) return true;
            return false; 
        }

        int& operator[](size_t i)
        {
            return buff[i];
        }

        const int &operator[](size_t i) const
        {
            return buff[i];
        }

        int& At(size_t i)
        {
            if(i>=size_) throw std::out_of_range("index out of range");
            return buff[i];
        }

        const int &At(size_t i) const
        {
            if (i >= size_)
                throw std::out_of_range("index out of range");
            return buff[i];
        }

    private:
        size_t size_;
        int* buff;
        bool valid;
};