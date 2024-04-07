#include <iostream>
#include <vector>
#include <compare>
#include <cassert>

// simple comment

template<typename T>
class Deque {
private:
    static const size_t pack_size = 32;
    static const size_t expansion_coef = 3;

    std::vector<T*> data;
    
    size_t stored;
    size_t front_pack;
    size_t front_pos;
    size_t back_pack;
    size_t back_pos;

    template<bool IsConst>
    struct BasicIterator {
    public:
        using difference_type = ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::random_access_iterator_tag;

    private:
        pointer item;
        int item_pack;
        size_t item_pos;
        const std::vector<T*>* data_ptr;

        void set_item() {
            if (item == nullptr) {
                item = (*data_ptr)[static_cast<size_t>(item_pack)] + item_pos;
            }
        }

    public:
        BasicIterator(): item(nullptr), item_pack(0), item_pos(0), data_ptr(nullptr) {}

        BasicIterator(size_t front_pack, size_t front_pos, const std::vector<T*>* data_pointer):
                        item(nullptr), item_pack(static_cast<int>(front_pack)), item_pos(front_pos),
                        data_ptr(data_pointer) {
            set_item();
        }

        reference operator*() {
            set_item();
            return *item;
        }

        pointer operator->() {
            set_item();
            return item;
        }

        BasicIterator& operator++() {
            return (*this) += 1;
        }

        BasicIterator operator++(int) {
            auto it = *this;
            ++(*this);
            return it;
        }

        BasicIterator& operator--() {
            return (*this) -= 1;
        }

        BasicIterator operator--(int) {
            auto it = *this;
            --(*this);
            return it;
        }

        BasicIterator& operator+=(difference_type val) {
            int old_item_pack = item_pack;
            int item_pos_temp = static_cast<int>(item_pos) + static_cast<int>(val);
            if (item_pos_temp >= 0) {
                item_pack += item_pos_temp / static_cast<int>(pack_size);
                item_pos = static_cast<size_t>(item_pos_temp) % pack_size;
            } else {
                item_pack -= abs(item_pos_temp + 1) / static_cast<int>(pack_size) + 1;
                item_pos = pack_size - 1 - static_cast<size_t>(abs(item_pos_temp + 1)) % pack_size;
            }
            if (old_item_pack == item_pack && item != nullptr) item += val;
            else item = nullptr;
            return *this;
        }

        BasicIterator& operator-=(difference_type val) {
            return *this += -val;
        }

        BasicIterator operator+(difference_type dif) const {
            auto copy = *this;
            copy += dif;
            return copy;
        }

        BasicIterator operator-(difference_type dif) const {
            auto copy = *this;
            copy -= dif;
            return copy;
        }

        difference_type operator-(const BasicIterator& other) const {
            if (item_pack == other.item_pack) {
                return static_cast<difference_type>(item_pos - other.item_pos);
            }
            return static_cast<difference_type>(static_cast<size_t>(item_pack - other.item_pack) * pack_size + item_pos - other.item_pos);
        }

        std::strong_ordering operator<=>(const BasicIterator& other) const {
            if (item_pack != other.item_pack) {
                return item_pack <=> other.item_pack;
            }
            return item_pos <=> other.item_pos;
        }

        bool operator==(const BasicIterator& other) const {
            return item_pack == other.item_pack && item_pos == other.item_pos;
        }

        operator BasicIterator<true>() const {
            return BasicIterator<true>(static_cast<size_t>(item_pack), item_pos, data_ptr);
        }
    };

public:
    using value_type = T;
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:

