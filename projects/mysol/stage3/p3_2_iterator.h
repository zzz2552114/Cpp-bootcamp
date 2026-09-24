#pragma once

struct Node{
    int val;
    Node* prev;
    Node* next;
    explicit Node(int v) : val(v), prev(nullptr), next(nullptr) {}
};

class DLLIterator{
    public:
        DLLIterator(Node* curr,Node* tail):curr(curr),tail(tail){}

        int& operator*(){
            return curr->val;
        }
        const int& operator*() const{
            return curr->val;
        }
        DLLIterator& operator++(){
            curr = curr->next;
            return *this;
        }

        DLLIterator operator++(int){
            DLLIterator tmp = *this;
            curr = curr -> next;
            return tmp;
        }
        
        
        DLLIterator& operator--(){
            if(!curr) curr = tail;
            else curr = curr->prev;
            return *this;
        }

        DLLIterator operator--(int){
            DLLIterator tmp = *this;
            if(!curr) curr = tail;
            else curr = curr -> prev;
            return tmp;
        }

        bool operator==(const DLLIterator& other){
            if(curr == other.curr)
                return 1;
            return 0;
        }

        bool operator!=(const DLLIterator &other)
        {
            return ! (*this == other);
        }

        Node* Raw() const{
            return curr;
        }

        Node* curr;
        Node* tail;
};


class DLL{
    public:

        DLL():head(),tail(),size_(0){};
        void InsertAtHead(int v){
            Node* t = new Node(v);
            if(!head){
                head = t;
                tail = t;
            }
            else{
                t->next = head;
                head->prev = t;
                head = t;
            }
            ++size_;
        }

        DLL(const DLL&) = delete;
        DLL& operator=(const DLL&) = delete;
        DLLIterator Begin(){
            return DLLIterator(head,tail);
        }
        DLLIterator End(){
            return DLLIterator(nullptr,tail);
        }

        DLLIterator begin(){
            return Begin();
        }

        DLLIterator end(){
            return End();
        }

        size_t Size() const {return size_;}
        bool Empty() const {return size_==0;}
        int Front() const {
            return head -> val;
        }

        int Back() const{
            return tail -> val;
        }

        ~DLL(){
            DLLIterator del = Begin();
            while(del.curr){
                Node* tmp = del.curr;
                ++del;
                delete tmp;
            }
            head = nullptr;
        }

    private:
        Node* head;
        Node* tail;
        size_t size_ = 0;
};