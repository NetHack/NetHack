/***
*assert.h - define the assert macro
*
****/

#undef assert

#ifdef NDEBUG

#define assert(exp) ((void) 0)

#else

#define assert(exp) \
    (void)((exp) || (panic("%s at %s line %ld", #exp, __FILE__, __LINE__), 1))

#endif /* NDEBUG */
