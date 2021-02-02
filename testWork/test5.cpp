#include <iostream>
#include <algorithm>
#include <vector>

int main ()
{

    std::vector<int> data {1,2,3,4,5,6};


    auto lower = std::lower_bound(data.begin(), data.end(), 4);

    auto upper = std::upper_bound(data.begin(), data.end(), 6);

    std::cout << (*upper) << std::endl;
    std::cout << (*lower) << std::endl;

    return 0;
}