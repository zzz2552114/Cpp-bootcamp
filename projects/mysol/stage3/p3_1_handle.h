#pragma once

class Handle{
    public:
        inline static int acquires = 0,live = 0, releases = 0,moves = 0;


        explicit Handle(int id) : id(id),valid(1)
        {++acquires,++live;}    

        ~Handle(){
            if(valid){
                valid = 0;
                --live;
                ++releases;
            }
        }

        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;

        Handle(Handle&& other) noexcept: id(other.id),valid(other.valid){
            ++moves;
            other.valid = 0;
        }
        Handle& operator=(Handle&& other) noexcept{
            if(this != &other){
                if(other.valid) {
                    --live;
                    ++releases;
                } 
                id = other.id;
                valid = other.valid;
                ++moves;
                other.valid = 0;
            }
            return *this;
        }

        int Id() const {
            return id;
        }

        bool Valid() const{
            return valid;
        }
        static void ResetStats(){
            acquires = live = releases = moves = 0;
        }

    private:
        int id;
        bool valid;
};

inline Handle MakeHandle(int id){
    return Handle(id);
}