#include <stdlib.h>

/* header macros */
#define array__hdr(a) ((size_t*)(void*)(a) - 2)
#define array__cap(a) array__hdr(a)[0]
#define array__len(a) array__hdr(a)[1]

/* pushing into the array */
#define array__growth(a)     ((a) && (array__len(a) >= array__cap(a)) ? (2 * array__cap(a)) : 0)
#define array__make_space(a) (*((void**)&(a)) = array__resize((a), sizeof(*(a)), array__growth(a), 0))

#define array_push(a, v)     (array__make_space(a), (a)[array__len(a)++] = (v))

/* more functionality */
#define array_resize(a, n)   (*((void**)&(a)) = array__resize((a), sizeof(*(a)), (n), 1))
#define array_reserve(a, n)  (*((void**)&(a)) = array__resize((a), sizeof(*(a)), (n), 0))
#define array_free(a)        ((a) ? (free(array__hdr(a)), (a) = NULL) : 0);

/* utility macros */
#define array_len(a) ((a) ? array__len(a) : 0)
#define array_cap(a) ((a) ? array__cap(a) : 0)

#define array_pack(a)    ((a) ? (array_resize((a), array__len(a))) : 0)
#define array_clear(a)   ((a) ? array__len(a) = 0 : 0)
#define array_last(a)    ((a) + array_len(a))
#define array_sizeof(a)  (array_len(a) * sizeof(*(a)))

/*
 * resize function:
 * if arr == NULL and cap == 0 -> cap = 1 (first initialization)
 * if arr != NULL and cap < old_cap and not shrink -> no need to resize
 * if realloc failed return NULL
 */
void* array__resize(void* arr, size_t elem_size, size_t cap, int shrink)
{
    if (arr && cap < array__cap(arr) && !shrink) return arr;
    if (!arr && cap == 0) cap = 1;

    size_t size = (2 * sizeof(size_t)) + (cap * elem_size);
    size_t* hdr = realloc(arr ? array__hdr(arr) : NULL, size);

    if (!hdr) return NULL; /* out of memory */

    hdr[0] = cap;
    if (!arr) hdr[1] = 0;

    return hdr + 2;
}