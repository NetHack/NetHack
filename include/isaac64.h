/* CC0 (Public domain) - see http://creativecommons.org/publicdomain/zero/1.0/ for details */
#if !defined(_isaac64_H)
# define _isaac64_H (1)



typedef struct isaac64_ctx isaac64_ctx;



#define ISAAC64_SZ_LOG      (8)
#define ISAAC64_SZ          (1<<ISAAC64_SZ_LOG)
#define ISAAC64_SEED_SZ_MAX (ISAAC64_SZ<<3)



/*ISAAC is the most advanced of a series of pseudo-random number generators
   designed by Robert J. Jenkins Jr. in 1996.
  http://www.burtleburtle.net/bob/rand/isaac.html
  This is the 64-bit version.
  To quote:
    ISAAC-64 generates a different sequence than ISAAC, but it uses the same
     principles.
    It uses 64-bit arithmetic.
    It generates a 64-bit result every 19 instructions.
    All cycles are at least 2**72 values, and the average cycle length is
     2**16583.*/
struct isaac64_ctx{
  unsigned n;
  uint64_t r[ISAAC64_SZ];
  uint64_t m[ISAAC64_SZ];
  uint64_t a;
  uint64_t b;
  uint64_t c;
};


/**
 * isaac64_init - Initialize an instance of the ISAAC64 random number generator.
 * @_ctx:   The ISAAC64 instance to initialize.
 * @_seed:  The specified seed bytes.
 *          This may be NULL if _nseed is less than or equal to zero.
 * @_nseed: The number of bytes to use for the seed.
 *          If this is greater than ISAAC64_SEED_SZ_MAX, the extra bytes are
 *           ignored.
 */
void isaac64_init(isaac64_ctx *_ctx,const unsigned char *_seed,int _nseed);

/**
 * isaac64_reseed - Mix a new batch of entropy into the current state.
 * To reset ISAAC64 to a known state, call isaac64_init() again instead.
 * @_ctx:   The instance to reseed.
 * @_seed:  The specified seed bytes.
 *          This may be NULL if _nseed is zero.
 * @_nseed: The number of bytes to use for the seed.
 *          If this is greater than ISAAC64_SEED_SZ_MAX, the extra bytes are
 *           ignored.
 */
void isaac64_reseed(isaac64_ctx *_ctx,const unsigned char *_seed,int _nseed);
/**
 * isaac64_next_uint64 - Return the next random 64-bit value.
 * @_ctx: The ISAAC64 instance to generate the value with.
 */
uint64_t isaac64_next_uint64(isaac64_ctx *_ctx);
/**
 * isaac64_next_uint - Uniform random integer less than the given value.
 * @_ctx: The ISAAC64 instance to generate the value with.
 * @_n:   The upper bound on the range of numbers returned (not inclusive).
 *        This must be greater than zero and less than 2**64.
 *        To return integers in the full range 0...2**64-1, use
 *         isaac64_next_uint64() instead.
 * Return: An integer uniformly distributed between 0 and _n-1 (inclusive).
 */
uint64_t isaac64_next_uint(isaac64_ctx *_ctx,uint64_t _n);
/**
 * isaac64_next_float - Uniform random float in the range [0,1).
 * @_ctx: The ISAAC64 instance to generate the value with.
 * Returns a high-quality float uniformly distributed between 0 (inclusive)
 *  and 1 (exclusive).
 * All of the float's mantissa bits are random, e.g., the least significant bit
 *  may still be non-zero even if the value is less than 0.5, and any
 *  representable float in the range [0,1) has a chance to be returned, though
 *  values very close to zero become increasingly unlikely.
 * To generate cheaper float values that do not have these properties, use
 *   ldexpf((float)isaac64_next_uint64(_ctx),-64);
 */

#endif
