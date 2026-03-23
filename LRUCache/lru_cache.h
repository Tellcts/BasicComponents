// Requires C++17
#pragma once
#ifndef LRU_CACHE_H
  #define LRU_CACHE_H

  #include <cassert>
  #include <list>
  #include <type_traits>
  #include <unordered_map>

template <typename _Val>
struct defaultValDeleter {
    void operator()(_Val& __value) {
        if constexpr (std::is_pointer_v<_Val>) {
            delete __value;
        }
    }
};

template <typename _Key, typename _Val,
          typename _Deleter = defaultValDeleter<_Val>>
class LRUCache {
private:
    struct Node {
        _Key key;
        _Val value;
        int ref_count;
        typename std::list<Node>::iterator list_pos;

        Node(const _Key& __key, _Val&& __value)
            : key {__key}, value {std::move(__value)}, ref_count {1} {}

        Node(const _Key& __key, const _Val& __value)
            : key {__key}, value {__value}, ref_count {1} {}
    };

public:
    LRUCache(size_t __capacity = 0) : capacity_ {__capacity}, size_ {0} {}

    ~LRUCache() {
        assert(in_used_.empty());
        for (auto it = un_used_.begin(); it != un_used_.end();) {
            key_to_node_iter_.erase(it->key);
            deleter_(it->value);
            it = un_used_.erase(it);
            --size_;
        }
    }

    LRUCache(const LRUCache&) = delete;
    LRUCache(LRUCache&&) noexcept = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache& operator=(LRUCache&&) noexcept = delete;

    // RAII对象，自动管理 Node 的引用计数
    class NodeWrapper {
    public:
        NodeWrapper& operator=(const NodeWrapper&) = delete;
        NodeWrapper& operator=(NodeWrapper&&) noexcept = delete;
        NodeWrapper(const NodeWrapper&) = delete;

        NodeWrapper(NodeWrapper&& __other) noexcept
            : cache_ {__other.cache_}, node_ {__other.node_} {
            __other.node_ = nullptr;
            __other.cache_ = nullptr;
        }

        ~NodeWrapper() { reset(); }

        void reset() {
            if (is_valid()) {
                cache_->unref_node(node_);
                cache_ = nullptr;
                node_ = nullptr;
            }
        }

        _Val& value() const {
            assert(is_valid());
            return node_->value;
        }

        bool is_valid() const { return cache_ && node_; }

        explicit operator bool() const { return is_valid(); }

    private:
        LRUCache* cache_;
        Node* node_;
        friend class LRUCache;

        NodeWrapper() : cache_ {nullptr}, node_ {nullptr} {}

        explicit NodeWrapper(LRUCache* __cache, Node* __node)
            : cache_ {__cache}, node_ {__node} {
            if (cache_ && node_) {
                cache_->ref_node(node_);
            }
        }
    };

    NodeWrapper get(const _Key& __key) {
        auto it = key_to_node_iter_.find(__key);
        if (it != key_to_node_iter_.end()) {
            return NodeWrapper {this, &(*(it->second))};
        }
        return NodeWrapper {};
    }

    template <typename _V>
    NodeWrapper put(const _Key& __key, _V&& __value) {
        static_assert(std::is_constructible_v<_Val, _V>,
                      "_Val must be constructible from _V");
        auto it = key_to_node_iter_.find(__key);
        if (it != key_to_node_iter_.end()) {
            auto node_iter = it->second;
            node_iter->value = std::forward<_V>(__value);
            return NodeWrapper {this, &(*node_iter)};
        }
        auto node_iter =
            un_used_.emplace(un_used_.end(), __key, std::forward<_V>(__value));
        node_iter->list_pos = node_iter;
        ++size_;
        evict_if_needed();
        return NodeWrapper {this, &(*node_iter)};
    }

private:
    void ref_node(Node* __node) {
        if (__node->ref_count == 1) {
            in_used_.splice(in_used_.end(), un_used_, __node->list_pos);
        }
        ++__node->ref_count;
    }

    void unref_node(Node* __node) {
        assert(__node->ref_count > 1);
        if (--__node->ref_count == 1) {
            un_used_.splice(un_used_.end(), in_used_, __node->list_pos);
        }
    }

    void evict_if_needed() {
        if (capacity_ > 0) {
            while (size_ > capacity_ && ! un_used_.empty()) {
                auto node_iter = un_used_.begin();
                key_to_node_iter_.erase(node_iter->key);
                // Node& node = *node_iter;
                deleter_(node_iter->value);
                un_used_.erase(node_iter);
                --size_;
            }
        }
    }

    size_t capacity_;
    size_t size_;
    _Deleter deleter_;
    std::list<Node> in_used_;  // 被外部使用的缓存项链表
    std::list<Node> un_used_;  // 未被外部使用的缓存项链表
    using ListNodeIterator = typename std::list<Node>::iterator;
    std::unordered_map<_Key, ListNodeIterator> key_to_node_iter_;
};

#endif
