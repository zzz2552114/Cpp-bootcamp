#include<string>

inline std::string Which(int& x){
    return "lvalue ref";
}

inline std::string Which(const int &x){
    return "const lvalue ref";
}
inline std::string Which(int &&x){
    return "rvalue ref";
}

inline std::string CallWithConst(int &x){
    const int& cx = x;
    return Which(cx);
}
