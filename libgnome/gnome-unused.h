#ifndef __g_unused__
#ifdef __GNUC__
#define __g_unused__	__attribute__ ((unused))
#else
#define __g_unused__
#endif
#endif
