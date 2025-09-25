#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>

#include "unrolled_list.h"

template <class T, std::size_t C, class A>
void dump(const unrolled_list<T, C, A>& ul, const std::string& title) {
    std::cout << title << " [size=" << ul.size() << "]: ";
    for (const auto& x : ul) std::cout << x << ' ';
    std::cout << '\n';
}

struct Tracer {
    std::string s;
    static inline int ctor = 0, dtor = 0, move_ctor = 0, copy_ctor = 0;
    Tracer() : s() { ++ctor; }
    Tracer(std::string v) : s(std::move(v)) { ++ctor; }
    Tracer(const Tracer& o) : s(o.s) { ++copy_ctor; }
    Tracer(Tracer&& o) noexcept : s(std::move(o.s)) { ++move_ctor; }
    Tracer& operator=(const Tracer&) = default;
    Tracer& operator=(Tracer&&) noexcept = default;
    ~Tracer() { ++dtor; }
    friend std::ostream& operator<<(std::ostream& os, const Tracer& t) {
        return os << t.s;
    }
    friend bool operator==(const Tracer& a, const Tracer& b) { return a.s == b.s; }
};

int main() {
    try {
        std::cout << "== Базовые сценарии (int, capacity=4) ==\n";
        unrolled_list<int, 4> a;
        assert(a.empty());

        a.push_back(1);
        a.push_back(2);
        a.push_front(0);
        a.push_back(3);
        a.push_back(4);
        dump(a, "A после push_*");

        std::cout << "front=" << a.front() << ", back=" << a.back() << "\n";

        std::cout << "Прямой обход: ";
        for (auto it = a.begin(); it != a.end(); ++it) std::cout << *it << ' ';
        std::cout << "\n";

        const auto& ca = a;
        std::cout << "Const-обход: ";
        for (auto it = ca.cbegin(); it != ca.cend(); ++it) std::cout << *it << ' ';
        std::cout << "\n";

        std::cout << "Обратный обход: ";
        for (auto it = a.rbegin(); it != a.rend(); ++it) std::cout << *it << ' ';
        std::cout << "\n";

        auto mid = ++a.begin();
        a.insert(mid, 42);
        a.insert(a.cend(), 99);
        a.insert(++a.begin(), 2, 7);
        dump(a, "A после insert");

        std::vector<int> ext{8, 9, 10};
        a.insert(a.cbegin(), ext.begin(), ext.end());
        dump(a, "A после insert(range)");

        a.erase(++a.cbegin());
        dump(a, "A после erase(it)");

        {
            auto first = a.begin();
            auto last  = a.begin(); ++last; ++last; ++last;
            a.erase(first, last);
        }
        dump(a, "A после erase(first,last)");

        a.pop_front();
        a.pop_back();
        dump(a, "A после pop_front/back");

        unrolled_list<int, 4> b = a;
        assert(b == a);
        b.push_back(123);
        assert(b != a);
        dump(b, "B (копия A + push_back)");

        unrolled_list<int, 4> c = std::move(b);
        dump(c, "C (move-constructed из B)");
        assert(b.size() == 0);

        unrolled_list<int, 4> d;
        d = std::move(c);
        dump(d, "D = move(C)");
        assert(c.size() == 0);

        swap(a, d);
        dump(a, "A после swap(A,D)");
        dump(d, "D после swap(A,D)");

        std::cout << "\n== Конструкторы: size/value, initializer_list, диапазон ==\n";
        unrolled_list<std::string, 3> s1(5, "hi");
        dump(s1, "s1 (n,value)");
        unrolled_list<std::string, 3> s2{"a","b","c","d"};
        dump(s2, "s2 (init_list)");
        std::vector<std::string> v = {"x","y","z"};
        unrolled_list<std::string, 3> s3(v.begin(), v.end());
        dump(s3, "s3 (range)");
        assert(!(s1 == s2));
        s3.push_back("w");
        dump(s3, "s3 + push_back");

        std::cout << "\n== Тип с нетривиальной семантикой (Tracer, capacity=2) ==\n";
        {
            Tracer::ctor = Tracer::dtor = Tracer::move_ctor = Tracer::copy_ctor = 0;

            unrolled_list<Tracer, 2> t;
            t.push_back(Tracer{"A"});
            Tracer tmp{"B"};
            t.push_back(tmp);
            t.push_front(Tracer{"C"});
            dump(t, "t<Tracer>");

            auto it = ++t.begin();
            t.insert(it, Tracer{"M"});
            dump(t, "t после insert");

            while (!t.empty()) t.pop_back();
            std::cout << "Tracer: ctor=" << Tracer::ctor
                      << ", copy_ctor=" << Tracer::copy_ctor
                      << ", move_ctor=" << Tracer::move_ctor
                      << ", dtor=" << Tracer::dtor << "\n";
        }

        std::cout << "\n== Исключения на граничных операциях ==\n";
        unrolled_list<int, 4> e;
        try {
            e.front();
        } catch (const std::out_of_range& ex) {
            std::cout << "Ожидаемое исключение front(): " << ex.what() << "\n";
        }
        try {
            e.pop_back();
        } catch (const std::out_of_range& ex) {
            std::cout << "Ожидаемое исключение pop_back(): " << ex.what() << "\n";
        }

        std::cout << "\nВсе проверки прошли.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Неожиданное исключение: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}