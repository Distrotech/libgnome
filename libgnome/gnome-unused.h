#ifndef G_UNUSED
#ifdef __GNUC__
#define G_UNUSED	__attribute__ ((unused))
#else
#define G_UNUSED
#endif
#endif
