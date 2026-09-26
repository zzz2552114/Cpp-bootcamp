#pragma once

#include<set>
#include<string>
#include<map>

namespace s4{

    std::set<int> SortedUnique(const std::vector<int>& v){
        std::set<int> s(v.begin(),v.end());
        return s;
    }

    std::string JoinAscending(const std::set<int>& s){
        std::string str;
        for(int x:s){
            str += std::to_string(x);
            str += ' ';
        }
        size_t sz = str.size();
        str = str.substr(0,sz-1);
        return str;
    }

    struct SumCmp{
        bool operator()(const std::pair<int,int>& a,const std::pair<int,int>& b) const{
            if(a.first + a.second == b.first + b.second) return a < b;
            else return a.first + a.second < b.first + b.second;
        }
    };

    std::string JoinBySumOrder(const std::set<std::pair<int,int>,SumCmp>& s){
        std::string str;
        for(auto ele:s){
            str += "(";
            str += std::to_string(ele.first);
            str += ',';
            str += std::to_string(ele.second);
            str += ") ";
        }
        size_t sz = str.size();
        str = str.substr(0, sz - 1);
        return str;
    }

    std::map<std::string,int> WordFreq(const std::vector<std::string>& words){
        std::map<std::string, int> mp;
        for(auto s:words) ++mp[s];
        return mp;
    }

    std::string KeysJoined(const std::map<std::string, int> &m){
        std::string res;
        for(auto ele:m) res += ele.first;
        return res;
    }
}