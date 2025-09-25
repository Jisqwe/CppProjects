#pragma once
#include <memory>
#include <iterator>
#include <type_traits>
#include <utility>
#include <new>
#include <limits>
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <initializer_list>
#include <concepts>

template<typename T, std::size_t capacity = 10, typename Allocator = std::allocator<T>>
class unrolled_list {
    struct Node {
        std::size_t size{};
        Node* next{};
        Node* prev{};
        char buffer[capacity * sizeof(T)];

        T* data() noexcept {
            return reinterpret_cast<T*>(buffer);
        }

        const T* data() const noexcept {
            return reinterpret_cast<const T*>(buffer);
        }

        Node() noexcept = default;

        ~Node() {
            for (std::size_t i = 0; i < size; ++i) {
                data()[i].~T();
            }
        }
    };

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAT = std::allocator_traits<NodeAllocator>;
    using ValAT = std::allocator_traits<Allocator>;

    NodeAllocator node_alloc_;
    Allocator value_alloc_;
    std::size_t size_ = 0;
    Node* head_ = nullptr;
    Node* tail_ = nullptr;

    Node* create_new_node() {
        Node* n = NodeAT::allocate(node_alloc_, 1);
        try {
            NodeAT::construct(node_alloc_, n);
        } catch (...) {
            NodeAT::deallocate(node_alloc_, n, 1);
            throw;
        }
        return n;
    }

    void destroy_node(Node* n) noexcept {
        if (!n) {
            return;
        }
        NodeAT::destroy(node_alloc_, n);
        NodeAT::deallocate(node_alloc_, n, 1);
    }

    void link_after(Node* where, Node* n) noexcept {
        n->prev = where;
        n->next = where ? where->next : nullptr;
        if (where) {
            if (where->next) {
                where->next->prev = n;
            }
            where->next = n;
        }
        if (!where) {
            head_ = tail_ = n;
        } else if (where == tail_) {
            tail_ = n;
        }
    }

    void link_before(Node* where, Node* n) noexcept {
        n->next = where;
        n->prev = where ? where->prev : nullptr;
        if (where) {
            if (where->prev) {
                where->prev->next = n;
            }
            where->prev = n;
        }
        if (!where) {
            head_ = tail_ = n;
        } else if (where == head_) {
            head_ = n;
        }
    }

    void unlink_and_destroy(Node* n) noexcept {
        Node* prev_node = n->prev;
        Node* next_node = n->next;
        if (prev_node) {
            prev_node->next = next_node;
        } else {
            head_ = next_node;
        }
        if (next_node) {
            next_node->prev = prev_node;
        } else {
            tail_ = prev_node;
        }
        destroy_node(n);
    }

    void shift_right(Node* node, std::size_t from, std::size_t count = 1) {
        if (count == 0) {
            return;
        }
        if (node->size + count > capacity) {
            throw std::length_error("node overflow");
        }
        std::size_t old_size = node->size;
        try {
            for (std::size_t i = 0; i < old_size - from; ++i) {
                std::size_t src_idx = old_size - 1 - i;
                std::size_t dst_idx = src_idx + count;
                T* dst = std::addressof(node->data()[dst_idx]);
                ValAT::construct(value_alloc_, dst, std::move_if_noexcept(node->data()[src_idx]));
            }
        } catch (...) {
            for (std::size_t i = 0; i < old_size - from; ++i) {
                std::size_t idx = old_size - 1 - i + count;
                if (idx >= capacity) {
                    break;
                }
                node->data()[idx].~T();
            }
            throw;
        }
        for (std::size_t i = from; i < old_size; ++i) {
            node->data()[i].~T();
        }
        node->size += count;
    }

    void shift_left(Node* node, std::size_t from, std::size_t count = 1) {
        if (count == 0) {
            return;
        }
        std::size_t old_size = node->size;
        for (std::size_t i = from + count; i < old_size; ++i) {
            node->data()[i - count] = std::move(node->data()[i]);
        }
        for (std::size_t i = 0; i < count; ++i) {
            node->data()[old_size - 1 - i].~T();
        }
        node->size -= count;
    }

    Node* split_node(Node* node) {
        Node* new_node = create_new_node();
        link_after(node, new_node);
        const std::size_t mid = node->size / 2;
        const std::size_t move_count = node->size - mid;
        try {
            for (std::size_t i = 0; i < move_count; ++i) {
                T* src = std::addressof(node->data()[mid + i]);
                T* dst = std::addressof(new_node->data()[i]);
                ValAT::construct(value_alloc_, dst, std::move_if_noexcept(*src));
                new_node->size += 1;
            }
        } catch (...) {
            for (std::size_t i = 0; i < new_node->size; ++i) {
                new_node->data()[i].~T();
            }
            new_node->size = 0;
            Node* prev_node = new_node->prev;
            Node* next_node = new_node->next;
            if (prev_node) {
                prev_node->next = next_node;
            } else {
                head_ = next_node;
            }
            if (next_node) {
                next_node->prev = prev_node;
            } else {
                tail_ = prev_node;
            }
            destroy_node(new_node);
            throw;
        }
        for (std::size_t i = mid; i < node->size; ++i) {
            node->data()[i].~T();
        }
        node->size = mid;
        return new_node;
    }

public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename ValAT::pointer;
    using const_pointer = typename ValAT::const_pointer;

