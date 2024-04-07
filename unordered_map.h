#pragma once

#include <iostream>
#include <type_traits>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <vector>
#include <utility>
#include <stdexcept>
#include <iterator>
#include <memory>
#include <cassert>


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
    Node& operator=(const Node& other) = default;
    ~Node() = default;
};

template<typename T, typename Alloc = std::allocator<T>>
class List {
private:
    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node<T>>;
    using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

    NodeAlloc alloc;

    size_t num_elements;
    Node<T>* first_ptr;
    BaseNode fake;

    template<bool IsConst>
    struct BaseIterator {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
    
    private:
        BaseNode* item;
    
    public:
        BaseIterator() = default;

        BaseIterator(BaseNode* item): item(item) {}

        value_type& operator*() const {
            return static_cast<Node<value_type>*>(item)->value;
        }

        value_type* operator->() const {
            return &(static_cast<Node<value_type>*>(item)->value);
        }

        BaseIterator& operator++() {
            item = item->next;
            return *this;
        }

        BaseIterator operator++(int) {
            auto it = *this;
            ++(*this);
            return it;
        }

        BaseIterator& operator--() {
            item = item->prev;
            return *this;
        }

        BaseIterator operator--(int) {
            auto it = *this;
            --(*this);
            return it;
        }
        
        operator BaseIterator<true>() const {
            return BaseIterator<true>(item);
        }

        bool operator==(const BaseIterator& other) const {
            return item == other.item;
        }

        BaseNode& get_node() const {
            return *item;
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
        first_ptr = static_cast<Node<T>*>(fake.next);
    }

    void swap_all(List& other) {
        std::swap(num_elements, other.num_elements);
        std::swap(first_ptr, other.first_ptr);
        std::swap(fake, other.fake);
        if (num_elements == 0) {
            fake.next = fake.prev = &fake;
        } else {
            fake.prev->next = &fake;
            first_ptr->prev = &fake;
        }
        if (other.num_elements == 0) {
            other.fake.next = other.fake.prev = &other.fake;
        } else {
            other.fake.prev->next = &other.fake;
            other.first_ptr->prev = &other.fake;
        }
    }

public:

    using iterator = BaseIterator<false>;
    using const_iterator = BaseIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List()
        : alloc()
        , num_elements(0)
        , first_ptr(nullptr)
        , fake() {}
    
    List(Alloc alloc)
        : alloc(alloc)
        , num_elements(0)
        , first_ptr(nullptr)
        , fake() {}
    
    // NOLINTNEXTLINE
    List(size_t n): List(n, Alloc()) {}

    List(size_t n, Alloc alloc)
        : alloc(alloc)
        , num_elements(0)
        , first_ptr()
        , fake() {
        build(n);
    }

    // NOLINTNEXTLINE
    List(size_t n, const T& value): List(n, value, Alloc()) {}

    List(size_t n, const T& value, Alloc alloc)
        : alloc(alloc)
        , num_elements(0)
        , first_ptr()
        , fake() {
        build(n, value);
    }
    
    List(const List& other)
        : alloc(NodeAllocTraits::select_on_container_copy_construction(other.alloc))
        , num_elements(0)
        , first_ptr()
        , fake() {
        auto ptr = other.first_ptr;
        for (size_t i = 0; i < other.num_elements; ++i, ptr = static_cast<Node<T>*>(ptr->next)) {
            try {
                push_back(ptr->value);
            } catch (...) {
                for (size_t j = 0; j < i; ++j) {
                    pop_back();
                }
                throw;
            }
        }
        first_ptr = static_cast<Node<T>*>(fake.next);
    }

    List(List&& other) noexcept
        : alloc(std::move(other.alloc))
        , num_elements(other.num_elements)
        , first_ptr(other.first_ptr)
        , fake(other.fake) {
        if (num_elements == 0) {
            fake.prev = fake.next = &fake;
        } else {
            (fake.prev)->next = (fake.next)->prev = &fake;
        }
        other.num_elements = 0;
        other.first_ptr = nullptr;
        other.fake = BaseNode();
    }

    List& operator=(const List& other) {
        if (this != &other) {
            auto copy = other;
            swap_all(copy);
            if (NodeAllocTraits::propagate_on_container_copy_assignment::value) {
                alloc = other.alloc;
            }
        }
        return *this;
    }

