#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>

struct Point;
class Line;

class Polygon;
class Rectangle;
class Square;
class Triangle;

class Ellipse;
class Circle;

const double pi = 3.14159265359;

bool areEqual(double x, double y) {
    return std::abs(x - y) < 1e-5;
}

struct Point {
    double x;
    double y;

    Point() = default;

    Point(double x, double y): x(x), y(y) {}

    Point(const Point& p): x(p.x), y(p.y) {}

    ~Point() = default;


    Point& operator = (const Point& p) {
        x = p.x;
        y = p.y;
        return *this;
    }

    bool operator == (const Point& p) const {
        return (areEqual(x, p.x) && areEqual(y, p.y));
    }
    bool operator != (const Point& p) const {
        return (!areEqual(x, p.x) || !areEqual(y, p.y));
    }

    Point operator - () const {
        return Point(-x, -y);
    }

    Point& operator += (const Point& p) {
        x += p.x;
        y += p.y;
        return *this;
    }

    Point operator + (Point p) const {
        return p += *this;
    }

    Point& operator -= (const Point& p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    Point operator - (Point p) const {
        return -(p -= *this);
    }

    Point& operator *= (double k) {
        x *= k;
        y *= k;
        return *this;
    }

    Point operator * (double k) const {
        Point copy = *this;
        return copy *= k;
    }

    Point& operator /= (double k) {
        x /= k;
        y /= k;
        return *this;
    }

    Point operator / (double k) const {
        Point copy = *this;
        return copy /= k;
    }

    double operator * (Point p) const {
        return x * p.x + y * p.y;
    }

    double operator % (Point p) const {
        return x * p.y - y * p.x;
    }

    friend std::ostream& operator << (std::ostream& output, const Point& p) {
        output << '(' << p.x << ", " << p.y << ')';
        return output;
    }

    double getAngle(const Point& p) const {
        return atan2(*this % p, *this * p);
    }

    Point rotate(const Point& O, double angle) const {
        double cs = cos(angle);
        double sn = sin(angle);
        double xx = (x - O.x) * cs - (y - O.y) * sn;
        double yy = (x - O.x) * sn + (y - O.y) * cs;
        return O + Point(xx, yy);
    }

    double rad() const {
        return sqrt(x * x + y * y);
    }

    double rad2() const {
        return x * x + y * y;
    }

    Point scale(Point center, double coefficient) const {
        return center += (*this - center) * coefficient;
    }

    template<class Line>
    Point getProjection(const Line& t) const {
        double x1 = (t.b * t.b * x - t.a * t.c - t.a * t.b * y) / (t.a * t.a + t.b * t.b);
        double y1 = (t.a * t.a * y - t.b * t.c - t.a * t.b * x) / (t.a * t.a + t.b * t.b);
        return Point(x1, y1);
    }

    template<class Line>
    Line getPerpendicular(const Line& line) const {
        return Line(line.b, -line.a, line.a * y - line.b * x);
    }
};


class Line {

public:

    double a;
    double b;
    double c;

    Line() = default;

    Line(double a, double b, double c): a(a), b(b), c(c) {}

    Line(const Line& line): a(line.a), b(line.b), c(line.c) {}

    Line(const Point& A, const Point& B) {
        if (areEqual(A.x - B.x, 0)) {
            a = 1;
            b = 0;
            c = -A.x;
        }
        else {
            a = (B.y - A.y) / (A.x - B.x);
            b = 1;
            c = (A.x * B.y - B.x * A.y) / (B.x - A.x);
        }
    }

    Line(double d, double k): a(-d), b(1), c(-k) {}

    Line(const Point& A, double k) {
        Point B(A.x + 1, A.y + k);
        *this = Line(A, B);
    }

    ~Line() = default;

    Line& operator = (const Line& line) {
        a = line.a;
        b = line.b;
        c = line.c;
        return *this;
    }

    bool operator == (const Line& line) const {
        double k = 1;
        if (!areEqual(a, 0) && !areEqual(line.a, 0)) {
            k = line.a / a;
        }
        else if (!areEqual(b, 0) && !areEqual(line.b, 0)) {
            k = line.b / b;
        }
        else if (!areEqual(c, 0) && !areEqual(line.c, 0)) {
            k = line.c / c;
        }
        else {
            return false;
        }
        return (areEqual(a * k, line.a) && areEqual(b * k, line.b) && areEqual(c * k, line.c));
    }

    bool operator != (const Line& line) const {
        return !(*this == line);
    }

