#include<iostream>
#include<vector>

struct Statistics{
    std::vector<int> datas;

    void AddValue(int x){
        datas.push_back(x);
    }

    int Sum() const {
        int sum = 0;
        for(auto x:datas){
            sum += x;
        }
        return sum;
    }

    long long SumL() const{
        long long sum = 0;
        for(auto x:datas){
            sum +=x;
        }
        return sum;
    }

    double Average() const{
        int n = (int)datas.size();
        return n ? (double)SumL()/n : 0;
    }

};

int main(){
    Statistics s;
    const Statistics& s1 = s;
    int n = 50;
    for(int i = 1;i<=n;i++){
        s.AddValue(i);
    }

    std::cout <<"s-sum = " << s.Sum() << '\n';
    std::cout << "s-average=  " << s.Average() << '\n';
    std::cout << "s1-sum = " << s1.Sum() << '\n';
    std::cout << "s1-average=  " << s1.Average() << '\n';

    return 0;
}