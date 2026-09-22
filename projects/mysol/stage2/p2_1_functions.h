template<typename T>
T Min(const T& a,const T& b){
    return a < b ? a : b; 
}

template <typename T>
T Max(const T &a, const T &b)
{
    return a < b ? b : a;
}

template <typename T> constexpr
T MinC(const T &a, const T &b)
{
    return a < b ? a : b;
}

template<typename T,size_t N> constexpr
size_t ArrayLen(const T (&arr)[N]){
    return N;
}

// 上面这个函数实现了：传入一个数组，可以解析出数组长度，
// 并且会建立一个实例化函数
// 目的是获得数组长度，实际上数组长度没那么好获取。
// 具体内容参见 [对ArrayLen的解释](https://github.com/zzz2552114/Notes/blob/main/cmu15-445/Cpp-bootcamp/P2-%E5%BC%95%E7%94%A8%E6%95%B0%E7%BB%84%E4%B8%8Econstexpr.md)