    List& operator=(List&& other) noexcept {
        if (this != &other) {
            clear();
            if (NodeAllocTraits::propagate_on_container_move_assignment::value) {
                alloc = other.alloc;
            } 
            num_elements = other.num_elements;
            first_ptr = other.first_ptr;
            fake = other.fake;
            if (other.num_elements == 0) {
                fake.prev = fake.next = &fake;
            } else {
                (fake.prev->next) = (fake.next)->prev = &fake;
            }
            other.num_elements = 0;
            other.first_ptr = nullptr;
            other.fake = BaseNode();
        }
        return *this;
    }

    void move_list(List&& other) noexcept {
        if (this != &other) {
            clear();
            if (NodeAllocTraits::propagate_on_container_move_assignment::value) {
                alloc = other.alloc;
            } 
            num_elements = other.num_elements;
            first_ptr = other.first_ptr;
            fake = other.fake;
            if (other.num_elements == 0) {
                fake.prev = fake.next = &fake;
            } else {
                (fake.prev->next) = (fake.next)->prev = &fake;
            }
            for (auto ptr = fake.next; ptr != &fake; ) {
                auto next_ptr = ptr->next;
                auto node_ptr = static_cast<Node<T>*>(ptr);
                auto new_ptr = NodeAllocTraits::allocate(alloc, 1);
                NodeAllocTraits::construct(alloc, new_ptr, std::move((*node_ptr).value));
                new_ptr->prev = ptr->prev;
                new_ptr->next = ptr->next;
                ptr->prev->next = ptr->next->prev = new_ptr;
                NodeAllocTraits::destroy(alloc, node_ptr);
                NodeAllocTraits::deallocate(alloc, node_ptr, 1);
                ptr = next_ptr;
            }
            first_ptr = static_cast<Node<T>*>(fake.next);
            other.num_elements = 0;
            other.first_ptr = nullptr;
            other.fake = BaseNode();
        }
    }

    ~List() {
        clear();
    }

    Alloc get_allocator() const {
        return alloc;
    }

    size_t size() const {
        return num_elements;
    }

    void clear() {
        while (num_elements > 0) {
            pop_back();
        }
    }

    template<class... Args>
    void emplace_back(Args&&... args) {
        auto ptr = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, ptr, std::forward<Args>(args)...);
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
        ptr->prev = fake.prev;
        ptr->next = &fake;
        ptr->prev->next = ptr;
        ptr->next->prev = ptr;
        first_ptr = static_cast<Node<T>*>(fake.next);
        ++num_elements;
    }

