//
// Created by nanuns on 2020/6/7.
//

#ifndef NANGBD_AVQUEUE_CPP
#define NANGBD_AVQUEUE_CPP

#include <deque>
#include <memory>
#include <mutex>
#include <condition_variable>

/**
 * 线程安全队列
 */
template<typename T>
class AVQueue {
public:
    AVQueue() {}

    AVQueue(AVQueue const &other) {
        std::lock_guard<std::mutex> lk(other.mtx);
        que = other.que;
    }

    //入队操作
    void push(T value) {
        std::lock_guard<std::mutex> lk(mtx);
        que.push_back(value);
        cond.notify_one();
    }

    void push_front(T value) {
        std::lock_guard<std::mutex> lk(mtx);
        que.push_front(value);
        cond.notify_one();
    }

    //直到有元素可以删除为止
    void wait_and_pop(T &value) {
        std::unique_lock<std::mutex> lk(mtx);
        cond.wait(lk, [this] { return !que.empty(); });
        if (que.empty()) {
            return;
        }
        value = std::move(que.front());
        que.pop_front();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mtx);
        cond.wait(lk, [this] { return !que.empty(); });
        if (que.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(std::move(que.front())));
        que.pop_front();
        return res;
    }

    //不管有没有队首元素直接返回
    bool try_pop(T &value) {
        std::lock_guard<std::mutex> lk(mtx);
        if (que.empty())
            return false;
        value = std::move(que.front());
        que.pop_front();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mtx);
        if (que.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(std::move(que.front())));
        que.pop_front();
        return res;
    }

    bool front(T &value) {
        std::lock_guard<std::mutex> lk(mtx);
        if (que.empty())
            return false;
        value = que.front();
        return true;
    }

    bool back(T &value) {
        std::lock_guard<std::mutex> lk(mtx);
        if (que.empty())
            return false;
        value = que.back();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lk(mtx);
        return que.empty();
    }

    int size() {
        std::lock_guard<std::mutex> lk(mtx);
        return que.size();
    }

    bool at(int index, T &value) {
        std::lock_guard<std::mutex> lk(mtx);
        if (que.empty())
            return false;
        if (index < 0 || index >= que.size()) {
            return false;
        }
        value = que.at(index);
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lk(mtx);
        std::deque<T> empty;
        swap(empty, que);
    }

private:
    mutable std::mutex mtx;
    mutable std::condition_variable cond;
    std::deque<T> que;
};

#endif // NANGBD_AVQUEUE_CPP
