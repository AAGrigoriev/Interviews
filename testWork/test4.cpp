#include <iostream>
#include <list>

template <class iter>
std::size_t sum(iter beg, iter end)
{
    if (beg != end)
        return *beg + sum(++beg, end);
    else
        return 0;
}

template <class iter>
std::size_t count(iter beg, iter end)
{
    if (beg != end)
        return 1 + count(++beg, end);
    else
        return 0;
}

template <class iter>
iter max_list_impl2(iter beg, iter end)
{
    if (beg != std::prev(end, 1))
    {
        iter max_next = max_list_impl2(++beg, end);
        return *beg > *max_next ? beg : max_next;
    }
    else
    {
        return beg;
    }
}

std::list<int>::iterator max_list2(std::list<int> &list)
{
    if (list.size() < 1)
        return list.end();
    return max_list_impl2(list.begin(), list.end());
}

template <class iter>
iter max_list_impl1(iter beg, iter end)
{
    if (std::next(beg, 2) == end)
    {
        auto next_beg = std::next(beg, 1);
        return *beg > *next_beg ? beg : next_beg;
    }

    auto max_iter_next = max_list_impl2(std::next(beg, 1), end);

    return *beg > *max_iter_next ? beg : max_iter_next;
}

std::list<int>::iterator max_list1(std::list<int> &list)
{
    if (list.size() < 2)
        return list.begin();
    return max_list_impl1(list.begin(), list.end());
}

auto foo(int a, float b) -> decltype(a > b ? a : b)
{
    if (a > b)
        return a;
    else
        return b;
}

int main()
{

    int a;
    float b;
    std::cin >> a >> b;


    auto temp = foo(a,b);
    

    bool value = std::is_same<decltype(temp),float>::value;

    std::cout << value << " " << std::endl;

    std::list<int> list1;
    list1.push_back(1);
    list1.push_back(2);
    list1.push_back(3);

    std::list<int> list2;

    std::cout << sum(list1.begin(), list1.end()) << std::endl;

    std::cout << sum(list2.begin(), list2.end()) << std::endl;

    std::cout << count(list1.begin(), list1.end()) << std::endl;

    std::cout << count(list2.begin(), list2.end()) << std::endl;

    auto iter1 = max_list1(list1);

    if (iter1 != list1.end())
    {
        std::cout << *iter1 << std::endl;
    }

    auto iter2 = max_list1(list2);

    if (iter2 != list2.end())
    {
        std::cout << *iter2 << std::endl;
    }

    auto iter21 = max_list2(list1);

    if (iter21 != list1.end())
    {
        std::cout << *iter21 << std::endl;
    }

    auto iter22 = max_list2(list2);

    if (iter22 != list2.end())
    {
        std::cout << *iter22 << std::endl;
    }

    return 0;
}