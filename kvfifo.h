#ifndef __KVFIFO_H__
#define __KVFIFO_H__

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

template <typename K, typename V>
class kvfifo {
private:
    using k_set = std::set<K>;
    using kv_queue = std::list<std::pair<typename k_set::iterator, V>>;
    using kv_map = std::map<K, std::list<typename kv_queue::iterator>>;

    std::shared_ptr<k_set> keys;
    std::shared_ptr<kv_queue> queue;
    std::shared_ptr<kv_map> iters;
    bool modifiable_from_outside;

    bool is_copy_needed() const noexcept;
    void copy_if_needed();
    kvfifo create_copy() const;

    void swap(kvfifo &other) noexcept;

public:
    kvfifo();
    kvfifo(kvfifo const &);
    kvfifo(kvfifo &&) noexcept;
    ~kvfifo() noexcept;

    kvfifo& operator=(kvfifo other);

    void push(K const &k, V const &v);

    void pop();
    void pop(K const &);

    void move_to_back(K const &k);

    std::pair<K const &, V &> front();
    std::pair<K const &, V const &> front() const;
    std::pair<K const &, V &> back();
    std::pair<K const &, V const &> back() const;

    std::pair<K const &, V &> first(K const &key);
    std::pair<K const &, V const &> first(K const &key) const;
    std::pair<K const &, V &> last(K const &key);
    std::pair<K const &, V const &> last(K const &key) const;

    size_t size() const noexcept;
    size_t count(K const &) const;
    bool empty() const noexcept;

    void clear();

    using k_iterator = k_set::const_iterator;

    k_iterator k_begin() const noexcept;
    k_iterator k_end() const noexcept;
};

template <typename K, typename V>
kvfifo<K, V>::kvfifo()
    : keys(std::make_shared<k_set>()),
      queue(std::make_shared<kv_queue>()),
      iters(std::make_shared<kv_map>()),
      modifiable_from_outside(false) {}

template <typename K, typename V>
kvfifo<K, V>::kvfifo(kvfifo<K, V> const &other)
    : keys(other.keys), 
      queue(other.queue), 
      iters(other.iters), 
      modifiable_from_outside(false) {
    if (other.modifiable_from_outside) {
        create_copy().swap(*this);
    }
}

template <typename K, typename V>
kvfifo<K, V>::kvfifo(kvfifo<K, V> &&) noexcept = default;

template <typename K, typename V>
kvfifo<K, V>::~kvfifo() noexcept = default;

template <typename K, typename V>
kvfifo<K, V>& kvfifo<K, V>::operator=(kvfifo<K, V> other) {
    swap(other);
    return *this;
}

template <typename K, typename V>
bool kvfifo<K, V>::is_copy_needed() const noexcept {
    return queue.use_count() > 1;
}

template <typename K, typename V>
void kvfifo<K, V>::copy_if_needed() {
    if (is_copy_needed()) {
        create_copy().swap(*this);
    }
}

template <typename K, typename V>
kvfifo<K, V> kvfifo<K, V>::create_copy() const {
    kvfifo<K, V> copy;
    copy.keys = std::make_shared<k_set>(*keys);
    copy.queue = std::make_shared<kv_queue>(*queue);
    copy.iters = std::make_shared<kv_map>(*iters);
    for (auto it = copy.queue->begin(); it != copy.queue->end(); ++it) {
        it->first = copy.keys->find(*it->first);
        auto map_it = copy.iters->find(*it->first);
        map_it->second.pop_front();
        map_it->second.push_back(it);
    }
    return copy;
}

template <typename K, typename V>
void kvfifo<K, V>::swap(kvfifo<K, V> &other) noexcept {
    other.keys.swap(keys);
    other.queue.swap(queue);
    other.iters.swap(iters);
    std::swap(other.modifiable_from_outside, modifiable_from_outside);
}

#include <iostream>
template <typename K, typename V>
void kvfifo<K, V>::push(K const &k, V const &v) {
    auto copy = is_copy_needed() ? create_copy() : *this;
    swap(copy);
    try {
        auto [key_it, key_inserted] = keys->insert(k);
        try {
            queue->emplace_back(key_it, v);
            try {
                auto [it, key_created] = iters->try_emplace(
                        k, std::list<typename kv_queue::iterator>());
                try {
                    it->second.push_back(--queue->end());
                } catch (...) {
                    if (key_created) {
                        iters->erase(it);
                    }
                    throw;
                }
            } catch (...) {
                queue->pop_back();
                throw;
            }
        } catch (...) {
            if (key_inserted) {
                keys->erase(key_it);
            }
            throw;
        }
    } catch (...) {
        swap(copy);
        throw;
    }
    modifiable_from_outside = false;
}

template <typename K, typename V>
void kvfifo<K, V>::pop() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    pop(*queue->front().first);
}

template <typename K, typename V>
void kvfifo<K, V>::pop(K const &k) {
    auto it = iters->find(k);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    if (is_copy_needed()) {
        auto copy = create_copy();
        it = copy.iters->find(k);
        swap(copy);
    }
    auto key_it = it->second.front()->first;
    queue->erase(it->second.front());
    it->second.pop_front();
    if (it->second.empty()) {
        iters->erase(it);
        keys->erase(key_it);
    }
    modifiable_from_outside = false;
}

template <typename K, typename V>
void kvfifo<K, V>::move_to_back(K const &k) {
    auto it = iters->find(k);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    if (is_copy_needed()) {
        auto copy = create_copy();
        it = copy.iters->find(k);
        swap(copy);
    }
    for (auto const &i : it->second) {
        queue->splice(queue->end(), *queue, i);
    }
    modifiable_from_outside = false;
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::front() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    copy_if_needed();
    modifiable_from_outside = true;
    return {*queue->front().first, queue->front().second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::front() const {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    return {*queue->front().first, queue->front().second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::back() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    copy_if_needed();
    modifiable_from_outside = true;
    return {*queue->back().first, queue->back().second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::back() const {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    return {*queue->back().first, queue->back().second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::first(K const &key) {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    if (is_copy_needed()) {
        auto copy = create_copy();
        it = copy.iters->find(key);
        swap(copy);
    }
    modifiable_from_outside = true;
    return {*it->second.front()->first, it->second.front()->second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::first(K const &key) const {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    return {*it->second.front()->first, it->second.front()->second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::last(K const &key) {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    if (is_copy_needed()) {
        auto copy = create_copy();
        it = copy.iters->find(key);
        swap(copy);
    }
    modifiable_from_outside = true;
    return {*it->second.back()->first, it->second.back()->second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::last(K const &key) const {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    return {*it->second.back()->first, it->second.back()->second};
}

template <typename K, typename V>
size_t kvfifo<K, V>::size() const noexcept {
    return queue->size();
}

template <typename K, typename V>
size_t kvfifo<K, V>::count(K const &k) const {
    auto it = iters->find(k);
    return it == iters->end() ? 0 : it->second.size();
}

template <typename K, typename V>
bool kvfifo<K, V>::empty() const noexcept {
    return queue->empty();
}

template <typename K, typename V>
void kvfifo<K, V>::clear() {
    kvfifo<K, V>().swap(*this);
}

template <typename K, typename V>
typename kvfifo<K, V>::k_iterator kvfifo<K, V>::k_begin() const noexcept {
    return keys->cbegin();
}

template <typename K, typename V>
typename kvfifo<K, V>::k_iterator kvfifo<K, V>::k_end() const noexcept {
    return keys->cend();
}

#endif  // __KVFIFO_H__
