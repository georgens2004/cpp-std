#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <cstring>
#include <compare>
#include <complex>
#include <math.h>

class BigInteger;

BigInteger operator + (const BigInteger& n1, const BigInteger& n2);
BigInteger operator - (const BigInteger& n1, const BigInteger& n2);

class BigInteger {

private:

    bool is_negative = false;
    std::vector<int> data;  // each int from 0000 to 9999

    constexpr static const int degree_of_10[6] = {1, 10, 100, 1000, 10000, 100000};
    constexpr static const int base_size = 4;
    constexpr static const int base = 10'000;
    constexpr static const int number3 = 3;
    constexpr static const int number4 = 4;
    constexpr static const long double math_pi = 3.1415926535897L;
    constexpr static const long double half = 0.5L;

    static std::vector<size_t> build_reversed_bits(size_t len, size_t deg2) {
        std::vector<size_t> reversed_bits(len);
        for (size_t i = 0; i < len; ++i) {
            reversed_bits[i] = 0;
            for (size_t j = 0; j < deg2; ++j) {
                if (static_cast<bool>((i >> j) & 1)) {
                    reversed_bits[i] += static_cast<size_t>(1 << (deg2 - 1 - j));
                }
            }
        }
        return reversed_bits;
    }


    static void fast_fourier_transform(std::vector<std::complex<long double>>& polynom, size_t deg2) {
        size_t len = (1 << deg2);    // polynom array length
        std::vector<size_t> reversed_bits = build_reversed_bits(len, deg2);    // array of reversed numbers
        std::vector<std::complex<long double>> phi_pow(len / 2);    // array of phi ^ k
        phi_pow[0] = 1;

        for (size_t i = 0; i < len; ++i) {
            if (i < reversed_bits[i]) {
                swap(polynom[i], polynom[reversed_bits[i]]);
            }
        }
        for (size_t j = 2; j <= len; j <<= 1) {
            std::complex<long double> phi = {cosl(2 * math_pi / j), sinl(2 * math_pi / j)};
            for (size_t i = 1; i < j / 2; ++i) {
                phi_pow[i] = phi_pow[i - 1] * phi;
            }
            for (size_t start = 0; start < len; start += j) {
                auto polynom_it1 = polynom.begin() + static_cast<int>(start);
                auto polynom_it2 = polynom.begin() + static_cast<int>(start + j / 2);
                auto polynom_end_it = polynom_it2;
                auto phi_pow_it = phi_pow.begin();
                for (; polynom_it1 != polynom_end_it; ++polynom_it1, ++polynom_it2, ++phi_pow_it) {
                    std::complex<long double> mult = (*phi_pow_it) * (*polynom_it2);
                    *polynom_it2 = *polynom_it1 - mult;
                    *polynom_it1 += mult;
                }
            }
        }
    }

    static void polynom_multiplication(std::vector<std::complex<long double>>& p1, 
                                std::vector<std::complex<long double>>& p2, size_t deg2) {
        size_t len = (1 << deg2);
        for (size_t i = 0; i < len; ++i) {
            p1[i] = {p1[i].real(), p2[i].real()};
        }

        fast_fourier_transform(p1, deg2);
        for (size_t i = 0; i < len; ++i) {
            p1[i] = p1[i] * p1[i];
            p1[i] = {p1[i].real() / len, p1[i].imag() / len};
        }

        fast_fourier_transform(p1, deg2);
        std::reverse(p1.begin() + 1, p1.end());
        for (size_t i = 0; i < len; ++i) {
            p1[i] = {p1[i].imag() / 2, 0};
        }
    }

    size_t optimal_deg2(size_t num_data_size) {
        size_t this_deg2 = 1;
        size_t data_size = data.size();
        while (data_size > 0) {
            data_size >>= 1;
            ++this_deg2;
        }
        size_t num_deg2 = 1;
        while (num_data_size > 0) {
            num_data_size >>= 1;
            ++num_deg2;
        }
        return std::max(this_deg2, num_deg2);
    }

    std::vector<std::complex<long double>> build_polynom(size_t poly_size) const {
        std::vector<std::complex<long double>> poly(poly_size);
        for (size_t i = 0; i < poly_size; ++i) {
            if (i < data.size()) {
                poly[i] = {static_cast<long double>(data[i]), 0};
            } else {
                poly[i] = {0, 0};
            }
        }
        return poly;
    }
    