    Point intersection(const Line& line) {
        double x = (line.c * b - c * line.b) / (a * line.b - line.a * b);
        double y = (line.c * a - c * line.a) / (line.a * b - a * line.b);
        return Point(x, y);
    }
};


class Shape {

public:

    virtual double perimeter() const = 0;
    virtual double area() const = 0;
    virtual bool isCongruentTo(const Shape& other) const = 0;
    virtual bool isSimilarTo(const Shape& other) const = 0;
    virtual bool containsPoint(const Point& p) const = 0;
    virtual void rotate(const Point& center, double angle) = 0;
    virtual void reflect(const Point& center) = 0;
    virtual void reflect(const Line& axis) = 0;
    virtual void scale(const Point& center, double coefficient) = 0;
    virtual ~Shape() = default;

};


int timer = 0;


class Polygon: public Shape {

protected:

    std::vector<Point> verts;

    double areVertsSimilar(std::vector<Point>& other_verts) const {
        for (size_t x = 0; x < verts.size(); ++x) {
            double coef = -1.0;
            bool straight = false;
            bool reversed = false;
            for (size_t t = x; t < x + verts.size(); ++t) {
                size_t i1 = t - x;
                size_t i2 = (i1 + 1 < verts.size() ? i1 + 1 : i1 + 1 - verts.size());
                size_t i3 = (i2 + 1 < verts.size() ? i2 + 1 : i2 + 1 - verts.size());
                size_t j1 = (t < verts.size() ? t : t - verts.size());
                size_t j2 = (j1 + 1 < verts.size() ? j1 + 1 : j1 + 1 - verts.size());
                size_t j3 = (j2 + 1 < verts.size() ? j2 + 1 : j2 + 1 - verts.size());
                double angle1 = (verts[i1] - verts[i2]).getAngle(verts[i3] - verts[i2]);
                double angle2 = (other_verts[j1] - other_verts[j2]).getAngle(other_verts[j3] - other_verts[j2]);
                double segment1 = (verts[i1] - verts[i2]).rad2();
                double segment2 = (other_verts[j1] - other_verts[j2]).rad2();
                if (areEqual(angle1, angle2)) {
                    straight = true;
                }
                else if (areEqual(angle1, -angle2)) {
                    reversed = true;
                }
                else {
                    coef = -1.0;
                    break;
                }
                if (!areEqual(coef, -1.0) && !areEqual(coef, segment2 / segment1)) {
                    coef = -1.0;
                    break;
                }
                coef = segment2 / segment1;
            }
            if (!areEqual(coef, -1.0) && (straight ^ reversed)) {
                return coef;
            }
        }
        std::vector<Point> rverts = verts;
        for (size_t i = 0; i < verts.size() / 2; ++i) {
            Point temp = rverts[i];
            rverts[i] = rverts[verts.size() - 1 - i];
            rverts[verts.size() - 1 - i] = temp;
        }
        for (size_t x = 0; x < verts.size(); ++x) {
            double coef = -1.0;
            bool straight = false;
            bool reversed = false;
            for (size_t t = x; t < x + verts.size(); ++t) {
                size_t i1 = t - x;
                size_t i2 = (i1 + 1 < verts.size() ? i1 + 1 : i1 + 1 - verts.size());
                size_t i3 = (i2 + 1 < verts.size() ? i2 + 1 : i2 + 1 - verts.size());
                size_t j1 = (t < verts.size() ? t : t - verts.size());
                size_t j2 = (j1 + 1 < verts.size() ? j1 + 1 : j1 + 1 - verts.size());
                size_t j3 = (j2 + 1 < verts.size() ? j2 + 1 : j2 + 1 - verts.size());
                double angle1 = (rverts[i1] - rverts[i2]).getAngle(rverts[i3] - rverts[i2]);
                double angle2 = (other_verts[j1] - other_verts[j2]).getAngle(other_verts[j3] - other_verts[j2]);
                double segment1 = (rverts[i1] - rverts[i2]).rad2();
                double segment2 = (other_verts[j1] - other_verts[j2]).rad2();
                if (areEqual(angle1, angle2)) {
                    straight = true;
                }
                else if (areEqual(angle1, -angle2)) {
                    reversed = true;
                }
                else {
                    coef = -1.0;
                    break;
                }
                if (!areEqual(coef, -1.0) && !areEqual(coef, segment2 / segment1)) {
                    coef = -1.0;
                    break;
                }
                coef = segment2 / segment1;
            }
            if (!areEqual(coef, -1.0) && (straight ^ reversed)) {
                return coef;
            }
        }
        return -1.0;
    }

