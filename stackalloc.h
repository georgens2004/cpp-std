#pragma once

#include <iostream>
#include <type_traits>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <memory>


template<size_t N>
class StackStorage {
private:
    char data[N];
    size_t data_end;

public:
    StackStorage(): data(), data_end(0) {}
    StackStorage(const StackStorage<N>& other) = delete;
    StackStorage& operator=(const StackStorage<N>& other) = delete;
    ~StackStorage() = default;

    char* take_memory(size_t type_size, size_t n) {
        if ((data_end + type_size - 1) / type_size * type_size + type_size * n > N) {
            throw std::bad_alloc();
        }
        data_end = (data_end + type_size - 1) / type_size * type_size;
        size_t begin = data_end;
        data_end += type_size * n;
        return (data + begin);
    }

    size_t get_data_end() {
        return data_end;
    }

    template<typename T>
    void free_data(T* ptr, size_t n) {
        auto p = reinterpret_cast<char*>(ptr);
        for (size_t i = 0; i < n; ++i, ++p) {
            
        }
    }
};


template<typename T, size_t N>
class StackAllocator {
private:
    StackStorage<N>* storage;

public:
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    StackAllocator(): storage(nullptr) {}

    StackAllocator(StackStorage<N>& given_storage): storage(&given_storage) {}

    StackAllocator(const StackAllocator& other) = default;

    StackAllocator& operator=(const StackAllocator& other) = default;

    template<typename U>
    StackAllocator(const StackAllocator<U, N>& other): storage(other.get_storage()) {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>(storage->take_memory(sizeof(T), n));
    }

    void deallocate(T* ptr, size_t n) {
        (void)ptr;
        (void)n;
    }

    bool operator==(const StackAllocator& other) {
        return storage == other.storage;
    }

    template<typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    StackStorage<N>* get_storage() const {
        return storage;
    }
    size_t get_end() const {
        return storage->get_data_end();
    }
};

struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode(): prev(this), next(this) {}
};

template<typename T>
struct Node: BaseNode {
    T value;
    Node(): BaseNode(), value() {}
    Node(const Node& other): BaseNode(static_cast<BaseNode>(other)), value(other.value) {}
    template<typename... Args>
    Node(Args&&... args)
        : BaseNode()
        , value(std::forward<Args>(args)...) 
        {}
};

template<typename T, typename Alloc = std::allocator<T>>
class List {
private:
    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node<T>>;
    using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

    NodeAlloc alloc_;

    size_t size_;
    Node<T>* first_;
    BaseNode fake_;

    template<bool is_const>
    struct basic_iterator {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::conditional_t<is_const, const T, T>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
    
    private:
        BaseNode* item_;
    
    public:
        basic_iterator() = default;

        basic_iterator(BaseNode* item): item_(item) {}

        value_type& operator*() const {
            return static_cast<Node<value_type>*>(item_)->value;
        }

        value_type* operator->() const {
            return &(static_cast<Node<value_type>*>(item_)->value);
        }

        basic_iterator& operator++() {
            item_ = item_->next;
            return *this;
        }

        basic_iterator operator++(int) {
            auto it = *this;
            ++(*this);
            return it;
        }

        basic_iterator& operator--() {
            item_ = item_->prev;
            return *this;
        }

        basic_iterator operator--(int) {
            auto it = *this;
            --(*this);
            return it;
        }
        
        operator basic_iterator<true>() const {
            return basic_iterator<true>(item_);
        }

        bool operator==(const basic_iterator& other) const {
            return item_ == other.item_;
        }

        BaseNode& get_node() const {
            return *item_;
        }
    };

    template<class... Args>
    void build(size_t n, Args&&... args) {
        for (size_t i = 0; i < n; ++i) {
            try {
                emplace_back(std::forward<Args>(args)...);
            } catch (...) {
                for (size_t j = 0; j < i; ++j) {
                    pop_back();
                }
                throw;
            }
        }
        first_ = static_cast<Node<T>*>(fake_.next);
    }

