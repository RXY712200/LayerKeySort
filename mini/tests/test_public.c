#include "layerkeysort_mini.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); } } while (0)
#define OK(expr) CHECK((expr) == LKS_MINI_OK)
#define BAD(expr) CHECK((expr) == LKS_MINI_INVALID_ARGUMENT)
#define WRONG(expr) CHECK((expr) == LKS_MINI_WRONG_ORDER)

static void sequence(LksMiniOrder *order, LksMiniHandle **expected, size_t count)
{
    LksMiniHandle *cursor = NULL, *next = NULL;
    size_t size = 99;
    OK(lks_mini_size(order, &size)); CHECK(size == count);
    OK(lks_mini_first(order, &cursor));
    for (size_t i = 0; i < count; ++i) {
        CHECK(cursor == expected[i]);
        OK(lks_mini_next(order, cursor, &next)); cursor = next;
    }
    CHECK(cursor == NULL);
    OK(lks_mini_last(order, &cursor));
    for (size_t i = count; i > 0; --i) {
        CHECK(cursor == expected[i - 1]);
        OK(lks_mini_prev(order, cursor, &next)); cursor = next;
    }
    CHECK(cursor == NULL);
}

static void operations(void)
{
    LksMiniOrder *order = NULL;
    LksMiniHandle *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL, *f = NULL;
    int payload = 123;
    void *item = NULL;
    OK(lks_mini_create(&order));
    sequence(order, NULL, 0);
    lks_mini_destroy(NULL);
    OK(lks_mini_insert_back(order, &payload, &b));
    sequence(order, (LksMiniHandle *[]){b}, 1);
    OK(lks_mini_insert_front(order, &payload, &a)); CHECK(a != b);
    sequence(order, (LksMiniHandle *[]){a,b}, 2);
    OK(lks_mini_insert_after(order, b, NULL, &d));
    sequence(order, (LksMiniHandle *[]){a,b,d}, 3);
    OK(lks_mini_insert_before(order, d, &payload, &c));
    sequence(order, (LksMiniHandle *[]){a,b,c,d}, 4);
    OK(lks_mini_insert_before(order, a, NULL, &e));
    sequence(order, (LksMiniHandle *[]){e,a,b,c,d}, 5);
    OK(lks_mini_insert_after(order, a, NULL, &f));
    sequence(order, (LksMiniHandle *[]){e,a,f,b,c,d}, 6);
    OK(lks_mini_item(order, d, &item)); CHECK(item == NULL);
    OK(lks_mini_item(order, b, &item)); CHECK(item == &payload);
    OK(lks_mini_remove(order, e));
    sequence(order, (LksMiniHandle *[]){a,f,b,c,d}, 5);
    OK(lks_mini_remove(order, f));
    sequence(order, (LksMiniHandle *[]){a,b,c,d}, 4);
    OK(lks_mini_remove(order, d));
    sequence(order, (LksMiniHandle *[]){a,b,c}, 3);
    OK(lks_mini_remove(order, a));
    sequence(order, (LksMiniHandle *[]){b,c}, 2);
    OK(lks_mini_item(order, b, &item)); CHECK(item == &payload);
    OK(lks_mini_item(order, c, &item)); CHECK(item == &payload);
    OK(lks_mini_remove(order, c));
    sequence(order, (LksMiniHandle *[]){b}, 1);
    OK(lks_mini_remove(order, b)); sequence(order, NULL, 0);
    OK(lks_mini_insert_front(order, &payload, &a));
    sequence(order, (LksMiniHandle *[]){a}, 1);
    lks_mini_destroy(order); CHECK(payload == 123);
    puts("public operations: PASS");
}

static void errors(void)
{
    LksMiniOrder *order = NULL, *other = NULL;
    LksMiniHandle *h = NULL, *foreign = NULL, *out = NULL;
    void *item = NULL;
    int value = 7;
    size_t size = 7;
    OK(lks_mini_create(&order)); OK(lks_mini_create(&other));
    OK(lks_mini_insert_back(order, &value, &h));
    OK(lks_mini_insert_back(other, NULL, &foreign));
    BAD(lks_mini_create(NULL));
    BAD(lks_mini_size(NULL, &size)); CHECK(size == 0);
    BAD(lks_mini_size(order, NULL));
    out = h; BAD(lks_mini_first(NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_first(order, NULL));
    out = h; BAD(lks_mini_last(NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_last(order, NULL));
    out = h; BAD(lks_mini_next(NULL, h, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_next(order, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_next(order, h, NULL));
    out = h; WRONG(lks_mini_next(order, foreign, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_prev(NULL, h, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_prev(order, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_prev(order, h, NULL));
    out = h; WRONG(lks_mini_prev(order, foreign, &out)); CHECK(out == NULL);
    item = &value; BAD(lks_mini_item(NULL, h, &item)); CHECK(item == NULL);
    item = &value; BAD(lks_mini_item(order, NULL, &item)); CHECK(item == NULL);
    BAD(lks_mini_item(order, h, NULL));
    item = &value; WRONG(lks_mini_item(order, foreign, &item)); CHECK(item == NULL);
    out = h; BAD(lks_mini_insert_front(NULL, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_insert_front(order, NULL, NULL));
    out = h; BAD(lks_mini_insert_back(NULL, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_insert_back(order, NULL, NULL));
    out = h; BAD(lks_mini_insert_before(NULL, h, NULL, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_insert_before(order, NULL, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_insert_before(order, h, NULL, NULL));
    out = h; WRONG(lks_mini_insert_before(order, foreign, NULL, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_insert_after(NULL, h, NULL, &out)); CHECK(out == NULL);
    out = h; BAD(lks_mini_insert_after(order, NULL, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_insert_after(order, h, NULL, NULL));
    out = h; WRONG(lks_mini_insert_after(order, foreign, NULL, &out)); CHECK(out == NULL);
    BAD(lks_mini_remove(NULL, h)); BAD(lks_mini_remove(order, NULL));
    WRONG(lks_mini_remove(order, foreign));
    sequence(order, (LksMiniHandle *[]){h}, 1);
    sequence(other, (LksMiniHandle *[]){foreign}, 1);
    lks_mini_destroy(order); lks_mini_destroy(other);
    puts("public errors and defaults: PASS");
}

int main(void)
{
    operations(); errors(); return 0;
}