    template<class First, class... Second>
    void addVertices(const First& first_element, const Second&... other_elements) {
        verts.push_back(first_element);
        addVertices(other_elements...);
    }

    template<class Point>
    void addVertices(const Point& p) {
        verts.push_back(p);
    }

    void addVertices() {}

    static double getTriangleSignedArea(const Point& A, const Point& B, const Point& C) {
        return (B - A) % (C - A) / 2;
    }

    bool isSimilarClass(const Shape& other) const;

public:

    Polygon() = default;

    Polygon(const Polygon& other): verts(other.verts) {}

    Polygon& operator = (const Polygon& other) = default;

    ~Polygon() = default;

    Polygon(std::vector<Point> verts): verts(verts) {}

    Polygon(const std::initializer_list<Point>& lst): verts(lst) {}

    template<class... Points>
    explicit Polygon(const Points&... points) {
        addVertices(points...);
    }


    size_t verticesCount() const {
        return verts.size();
    }

    std::vector<Point> getVertices() const {
        return verts;
    }
    
    bool isConvex() const {
        bool exists_positive = false, exists_negative = false;
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t j = i + 1, k = i + 2;
            if (j == verts.size()) {
                j -= verts.size();
            }
            if (k >= verts.size()) {
                k -= verts.size();
            }
            double angle = (verts[i] - verts[j]).getAngle(verts[k] - verts[j]);
            if (!areEqual(angle, pi)) {
                if (angle > 0) {
                    exists_positive = true;
                }
                else {
                    exists_negative = true;
                }
            }
        }
        return (exists_positive ^ exists_negative);
    }
    
    void rotate(const Point& center, double angle) override {
        angle = angle / 180 * pi;
        for (Point& p : verts) {
            p = p.rotate(center, angle);
        }
    }

    void scale(const Point& center, double coefficient) override {
        for (Point& p : verts) {
            p = p.scale(center, coefficient);
        }
    }

    void reflect(const Point& center) override {
        scale(center, -1);
    }

    void reflect(const Line& axis) override {
        for (Point& p : verts) {
            Point p1 = p.getProjection(axis);
            p = p1 * 2 - p;
        }
    }

    double perimeter() const override {
        double sum = 0;
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t j = (i + 1 == verts.size() ? 0 : i + 1);
            sum += (verts[i] - verts[j]).rad();
        }
        return sum;
    }

    double area() const override {
        double sum = 0;
        Point p(0, 0);
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t j = (i + 1 == verts.size() ? 0 : i + 1);
            sum += getTriangleSignedArea(p, verts[i], verts[j]);
        }
        return std::abs(sum);
    }

    bool areVertsSame(std::vector<Point>& other_verts) const {
        for (size_t i = 0; i < verts.size(); ++i) {
            bool same = true;
            for (size_t j = i; j < i + verts.size(); ++j) {
                size_t k = (j >= verts.size() ? j - verts.size() : j);
                if (verts[j - i] != other_verts[k]) {
                    same = false;
                    break;
                }
            }
            if (same) {
                return true;
            }
        }
        std::vector<Point> rverts = verts;
        for (size_t i = 0; i < verts.size() / 2; ++i) {
            Point temp = rverts[i];
            rverts[i] = rverts[verts.size() - 1 - i];
            rverts[verts.size() - 1 - i] = temp;
        }
        for (size_t i = 0; i < verts.size(); ++i) {
            bool same = true;
            for (size_t j = i; j < i + verts.size(); ++j) {
                size_t k = (j >= verts.size() ? j - verts.size() : j);
                if (rverts[j - i] != other_verts[k]) {
                    same = false;
                    break;
                }
            }
            if (same) {
                return true;
            }
        }
        return false;
    }

    bool isCongruentTo(const Shape& other) const override {
        if (!isSimilarClass(other)) {
            return false;
        }
        const Polygon& pol = dynamic_cast<const Polygon&>(other);
        std::vector<Point> other_verts = pol.getVertices();
        if (areEqual(areVertsSimilar(other_verts), 1.0)) {
            return true;
        }
        return false;
    }

    bool isSimilarTo(const Shape& other) const override {
        if (!isSimilarClass(other)) {
            return false;
        }
        const Polygon& pol = dynamic_cast<const Polygon&>(other);
        std::vector<Point> other_verts = pol.getVertices();
        if (!areEqual(areVertsSimilar(other_verts), -1.0)) {
            return true;
        }
        return false;
    }

    bool containsPoint(const Point& p) const override {
        double sum = 0;
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t j = (i + 1 == verts.size() ? 0 : i + 1);
            sum += (verts[i] - p).getAngle(verts[j] - p);
        }
        return !areEqual(sum, 0);
    }

};