    void swap_all(List& other) {
        std::swap(size_, other.size_);
        std::swap(first_, other.first_);
        std::swap(fake_, other.fake_);
        if (size_ == 0) {
            fake_.next = fake_.prev = &fake_;
        } else {
            fake_.prev->next = &fake_;
            first_->prev = &fake_;
        }
        if (other.size_ == 0) {
            other.fake_.next = other.fake_.prev = &other.fake_;
        } else {
            other.fake_.prev->next = &other.fake_;
            other.first_->prev = &other.fake_;
        }
    }

public:

    using iterator = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List()
        : alloc_()
        , size_(0)
        , first_(nullptr)
        , fake_() {}
    
    List(Alloc alloc)
        : alloc_(alloc)
        , size_(0)
        , first_(nullptr)
        , fake_() {}
    
    List(size_t n): List(n, Alloc()) {}

    List(size_t n, Alloc alloc)
        : alloc_(alloc)
        , size_(0)
        , first_()
        , fake_() {
        build(n);
    }

    List(size_t n, const T& value): List(n, value, Alloc()) {}

    List(size_t n, const T& value, Alloc alloc)
        : alloc_(alloc)
        , size_(0)
        , first_()
        , fake_() {
        build(n, value);
    }
    
    List(const List& other)
        : alloc_(NodeAllocTraits::select_on_container_copy_construction(other.alloc_))
        , size_(0)
        , first_()
        , fake_() {
        auto ptr = other.first_;
        for (size_t i = 0; i < other.size_; ++i, ptr = static_cast<Node<T>*>(ptr->next)) {
            try {
                push_back(ptr->value);
            } catch (...) {
                for (size_t j = 0; j < i; ++j) {
                    pop_back();
                }
                throw;
            }
        }
        first_ = static_cast<Node<T>*>(fake_.next);
    }

    List(List&& other) noexcept
        : alloc_(std::move(other.alloc_))
        , size_(other.size_)
        , first_(other.first_)
        , fake_(other.fake_) {
        
        other.size_ = 0;
        other.first_ = nullptr;
        other.fake_ = BaseNode();
    }

    List& operator=(const List& other) {
        auto copy = other;
        swap_all(copy);
        if (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
            alloc_ = other.alloc_;
        }
        return *this;
    }

    List& operator=(List&& other) noexcept {
        if (NodeAllocTraits::propagate_on_container_move_assignment::value) {
            alloc_ = other.alloc_;
        }
        size_ = other.size_;
        first_ = other.first_;
        fake_ = other.fake_;
        other.size_ = 0;
        other.first_ = nullptr;
        other.fake_ = BaseNode();
        return *this;
    }

    ~List() {
        while (size_ > 0) {
            pop_back();
        }
    }

    Alloc get_allocator() const {
        return alloc_;
    }

    size_t size() const {
        return size_;
    }

    template<class... Args>
    void emplace_back(Args&&... args) {
        auto ptr = NodeAllocTraits::allocate(alloc_, 1);
        try {
            NodeAllocTraits::construct(alloc_, ptr, std::forward<Args>(args)...);
        } catch (...) {
            NodeAllocTraits::deallocate(alloc_, ptr, 1);
            throw;
        }
        ptr->prev = fake_.prev;
        ptr->next = &fake_;
        (fake_.prev)->next = ptr;
        fake_.prev = ptr;
        first_ = static_cast<Node<T>*>(fake_.next);
        ++size_;
    }

    void push_back(const T& value) {
        auto ptr = NodeAllocTraits::allocate(alloc_, 1);
        try {
            NodeAllocTraits::construct(alloc_, ptr, value);
        } catch (...) {
            NodeAllocTraits::deallocate(alloc_, ptr, 1);
            throw;
        }
        ptr->prev = fake_.prev;
        ptr->next = &fake_;
        (fake_.prev)->next = ptr;
        fake_.prev = ptr;
        first_ = static_cast<Node<T>*>(fake_.next);
        ++size_;
    }
    