    void allocate_data() {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = reinterpret_cast<T*>(new char[pack_size * sizeof(T)]);
        }
    }

    void deallocate_data() {
        for (size_t i = 0; i < data.size(); ++i) {
            delete[] data[i];
        }
    }

    void destroy_data() {
        for (auto it = begin(); it != end(); ++it) {
            it -> ~T();
        }
    }

    void copy_data(const Deque& other) {
        for (size_t i = 0; i < other.data.size(); ++i) {
            if (other.data[i]) {
                for (size_t j = 0; j < pack_size; ++j) {
                    data[i][j] = other.data[i][j];
                }
            }
        }
    }

    void fill(const T& val) {
        for (size_t i = 0; i < data.size(); ++i) {
            for (size_t j = 0; j < pack_size; ++j) {
                data[i][j] = val;
            }
        }
    }

    void realloc_with_expansion() {
        std::vector<T*> new_data(data.size() * expansion_coef);
        front_pack += data.size();
        back_pack += data.size();
        for (size_t i = data.size(); i < data.size() * 2; ++i) {
            new_data[i] = std::move(data[i - data.size()]);
        }
        for (size_t i = 0; i < data.size(); ++i) {
            new_data[i] = reinterpret_cast<T*>(new char[pack_size * sizeof(T)]);
        }
        for (size_t i = data.size() * 2; i < data.size() * expansion_coef; ++i) {
            new_data[i] = reinterpret_cast<T*>(new char[pack_size * sizeof(T)]);
        }
        std::swap(new_data, data);
    }

    void expand_front() {
        if (front_pos == 0) {
            front_pos = pack_size - 1;
            --front_pack;
        } else {
            --front_pos;
        }
    }
    void narrow_front() {
        if (front_pos == pack_size - 1) {
            front_pos = 0;
            ++front_pack;
        } else {
            ++front_pos;
        }
    }
    void expand_back() {
        if (back_pos == pack_size - 1) {
            back_pos = 0;
            ++back_pack;
        } else {
            ++back_pos;
        }
    }
    void narrow_back() {
        if (back_pos == 0) {
            back_pos = pack_size - 1;
            --back_pack;
        } else {
            --back_pos;
        }
    }

