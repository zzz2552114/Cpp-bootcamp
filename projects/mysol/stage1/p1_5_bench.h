#include<vector>
#include<string>



struct Tracked{
    inline static int copies = 0,moves = 0;
    int v;
    explicit Tracked(int x) : v(x){};
    Tracked(const Tracked& other) : v(other.v){
        ++copies;
    }
    Tracked(Tracked&& other) noexcept:v(other.v){
        ++moves;
    }

    static void Reset(){
        copies = 0,moves = 0;
    }
};

std::vector<std::string> MakeLongStrings(size_t n){
    std::string s = "x";
    for(int i = 1;i<=6;i++) s+=s;
    std::vector<std::string> vec(n,s);
    return vec;
}

size_t TotalLength(const std::vector<std::string>& v){
    size_t res = 0;
    for(int i = 0;i<v.size();i++){
        res += v[i].size();
    }
    return res;
}

long long SumValues(const std::vector<Tracked> &v){
    long long res = 0;
    for(int i = 0;i<v.size();i++){
        res += v[i].v;
    }
    return res;
}