    static void fix_polynom(std::vector<std::complex<long double>>& poly) {
        size_t poly_size = poly.size();
        for (size_t i = 0; ; ++i) {
            poly[i].real(static_cast<long long>(poly[i].real() + half));
            if (poly[i].real() < base) {
                if (i >= poly_size - 1) {
                    break;
                }
            } else {
                if (i + 1 == poly.size()) {
                    poly.push_back({0, 0});
                }
                poly[i + 1].real(poly[i + 1].real() + static_cast<long long>(poly[i].real()) / base);
                poly[i].real(static_cast<long long>(poly[i].real()) % base);
            }
        }
    }

    void clean_trailing_zeros() {
        while(data.size() > 1 && data.back() == 0) {
            data.pop_back();
        }
    }

    bool is_zero() const {
        return (data.size() == 1 && data[0] == 0);
    }

    std::strong_ordering is_lt_modulo(const BigInteger& num) const {
        if (data.size() < num.data.size()) {
            return std::strong_ordering::less;
        }
        if (data.size() > num.data.size()) {
            return std::strong_ordering::greater;
        }
        for (size_t i = data.size() - 1; i < data.size(); --i) {
            if (data[i] < num.data[i]) {
                return std::strong_ordering::less;
            }
            if (data[i] > num.data[i]) {
                return std::strong_ordering::greater;
            };
        }
        return std::strong_ordering::greater;
    }

    /*void add_one_modulo() {
        ++data[0];
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] == base) {
                data[i] = 0;
                if (i + 1 == data.size()) {
                    data.push_back(0);
                }
                ++data[i + 1];
            } else {
                break;
            }
        }
    }

    void subtract_one_modulo() {
        if (data.size() == 1 && data[0] == 0) {
            data[0] = 1;
            is_negative = !is_negative;
            return;
        }
        --data[0];
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] == -1) {
                data[i] = base - 1;
                --data[i + 1];
            } else {
                break;
            }
        }
        clean_trailing_zeros();
        if (is_zero() && is_negative) {
            is_negative = false;
        }
    }*/

    void add_bigint_modulo(const BigInteger& num) {
        for (size_t i = 0; i < num.data.size(); ++i) {
            if (i == data.size()) {
                data.push_back(num.data[i]);
            } else {
                data[i] += num.data[i];
            }
        }
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] < base) {
                if (i >= num.data.size() - 1) {
                    break;
                }
            }
            int overflow = data[i] / base;
            if (overflow > 0) {
                data[i] %= base;
                if (i + 1 == data.size()) {
                    data.push_back(overflow);
                } else {
                    data[i + 1] += overflow;
                }
            }
        }
    }

    void subtract_bigint_modulo(const BigInteger& num) { 
        int lt_factor = (is_lt_modulo(num) == std::strong_ordering::less ? 1 : -1);
        if (lt_factor == 1) {
            is_negative = !is_negative;
        }

        for (size_t i = 0; i < num.data.size(); ++i) {
            if (i == data.size()) {
                data.push_back(num.data[i] * lt_factor);
            } else {
                data[i] = (data[i] - num.data[i]) * -lt_factor;
            }
        }
        for (size_t i = 0; i < data.size(); ++i) {
            if (i >= num.data.size() && data[i] >= 0) {
                break;
            }
            if (data[i] < 0) {
                data[i] += base;
                --data[i + 1];
            }
        }
        clean_trailing_zeros();
        if (is_zero() && is_negative) {
            is_negative = false;
        }
    }

    void change_by_bigint_modulo(const BigInteger& num, bool are_adding) {
        if (are_adding) {
            add_bigint_modulo(num);
        } else {
            subtract_bigint_modulo(num);
        }
    } 

    void solve_first_division_case(const BigInteger& num, BigInteger& result) {
        for (int i = 1; i < degree_of_10[1]; ++i) {
            *this -= num;
            if (*this < num) {
                result += BigInteger(i);
                break;
            }
        }
    }
    void solve_second_division_case(const BigInteger& num, BigInteger& result, 
                                    size_t digits_number, size_t num_digits_number) {
        BigInteger num_shifted = num;
        num_shifted.shift(digits_number - num_digits_number - 1);
        BigInteger num_shifted_again = num_shifted;
        num_shifted_again.shift(1);
        if (*this < num_shifted_again) {
            for (int i = 1; i < degree_of_10[1]; ++i) {
                *this -= num_shifted;
                if (*this < num_shifted) {
                    BigInteger i_shifted(i);
                    i_shifted.shift(digits_number - num_digits_number - 1);
                    result += i_shifted;
                    break;
                }
            }
        } else {
            for (int i = 1; i < degree_of_10[1]; ++i) {
                *this -= num_shifted_again;
                if (*this < num_shifted_again) {
                    BigInteger i_shifted(i);
                    i_shifted.shift(digits_number - num_digits_number);
                    result += i_shifted;
                    break;
                }
            }
        }
    } 
    
    void add_new_data_to_begin(std::vector<int>& new_data, size_t pack_idx, size_t rem) {
        size_t digits_number = static_cast<size_t>(get_digits_number());
        for (size_t i = 0, data_rem = 0; i < digits_number; ++i, data_rem = (data_rem == base_size - 1 ? 0 : data_rem + 1)) {
            if (rem == 0) {
                new_data.push_back(0);
            }
            int digit = data[i / base_size] / degree_of_10[data_rem] % degree_of_10[1];
            new_data[pack_idx] += digit * degree_of_10[rem];
            ++rem;
            if (rem == base_size) {
                rem = 0;
                ++pack_idx;
            }
        }
        std::swap(data, new_data);
    }