public:
    Deque(): data(std::vector<T*>(1)), stored(0), 
            front_pack(0), front_pos(0), 
            back_pack(0), back_pos(0) {
        allocate_data();
    }

    Deque(const Deque& other): data(std::vector<T*>(other.data.size())), stored(other.stored),
                            front_pack(other.front_pack), front_pos(other.front_pos), 
                            back_pack(other.back_pack), back_pos(other.back_pos) {
        allocate_data();
        const_iterator other_it = other.begin();
        for (iterator it = begin(); it != end(); ++it, ++other_it) {
            try {
                new (&(*it)) T(*other_it);
            } catch(...) {
                for (auto it2 = begin(); it2 < it; ++it2) {
                    it2 -> ~T();
                }
                for (size_t i = 0; i < data.size(); ++i) {
                    delete[] reinterpret_cast<char*>(data[i]);
                }
                throw;
            }
        }
    }

    Deque(size_t n): data(std::vector<T*>(n / pack_size + 1)), stored(n), 
                    front_pack(0), front_pos(0),
                    back_pack(n / pack_size), back_pos(n % pack_size) {
        allocate_data();
        for (auto it = begin(); it != end(); ++it) {

            try {
                new (&(*it)) T(T());
            } catch(...) {
                for (auto it2 = begin(); it2 < it; ++it2) {
                    it2 -> ~T();
                }
                for (size_t i = 0; i < data.size(); ++i) {
                    delete[] reinterpret_cast<char*>(data[i]);
                }
                throw;
            }
        }
    }

    Deque(size_t n, const T& val): data(std::vector<T*>(n / pack_size + 1)), stored(n), 
                front_pack(0), front_pos(0),
                back_pack(n / pack_size), back_pos(n % pack_size) {
        allocate_data();
        for (auto it = begin(); it != end(); ++it) {

            try {
                new (&(*it)) T(val);
            } catch(...) {
                for (auto it2 = begin(); it2 < it; ++it2) {
                    it2 -> ~T();
                }
                for (size_t i = 0; i < data.size(); ++i) {
                    delete[] reinterpret_cast<char*>(data[i]);
                }
                throw;
            }
        }
    }

    ~Deque() {
        for (auto it = begin(); it != end(); ++it) {
            it -> ~T();
        }
        for (size_t i = 0; i < data.size(); ++i) {
            delete[] reinterpret_cast<char*>(data[i]);
        }
    }

    Deque& operator=(const Deque& other) {
        deallocate_data();
        destroy_data();
        data.resize(other.data.size());
        allocate_data();
        copy_data(other);
        stored = other.stored;
        front_pack = other.front_pack;
        front_pos = other.front_pos;
        back_pack = other.back_pack;
        back_pos = other.back_pos;
        return *this;
    }

    size_t size() const {
        return stored;
    }

    T& operator[](size_t idx) {
        return data[front_pack + (front_pos + idx) / pack_size][(front_pos + idx) % pack_size];
    }

    const T& operator[](size_t idx) const {
        return data[front_pack + (front_pos + idx) / pack_size][(front_pos + idx) % pack_size];
    }

    T& at(size_t idx) {
        if (front_pos + idx >= stored) {
            throw std::out_of_range("Caught out of range exception");
        }
        return data[front_pack + (front_pos + idx) / pack_size][(front_pos + idx) % pack_size];
    }

    const T& at(size_t idx) const {
        if (front_pos + idx >= stored) {
            throw std::out_of_range("Caught out of range exception");
        }
        return data[front_pack + (front_pos + idx) / pack_size][(front_pos + idx) % pack_size];
    }

    void push_front(const T& val) {
        if (front_pos == 0 && front_pack == 0) {
            realloc_with_expansion();
        }
        try {
            expand_front();
            new (data[front_pack] + front_pos) T(val);
            ++stored;
        } catch(...) {
            narrow_front();
        }   
    }

    void pop_front() {
        begin() -> ~T();
        narrow_front();
        --stored;
    }

    void push_back(const T& val) {
        if (back_pack == data.size()) {
            realloc_with_expansion();
        }
        new (data[back_pack] + back_pos) T(val);
        expand_back();
        ++stored;
    }

    void pop_back() {
        (--end()) -> ~T();
        narrow_back();
        --stored;
    }

    iterator begin() {
        return iterator(front_pack, front_pos, &data);
    }
    
    iterator end() {
        return begin() + static_cast<ptrdiff_t>(stored);
    }

    const_iterator begin() const {
        return iterator(front_pack, front_pos, &data);
    }

    const_iterator end() const {
        return begin() + static_cast<ptrdiff_t>(stored);
    }

    const_iterator cbegin() const {
        return static_cast<const_iterator>(begin());
    }

    const_iterator cend() const {
        return static_cast<const_iterator>(end());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const {
        return reverse_iterator(end());
    }

    const_reverse_iterator crend() const {
        return reverse_iterator(begin());
    }

    iterator insert(iterator iter, const T& val) {
        if (back_pack == data.size()) {
            ptrdiff_t iter_diff = iter - begin();
            realloc_with_expansion();
            iter = begin() + iter_diff;
        }
        // честно, не до конца в итоге понял, что делаем
        // если, допустим, мы сдвигаем, и где-то в середине всех сдвигов 
        // вылетает исключение, мы же должны обратно тогда всё сдвигать,
        // что снова может кинуть исключение. как это тогда обрабатывать?
        // единственное - можно верить, что ничего не упадёт во время сдвигов,
        // и проблемы максимум будут в моменте непосредственно placement-new для
        // нового объекта T(val)
        for (auto it = end() - 1; it >= iter; --it) {
            new (&(*(it + 1))) T(std::move_if_noexcept(*it));
        }
        try {
            new (&(*iter)) T(val);
        } catch(...) {
            for (auto it = iter; it != end(); ++it) {
                new (&(*it)) T(std::move_if_noexcept(*(it + 1)));
            }
            throw;
        }
        if (back_pos == pack_size - 1) {
            back_pos = 0;
           ++back_pack;
        } else {
            ++back_pos;
        }
        ++stored;
        return iter;
    }

    iterator erase(iterator iter) {
        iter -> ~T();
        for (auto it = iter; it < end(); ++it) {
            new (&(*it)) T(std::move(*(it + 1)));
        }
        if (back_pos == 0) {
            back_pos = pack_size - 1;
            --back_pack;
        } else {
            --back_pos;
        }
        --stored;
        return iter;
    }
};
