#pragma once
#include<unordered_map>
#include<utility>
#include<string>

inline const int mod = 100003;
namespace u4{

    struct PairHash{
        size_t operator()(const std::pair<int,int>& p) const{
            int h1 = (p.first % mod + mod)%mod;
            int h2 = (p.second % mod + mod) % mod;
            return (h1 ^ h2) << 1 ;
        }
    };

    using PairMap = std::unordered_map<std::pair<int,int>,std::string,PairHash>;

    struct Point{
        int x,y;
        const bool operator==(const Point& other) const{
            return this->x == other.x && this->y == other.y;
        }
    };

    std::unordered_map<std::string,int> MakeBasicMap(){
        std::unordered_map<std::string, int> mp;
        mp["foo"] = 2;
        mp["jignesh"] = 445;
        mp["spam"] = 1;
        mp["eggs"] = 2;
        return mp;
    }
}
namespace std
{
    template <>
    struct hash<u4::Point>
    {
        size_t operator()(const u4::Point &p) const noexcept
        {
            int h1 = (p.x % mod + mod) % mod;
            int h2 = (p.y % mod + mod) % mod;
            return (h1 ^ h2) << 1;
        }
    };
}
