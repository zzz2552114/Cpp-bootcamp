// P7.2 Mini BufferPool：固定帧 + free list + page table + 条件变量
//
// 范围（刻意收窄，不越纲）：
//   全内存；不做磁盘 I/O；不做替换策略（LRU/Clock）；不实现 pin count。
#include <array>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace bp7 {

struct Page {
  int page_id = -1;
  std::array<int, 8> data{};        // 随便塞点 payload
};

class MiniBufferPool {
public:
  explicit MiniBufferPool(size_t frame_count) : frames_(frame_count), free_list_(frame_count) {
    assert(frame_count >= 1);
    for (size_t i = 0; i < frame_count; ++i) free_list_[i] = frame_count - 1 - i;
  }

  // 命中 → 返回该帧；未命中 → 占一个空闲帧；没有空闲帧 → 等待
  //
  // ⚠️ 返回的 Page* 指向池内固定内存。本简化版没有 pin count，
  //    所以调用者拿到指针后如果别的线程 Unpin 了这个页、帧又被新页复用，
  //    指针内容就会变。真实 buffer pool 必须有 pin count + latch（15-445 P2 的功课）。
  Page *Pin(int page_id) {
    std::unique_lock<std::mutex> lk(m_);

    // ★ 关键：把"这一页已经被别人放进来了"也写进等待谓词。
    //   如果只在 wait 之前 find 一次，那么等待期间别的线程可能已经把这页装进池里，
    //   醒来后再分配一个帧并执行 `page_table_[page_id] = fid` 就会【覆盖】原来那条记录，
    //   被覆盖的那个帧既不在 free_list_ 也不在 page_table_ 里 → 帧泄漏。
    cv_.wait(lk, [this, page_id] {
      return page_table_.count(page_id) > 0 || !free_list_.empty();
    });

    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) return &frames_[it->second];   // 等待期间被装进来了

    const size_t fid = free_list_.back();
    free_list_.pop_back();

    frames_[fid].page_id = page_id;
    frames_[fid].data.fill(0);
    page_table_[page_id] = fid;
    return &frames_[fid];
  }

  // 简化语义：Unpin = "这个页用完了，回收帧"
  void Unpin(int page_id) {
    std::unique_lock<std::mutex> lk(m_);
    auto it = page_table_.find(page_id);
    if (it == page_table_.end()) return;                    // 不在池里 → 无害返回
    free_list_.push_back(it->second);
    page_table_.erase(it);
    lk.unlock();
    // 用 notify_all：等待者可能是"在等这个页"也可能是"在等空闲帧"，
    // 只唤醒一个有可能叫醒一个并不需要的人（简化实现里这样最稳）。
    cv_.notify_all();
  }

  size_t NumPages() const {
    std::lock_guard<std::mutex> lk(m_);
    return page_table_.size();
  }
  size_t FreeFrames() const {
    std::lock_guard<std::mutex> lk(m_);
    return free_list_.size();
  }

  // ★ 需要"同时读到两个量"时，必须在一个临界区里一次取完。
  //   分别调用 NumPages() 和 FreeFrames() 会在两次加锁之间被别人改掉状态，
  //   于是 NumPages()+FreeFrames() 可能 ≠ FrameCount（不是数据损坏，是观测方式错了）。
  struct Stats {
    size_t pages;
    size_t free_frames;
  };
  Stats GetStats() const {
    std::lock_guard<std::mutex> lk(m_);
    return {page_table_.size(), free_list_.size()};
  }
  size_t FrameCount() const { return frames_.size(); }
  bool HasPage(int page_id) const {
    std::lock_guard<std::mutex> lk(m_);
    return page_table_.find(page_id) != page_table_.end();
  }

private:
  std::vector<Page> frames_;
  std::vector<size_t> free_list_;
  std::unordered_map<int, size_t> page_table_;
  mutable std::mutex m_;
  std::condition_variable cv_;
};

}  // namespace bp7
