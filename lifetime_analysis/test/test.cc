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
    int* target(int* a, int* b, unsigned x) {
      int* v[2] = {a, b};
      return v[x & 1];
    }

    void nontarget(int** array, int* p, int* q) {
      array[0] = p;
      array[1] = q;
    }
    )";
    std::cout << x.GetLifetimes(test) << "\n";
    return 0;
}