class Ellipse: public Shape {

protected:

    Point A;
    Point B;
    double dist;

    bool isSimilarClass(const Shape& other) const;

public:

    Ellipse() = default;

    Ellipse(const Ellipse& other): A(other.A), B(other.B), dist(other.dist) {}

    ~Ellipse() = default;


    Ellipse(const Point& A, const Point& B, double dist): A(A), B(B), dist(dist) {}

    std::pair<Point, Point> focuses() const {
        return {A, B};
    }

    std::pair<Line, Line> directrices() const {
        Point A1 = A + (A - B) / (A - B).rad() * ((dist - (A - B).rad()) / 2 * (1 + 1 / eccentricity()));
        Point B1 = B + (B - A) / (B - A).rad() * ((dist - (B - A).rad()) / 2 * (1 + 1 / eccentricity()));
        Line AB = Line(A, B);
        Line line1 = A1.getPerpendicular(AB);
        Line line2 = B1.getPerpendicular(AB);
        return {line1, line2};
    }

    double eccentricity() const {
        return (A - B).rad() / dist;
    }

    Point center() const {
        return (A + B) / 2;
    }

    void rotate(const Point& center, double angle) override {
        angle = angle / 180 * pi;
        A = A.rotate(center, angle);
        B = B.rotate(center, angle);
    }

    void scale(const Point& center, double coefficient) override {
        A = A.scale(center, coefficient);
        B = B.scale(center, coefficient);
        dist *= std::abs(coefficient);
    }

    void reflect(const Point& center) override {
        scale(center, -1);
    }

    void reflect(const Line& axis) override {
        Point A1 = A.getProjection(axis);
        Point B1 = B.getProjection(axis);
        A = A1 * 2 - A;
        B = B1 * 2 - B;
    }

    double perimeter() const override {
        double a = dist / 2;
        double b = a * sqrt(1 - (A - B).rad2() / dist / dist);
        return a * 4 * std::comp_ellint_2(sqrt(1 - b * b / a / a));
    }

    double area() const override {
        double a = dist / 2;
        double b = a * sqrt(1 - (A - B).rad2() / dist / dist);
        return pi * a * b;
    }

    bool isCongruentTo(const Shape& other) const override {
        if (!isSimilarClass(other)) {
            return false;
        }
        const Ellipse& ell = dynamic_cast<const Ellipse&>(other);
        std::pair<Point, Point> f1 = focuses();
        std::pair<Point, Point> f2 = ell.focuses();
        return areEqual((f1.first - f1.second).rad2(), (f2.first - f2.second).rad2()) && 
                areEqual(eccentricity(), ell.eccentricity());
    }

    bool isSimilarTo(const Shape& other) const override {
        if (!isSimilarClass(other)) {
            return false;
        }
        const Ellipse& ell = dynamic_cast<const Ellipse&>(other);
        return areEqual(eccentricity(), ell.eccentricity());
    }

    bool containsPoint(const Point& p) const override {
        return (A - p).rad() + (B - p).rad() <= dist + 1e-9;
    }

};


class Circle: public Ellipse {

public:

    Circle() = default;

    Circle(const Circle& other) = default;

    ~Circle() = default;

    Circle(const Point& O, double r): Ellipse() {
        A = O;
        B = O;
        dist = r * 2;
    }


    double radius() {
        return dist / 2;
    }

};


class Rectangle: public Polygon {

public:

    Rectangle() = default;

    Rectangle(const Rectangle& other) = default;

    Rectangle(const Point& A, const Point& C, double ratio) {
        if (ratio < 1.0) {
            ratio = 1 / ratio;
        }
        Point O = (A + C) / 2;
        double angle = atan(ratio) * 2;
        Point B = A.rotate(O, angle);
        Point D = A + C - B;
        verts = {A, B, C, D};
    }

    ~Rectangle() = default;

    Point center() {
        return (verts[0] + verts[2]) / 2;
    }

    std::pair<Line, Line> diagonals() {
        return std::pair<Line, Line>(Line(verts[0], verts[2]), Line(verts[1], verts[3]));
    }

};


class Square: public Rectangle {

public:

    Square() = default;

    Square(const Square& other) = default;

