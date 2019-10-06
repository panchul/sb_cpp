// $ g++ -std=c++1z pair_sort.cpp
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
int main() {
    std::vector<std::pair<int,std::string> > v;
    v.push_back(std::make_pair(1,"aaa"));
    v.push_back(std::make_pair(9,"dda"));
    v.push_back(std::make_pair(3,"b aa"));
    v.push_back(std::make_pair(7,"ccc a"));

    std::for_each(v.begin(), v.end(),
     [](const auto& p){std::cout << p.first << " " << p.second << "\n";});

    std::sort(v.begin(), v.end(),
     [](const auto& l,const auto& r){ return l.first < r.first;});

    std::for_each(v.begin(), v.end(),
     [](const auto& p){std::cout << p.first << " " << p.second << "\n";});
}