#include "min.h"

namespace minimath{
    int Add(int a,int b){
        return a+b;
    }
    template int Min<int>(const int& a,const int& b);
}