    void push_back(T&& value) {
        auto ptr = NodeAllocTraits::allocate(alloc_, 1);
        try {
            NodeAllocTraits::construct(alloc_, ptr, std::move(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc_, ptr, 1);
            throw;
        }
        ptr->prev = fake_.prev;
        ptr->next = &fake_;
        (fake_.prev)->next = ptr;
        fake_.prev = ptr;
        first_ = static_cast<Node<T>*>(fake_.next);
        ++size_;
    }

    void pop_back() noexcept {
        auto last = static_cast<Node<T>*>(fake_.prev);
        last->prev->next = last->next;
        fake_.prev = last->prev;
        NodeAllocTraits::destroy(alloc_, last);
        NodeAllocTraits::deallocate(alloc_, last, 1);
        if (size_ > 1) {
            first_ = static_cast<Node<T>*>(fake_.next);
        }
        --size_;
    }

    void push_front(const T& value) {
        auto ptr = NodeAllocTraits::allocate(alloc_, 1);
        try {
            NodeAllocTraits::construct(alloc_, ptr, Node(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc_, ptr, 1);
            throw;
        }
        if (size_ > 0) {
            ptr->next = first_;
            ptr->prev = &fake_;
            fake_.next = ptr;
            first_->prev = ptr;
        } else {
            fake_.prev = fake_.next = ptr;
            ptr->prev = ptr->next = &fake_;
        }
        first_ = ptr;
        ++size_;
    }

    void pop_front() noexcept {
        auto front = static_cast<Node<T>*>(fake_.next);
        front->next->prev = &fake_;
        fake_.next = front->next;
        NodeAllocTraits::destroy(alloc_, front);
        NodeAllocTraits::deallocate(alloc_, front, 1);
        if (size_ > 1) {
            first_ = static_cast<Node<T>*>(fake_.next);
        }
        --size_;
    }

    iterator begin() {
        return (size_ > 0 ? iterator(first_) : end());
    }

    iterator end() {
        return iterator(&fake_);
    }

    const_iterator begin() const {
        return (size_ > 0 ? iterator(first_) : end());
    }

    const_iterator end() const {
        return iterator(first_->prev);
    }

    const_iterator cbegin() const {
        return static_cast<const_iterator>(begin());
    }    

    const_iterator cend() const {
        return static_cast<const_iterator>(end());
    }

    reverse_iterator rbegin() {
        auto it = begin();
        return std::reverse_iterator<iterator>(--it);
    }

    reverse_iterator rend() {
        auto it = end();
        return std::reverse_iterator<iterator>(--it);
    }

    const_reverse_iterator rbegin() const {
        auto it = begin();
        return std::reverse_iterator<const_iterator>(--it);
    }

    const_reverse_iterator rend() const {
        auto it = end();
        return std::reverse_iterator<const_iterator>(--it);
    }

    const_reverse_iterator crbegin() const {
        auto it = begin();
        return std::reverse_iterator<const_iterator>(--it);
    }

    const_reverse_iterator crend() const {
        auto it = end();
        return std::reverse_iterator<const_iterator>(--it);
    }

    T& back() {
        return *(--end());
    }

    T back() const {
        return *(--end());
    }

    iterator insert(const_iterator iter, const T& value) {
        auto ptr = NodeAllocTraits::allocate(alloc_, 1);
        try {
            NodeAllocTraits::construct(alloc_, ptr, Node(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc_, ptr, 1);
            throw;
        }
        auto next = &(iter.get_node());
        auto prev = next->prev;
        ptr->prev = prev;
        ptr->next = next;
        prev->next = ptr;
        next->prev = ptr;
        first_ = static_cast<Node<T>*>(fake_.next);
        ++size_;
        return iterator(ptr);
    }

    void erase(const_iterator iter) {
        auto ptr = static_cast<Node<T>*>(&(iter.get_node()));
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        NodeAllocTraits::destroy(alloc_, ptr);
        NodeAllocTraits::deallocate(alloc_, ptr, 1);
        if (size_ > 1) {
            first_ = static_cast<Node<T>*>(fake_.next);
        }
        --size_;
    }

    void print() const {
        size_t i = 0;
        for (auto x = first_; i < size_; x = static_cast<Node<T>*>(x->next), ++i) {
            std::cout << x->value << " ";
        }
        std::cout << std::endl;
    }
};