    template<typename Category, typename Type>
    struct base_iterator {
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_const_t<Type>;
        using pointer = Type*;
        using reference = Type&;
        using iterator_category = Category;
    };

    class iterator : public base_iterator<std::bidirectional_iterator_tag, T> {
        Node* node_{};
        std::size_t pos_{};
        Node* tail_{};
    public:
        iterator() noexcept = default;

        iterator(Node* n, std::size_t prev_node, Node* t) noexcept : node_(n), pos_(prev_node), tail_(t) {}

        T& operator*() const {
            return node_->data()[pos_];
        }

        T* operator->() const {
            return std::addressof(node_->data()[pos_]);
        }

        iterator& operator++() {
            if (!node_) {
                return *this;
            }
            if (++pos_ == node_->size) {
                node_ = node_->next;
                pos_ = 0;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        iterator& operator--() {
            if (!node_) {
                node_ = tail_;
                if (node_) {
                    pos_ = node_->size ? node_->size - 1 : 0;
                }
                return *this;
            }
            if (pos_ == 0) {
                node_ = node_->prev;
                if (node_) {
                    pos_ = node_->size ? node_->size - 1 : 0;
                }
            } else {
                --pos_;
            }
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const iterator& r) const {
            return node_ == r.node_ && pos_ == r.pos_;
        }

        bool operator!=(const iterator& r) const {
            return !(*this == r);
        }

        Node* get_node() const noexcept {
            return node_;
        }

        std::size_t get_pos() const noexcept {
            return pos_;
        }

        Node* get_tail_node() const noexcept {
            return tail_;
        }
    };

    class const_iterator : public base_iterator<std::bidirectional_iterator_tag, const T> {
        const Node* node_{};
        std::size_t pos_{};
        const Node* tail_{};
    public:
        const_iterator() noexcept = default;

        const_iterator(const Node* n, std::size_t prev_node, const Node* t) noexcept : node_(n), pos_(prev_node), tail_(t) {}

        const_iterator(const iterator& it) noexcept : node_(it.get_node()), pos_(it.get_pos()), tail_(it.get_tail_node()) {}

        const T& operator*() const {
            return node_->data()[pos_];
        }

        const T* operator->() const {
            return std::addressof(node_->data()[pos_]);
        }

        const_iterator& operator++() {
            if (!node_) {
                return *this;
            }
            if (++pos_ == node_->size) {
                node_ = node_->next;
                pos_ = 0;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_iterator& operator--() {
            if (!node_) {
                node_ = tail_;
                if (node_) {
                    pos_ = node_->size ? node_->size - 1 : 0;
                }
                return *this;
            }
            if (pos_ == 0) {
                node_ = node_->prev;
                if (node_) {
                    pos_ = node_->size ? node_->size - 1 : 0;
                }
            } else {
                --pos_;
            }
            return *this;
        }

        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const const_iterator& r) const {
            return node_ == r.node_ && pos_ == r.pos_;
        }

        bool operator!=(const const_iterator& r) const {
            return !(*this == r);
        }

        const Node* get_node() const noexcept {
            return node_;
        }

        std::size_t get_pos() const noexcept {
            return pos_;
        }

        const Node* get_tail_node() const noexcept {
            return tail_;
        }
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    explicit unrolled_list(const Allocator& a = Allocator()) : node_alloc_(a), value_alloc_(a) {}

    unrolled_list(size_type n, const T& value = T(), const Allocator& a = Allocator()) : node_alloc_(a), value_alloc_(a) {
        for (size_type i = 0; i < n; ++i) {
            push_back(value);
        }
    }

    unrolled_list(std::initializer_list<T> il, const Allocator& a = Allocator()) : node_alloc_(a), value_alloc_(a) {
        for (const auto& v : il) {
            push_back(v);
        }
    }

    template<class InputIt>
    unrolled_list(InputIt first, InputIt last, const Allocator& a = Allocator()) : node_alloc_(a), value_alloc_(a) {
        try {
            for (; first != last; ++first) {
                push_back(*first);
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    unrolled_list(const unrolled_list& other) : node_alloc_(other.node_alloc_), value_alloc_(other.value_alloc_) {
        for (const auto& v : other) {
            push_back(v);
        }
    }

    unrolled_list(unrolled_list&& other) noexcept : node_alloc_(std::move(other.node_alloc_)), value_alloc_(std::move(other.value_alloc_)), 
        size_(other.size_), head_(other.head_), tail_(other.tail_) {
        other.size_ = 0;
        other.head_ = nullptr;
        other.tail_ = nullptr;
    }

    unrolled_list(unrolled_list&& other, const Allocator& a) : node_alloc_(a), value_alloc_(a) {
        if (a == other.value_alloc_) {
            size_ = other.size_;
            head_ = other.head_;
            tail_ = other.tail_;
            other.size_ = 0;
            other.head_ = nullptr;
            other.tail_ = nullptr;
        } else {
            for (auto& v : other) {
                push_back(std::move(v));
            }
            other.clear();
        }
    }

    ~unrolled_list() {
        clear();
    }

    unrolled_list& operator=(const unrolled_list& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        value_alloc_ = other.value_alloc_;
        node_alloc_ = other.node_alloc_;
        for (const auto& v : other) {
            push_back(v);
        }
        return *this;
    }

    unrolled_list& operator=(unrolled_list&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        clear();
        value_alloc_ = std::move(other.value_alloc_);
        node_alloc_ = std::move(other.node_alloc_);
        size_ = other.size_;
        head_ = other.head_;
        tail_ = other.tail_;
        other.size_ = 0;
        other.head_ = nullptr;
        other.tail_ = nullptr;
        return *this;
    }

    unrolled_list& operator=(std::initializer_list<T> il) {
        unrolled_list tmp(il, value_alloc_);
        swap(tmp);
        return *this;
    }

    void swap(unrolled_list& other) noexcept {
        using std::swap;
        swap(value_alloc_, other.value_alloc_);
        swap(node_alloc_, other.node_alloc_);
        swap(size_, other.size_);
        swap(head_, other.head_);
        swap(tail_, other.tail_);
    }

    allocator_type get_allocator() const noexcept {
        return value_alloc_;
    }

    iterator begin() noexcept {
        return iterator(head_, 0, tail_);
    }

    iterator end() noexcept {
        return iterator(nullptr, 0, tail_);
    }

    const_iterator begin() const noexcept {
        return const_iterator(head_, 0, tail_);
    }

    const_iterator end() const noexcept {
        return const_iterator(nullptr, 0, tail_);
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(head_, 0, tail_);
    }

    const_iterator cend() const noexcept {
        return const_iterator(nullptr, 0, tail_);
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    size_type size() const noexcept {
        return size_;
    }

    size_type max_size() const noexcept {
        const auto max_nodes = NodeAT::max_size(node_alloc_);
        if (max_nodes == 0) {
            return 0;
        }
        const auto by_nodes = static_cast<size_type>(max_nodes) * capacity;
        return (std::min)(by_nodes, std::numeric_limits<size_type>::max());
    }

    reference front() {
        if (empty()) {
            throw std::out_of_range("unrolled_list::front: empty");
        }
        return head_->data()[0];
    }

    const_reference front() const {
        if (empty()) {
            throw std::out_of_range("unrolled_list::front: empty");
        }
        return head_->data()[0];
    }

    reference back() {
        if (empty()) {
            throw std::out_of_range("unrolled_list::back: empty");
        }
        return tail_->data()[tail_->size - 1];
    }

    const_reference back() const {
        if (empty()) {
            throw std::out_of_range("unrolled_list::back: empty");
        }
        return tail_->data()[tail_->size - 1];
    }

    void push_back(const T& v) {
        if (!tail_ || tail_->size == capacity) {
            Node* n = create_new_node();
            if (!head_) {
                head_ = n;
                tail_ = n;
            } else {
                tail_->next = n;
                n->prev = tail_;
                tail_ = n;
            }
        }
        T* prev_node = std::addressof(tail_->data()[tail_->size]);
        ValAT::construct(value_alloc_, prev_node, v);
        tail_->size += 1;
        size_ += 1;
    }

    void push_back(T&& v) {
        if (!tail_ || tail_->size == capacity) {
            Node* n = create_new_node();
            if (!head_) {
                head_ = n;
                tail_ = n;
            } else {
                tail_->next = n;
                n->prev = tail_;
                tail_ = n;
            }
        }
        T* prev_node = std::addressof(tail_->data()[tail_->size]);
        ValAT::construct(value_alloc_, prev_node, std::move(v));
        tail_->size += 1;
        size_ += 1;
    }

    void push_front(const T& v) {
        if (!head_ || head_->size == capacity) {
            Node* n = create_new_node();
            link_before(head_, n);
        }
        shift_right(head_, 0, 1);
        ValAT::construct(value_alloc_, std::addressof(head_->data()[0]), v);
        size_ += 1;
    }

    void push_front(T&& v) {
        if (!head_ || head_->size == capacity) {
            Node* n = create_new_node();
            link_before(head_, n);
        }
        shift_right(head_, 0, 1);
        ValAT::construct(value_alloc_, std::addressof(head_->data()[0]), std::move(v));
        size_ += 1;
    }

    void pop_back() {
        if (empty()) {
            throw std::out_of_range("unrolled_list::pop_back: empty");
        }
        T* prev_node = std::addressof(tail_->data()[tail_->size - 1]);
        ValAT::destroy(value_alloc_, prev_node);
        tail_->size -= 1;
        size_ -= 1;
        if (tail_->size == 0) {
            Node* prev = tail_->prev;
            unlink_and_destroy(tail_);
            if (prev) {
                tail_ = prev;
            } else {
                tail_ = nullptr;
            }
        }
    }

    void pop_front() {
        if (empty()) {
            throw std::out_of_range("unrolled_list::pop_front: empty");
        }
        ValAT::destroy(value_alloc_, std::addressof(head_->data()[0]));
        shift_left(head_, 0, 1);
        size_ -= 1;
        if (head_->size == 0) {
            Node* next = head_->next;
            unlink_and_destroy(head_);
            if (next) {
                head_ = next;
            } else {
                head_ = nullptr;
            }
        }
    }

    iterator insert(const_iterator pos, const T& value) {
        if (pos == cend()) {
            push_back(value);
            return iterator(tail_, tail_->size - 1, tail_);
        }
        Node* node = const_cast<Node*>(pos.get_node());
        std::size_t idx = pos.get_pos();
        if (node->size == capacity) {
            Node* right = split_node(node);
            std::size_t mid = node->size;
            if (idx >= mid) {
                node = right;
                idx -= mid;
            }
        }
        shift_right(node, idx, 1);
        T* prev_node = std::addressof(node->data()[idx]);
        ValAT::construct(value_alloc_, prev_node, value);
        size_ += 1;
        return iterator(node, idx, tail_);
    }

    iterator insert(const_iterator pos, T&& value) {
        if (pos == cend()) {
            push_back(std::move(value));
            return iterator(tail_, tail_->size - 1, tail_);
        }
        Node* node = const_cast<Node*>(pos.get_node());
        std::size_t idx = pos.get_pos();
        if (node->size == capacity) {
            Node* right = split_node(node);
            std::size_t mid = node->size;
            if (idx >= mid) {
                node = right;
                idx -= mid;
            }
        }
        shift_right(node, idx, 1);
        T* prev_node = std::addressof(node->data()[idx]);
        ValAT::construct(value_alloc_, prev_node, std::move(value));
        size_ += 1;
        return iterator(node, idx, tail_);
    }

    iterator insert(const_iterator pos, size_type n, const T& value) {
        iterator it(const_cast<Node*>(pos.get_node()), pos.get_pos(), tail_);
        for (size_type i = 0; i < n; ++i) {
            it = insert(it, value);
        }
        return it;
    }

    template<class InputIt>
        requires (!std::integral<InputIt>)
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        iterator it(const_cast<Node*>(pos.get_node()), pos.get_pos(), tail_);
        for (; first != last; ++first) {
            it = insert(it, *first);
        }
        return it;
    }

    iterator erase(const_iterator cpos) {
        if (cpos == cend()) {
            return end();
        }
        Node* node = const_cast<Node*>(cpos.get_node());
        std::size_t idx = cpos.get_pos();
        ValAT::destroy(value_alloc_, std::addressof(node->data()[idx]));
        shift_left(node, idx, 1);
        size_ -= 1;
        if (node->size == 0) {
            Node* next_node = node->next;
            unlink_and_destroy(node);
            return iterator(next_node, 0, tail_);
        }
        return iterator(node, idx, tail_);
    }

    iterator erase(const_iterator first, const_iterator last) {
        iterator it(const_cast<Node*>(first.get_node()), first.get_pos(), tail_);
        while (it != iterator(const_cast<Node*>(last.get_node()), last.get_pos(), tail_)) {
            it = erase(const_iterator(it));
        }
        return it;
    }

    void clear() noexcept {
        Node* cur = head_;
        while (cur) {
            Node* next_node = cur->next;
            destroy_node(cur);
            cur = next_node;
        }
        head_ = nullptr;
        tail_ = nullptr;
        size_ = 0;
    }

    bool operator==(const unrolled_list& other) const {
        if (size_ != other.size_) {
            return false;
        }
        auto it1 = begin();
        auto it2 = other.begin();
        for (; it1 != end(); ++it1, ++it2) {
            if (!(*it1 == *it2)) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const unrolled_list& other) const {
        return !(*this == other);
    }
};

template<class T, std::size_t C, class A>
void swap(unrolled_list<T, C, A>& a, unrolled_list<T, C, A>& b) noexcept {
    a.swap(b);
}