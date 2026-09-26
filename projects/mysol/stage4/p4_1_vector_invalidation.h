#include<vector>

namespace v4{
    struct CapSize{
        size_t cap,size;
    };

    CapSize PushNTimes(size_t n){
        std::vector<int> vec;
        while(n--){
            vec.push_back(1);
        }
        return {vec.capacity(),vec.size()};
    }

    CapSize ReserveThenPush(size_t res_n,size_t push_n){
        std::vector<int> vec;
        vec.reserve(res_n);
        while(push_n--) vec.push_back(1);
        return {vec.capacity(),vec.size()};
    }

    uintptr_t DataAddr(const std::vector<int>& v){
        return reinterpret_cast<uintptr_t> (v.data());
    }

    bool AddressStableWithinCapacity(size_t cap,size_t fill){
        std::vector<int> vec;
        vec.reserve(cap);
        uintptr_t start = DataAddr(vec);
        while(fill--){
            vec.push_back(1);
        }
        uintptr_t end = DataAddr(vec);
        return start == end;
    }

    bool AddressChangesPastCapacity(size_t exceed){
        std::vector<int> vec;
        uintptr_t start = DataAddr(vec);
        while (exceed--)
        {
            vec.push_back(1);
        }
        uintptr_t end = DataAddr(vec);
        return start != end;
    }
}