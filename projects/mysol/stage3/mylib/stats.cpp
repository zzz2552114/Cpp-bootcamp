#include"stats.h"
#include<vector>

namespace mylib{
    double Average(const std::vector<int> &v){
        int n = (int)v.size();
        if(n==0) return 0;
        else return (double)Sum(v)/n;
    }
    long long Sum(const std::vector<int> &v){
        long long res = 0;
        for(auto x:v) res += x;
        return res;
    }
}