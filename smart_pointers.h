#include <iostream>

template<typename T>
class EnableSharedFromThis;

template<typename T>
class WeakPtr;

struct BaseControlBlock {
    size_t shared_cnt;
    size_t weak_cnt;
    BaseControlBlock(int shared_cnt = 0, int weak_cnt = 0)
        : shared_cnt(shared_cnt)
        , weak_cnt(weak_cnt)
        {}
    virtual void destroy() = 0;
    virtual void deallocate_self() = 0;
    virtual void* get_ptr() = 0;
    virtual ~BaseControlBlock() = default;
};

template<typename T>
class SharedPtr {
private:
    T* real_ptr;
    
    template<typename Y, typename Deleter, typename Alloc>
    struct ControlBlock: BaseControlBlock {
        Y* ptr;
        Deleter deleter;
        Alloc alloc;
        ControlBlock(): BaseControlBlock() {}
        ControlBlock(Y* ptr, const Deleter& deleter, const Alloc& alloc)
            : BaseControlBlock()
            , ptr(ptr)
            , deleter(deleter)
            , alloc(alloc)
            {}
        void destroy() override {
            deleter(ptr);
        }
        void deallocate_self() override {
            using ControlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlock<Y, Deleter, Alloc>>;
            ControlBlockAlloc block_alloc = alloc;
            using ControlBlockAllocTraits = std::allocator_traits<ControlBlockAlloc>;
            this->~ControlBlock();
            ControlBlockAllocTraits::deallocate(block_alloc, this, 1);
        }
        void* get_ptr() override {
            return static_cast<T*>(ptr);
        }
    };
    template<typename Alloc>
    struct ControlBlockShared: BaseControlBlock {
        char object[sizeof(T)];
        Alloc alloc;
        ControlBlockShared(const Alloc& alloc)
            : BaseControlBlock()
            , alloc(alloc) 
            {}
        void destroy() override {
            using TAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
            TAlloc t_alloc = alloc;
            using TAllocTraits = std::allocator_traits<TAlloc>;
            TAllocTraits::destroy(t_alloc, reinterpret_cast<T*>(object));
        }
        void deallocate_self() override {
            using ControlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockShared<Alloc>>;
            ControlBlockAlloc block_alloc = alloc;
            using ControlBlockAllocTraits = std::allocator_traits<ControlBlockAlloc>;
            this->~ControlBlockShared();
            ControlBlockAllocTraits::deallocate(block_alloc, this, 1);
        }
        void* get_ptr() override {
            return reinterpret_cast<T*>(object);
        }
    };

    struct AllocateSharedTag {};

    BaseControlBlock* control_block;

    template<typename Y, typename... Args>
    friend SharedPtr<Y> makeShared(Args&&... args);

    template<typename Y, typename Alloc, typename... Args>
    friend SharedPtr<Y> allocateShared(const Alloc& alloc, Args&&... args);

    template<typename Alloc>
    SharedPtr(ControlBlockShared<Alloc>* control_block)
        : control_block(control_block)
        {}

    inline void decrease_shared_counter() {
        if (!control_block) return;
        if (--control_block->shared_cnt == 0) {
            control_block->destroy();
            if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
                return;
            }
            if (control_block->weak_cnt == 0) {
                control_block->deallocate_self();
            }
        }
    }
    inline void increase_shared_counter() {
        if (control_block) ++control_block->shared_cnt;
    }

    template<typename Y>
    friend class SharedPtr;

    template<typename Alloc, typename... Args>
    SharedPtr(const AllocateSharedTag& empty_object, const Alloc& alloc, Args&&... args) {
        std::ignore = empty_object;
        using ControlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockShared<Alloc>>;
        ControlBlockAlloc block_alloc = alloc;
        using ControlBlockAllocTraits = std::allocator_traits<ControlBlockAlloc>;
        ControlBlockShared<Alloc>* control_block_ptr = ControlBlockAllocTraits::allocate(block_alloc, 1);
        new (control_block_ptr) ControlBlockShared<Alloc>(alloc);
        using TAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
        TAlloc t_alloc = alloc;
        using TAllocTraits = std::allocator_traits<TAlloc>;
        real_ptr = reinterpret_cast<T*>(control_block_ptr->object);
        TAllocTraits::construct(t_alloc, real_ptr, std::forward<Args>(args)...);
        control_block = control_block_ptr;
        increase_shared_counter();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            real_ptr->weak_ptr = *this;
        }
    }

    template<typename Y>
    friend class WeakPtr;

    SharedPtr(const WeakPtr<T>& other)
        : control_block(other.control_block) {
        real_ptr = reinterpret_cast<T*>(control_block->get_ptr());
        increase_shared_counter();
    }