public:

    BigInteger() {
        is_negative = false;
        data = {0};
    }

    // BigInteger(const BigInteger& other): is_negative(other.is_negative), data(other.data) {}

    BigInteger(int number) {
        is_negative = false;
        if (number < 0) {
            is_negative = true;
            number = -number;
        }
        do {
            data.push_back(number % base);
            number /= base;
        } while (number > 0);
    }

    BigInteger(const std::string& str) {
        if (str[0] == '-' && str != "-0") {
            is_negative = true;
        }
        size_t pack_idx = 0;
        size_t rem = 0;
        for (auto it = str.rbegin(); it != str.rend() && *it != '-'; ++it) {
            if (rem == 0) {
                data.push_back(0);
            }
            data[pack_idx] += (*it - '0') * degree_of_10[rem];
            ++rem;
            if (rem == base_size) {
                rem = 0;
                ++pack_idx;
            }
        }
        clean_trailing_zeros();
    }

    std::strong_ordering operator <=> (const BigInteger& other) const {
        std::strong_ordering sign_ordering = static_cast<int>(other.is_negative) <=> static_cast<int>(is_negative);
        if (is_negative != other.is_negative) {
            return sign_ordering;
        }
        /*if (is_negative && !other.is_negative) {
            return std::strong_ordering::less;
        }
        if (!is_negative && other.is_negative) {
            return std::strong_ordering::greater;
        }*/
        if (data.size() != other.data.size()) {
            bool size_comp_factor = data.size() < other.data.size();
            return ((size_comp_factor != is_negative) ? std::strong_ordering::less : std::strong_ordering::greater);
        }

        for (size_t i = data.size() - 1; i < data.size(); --i) {
            if (data[i] != other.data[i]) {
                bool data_comp_factor = data[i] < other.data[i];
                return ((data_comp_factor != is_negative) ? std::strong_ordering::less : std::strong_ordering::greater);
            }
        }
        return std::strong_ordering::equal;
    }

    bool operator == (const BigInteger& other) const = default;
        /*{
        if (data.size() != other.data.size()) {
            return false;
        }
        if (is_negative != other.is_negative) {
            return false;
        }
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }*/

    /*bool operator != (const BigInteger& other) const {
        return !(*this == other);
    }*/

    std::string toString() const {
        std::string res;
        for (auto it = data.rbegin(); it != data.rend(); ++it) {
            int num = (*it);
            res += char('0' + num / degree_of_10[1 + 2] % degree_of_10[1]);
            res += char('0' + num / degree_of_10[2] % degree_of_10[1]);
            res += char('0' + num / degree_of_10[1] % degree_of_10[1]);
            res += char('0' + num % degree_of_10[1]);
        }
        size_t begin_without_0 = 0;
        while (begin_without_0 < res.size() && res[begin_without_0] == '0') {
            ++begin_without_0;
        }
        res = (is_negative ? "-" : "") + res.substr(begin_without_0, res.size() - begin_without_0 );
        if (res == "-" || res.empty()) {
            res = "0";
        }
        return res;
    }

    void change_sign() {
        is_negative = !is_negative;
    }

    int get_digits_number() const {
        if (data.back() / degree_of_10[number3] % degree_of_10[1] != 0) {
            return base_size * static_cast<int>(data.size() - 1) + number4;
        }
        if (data.back() / degree_of_10[2] % degree_of_10[1] != 0) {
            return base_size * static_cast<int>(data.size() - 1) + number3;
        }
        if (data.back() / degree_of_10[1] % degree_of_10[1] != 0) {
            return base_size * static_cast<int>(data.size() - 1) + 2;
        }
        return base_size * static_cast<int>(data.size() - 1) + 1;
    }

    void shift(size_t pow10) {
        if (*this == 0) {
            return;
        }
        std::vector<int> new_data;
        size_t pack_idx = 0;
        size_t rem = 0;
        for (size_t i = 0; i < pow10; ++i) {
            if (rem == 0) {
                new_data.push_back(0);
            }
            new_data[pack_idx] = 0;
            ++rem;
            if (rem == base_size) {
                rem = 0;
                ++pack_idx;
            }
        }
        add_new_data_to_begin(new_data, pack_idx, rem);
    }

    /*BigInteger& operator = (const BigInteger& other) {
        is_negative = other.is_negative;
        data = other.data;
        return *this;
    }*/

    BigInteger operator - () const {
        BigInteger num = *this;
        if (!is_zero()) {
            num.is_negative = !is_negative;
        }
        return num;
    }

    BigInteger& operator += (const BigInteger& num) {
        change_by_bigint_modulo(num, is_negative == num.is_negative);
        return *this;
    }

    BigInteger& operator -= (const BigInteger& num) {
        return *this += -num;
    }

    BigInteger& operator ++ () {
        return *this += 1;
        /*if (!is_negative) {
            *this += 1;
            add_one_modulo();
        } else {
            subtract_one_modulo();
        }
        return *this;*/
    }

    BigInteger operator ++ (int) {
        BigInteger return_value = *this;
        ++(*this);
        return return_value;
    }
    
    BigInteger& operator -- () {
        return *this -= 1;
        /*if (is_negative) {
            add_one_modulo();
        } else {
            subtract_one_modulo();
        }
        return *this;*/
    }

    BigInteger operator -- (int) {
        BigInteger return_value = *this;
        --(*this);
        return return_value;
    }
 
    BigInteger& operator *= (const BigInteger& num) {
        if (!(*this) || !num) {
            data = {0};
            is_negative = false;
            return *this;
        }
        if (num.is_negative) {
            is_negative = !is_negative;
        }
        size_t deg2 = optimal_deg2(num.data.size());
        size_t poly_size = static_cast<size_t>(1 << deg2);
        std::vector<std::complex<long double>> poly = build_polynom(poly_size);
        std::vector<std::complex<long double>> num_poly = num.build_polynom(poly_size);
        polynom_multiplication(poly, num_poly, deg2);
        fix_polynom(poly);
        for (size_t i = 0; i < poly.size(); ++i) {
            if (i == data.size()) {
                data.push_back(static_cast<int>(poly[i].real() + half));
            } else {
                data[i] = static_cast<int>(poly[i].real() + half);
            }
        }
        clean_trailing_zeros();
        return *this;
    }

    
    BigInteger& operator /= (BigInteger num) {
        bool negativeness = is_negative;
        is_negative = false;
        if (!(*this)) {
            return *this;
        }
        if (num.is_negative) {
            negativeness = !negativeness;
        }
        num.is_negative = false;
        BigInteger result = 0;
        size_t num_digits_number = static_cast<size_t>(num.get_digits_number());
        while (*this >= num) {
            size_t digits_number = static_cast<size_t>(get_digits_number());
            if (digits_number == num_digits_number) {
                solve_first_division_case(num, result);
            } else {
                solve_second_division_case(num, result, digits_number, num_digits_number);
            }
        }
        *this = result;
        clean_trailing_zeros();
        is_negative = negativeness;
        if (data.size() == 1 && data[0] == 0) {
            is_negative = false;
        }
        return *this;
    }

    BigInteger& operator %= (BigInteger num) {
        bool negativeness = is_negative;
        is_negative = false;
        num.is_negative = false;
        BigInteger copy = *this;
        *this -= ((copy /= num) *= num);
        is_negative = negativeness;
        return *this;
    }

    explicit operator bool() const {
        return !is_zero();
    }

};

