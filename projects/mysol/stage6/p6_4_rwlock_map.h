#include<mutex>
#include<shared_mutex>
#include<unordered_map>
#include<optional>
#include<iostream>

namespace rw6{
    class ConcurrentMap{
        public:
        void Put(int key,int value){
            std::unique_lock ulk(m_);
            umap_[key] = value;
        }

        bool Get(int key,int &value) const{
            std::shared_lock slk(m_);
            if(umap_.find(key)==umap_.end()) return 0;
            value = umap_.at(key);
            return 1;
        }

        std::optional<int> Get(int key) const{
            std::shared_lock slk(m_);
            if (umap_.find(key) == umap_.end())
                return std::nullopt;
            return umap_.at(key);
        }

        bool Remove(int key){
            std::unique_lock ulk(m_);
            if (umap_.find(key) == umap_.end())
                return 0;
            umap_.erase(key);
            return 1;
        }

        bool Contains(int key) const{
            std::shared_lock slk(m_);
            if (umap_.find(key) == umap_.end())
                return 0;
            return 1;
        } 

        size_t Size() const{
            std::shared_lock slk(m_);
            return umap_.size();
        }

        void Clear(){
            std::unique_lock ulk(m_);
            umap_.clear();
        }

        private:
        std::unordered_map<int,int> umap_;
        mutable std::shared_mutex m_;     

    };
}