namespace minimath{
    template<typename T>
    T Min(const T& a,const T& b){
        return a < b ? a : b; 
    }
    int Add(int a,int b);

    template<typename T> 
    class Box{
        public:
            explicit Box(T x):v(x){};

            const T& Get() const{
                return v;
            }
            
        private:
            T v;
    };
}