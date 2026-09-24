#pragma once
#include<memory>


namespace r3{
    inline static int g_allocs = 0,g_frees = 0;
    static void ResetCounters(){
        g_allocs = 0, g_frees = 0;
    }



    struct N{
        int v = 0;
        N* next = nullptr;
        N() = default;
        explicit N(int val) : v(val){++g_allocs;}
        ~N(){
            ++g_frees;
        }
    };


    class NaiveList{
        public:
            N* head = nullptr;
            void Push(int v){
                N* hd = new N(v);
                if(head){
                    hd->next = head;
                }
                head = hd;
            }
            ~NaiveList(){
                N* curr = head;
                while(curr){
                    N* tmp = curr->next;
                    delete curr;
                    curr = tmp;
                }
                head = nullptr;
            }
    };

    class List3{
    public:
        N *head = nullptr;
        List3() = default;

        List3(const List3& other):head(nullptr){
            if(!other.head) return;
            head = new N(other.head->v);
            N* curr = head;
            N* ocurr = other.head->next;
            while(ocurr){
                N* tmp = new N(ocurr->v);
                curr->next = tmp;
                curr = curr->next;
                ocurr = ocurr->next;
            }
            curr->next = nullptr;
        }

        List3& operator=(const List3& other){
            if(this != &other){
                head = new N(other.head->v);
                N *curr = head;
                N *ocurr = other.head->next;
                while (ocurr)
                {
                    N *tmp = new N(ocurr->v);
                    curr->next = tmp;
                    curr = curr->next;
                    ocurr = ocurr->next;
                }
                curr->next = nullptr;
            }
            return *this;
        }

        void Push(int v)
        {
            N *hd = new N(v);
            if (head)
                hd->next = head;
            head = hd;
        }

        const N* Head() const {return head;}

        

        ~List3()
        {
            N *curr = head;
            while (curr)
            {
                N *tmp = curr->next;
                delete curr;
                curr = tmp;
            }
        }
    };

    class List5
    {
    public:
        N *head = nullptr;
        List5() = default;

        List5(const List5 &other)
        {
            if (!other.head)
                return;
            head = new N(other.head->v);
            N *curr = head;
            N *ocurr = other.head->next;
            while (ocurr)
            {
                N *tmp = new N(ocurr->v);
                curr->next = tmp;
                curr = curr->next;
                ocurr = ocurr->next;
            }
            curr->next = nullptr;
        }

        List5 &operator=(const List5 &other)
        {
            if (this != &other)
            {
                head = new N(other.head->v);
                N *curr = head;
                N *ocurr = other.head->next;
                while (ocurr)
                {
                    N *tmp = new N(ocurr->v);
                    curr->next = tmp;
                    curr = curr->next;
                    ocurr = ocurr->next;
                }
                curr->next = nullptr;
            }
            return *this;
        }

        List5(List5&& other) noexcept:head(other.head){other.head = nullptr;}
        List5& operator=(List5&& other) noexcept{
            if(this!=&other){
                Clear();
                head = other.head;
                other.head = nullptr;
            }
            return *this;
        }


        void Push(int v)
        {
            N *hd = new N(v);
            if (head)
                hd->next = head;
            head = hd;
        }

        const N *Head() const { return head; }
        N* Head() {return head;}

        void Clear(){
            N *curr = head;
            while (curr)
            {
                N *tmp = curr->next;
                delete curr;
                curr = tmp;
            }
            head = nullptr;
        }
        ~List5(){Clear();}
    };

    class List0{
    public:
        struct N{
            int val;
            std::unique_ptr<N> next;
            explicit N(int x):val(x),next(nullptr){++g_allocs;}
            ~N(){
                ++g_frees;
            }
        };
    
        
        void Push(int x){
            std::unique_ptr<N> hd = std::make_unique<N>(x);
            if(head){
                hd->next = std::move(head);
            }
            head = std::move(hd);
            ++size_;
        }

        int Head() const{
            return head->val;
        }
        bool Empty() const{
            if(head==nullptr) return 1;
            return 0;
        }
        size_t Size() const {
            return size_;
        }


    private:
        std::unique_ptr<N> head = nullptr;
        size_t size_ = 0;
    };
}