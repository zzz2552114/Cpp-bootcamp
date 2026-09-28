#pragma once
#include<memory>

namespace own5{
    
    struct Widget;
    using uptr = std::unique_ptr<Widget>;
    struct Widget{
        inline static int live = 0;
        int v;
        explicit Widget(int x):v(x){++live;}
        ~Widget(){--live;}
    };

    void Borrow (uptr &up){
        ++(up->v); 
    }

    int Observe(const Widget* raw){
        // 这里意思是，这只是一个裸指针，不负责管理只负责观察
        /*
        例如：
        std::unique_ptr<Widget> up = std::make_unique<Widget>(10);
        Observe(up.get());  // up.get() 只是把地址借给 Observe 看看
        */
        return raw ? raw->v : -1;
    }

    uptr Take(uptr up){
        uptr res;
        res.reset(up.release());
        return res;
    }

    uptr Make(int v){
        uptr res = std::make_unique<Widget>(v);
        return res;
    }

    void ResetKeepingSamePointer(std::unique_ptr<Widget> &up){
        auto tmp = up.release();
        up.reset(tmp);
    }

    int TakeAndDestroy(std::unique_ptr<Widget> up){
        return up->v;
    }
}