#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>

#ifdef WORDS_BIGENDIAN
#ifndef MAKEFOURCC
#define MAKEFOURCC( a, b, c, d) \
    (((uint32_t)d) | (((uint32_t)c) << 8) | (((uint32_t)b) << 16) | (((uint32_t)a) << 24))
#endif
#else//WORD_BIGENDIAN
#ifndef MAKEFOURCC
#define MAKEFOURCC( a, b, c, d) \
    (((uint32_t)a) | (((uint32_t)b) << 8) | (((uint32_t)c) << 16) | (((uint32_t)d) << 24))
#endif
#endif

static inline unsigned clz (unsigned x)
{
//#if VLC_GCC_VERSION(3,4)
    return __builtin_clz (x);
//#else
//    unsigned i = sizeof (x) * 8;

//    while (x)
//    {
//        x >>= 1;
//        i--;
//    }
//    return i;
//#endif
}

static inline int GetAlignedSize(unsigned size)
{
    /* Return the smallest larger or equal power of 2 */
    unsigned align = 1 << (8 * sizeof (unsigned) - clz(size));
    return ((align >> 1) == size) ? size : align;
}

static inline bool HasExtension(const char *apis, const char *api)
{
    size_t apilen = strlen(api);
    while (apis) {
        while (*apis == ' ')
            apis++;
        if (!strncmp(apis, api, apilen) && memchr(" ", apis[apilen], 2))
            return true;
        apis = strchr(apis, ' ');
    }
    return false;
}



#endif // DEFINES_H
