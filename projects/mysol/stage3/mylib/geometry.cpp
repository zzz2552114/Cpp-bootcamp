#include<cmath>
#include"geometry.h"

namespace{
    int ABS(int x) {return abs(x);}
}

namespace mylib{
    int Add(int a,int b){
        return a + b;
    }

    int Sub(int a,int b){
        return ABS(a-b);
    }

    int Abs(int x){return ABS(x);}
}

namespace other{
    int Add(int a,int b){return a + b + 7; }
}