#include<string>
#include<memory>


using std::string;

namespace weak5{
    inline static int g_live = 0;
    static void ResetLive(){
        g_live = 0;
    }


    struct BadNode{
        string name;
        std::shared_ptr<BadNode> next;
        explicit BadNode(string n):name(n){++g_live;}
        ~BadNode(){--g_live;}
    };


    struct GoodNode{
        string name;
        std::shared_ptr<GoodNode> next;
        std::weak_ptr<GoodNode> prev;
        explicit GoodNode(string n):name(n){++g_live;}
        ~GoodNode(){--g_live;}
    };


    struct Child;

    struct Parent{
        string name;
        std::shared_ptr<Child> child;
        explicit Parent(string n):name(n){++g_live;}
        ~Parent(){--g_live;}
    };

    struct Child{
        string name;
        std::weak_ptr<Parent> parent;
        explicit Child(string n):name(n){++g_live;}
        ~Child(){--g_live;}
    };

}