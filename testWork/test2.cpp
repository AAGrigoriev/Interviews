/* Строковый велосипед */
#include <bits/stdc++.h>

using namespace std;

/* Можно было рекурсивно сдвигать строку с наихудшей сложностью N^2*/
void removeDuplicate(char* str, int n)
{
    if(n < 2) return;

    int index = 0;
    int start = 0;

    for (int i = 0; i < n; i++)
    {
        int j;

        for (j = start; j < i; j++)

            if (str[i] == str[j])

                break;

        if (j == i)
        {
            std::cout << i << std::endl;
            std::cout << index << std::endl;
            
            str[index++] = str[i];
            start = i;
        }
    }
}

int main()
{
    char str[] = "AAA BBB AAA AAA BBB123";

    int n = sizeof(str) / sizeof(str[0]);

    removeDuplicate(str,n);

    cout << str << std::endl;

    return 0;
}