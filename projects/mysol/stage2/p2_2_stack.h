#include<vector>

template<typename T>
class Stack
{
    public:
        void Push(const T& ele) {
            data.push_back(ele);
        }

        void Push(T&& ele){
            data.push_back(std::move(ele));
        }

        void Pop(){
            if(data.empty()) throw std::out_of_range("stack is empty");
            data.pop_back();
        }

        T& Top() {
            if (data.empty())
                throw std::out_of_range("stack is empty");
            return data.back();
        }

        const T &Top()
        const {
            if (data.empty())
                throw std::out_of_range("stack is empty");
            return data.back();
        }

        bool Empty() const{
            if(data.empty()) return true;
            return false;
        }

        size_t Size() const{
            return data.size();
        }

    private:
        std::vector<T> data;
};