BigInteger operator + (const BigInteger& n1, const BigInteger& n2) {
    BigInteger n1_copy = n1;
    return n1_copy += n2;
}

BigInteger operator - (const BigInteger& n1, const BigInteger& n2) {
    BigInteger n1_copy = n1;
    return n1_copy -= n2;
}

BigInteger operator * (const BigInteger& n1, const BigInteger& n2) {
    BigInteger n1_copy = n1;
    return n1_copy *= n2;
}

BigInteger operator / (const BigInteger& n1, const BigInteger& n2) {
    BigInteger n1_copy = n1;
    return n1_copy /= n2;
}

BigInteger operator % (const BigInteger& n1, const BigInteger& n2) {
    BigInteger n1_copy = n1;
    return n1_copy %= n2;
}

BigInteger operator "" _bi(const char* chars) {
    return BigInteger(std::string(chars, std::strlen(chars)));
}

BigInteger operator ""_bi(unsigned long long num) {
    return BigInteger(std::to_string(num));
}

std::istream& operator >> (std::istream& input, BigInteger& num) {
    std::string str;
    input >> str;
    num = BigInteger(str);
    return input;
}

std::ostream& operator << (std::ostream& output, const BigInteger& num) {
    output << num.toString();
    return output;
}

BigInteger abs(const BigInteger& num) {
    if (num < 0) {
        return -num;
    }
    return num;
}


