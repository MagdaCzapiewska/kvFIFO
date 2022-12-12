#ifndef __KVFIFO_H__
#define __KVFIFO_H__

#include <utility>
#include <tuple>
#include <cstdint>
#include <map>
#include <list>
#include <memory>
#include <stdexcept>
#include <iostream>


// lista samych wartości (tzn.par klucz, wartość)
// mapa klucz klucz, wartość: lista itratorów

template <typename K, typename V> class kvfifo {

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

    size_t size() const;
    size_t count(K const &) const;
    bool empty() const;

    void clear();

    /*
    class k_iterator() {
    private:
        std::map<K, std::list<typename std::list<std::pair<K, V>>::iterator>>::iterator to_map;

    public:

    }

    k_begin();
    */
    // Iterator k_iterator oraz metody k_begin i k_end
    // pozwalające przeglądać zbiór kluczy w rosnącej kolejności wartości

    typedef std::map<K, std::list<typename std::list<std::pair<K, V>>::iterator>> Map;
    typedef typename Map::iterator MapIterator;

    class k_iterator : public MapIterator
    {
    public:
        k_iterator() : MapIterator() {};
        k_iterator(MapIterator s) : MapIterator(s) {};
        K* operator->() { return (K* const)&(MapIterator::operator->()->first); }
        K operator*() { return MapIterator::operator*().first; }
    };

    k_iterator k_begin();
    k_iterator k_end();

private:
    std::shared_ptr<std::list<std::pair<K, V>>> queue;
    std::shared_ptr<std::map<K, std::list<typename std::list<std::pair<K, V>>::iterator>>> iters;
    bool modifiable_from_outside;

    void copy_if_needed();
};

template <typename K, typename V>
kvfifo<K, V>::kvfifo() {
    queue = std::make_shared<std::list<std::pair<K, V>>>();
    iters = std::make_shared<std::map<K, std::list<typename std::list<std::pair<K, V>>::iterator>>>();
    modifiable_from_outside = false;
}

template <typename K, typename V>
kvfifo<K, V>::kvfifo(kvfifo<K, V> const &other) : queue(other.queue), iters(other.iters){
    if (other.modifiable_from_outside) {
        copy_if_needed();
    }
    modifiable_from_outside = false;
}

template <typename K, typename V>
kvfifo<K, V>::kvfifo(kvfifo<K, V> &&) noexcept = default;

template <typename K, typename V>
kvfifo<K, V>::~kvfifo() noexcept = default;

template <typename K, typename V>
kvfifo<K, V>& kvfifo<K, V>::operator=(kvfifo<K, V> other) {
    if (other.modifiable_from_outside) {
        other.copy_if_needed();
    }
    queue = other.queue;
    iters = other.iters;
    modifiable_from_outside = false;
    return *this;
}

template <typename K, typename V>
void kvfifo<K, V>::copy_if_needed() {
    if (queue.use_count() == 1) {
        return;
    }

    auto queue_copy = std::make_shared<std::list<std::pair<K, V>>>(*queue);
    auto iters_copy = std::make_shared<std::map<K, std::list<typename std::list<std::pair<K, V>>::iterator>>>(*iters);

    for (auto it = queue_copy->begin(); it != queue_copy->end(); ++it) {
        auto pair_it = iters_copy->find(it->first);
        pair_it->second.pop_front();
        pair_it->second.push_back(it);
    }

    queue = queue_copy;
    iters = iters_copy;
}

template <typename K, typename V>
void kvfifo<K, V>::push(K const &k, V const &v) {
    this->copy_if_needed();
    queue->emplace_back(k, v);
    auto it = iters->end();
    bool key_created = false;
    try {
        std::tie(it, key_created) = iters->emplace(k, std::list<typename std::list<std::pair<K, V>>::iterator>());
        it->second.push_back(--queue->end());
    } catch (...) {
        if (key_created) {
            iters->erase(it);
        }
        queue->pop_back();
        throw;
    }
}

template <typename K, typename V>
void kvfifo<K, V>::pop() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }

    pop(queue->front().first);
}

template <typename K, typename V>
void kvfifo<K, V>::pop(K const &k) {
    auto it = iters->find(k);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    this->copy_if_needed();
    queue->erase(it->second.front());
    it->second.pop_front();
    if (it->second.empty()) {
        iters->erase(it);
    }
}

template <typename K, typename V>
void kvfifo<K, V>::move_to_back(K const &k) {
    auto it = iters->find(k);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    this->copy_if_needed();
    for (auto const &i : it->second) {
        queue->splice(queue->end(), *queue, i);
    }
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::front() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    this->copy_if_needed();
    modifiable_from_outside = true;
    return {queue->front().first, queue->front().second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::front() const {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    return {queue->front().first, queue->front().second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::back() {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    this->copy_if_needed();
    modifiable_from_outside = true;
    return {queue->back().first, queue->back().second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::back() const {
    if (queue->empty()) {
        throw std::invalid_argument("Queue is empty!");
    }
    return {queue->back().first, queue->back().second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::first(K const &key) {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    this->copy_if_needed();
    modifiable_from_outside = true;
    return {(*it->second.front()).first, (*it->second.front()).second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::first(K const &key) const {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    return {(*it->second.front()).first, (*it->second.front()).second};
}

template <typename K, typename V>
std::pair<K const &, V &> kvfifo<K, V>::last(K const &key) {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    this->copy_if_needed();
    modifiable_from_outside = true;
    return {(*it->second.back()).first, (*it->second.back()).second};
}

template <typename K, typename V>
std::pair<K const &, V const &> kvfifo<K, V>::last(K const &key) const {
    auto it = iters->find(key);
    if (it == iters->end()) {
        throw std::invalid_argument("No such key in the queue!");
    }
    return {(*it->second.back()).first, (*it->second.back()).second};
}

template <typename K, typename V>
size_t kvfifo<K, V>::size() const {
    return queue->size();
}

template <typename K, typename V>
size_t kvfifo<K, V>::count(K const &k) const {
    auto it = iters->find(k);
    return it == iters->end() ? 0 : it->second.size();
}

template <typename K, typename V>
bool kvfifo<K, V>::empty() const {
    return queue->empty();
}

template <typename K, typename V>
void kvfifo<K, V>::clear() {
    this->copy_if_needed();
    queue->clear();
    iters->clear();
}

template <typename K, typename V>
typename kvfifo<K, V>::k_iterator kvfifo<K, V>::k_begin() {

    return kvfifo<K, V>::k_iterator(iters->begin());
}

template <typename K, typename V>
typename kvfifo<K, V>::k_iterator kvfifo<K, V>::k_end() {
    return kvfifo<K, V>::k_iterator(iters->end());
}



#endif // __KVFIFO_H__



