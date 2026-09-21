#include<vector>

class Counted{
    public:
        inline static int copies = 0,moves = 0;
        explicit Counted(int v) : v(v)
        {}

        Counted(const Counted &other):v(other.v)
        {
            ++copies;
        }

        Counted(Counted &&other) noexcept : v(std::move(other.v))
        {
            ++moves;
        }

        static void Reset(){
            copies = 0,moves = 0;
        }
        int v;
};

class CountedThrowy
{
public:
    inline static int copies = 0, moves = 0;
    explicit CountedThrowy(int v) : v(v)
    {}

    CountedThrowy(const CountedThrowy &other) : v(other.v)
    {
        ++copies;
    }

    CountedThrowy(CountedThrowy &&other) : v(other.v)
    {
        ++moves;
    }

    static void Reset()
    {
        copies = 0, moves = 0;
    }

    int v;
};

class MoveOnlyThrowy
{
public:
    inline static int copies = 0, moves = 0;
    explicit MoveOnlyThrowy(int v) : v(v)
    {}

    MoveOnlyThrowy(const MoveOnlyThrowy &other) = delete;
    MoveOnlyThrowy& operator=(const MoveOnlyThrowy& other) = delete;
    MoveOnlyThrowy(MoveOnlyThrowy &&other) : v(std::move(other.v))
    {
        ++moves;
    }

    static void Reset()
    {
        moves = 0;
    }

    int v;
};

template<typename T>
std::vector<T> GrowVector(int n){
    std::vector<T> vec;
    for(int i = 0;i<n;i++){
        vec.push_back(T(i));
    }
    return vec;
}