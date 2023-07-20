#include <cstdio>
#include <iostream>

#include "lifetime_analysis/test/lifetime_demo.h"

using namespace clang;
using namespace tidy;
using namespace lifetimes;

int main()
{
    LifetimeDemo x;

    std::string test = R"(
    #include <cstdio>
    #include <iostream>
    int* target(int* a, int* b, unsigned x) {
      int* v[2] = {a, b};
      std::cout << "test";
      return v[x & 1];
    }
    )";

    std::string test1 = R"(
    void target(int** array, int* p, int* q) {
      array[0] = p;
      array[1] = q;
    }
    )";

    std::string test2 = R"(
    void DemoServiceImpl::hello(const std::string& hi, std::string* reply, rrr::DeferredReply* defer) {
        *reply += std::string("Re: ") + hi;
        defer->reply();
    }

    void DemoServiceImpl::sum(const rrr::i32& a, const rrr::i32& b, const rrr::i32& c, rrr::i32* result, rrr::DeferredReply* defer) {
        *result = a + b + c;
        defer->reply();
    }
    )";

    std::string test3 = R"(
      int* f(int* a) {
        return a;
      }

      int* target(int* a) {
        return f(a);
      }
    )";

    std::string test4 = R"(
      int* target(int* a) {
        return a;
      }

      int* f(int* x) {
        int y = 2;
        return &y;
      }
    )";

    std::cout << x.GetLifetimes(test) << "\t#0\n";
    std::cout << x.GetLifetimes(test1) << "\t#1\n";
    std::cout << x.GetLifetimes(test2) << "\t#2\n";
    std::cout << x.GetLifetimes(test3) << "\t#3\n";
    std::cout << x.GetLifetimes(test4) << "\t#4\n";
    return 0;
}
