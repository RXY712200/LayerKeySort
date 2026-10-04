#include "layerkeysort.h"

extern "C" int compare_int_cpp(const void *left, const void *right, void *context)
{
    const int a = *static_cast<const int *>(left);
    const int b = *static_cast<const int *>(right);
    (void)context;
    return (a > b) - (a < b);
}

int main()
{
    int values[] = {2, 1};
    void *items[] = {&values[0], &values[1]};
    if (lks_sort(items, 2, compare_int_cpp, nullptr) != LKS_STATUS_OK ||
        items[0] != &values[1]) return 1;
    return 0;
}
