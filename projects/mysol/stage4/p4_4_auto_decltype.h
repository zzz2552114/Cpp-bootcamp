#include<string>
#include<map>

namespace a4{

    struct Big{
        inline static int copies = 0;
        static void Reset() { copies = 0; }
        int v;


        explicit Big(int x):v(x){};
        Big() = default;

        Big(const Big& other):v(other.v){++copies;}

        Big& operator=(const Big& other){
            if(this != &other){
                v = other.v;
                ++copies;
            }
            return *this;
        }
    };

    template<typename C>
    decltype(auto) AtRef(C& c,size_t i){
        return c[i];
    }

    template<typename C> auto AtVal(C& c,size_t i){
        return c[i];
    }

    std::string ScaleValues(std::map<std::string, int> &m, int factor){
        std::string res;
        for(auto& [k,v]:m){
            res += k;
            v *= factor;
            res += std::to_string(v);
        }
        return res;
    }

    int SumPairs(const std::vector<std::pair<int, int>> &v){
        int res = 0;
        for(const auto& [a,b]:v) res += (a+b);
        return res;
    }
}