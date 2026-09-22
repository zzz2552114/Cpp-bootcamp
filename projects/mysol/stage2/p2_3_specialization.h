#include<string>


template<typename T> 
std::string Describe(const T&){
    return "default";
}
template <>
std::string Describe<double>(const double& s)
{
    return "double";
}

template <>
std::string Describe<const char*>(const char* const &s)
{
    return "c-string";
}

template <typename T> struct Printer{
    static std::string Name(){
        return "generic";
    };
};

template <>
struct Printer<float>
{
    static std::string Name()
    {
        return "float";
    };
};

template <>
struct Printer < std::string>
{
    static std::string Name()
    {
        return "string";
    };
};

template<size_t N>
struct FixedArray{
    int a[N] = {};
    constexpr size_t Size() const{
        return N;
    }
};


template<int T>
struct Constant{
    inline static const int value = T;
};

template<size_t N> constexpr
size_t Factorial(){
    return N*Factorial<N-1>();
}

template<> constexpr
size_t Factorial<0>(){
    return 1;
}

template<typename T> 
std::string ToString(const T& x){
    if constexpr (std::is_integral_v<T> ){
        return std::to_string(x);
    }
    else if constexpr (std::is_floating_point_v<T>){
        return std::to_string(x);
    }
    return "non-numeric";
}