    Square(const Point& A, const Point& C) {
        Point B = A.rotate((A + C) / 2, pi / 2);
        Point D = A + C - B;
        verts = {A, B, C, D};
    }

    ~Square() = default;

    Circle circumscribedCircle() const {
        return Circle((verts[0] + verts[2]) / 2, ((verts[0] - verts[2]) / 2).rad());
    }

    Circle inscribedCircle() const {
        return Circle((verts[0] + verts[2]) / 2, ((verts[0] - verts[2]) / 2).rad() / sqrt(2));
    }
};


class Triangle: public Polygon {

public:

    Triangle() = default;

    Triangle(const Triangle& other) = default;

    template<class... Points>
    explicit Triangle(const Points&... points): Polygon(points...) {}

    Triangle(const std::initializer_list<Point>& lst): Polygon(lst) {}

    ~Triangle() = default;

    Circle circumscribedCircle() const {
        double angle01 = (verts[0] - verts[2]).getAngle(verts[1] - verts[2]);
        Point O = verts[1].scale(verts[0],  1 / (2 * sin(std::abs(angle01))));
        if (angle01 > 0) {
            O = O.rotate(verts[0], pi / 2 - angle01);
        }
        else {
            O = O.rotate(verts[0], -pi / 2 - angle01);
        }
        return Circle(O, (O - verts[0]).rad());
    }

    Circle inscribedCircle() const {
        double ab = (verts[0] - verts[1]).rad();
        double bc = (verts[1] - verts[2]).rad();
        double ca = (verts[2] - verts[0]).rad();
        double a = (ab + ca - bc) / 2;
        Point B1 = verts[1].scale(verts[0], a / ab);
        Point C1 = verts[2].scale(verts[0], a / ca);
        Point I = (B1.getPerpendicular(Line(verts[0], verts[1]))).intersection(C1.getPerpendicular(Line(verts[0], verts[2])));
        return Circle(I, (I - B1).rad());
    }

    Point centroid() const {
        return (verts[0] + verts[1] + verts[2]) / 3;
    }

    Point orthocenter() const {
        Line h1 = verts[1].getPerpendicular(Line(verts[0], verts[2]));
        Line h2 = verts[2].getPerpendicular(Line(verts[0], verts[1]));
        return h1.intersection(h2);
    }

    Line EulerLine() const {
        return Line(circumscribedCircle().center(), orthocenter());
    }

    Circle ninePointsCircle() const {
        Point H = orthocenter();
        Point H0 = (verts[0] + H) / 2;
        Point H1 = (verts[1] + H) / 2;
        Point H2 = (verts[2] + H) / 2;
        return Triangle(H0, H1, H2).circumscribedCircle();
    }

};

bool operator == (const Shape& a, const Shape& b) {
    bool is_a_ellipse = false;
    if (typeid(a) == typeid(Ellipse) || typeid(a) == typeid(Circle)) {
        is_a_ellipse = true;
    }
    bool is_b_ellipse = false;
    if (typeid(b) == typeid(Ellipse) || typeid(b) == typeid(Circle)) {
        is_b_ellipse = true;
    }
    if (is_a_ellipse && is_b_ellipse) {
        const Ellipse& a_ell = dynamic_cast<const Ellipse&>(a);
        const Ellipse& b_ell = dynamic_cast<const Ellipse&>(b);
        std::pair<Point, Point> f1 = a_ell.focuses();
        std::pair<Point, Point> f2 = b_ell.focuses();
        return ((f1.first == f2.first && f1.second == f2.second) || 
                (f1.first == f2.second && f1.second == f2.first)) && 
                areEqual(a_ell.eccentricity(), b_ell.eccentricity());
    }
    if (!is_a_ellipse && !is_b_ellipse) {
        const Polygon& a_pol = dynamic_cast<const Polygon&>(a);
        const Polygon& b_pol = dynamic_cast<const Polygon&>(b);
        std::vector<Point> a_verts = a_pol.getVertices();
        std::vector<Point> b_verts = b_pol.getVertices();
        if (a_verts.size() != b_verts.size()) {
            return false;
        }
        return a_pol.areVertsSame(b_verts);
    }
    return false;
}

bool Polygon::isSimilarClass(const Shape& other) const {
    return (typeid(other) != typeid(Ellipse) && typeid(other) != typeid(Circle));
}

bool Ellipse::isSimilarClass(const Shape& other) const {
    return (typeid(other) == typeid(Ellipse) || typeid(other) == typeid(Circle));
}