public:
    SharedPtr() = default;

    template<typename Y, typename Deleter, typename Alloc>
    SharedPtr(Y* ptr, Deleter deleter, Alloc alloc) {
        using ControlBlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlock<Y, Deleter, Alloc>>;
        ControlBlockAlloc block_alloc = alloc;
        using ControlBlockAllocTraits = std::allocator_traits<ControlBlockAlloc>;
        ControlBlock<Y, Deleter, Alloc>* control_block_ptr = ControlBlockAllocTraits::allocate(block_alloc, 1);
        new (control_block_ptr) ControlBlock<Y, Deleter, Alloc>(ptr, deleter, alloc);
        control_block = control_block_ptr;
        real_ptr = reinterpret_cast<T*>(control_block->get_ptr());
        increase_shared_counter();
        if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
            real_ptr->weak_ptr = *this;
        }
    }
    
    template<typename Y>
    SharedPtr(Y* ptr)
        : SharedPtr(ptr, std::default_delete<Y>(), std::allocator<Y>())
        {}

    template<typename Y, typename Deleter>
    SharedPtr(Y* ptr, Deleter deleter)
        : SharedPtr(ptr, deleter, std::allocator<Y>())
        {}

    SharedPtr(const SharedPtr& other)
        : real_ptr(other.real_ptr)
        , control_block(other.control_block) {
        increase_shared_counter();
    }
    template<typename Y>
    SharedPtr(const SharedPtr<Y>& other)
        : real_ptr(other.real_ptr)
        , control_block(other.control_block) {
        increase_shared_counter();
    }

    SharedPtr(SharedPtr&& other)
        : real_ptr(other.real_ptr)
        , control_block(other.control_block) {
        other.real_ptr = nullptr;
        other.control_block = nullptr;
    }
    template<typename Y>
    SharedPtr(SharedPtr<Y>&& other)
        : real_ptr(other.real_ptr)
        , control_block(other.control_block) {
        other.real_ptr = nullptr;
        other.control_block = nullptr;
    }

    void swap(SharedPtr& other) {
        std::swap(real_ptr, other.real_ptr);
        std::swap(control_block, other.control_block);
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (control_block != other.control_block) {
            SharedPtr copy(other);
            swap(copy);
        }
        return *this;
    }
    template<typename Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        if (control_block != other.control_block) {
            SharedPtr copy(other);
            swap(copy);
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (control_block != other.control_block) {
            SharedPtr copy(other);
            swap(copy);
        }
        other.reset();
        return *this;
    }
    template<typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        if (control_block != other.control_block) {
            SharedPtr copy(other);
            swap(copy);
        }
        other.reset();
        return *this;
    }

    size_t use_count() const {
        if (control_block == nullptr) return 0;
        return control_block->shared_cnt;
    }

    void reset() {
        decrease_shared_counter();
        control_block = nullptr;
    }
    void reset(T* ptr) {
        SharedPtr sptr(ptr);
        swap(sptr);
    }
    template<typename Y>
    void reset(Y* ptr) {
        SharedPtr<Y> sptr(ptr);
        swap(sptr);
    }

    T& operator*() const {
        return *static_cast<T*>(real_ptr);
    }

    T* operator->() const {
        if (!control_block) return nullptr;
        return static_cast<T*>(real_ptr);
    }

    T* get() const {
        if (!control_block) return nullptr;
        return static_cast<T*>(real_ptr);
    }

    ~SharedPtr() {
        decrease_shared_counter();
    }
};

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    using Alloc = std::allocator<T>;
    return SharedPtr<T>(typename SharedPtr<T>::AllocateSharedTag(), Alloc(), std::forward<Args>(args)...);
}

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
    return SharedPtr<T>(typename SharedPtr<T>::AllocateSharedTag(), alloc, std::forward<Args>(args)...);
}

template<typename T>
class WeakPtr {
private:
    template<typename Y>
    friend class SharedPtr;
    
    template<typename Y>
    friend class WeakPtr;

    BaseControlBlock* control_block;

    inline void increase_weak_cnt() {
        if (control_block != nullptr)
            ++control_block->weak_cnt;
    }

    inline void decrease_weak_cnt() {
        if (!control_block) return;
        if (--control_block->weak_cnt == 0 && control_block->shared_cnt == 0) {
            control_block->deallocate_self();
        }
    }

public:
    WeakPtr()
        : control_block(nullptr)
        {}
    
    template<typename Y>
    WeakPtr(const SharedPtr<Y>& other)
        : control_block(other.control_block) {
        increase_weak_cnt();    
    }

    WeakPtr(const WeakPtr& other)
        : control_block(other.control_block) {
        increase_weak_cnt();
    }
    template<typename Y>
    WeakPtr(const WeakPtr<Y>& other)
        : control_block(other.control_block) {
        increase_weak_cnt();
    }

    WeakPtr(WeakPtr&& other)
        : control_block(other.control_block) {
        other.control_block = nullptr;
    }
    template<typename Y>
    WeakPtr(WeakPtr<Y>&& other)
        : control_block(other.control_block) {
        other.control_block = nullptr;
    }

    void swap(WeakPtr& other) {
        std::swap(control_block, other.control_block);
    }

    WeakPtr& operator=(const WeakPtr& other) {
        WeakPtr copy(other);
        swap(copy);
        return *this;
    }
    template<typename Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) {
        WeakPtr copy(other);
        swap(copy);
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        WeakPtr copy(std::move(other));
        swap(copy);
        return *this;
    }
    template<typename Y>
    WeakPtr& operator=(WeakPtr<Y>&& other) {
        WeakPtr copy(std::move(other));
        swap(copy);
        return *this;
    }

    size_t use_count() const {
        if (control_block == nullptr) return 0;
        return control_block->shared_cnt;
    }

    bool expired() const {
        return control_block == nullptr || control_block->shared_cnt == 0;
    }

    SharedPtr<T> lock() const {
        if (control_block == nullptr) return SharedPtr<T>();
        if (control_block->shared_cnt == 0) return SharedPtr<T>();
        return SharedPtr<T>(*this);
    }

    ~WeakPtr() {
        decrease_weak_cnt();
    }
};

template<typename T>
class EnableSharedFromThis {
private:
    WeakPtr<T> weak_ptr;

    template<typename Y>
    friend class SharedPtr;

protected:
    SharedPtr<T> shared_from_this() const {
        return weak_ptr.lock();
    }

public:
    virtual ~EnableSharedFromThis() = default;
};
