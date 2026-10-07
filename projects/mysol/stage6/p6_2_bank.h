#include<mutex>


namespace bank6{
    class Account{
    public:
        explicit Account(int money):balance_(money){}
        void Transfer(Account& other,int money){
            if(this == &other) return ;
            std::scoped_lock lk(m_,other.m_);
            if(balance_ < money) return;
            balance_ -= money;
            other.balance_ += money;
        }

        int Balance() const{
            std::scoped_lock lk(m_);
            return balance_;
        }

        void Deposit(int money){
            std::scoped_lock lk(m_);
            balance_ += money;
        }

    private:
        int balance_ = 0;
        mutable std::mutex m_;

    };



    template<typename Container>
    long long TotalBalance(const Container& accts){
        long long res = 0;
        for(auto &t:accts){
            res += t.Balance();
        }
        return res;
    }
}