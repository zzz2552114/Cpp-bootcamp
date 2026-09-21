#include<vector>


struct Statistics{
    std::vector<int> data;
    inline static int copies = 0;
    Statistics() : data({}) {};

    Statistics(const Statistics &s1) : data(s1.data)
    {
        copies++;
    }
    Statistics& operator=(const Statistics& s1){
        data = s1.data;
        copies++;
        return *this;
    }

    Statistics(Statistics &&s2) noexcept : data(std::move(s2.data)) {}

    Statistics& operator=(Statistics&& s2) noexcept{
        if(this != &s2)
        {
            data = std::move(s2.data);
        }
        return *this;
    }

    void AddValue(int x)
    {
        data.push_back(x);
    }
    int Sum() const{
        int sum = 0;
        for(int ele : data)
            sum += ele;
        return sum;
    }

    long long SumL() const{
        long long suml = 0;
        for(int ele : data) suml += ele;
        return suml;
    }
    double Average() const {
        int n = (int)Size();
        return n ? (double)SumL()/n : 0; 
    }
    size_t Size() const{ return data.size();}
    bool Empty() const { return data.empty() ;}
};

int ReportSum(const Statistics& s){
    std::cout << s.copies;
    return s.Sum();
}

/*
int ReportSum1(const Statistics s)
{
    std::cout << s.copies;
    return s.Sum();
}
*/

// 如果是这样的话 copies 是 1，因为声明s之后，
// 把 s 传到这个函数里本质上是拷贝了一次，调用了拷贝构造