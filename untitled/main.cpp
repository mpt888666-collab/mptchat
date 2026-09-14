#include <iostream>
#include <vector>



int main() {
    int x = 10;
    auto f = [b = std::move(x)]() mutable {
        b = 100;
    };
    f();
    std::cout << x << std::endl;
    std::cout<<typeid(f).name();
    return 0;
}