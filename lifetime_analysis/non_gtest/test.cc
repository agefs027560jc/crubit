#include <cstdio>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "lifetime_analysis/non_gtest/lifetime_demo.h"

using namespace clang;
using namespace tidy;
using namespace lifetimes;

int main(int argc, char **argv) {
    LifetimeDemo demo;

    //std::string outbuf;
    std::ifstream ins(argv[1]);
    std::string outbuf( (std::istreambuf_iterator<char>(ins) ),
                      (std::istreambuf_iterator<char>()    ) );
    //std::copy_if(std::istreambuf_iterator<char>(ins),
    //             std::istreambuf_iterator<char>(),
    //             std::back_insert_iterator<std::string>(outbuf),
    //             [](char c) { return !std::isspace(c); });

    std::cout << demo.GetLifetimes(outbuf) << std::endl;
    //std::cout << outbuf << std::endl;
    return 0;
}
