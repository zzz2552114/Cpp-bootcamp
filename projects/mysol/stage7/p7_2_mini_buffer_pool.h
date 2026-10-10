#include<array>
#include<vector>
#include<unordered_map>
#include<mutex>
#include<condition_variable>


namespace bp7{
    struct Page{
        int page_id = -1;
        std::array<int,8> data{0};
    };

    class MiniBufferPool{
        public:
        explicit MiniBufferPool(size_t frame_count):count_(frame_count){
            frames_.resize(count_);
            for(size_t i = 0;i < count_;i++){
                free_.push_back(i);
            }
        }

        // free_是空闲的帧
        Page* Pin(int page_id){
            std::unique_lock ulk(m_);
            if(umap_.find(page_id)!=umap_.end()) return &frames_[umap_[page_id]];
            else if(free_.empty()){
                cv.wait(ulk,[&]{return !free_.empty() || umap_.find(page_id)!=umap_.end();});
            }
            if (umap_.find(page_id) != umap_.end())
                return &frames_[umap_[page_id]];
            size_t pos = free_.back();
            free_.pop_back();
            umap_[page_id] = pos;
            frames_[pos].page_id = page_id;
            frames_[pos].data.fill(0);
            return &frames_[pos];
        }

        void Unpin(int page_id){
            std::unique_lock ulk(m_);
            if(umap_.find(page_id)==umap_.end()) return ;
            size_t pos = umap_[page_id];
            umap_.erase(page_id);
            free_.push_back(pos);
            cv.notify_all();
        }


        size_t NumPages() const{
            std::unique_lock ulk(m_);
            return umap_.size();
        }

        size_t FreeFrames() const{
            std::unique_lock ulk(m_);
            return free_.size();
        }

        size_t FrameCount() const{
            return count_;
        }

        bool HasPage(int page_id) const{
            std::unique_lock ulk(m_);
            if (umap_.find(page_id) != umap_.end())
                return 1;
            return 0;
        }


        struct Stats{
            size_t pages,free_frames;
        };

        Stats GetStats() const{
            std::unique_lock ulk(m_);
            return {umap_.size(),free_.size()};
        }


        private:
        // frames_里面放Page，大小是count_
        // free 里面放空闲的frame_的空，也就是下标
        size_t count_;
        std::vector<Page> frames_;
        std::vector<size_t> free_;

        std::unordered_map<int,size_t> umap_;
        mutable std::mutex m_;
        std::condition_variable cv;
    };




}