    void push_back(const T& value) {
        auto ptr = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, ptr, value);
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
        ptr->prev = fake.prev;
        ptr->next = &fake;
        (fake.prev)->next = ptr;
        fake.prev = ptr;
        first_ptr = static_cast<Node<T>*>(fake.next);
        ++num_elements;
    }
    
    void push_back(T&& value) {
        auto ptr = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, ptr, std::move(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
        ptr->prev = fake.prev;
        ptr->next = &fake;
        (fake.prev)->next = ptr;
        fake.prev = ptr;
        first_ptr = static_cast<Node<T>*>(fake.next);
        ++num_elements;
    }

    void pop_back() noexcept {
        auto last = static_cast<Node<T>*>(fake.prev);
        last->prev->next = &fake;
        fake.prev = last->prev;
        NodeAllocTraits::destroy(alloc, last);
        NodeAllocTraits::deallocate(alloc, last, 1);
        if (num_elements == 1) {
            fake.prev = &fake;
            fake.next = &fake;
        }
        --num_elements;
    }

    void push_front(const T& value) {
        auto ptr = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, ptr, Node(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
        if (num_elements > 0) {
            ptr->next = first_ptr;
            ptr->prev = &fake;
            fake.next = ptr;
            first_ptr->prev = ptr;
        } else {
            fake.prev = fake.next = ptr;
            ptr->prev = ptr->next = &fake;
        }
        first_ptr = ptr;
        ++num_elements;
    }

    void pop_front() noexcept {
        auto front = static_cast<Node<T>*>(fake.next);
        front->next->prev = &fake;
        fake.next = front->next;
        NodeAllocTraits::destroy(alloc, front);
        NodeAllocTraits::deallocate(alloc, front, 1);
        if (num_elements == 1) {
            fake.prev = &fake;
            fake.next = &fake;
        }
        --num_elements;
    }

    iterator begin() {
        return (num_elements > 0 ? iterator(first_ptr) : end());
    }

    iterator end() {
        return iterator(&fake);
    }

    const_iterator begin() const {
        return (num_elements > 0 ? iterator(first_ptr) : end());
    }

    const_iterator end() const {
        return iterator(first_ptr->prev);
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
        auto ptr = NodeAllocTraits::allocate(alloc, 1);
        try {
            NodeAllocTraits::construct(alloc, ptr, Node(value));
        } catch (...) {
            NodeAllocTraits::deallocate(alloc, ptr, 1);
            throw;
        }
        auto next = &(iter.get_node());
        auto prev = next->prev;
        ptr->prev = prev;
        ptr->next = next;
        prev->next = ptr;
        next->prev = ptr;
        first_ptr = static_cast<Node<T>*>(fake.next);
        ++num_elements;
        return iterator(ptr);
    }

    void erase(const_iterator iter) {
        auto ptr = static_cast<Node<T>*>(&(iter.get_node()));
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        NodeAllocTraits::destroy(alloc, ptr);
        NodeAllocTraits::deallocate(alloc, ptr, 1);
        if (num_elements > 1) {
            first_ptr = static_cast<Node<T>*>(fake.next);
        }
        --num_elements;
    }

    void print() const {
        size_t idx = 0;
        for (auto ptr = first_ptr; idx < num_elements; ptr = static_cast<Node<T>*>(ptr->next), ++idx) {
            std::cout << ptr->value << " ";
        }
        std::cout << std::endl;
    }

    void iter() const {
        for (auto it = static_cast<BaseNode*>(first_ptr); it != &fake; it = it->next) {
            std::cout << it << " " << &fake << std::endl;
        }
        std::cout << std::endl << std::endl;
    }
};


template<typename Key,
         typename Value,
         typename Hash = std::hash<Key>,
         typename Equal = std::equal_to<Key>,
         typename Alloc = std::allocator< std::pair<const Key, Value> >
>
class UnorderedMap {
private:
    struct Node {
        size_t hash;
        std::pair<const Key, Value> item;
        Node(size_t hash, const std::pair<const Key, Value>& item)
            : hash(hash)
            , item(item) {}
        Node(size_t hash, std::pair<const Key, Value>&& item) 
            : hash(hash)
            , item(std::move(item)) {}
        template<typename... Args>
        Node(const Hash& hasher, Args&&... args)
            : hash(0)
            , item(std::forward<Args>(args)...) {
            std::ignore = hasher;
        }
        Node(const Node& other) 
            : hash(other.hash)
            , item(other.item) {}
        Node(Node&& other)
            : hash(other.hash)
            , item(std::move(other.item)) {}
        Node& operator=(const Node& other) = default;
        Node& operator=(Node&& other) = default;
        ~Node() = default;
    };

    using ListIterator = typename List<Node, Alloc>::iterator;
    using ListConstIterator = typename List<Node, Alloc>::const_iterator;

    template<bool IsConst>
    struct BaseIterator {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const std::pair<const Key, Value>, std::pair<const Key, Value>>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::bidirectional_iterator_tag;
    
    private:
        using list_iterator = std::conditional_t<IsConst, ListConstIterator, ListIterator>;    
        list_iterator iter;
    
    public:
        BaseIterator() = default;

        BaseIterator(const list_iterator& iter): iter(iter) {}

        reference operator*() const {
            return iter->item;
        }

        pointer operator->() const {
            return &(iter->item);
        }

        BaseIterator& operator++() {
            ++iter;
            return *this;
        }

        BaseIterator operator++(int) {
            BaseIterator copy = iter++;
            return copy;
        }

        BaseIterator& operator--() {
            --iter;
            return *this;
        }

        BaseIterator operator--(int) {
            BaseIterator copy = iter--;
            return copy;
        }
        
        operator BaseIterator<true>() const {
            return BaseIterator<true>(iter);
        }

        bool operator==(const BaseIterator& other) const {
            return iter == other.iter;
        }
    };

    using Bucket = std::vector<ListIterator>;
    using BucketArray = std::vector<Bucket>;
    using AllocTraits = std::allocator_traits<Alloc>;
    using iterator = BaseIterator<false>;
    using const_iterator = BaseIterator<true>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

    Alloc alloc;
    List<Node, Alloc> elements;
    BucketArray buckets;
    size_t num_buckets;
    size_t num_elements;
    Hash hasher_function;
    Equal equal_function;

    static const size_t default_size = 16;
    static constexpr float eps_rehash_constant = 0.001f;

    float max_load_factor_value = 1.0f;

    void rehash(size_t new_num_buckets) {
        BucketArray new_buckets(new_num_buckets);
        for (auto it = elements.begin(); it != elements.end(); ++it) {
            size_t bucket_idx = it->hash % new_num_buckets;
            assert(bucket_idx < new_buckets.size());
            new_buckets[bucket_idx].push_back(it);
        }
        buckets.swap(new_buckets);
        num_buckets = new_num_buckets;
    }

    inline bool need_to_rehash() const {
        return static_cast<float>(num_elements) + eps_rehash_constant >= static_cast<float>(num_buckets) * max_load_factor_value;
    }

public:

   UnorderedMap()
                : alloc()
                , elements()
                , buckets(default_size)
                , num_buckets(default_size)
                , num_elements(0)
                , hasher_function()
                , equal_function()
                , max_load_factor_value(1.0f) {}

    UnorderedMap(const UnorderedMap& other)
                : alloc(AllocTraits::select_on_container_copy_construction(other.alloc))
                , elements(other.elements.get_allocator())
                , buckets(other.num_buckets)
                , num_buckets(other.num_buckets)
                , num_elements(other.num_elements)
                , hasher_function(other.hasher_function)
                , equal_function(other.equal_function)
                , max_load_factor_value(other.max_load_factor_value) {
        for (const auto& element : other.elements) {
            elements.push_back(element);
            size_t bucket_idx = element.hash % num_buckets;
            buckets[bucket_idx].push_back(std::prev(elements.end()));
        }
    }

    UnorderedMap(UnorderedMap&& other) noexcept
                : alloc(std::move(other.alloc))
                , elements(std::move(other.elements))
                , buckets(std::move(other.buckets))
                , num_buckets(other.num_buckets)
                , num_elements(other.num_elements)
                , hasher_function(std::move(other.hasher_function))
                , equal_function(std::move(other.equal_function)) {
        other.num_buckets = 0;
        other.num_elements = 0;
    }

    UnorderedMap& operator=(const UnorderedMap& other) {
        if (this != &other) {
            clear();
            elements = other.elements;
            buckets = other.buckets;
            num_buckets = other.num_buckets;
            num_elements = other.num_elements;
            hasher_function = other.hasher_function;
            equal_function = other.equal_function;
            max_load_factor_value = other.max_load_factor_value;

            if (AllocTraits::propagate_on_container_copy_assignment::value) {
                alloc = other.alloc;
            }
        }
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept {
        if (this != &other) {
            clear();
            num_buckets = other.num_buckets;
            num_elements = other.num_elements;
            hasher_function = std::move(other.hasher_function);
            equal_function = std::move(other.equal_function);
            max_load_factor_value = other.max_load_factor_value;

            if (AllocTraits::propagate_on_container_move_assignment::value) {
                alloc = other.alloc;
                elements = std::move(other.elements);
                buckets = std::move(other.buckets);
            } else {
                elements.move_list(std::move(other.elements));
                for (auto &array : buckets) {
                    array.clear();
                }
                for (auto it = elements.begin(); it != elements.end(); ++it) {
                    size_t bucket_idx = (*it).hash % num_buckets;
                    buckets[bucket_idx].push_back(it);
                }
            }
            other.num_buckets = 0;
            other.num_elements = 0;
        }
        return *this;
    }

    size_t size() const {
        return num_elements;
    }

    bool empty() const {
        return num_elements == 0;
    }

    void clear() {
        elements.clear();
        for (auto& bucket : buckets) {
            bucket.clear();
        }
        num_elements = 0;
    }

    Value& operator[](const Key& key) {
        size_t hash = hasher_function(key);
        size_t bucket_idx = hash % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, key)) {
                return it->item.second;
            }
        }
        std::pair<const Key, Value> created_pair(key, Value());
        elements.push_back(Node(hash, created_pair));
        buckets[bucket_idx].push_back(std::prev(elements.end()));
        ++num_elements;
        if (need_to_rehash()) {
            rehash(num_buckets * 2);
        }
        return elements.back().item.second;
    }

    Value& operator[](Key&& key) {
        size_t hash = hasher_function(key);
        size_t bucket_idx = hash % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, key)) {
                return it->item.second;
            }
        }
        std::pair<const Key, Value> created_pair(std::move(key), Value());
        elements.push_back(Node(hash, std::move(created_pair)));
        buckets[bucket_idx].push_back(std::prev(elements.end()));
        ++num_elements;
        if (need_to_rehash()) {
            rehash(num_buckets * 2);
        }
        return elements.back().item.second;
    }

    size_t count(const Key& key) const {
        size_t bucket_idx = hasher_function(key) % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, key)) {
                return 1;
            }
        }
        return 0;
    }

    void erase(const_iterator pos) {
        if (pos == elements.end()) {
            return;
        }
        size_t bucket_idx = pos->hash % num_buckets;
        for (auto it = buckets[bucket_idx].begin(); it != buckets[bucket_idx].end(); ++it) {
            if (*it == pos) {
                elements.erase(pos);
                buckets[bucket_idx].erase(it);
                --num_elements;
                return;
            }
        }
    }

    void erase(const_iterator first, const_iterator last) {
        for (auto it = first; it != last;) {
            auto it1 = it;
            ++it;
            erase(it1);
        }
    }

    void erase(const Key& key) {
        size_t bucket_idx = hasher_function(key) % num_buckets;
        for (auto it = buckets[bucket_idx].begin(); it != buckets[bucket_idx].end(); ++it) {
            if (equal_function((*it)->item.first, key)) {
                elements.erase(*it);
                buckets[bucket_idx].erase(it);
                --num_elements;
                return;
            }
        }
    }

    std::pair<iterator, bool> insert(std::pair<Key, Value>&& node) {
        return emplace(std::move(node.first), std::move(node.second));
    }

    std::pair<iterator, bool> insert(const std::pair<Key, Value>& node) {
        return emplace(node.first, node.second);
    }

    template<typename input_iterator>
    void insert(input_iterator first, input_iterator last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        NodeAlloc node_alloc(alloc);
        auto node_ptr = NodeAllocTraits::allocate(node_alloc, 1);
        try {
            NodeAllocTraits::construct(node_alloc, node_ptr, hasher_function, std::forward<Args>(args)...);
            (*node_ptr).hash = hasher_function((*node_ptr).item.first);
        } catch (...) {
            NodeAllocTraits::deallocate(node_alloc, node_ptr, 1);
            throw;
        }
        Node& node = (*node_ptr);
        size_t bucket_idx = node.hash % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, node.item.first)) {
                NodeAllocTraits::destroy(node_alloc, node_ptr);
                NodeAllocTraits::deallocate(node_alloc, node_ptr, 1);
                return std::make_pair(it, false);
            }
        }
        elements.emplace_back(std::move(node));
        buckets[bucket_idx].push_back(std::prev(elements.end()));
        ++num_elements;
        if (need_to_rehash()) {
            rehash(num_buckets * 2);
        }
        NodeAllocTraits::destroy(node_alloc, node_ptr);
        NodeAllocTraits::deallocate(node_alloc, node_ptr, 1);
        return std::make_pair(iterator(std::prev(elements.end())), true);
    }

    iterator begin() {
        return iterator(elements.begin());
    }

    const_iterator begin() const {
        return const_iterator(elements.begin());
    }

    iterator end() {
        return iterator(elements.end());
    }

    const_iterator end() const {
        return const_iterator(elements.end());
    }

    const_iterator cbegin() const {
        return const_iterator(elements.cbegin());
    }

    const_iterator cend() const {
        return const_iterator(elements.cend());
    }

    iterator find(const Key& key) {
        size_t bucket_idx = hasher_function(key) % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, key)) {
                return it;
            }
        }
        return elements.end();
    }

    const_iterator find(const Key& key) const {
        size_t bucket_idx = hasher_function(key) % num_buckets;
        for (auto it : buckets[bucket_idx]) {
            if (equal_function(it->item.first, key)) {
                return const_iterator(it);
            }
        }
        return const_iterator(elements.end());
    }

    Value& at(const Key& key) {
        return const_cast<Value&>(static_cast<const UnorderedMap&>(*this).at(key));
    }

    const Value& at(const Key& key) const {
        auto it = find(key);
        if (it == const_iterator(elements.end())) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }

    void reserve(size_t new_num_buckets) {
        if (new_num_buckets > num_buckets) {
            rehash(new_num_buckets);
        }
    }

    float load_factor() const {
        return static_cast<float>(num_elements) / static_cast<float>(num_buckets);
    }

    void max_load_factor(float new_max_load_factor) {
        max_load_factor_value = new_max_load_factor;
        if (need_to_rehash()) {
            rehash(static_cast<size_t>(static_cast<float>(num_elements) / max_load_factor_value + 1));
        }
    }

    Alloc get_allocator() const noexcept {
        return elements.get_allocator();
    }

    void swap(UnorderedMap& other) noexcept {
        std::swap(elements, other.elements);
        std::swap(buckets, other.buckets);
        std::swap(num_buckets, other.num_buckets);
        std::swap(num_elements, other.num_elements);
        std::swap(hasher_function, other.hasher_function);
        std::swap(equal_function, other.equal_function);
        std::swap(max_load_factor_value, other.max_load_factor_value);
    }

    void iter() const {
        elements.iter();
    }

    ~UnorderedMap() {
        elements.clear();
    }
};
