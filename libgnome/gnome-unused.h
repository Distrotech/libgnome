#ifndef __unused__
#ifdef __GNUC__
#define __unused__	__attribute__ ((unused))
#else
#define __unused__
#endif
#endif
