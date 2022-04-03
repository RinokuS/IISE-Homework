#include <iostream>
#include <vector>
#include "lfstack.h"

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6};
    TLockFreeStack<int> lf;

    lf.EnqueueAll(vec.begin(), vec.end());

    std::vector<int> vec2;

    lf.DequeueAll(&vec2);

    for (int i = 0; i < vec2.size(); ++i) {
        std::cout << vec2[i] << std::endl;
    }
    return 0;
}
