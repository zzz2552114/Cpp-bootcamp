#include<string>
#include<memory>
#include<unordered_map>


using std::string;
namespace reg5{
    
    struct User{
        inline static int live = 0;
        string name_;
        explicit User(string n):name_(std::move(n)){
            ++live;
        }

        ~User(){
            --live;
        }
        const string& Name() const{return name_;}
    };

    using sptr = std::shared_ptr<User>;



    class UserRegistry{
    public:
        void AddUser(int id,string name){
            sptr user = std::make_shared<User>(name);
            umap[id] = user;
        }

        sptr GetUser(int id){
            if(umap.find(id)!=umap.end()) return umap[id];
            return nullptr;
        }


        sptr GetUser(int id) const{
            if (umap.find(id) != umap.end())
                return umap.at(id);
            return nullptr;
        }

        long PeekUseCount(int id) const{
            if (umap.find(id) != umap.end())
                return umap.at(id).use_count();
            return -1;
        }

        bool RemoveUser(int id){
            if (umap.find(id) != umap.end()){
                umap.erase(id);
                return true;
            }
            return false;
        }

        size_t Count() const{
            return umap.size();
        }

        bool Empty() const{
            return umap.size()==0;
        }
    private:
        std::unordered_map<int, sptr> umap;
    };



} // namespace reg5