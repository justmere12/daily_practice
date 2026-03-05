#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <iostream>
#include <unordered_map>
#include <memory>
#include <optional>

template<typename K, typename V>
class LRUCache {
private:
    struct Node {
        K key;
        V value;
        std::shared_ptr<Node> prev;
        std::shared_ptr<Node> next;

        Node(const K& k, const V& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };

    size_t capacity_;
    std::unordered_map<K, std::shared_ptr<Node>> cache_;
    std::shared_ptr<Node> head_;
    std::shared_ptr<Node> tail_;

    void removeNode(const std::shared_ptr<Node>& node)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void addToHead(const std::shared_ptr<Node>& node)
    {
        node->next = head_->next;
        node->prev = head_;
        head_->next->prev = node;
        head_->next = node;
    }

    void moveToHead(const std::shared_ptr<Node>& node)
    {
        removeNode(node);
        addToHead(node);
    }

    std::shared_ptr<Node> removeTail()
    {
        std::shared_ptr<Node> node = tail_->prev;
        removeNode(node);
        return node;
    }

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity)
    {
        head_ = std::make_shared<Node>(K{}, V{});
        tail_ = std::make_shared<Node>(K{}, V{});
        head_->next = tail_;
        tail_->prev = head_;
    }

    std::optional<V> get(const K& key)
    {
        if (auto it = cache_.find(key); it != cache_.end()) {
            std::shared_ptr<Node> node = it->second;
            moveToHead(node);
            return node->value;
        }
        return std::nullopt;
    }

    void put(const K& key, const V& value)
    {
        if (auto it = cache_.find(key); it != cache_.end()) {
            std::shared_ptr<Node> node = it->second;
            node->value = value;
            moveToHead(node);
        } else {
            std::shared_ptr<Node> newNode = std::make_shared<Node>(key, value);
            cache_[key] = newNode;
            addToHead(newNode);

            if (cache_.size() > capacity_) {
                std::shared_ptr<Node> tail = removeTail();
                cache_.erase(tail->key);
            }
        }
    }

    bool erase(const K& key)
    {
        if (auto it = cache_.find(key); it != cache_.end()) {
            std::shared_ptr<Node> node = it->second;
            removeNode(node);
            cache_.erase(it);
            return true;
        }
        return false;
    }

    bool contains(const K& key) const
    {
        return cache_.find(key) != cache_.end();
    }

    size_t size() const
    {
        return cache_.size();
    }

    void clear()
    {
        cache_.clear();
        head_->next = tail_;
        tail_->prev = head_;
    }

    void print() const
    {
        std::cout << "LRU Cache (capacity: " << capacity_ << ", size = " << size() << "):";
        std::shared_ptr<Node> current = head_->next;
        while (current != tail_) {
            std::cout << "[" << current->key << ":" << current->value << "]";
            current = current->next;
        }
        std::cout << std::endl;
    }

};

#endif