class Rational {

private:

    BigInteger numerator;
    BigInteger denominator;

    constexpr static size_t number_precision = 12;

    BigInteger gcd(const BigInteger& num1, const BigInteger& num2) {
        if (!num2) {
            return num1;
        }
        return gcd(num2, num1 % num2);
    }

    void reduce() {
        BigInteger greatest_common_divisor = gcd(abs(numerator), denominator);
        numerator /= greatest_common_divisor;
        denominator /= greatest_common_divisor;
    }

public:

    Rational(): numerator(0), denominator(1) {}
    // Rational(const Rational& other): numerator(other.numerator), denominator(other.denominator) {}
    Rational(const BigInteger& n): numerator(n), denominator(1) {}
    Rational(int number) {
        *this = Rational(BigInteger(number));
    }

    auto operator <=> (const Rational& num) const {
        if (numerator < 0 && num.numerator >= 0) {
            return std::strong_ordering::less;
        }
        if (numerator >= 0 && num.numerator < 0) {
            return std::strong_ordering::greater;
        }
        BigInteger left = numerator * num.denominator;
        BigInteger right = denominator * num.numerator;
        return left <=> right;
    }

    bool operator == (const Rational& num) const = default;

    std::string toString() const {
        std::string res = numerator.toString();
        if (denominator != 1) {
            res += "/" + denominator.toString();
        }
        return res;
    }

    // Rational& operator = (const Rational& num) = default;

    Rational operator - () const {
        Rational copy = *this;
        copy.numerator.change_sign();
        return copy;
    }

    Rational& operator += (const Rational& num) {
        numerator = numerator * num.denominator + denominator * num.numerator;
        denominator = denominator * num.denominator;
        reduce();
        return *this;
    }

    Rational& operator -= (const Rational& num) {
        return *this += -num;
    }

    Rational& operator *= (const Rational& num) {
        numerator *= num.numerator;
        denominator *= num.denominator;
        reduce();
        return *this;
    }

    Rational& operator /= (const Rational& num) {
        numerator *= num.denominator;
        denominator *= num.numerator;
        if (denominator < 0) {
            numerator.change_sign();
            denominator.change_sign();
        }
        reduce();
        return *this;
    }

    std::string asDecimal(size_t precision = 0) const {
        BigInteger shifted_numerator = numerator;
        shifted_numerator.shift(precision);
        std::string result = (shifted_numerator / denominator).toString();
        if (precision != 0) {
            size_t digits_number = (result[0] == '-' ? result.size() - 1 : result.size());
            if (digits_number <= precision) {
                std::string zeros = (numerator < 0 ? "-0." : "0.");
                for (size_t i = 0; i < precision - digits_number; ++i) {
                    zeros.push_back('0');
                }
                if (result[0] == '-') {
                    result.erase(result.begin());
                }
                result = zeros + result;
            } else {
                size_t position = result.size() - precision;
                result.insert(result.begin() + static_cast<int>(position), '.');
            }
        }
        return result;
    }

    explicit operator double() const {
        return std::stod(asDecimal(number_precision));
    }

};

Rational operator + (const Rational& n1, const Rational& n2) {
    Rational n1_copy = n1;
    return n1_copy += n2;
}

Rational operator - (const Rational& n1, const Rational& n2) {
    Rational n1_copy = n1;
    return n1_copy -= n2;
}

Rational operator * (const Rational& n1, const Rational& n2) {
    Rational n1_copy = n1;
    return n1_copy *= n2;
}

Rational operator / (const Rational& n1, const Rational& n2) {
    Rational n1_copy = n1;
    return n1_copy /= n2;
}
