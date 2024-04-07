#include <iostream>
#include <cstring>
#include <algorithm>

class String {

private:
    char* chrs = nullptr;
    size_t sz;
    size_t cap_with_zero;

    void set_terminating_zero() const {
        chrs[sz] = '\0';
    }

    // optional function for changing cap_with_zero
    void change_capacity(size_t new_cap_with_zero) {
        char* old_loc = chrs;
        chrs = new char[new_cap_with_zero];
        std::copy(old_loc, old_loc + sz, chrs);
        delete[] old_loc;
        cap_with_zero = new_cap_with_zero;
    }

    // checking for matching substring
    bool is_equal_substr(size_t start, const String& str) const {
        size_t idx = start;
        while (idx < start + str.sz) {
            if (chrs[idx] != str.chrs[idx - start]) return false;
            ++idx;
        }
        return true;
    }

    explicit String(size_t cap_with_zero): chrs(new char[cap_with_zero]), sz(0), cap_with_zero(cap_with_zero) {
        set_terminating_zero();
    }

    size_t find_substr(const String& str, bool iterate_from_begin) const {
        int start = (iterate_from_begin ? 0 : static_cast<int>(sz) - static_cast<int>(str.sz));
        int end = (iterate_from_begin ? sz - str.sz + 1 : -1);
        for (; start != end; (iterate_from_begin ? ++start : --start)) {
            if (is_equal_substr(static_cast<size_t>(start), str)) {
                return static_cast<size_t>(start);
            }
        }
        return sz;
    }


public:

    String(): chrs(new char[1]{'\0'}), sz(0), cap_with_zero(1) {}
    
    String(const String& other): chrs(new char[other.cap_with_zero]), sz(other.sz), cap_with_zero(other.cap_with_zero) {
        std::copy(other.chrs, other.chrs + sz + 1, chrs);
    }
    String(const char* other_chrs): chrs(new char[strlen(other_chrs) + 1]), sz(strlen(other_chrs)), cap_with_zero(strlen(other_chrs) + 1) {
        std::copy(other_chrs, other_chrs + sz, chrs);
        set_terminating_zero();
    }
    explicit String(size_t n, char c): chrs(new char[n + 1]), sz(n), cap_with_zero(n + 1) {
        std::fill(chrs, chrs + n, c);
        set_terminating_zero();
    }
    String(const std::initializer_list<char>& lst): chrs(new char[lst.size() + 1]), sz(lst.size()), cap_with_zero(lst.size() + 1) {
        std::copy(lst.begin(), lst.end(), chrs);
        set_terminating_zero();
    }
    ~String() {
        delete[] chrs;
    }

    void swap(String& other) {
        std::swap(chrs, other.chrs);
        std::swap(cap_with_zero, other.cap_with_zero);
        std::swap(sz, other.sz);
    }

    String& operator=(const String& other) {
        if (sz >= other.sz) {
            std::copy(other.chrs, other.chrs + other.sz, chrs);
            sz = other.sz;
        } else {
            String other_copy = other;
            swap(other_copy);
        }
        return *this;
    }

    char& operator[](size_t x) {
        return chrs[x];
    }
    const char& operator[](size_t x) const {
        return chrs[x];
    }

    size_t length() const {
        return sz;
    }
    size_t size() const {
        return sz;
    }
    size_t capacity() const {
        return cap_with_zero - 1;
    }

    void push_back(char c) {
        if (sz == cap_with_zero) change_capacity(cap_with_zero * 2 + 1);
        ++sz;
        chrs[sz - 1] = c;
        set_terminating_zero();
    }
    void pop_back() {
        --sz;
        set_terminating_zero();
    }

    char& front() {
        return *chrs;
    }
    const char& front() const {
        return *chrs;
    }
    char& back() {
        return chrs[sz - 1];
    }
    const char& back() const {
        return chrs[sz - 1];
    }

    String& operator+=(char c) {
        push_back(c);
        return *this;
    }
    String& operator+=(const String& other) {
        if (sz + other.sz >= cap_with_zero) {
            size_t new_cap_with_zero = cap_with_zero + std::max(cap_with_zero, other.cap_with_zero) + 1;
            change_capacity(new_cap_with_zero);
        }
        std::copy(other.chrs, other.chrs + other.sz + 1, chrs + sz);
        sz += other.sz;
        return *this;
    }

    String operator+(char c) const {
        String res = *this;
        res.push_back(c);
        return res;
    }

    size_t find(const String& str) const {
        return find_substr(str, true);
    }
    size_t rfind(const String& str) const {
        return find_substr(str, false);
    }

    String substr(size_t start, size_t count) const {
        String res(count + 1);
        res.sz = count;
        std::copy(chrs + start, chrs + start + count, res.chrs);
        res.set_terminating_zero();
        return res;
    }

    bool empty() const {
        return sz == 0;
    }

    void clear() {
        sz = 0;
        set_terminating_zero();
    }

    void shrink_to_fit() {
        change_capacity(sz + 1);
    }

    char* data() {
        return chrs;
    }
    const char* data() const {
        return chrs;
    }
};

std::istream& operator>>(std::istream &input, String& str) {
    str.clear();
    char c;
    while (input.get(c)) {
        if (c < '!' || c > '~') break;
        str.push_back(c);
    }
    return input;
}
std::ostream& operator<<(std::ostream &output, const String &str) {
    size_t idx = 0;
    while (idx < str.size()) {
        output << str[idx];
        ++idx;
    }
    return output;
}

// first - is equal, second - is less or equal
std::pair<bool, bool> check_prefix_less_or_equal(const String& str1, const String& str2) {
    size_t min_sz = std::min(str1.size(), str2.size());
    size_t idx = 0;
    while(idx < min_sz) {
        if (str1[idx] < str2[idx]) return {false, true};
        else if (str1[idx] > str2[idx]) return {false, false};
        ++idx;
    }
    if (str1.size() != str2.size()) {
        return {false, str1.size() < str2.size()};
    }
    return {true, true};
}

bool operator==(const String& str1, const String& str2) {
    return str1.size() == str2.size() && check_prefix_less_or_equal(str1, str2).first;
}
bool operator!=(const String& str1, const String& str2) {
    return !(str1 == str2);
}
bool operator<(const String& str1, const String& str2) {
    std::pair<bool, bool> checked_prefix = check_prefix_less_or_equal(str1, str2);
    return !checked_prefix.first && checked_prefix.second;
}
bool operator<=(const String& str1, const String& str2) {
    return str1 == str2 && str1 < str2;
}
bool operator>(const String& str1, const String& str2) {
    return !(str1 <= str2);
}
bool operator>=(const String& str1, const String& str2) {
    return !(str1 < str2);
}

String operator+(char c, const String& str) {
    String res(1, c);
    return res += str;
}

String operator+(String str1, const String& str2) {
    return str1 += str2;
}
