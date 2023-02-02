/*Written by Timothy B. Terriberry (tterribe@xiph.org) 1999-2009
  CC0 (Public domain) - see http://creativecommons.org/publicdomain/zero/1.0/
  for details.
  Based on the public domain ISAAC implementation by Robert J. Jenkins Jr.*/

/*
 * Changes for NetHack:
 *      include config.h;
 *      skip rest of file if USE_ISAAC64 isn't defined there;
 *      re-do 'inline' handling.
 */
#include "config.h"

#ifdef USE_ISAAC64
#include <string.h>
#include "isaac64.h"

#define ISAAC64_MASK ((uint64_t)0xFFFFFFFFFFFFFFFFULL)

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#if !defined(HAS_INLINE)
#define HAS_INLINE
#endif
#else
# if (defined(__GNUC__) && __GNUC__ >= 2 && !defined(inline))
# define inline __inline__
# endif
#endif
#if !defined(HAS_INLINE) && !defined(inline)
#define inline /*empty*/
#endif

/* Extract ISAAC64_SZ_LOG bits (starting at bit 3). */
static inline uint32_t lower_bits(uint64_t x)
{
    return (x & ((ISAAC64_SZ-1) << 3)) >>3;
}

/* Extract next ISAAC64_SZ_LOG bits (starting at bit ISAAC64_SZ_LOG+2). */
static inline uint32_t upper_bits(uint64_t y)
{
    return (y >> (ISAAC64_SZ_LOG+3)) & (ISAAC64_SZ-1);
}

static void isaac64_update(isaac64_ctx *_ctx){
  uint64_t *m;
  uint64_t *r;
  uint64_t  a;
  uint64_t  b;
  uint64_t  x;
  uint64_t  y;
  int       i;
  m=_ctx->m;
  r=_ctx->r;
  a=_ctx->a;
  b=_ctx->b+(++_ctx->c);
  for(i=0;i<ISAAC64_SZ/2;i++){
    x=m[i];
    a=~(a^a<<21)+m[i+ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a>>5)+m[i+ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a<<12)+m[i+ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a>>33)+m[i+ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
  }
  for(i=ISAAC64_SZ/2;i<ISAAC64_SZ;i++){
    x=m[i];
    a=~(a^a<<21)+m[i-ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a>>5)+m[i-ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a<<12)+m[i-ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
    x=m[++i];
    a=(a^a>>33)+m[i-ISAAC64_SZ/2];
    m[i]=y=m[lower_bits(x)]+a+b;
    r[i]=b=m[upper_bits(y)]+x;
  }
  _ctx->b=b;
  _ctx->a=a;
  _ctx->n=ISAAC64_SZ;
}

static void isaac64_mix(uint64_t _x[8]){
  static const unsigned char SHIFT[8]={9,9,23,15,14,20,17,14};
  int i;
  for(i=0;i<8;i++){
    _x[i]-=_x[(i+4)&7];
    _x[(i+5)&7]^=_x[(i+7)&7]>>SHIFT[i];
    _x[(i+7)&7]+=_x[i];
    i++;
    _x[i]-=_x[(i+4)&7];
    _x[(i+5)&7]^=_x[(i+7)&7]<<SHIFT[i];
    _x[(i+7)&7]+=_x[i];
  }
}


void isaac64_init(isaac64_ctx *_ctx,const unsigned char *_seed,int _nseed){
  _ctx->a=_ctx->b=_ctx->c=0;
  memset(_ctx->r,0,sizeof(_ctx->r));
  isaac64_reseed(_ctx,_seed,_nseed);
}

void isaac64_reseed(isaac64_ctx *_ctx,const unsigned char *_seed,int _nseed){
  uint64_t *m;
  uint64_t *r;
  uint64_t  x[8];
  int       i;
  int       j;
  m=_ctx->m;
  r=_ctx->r;
  if(_nseed>ISAAC64_SEED_SZ_MAX)_nseed=ISAAC64_SEED_SZ_MAX;
  for(i=0;i<_nseed>>3;i++){
    r[i]^=(uint64_t)_seed[i<<3|7]<<56|(uint64_t)_seed[i<<3|6]<<48|
     (uint64_t)_seed[i<<3|5]<<40|(uint64_t)_seed[i<<3|4]<<32|
     (uint64_t)_seed[i<<3|3]<<24|(uint64_t)_seed[i<<3|2]<<16|
     (uint64_t)_seed[i<<3|1]<<8|_seed[i<<3];
  }
  _nseed-=i<<3;
  if(_nseed>0){
    uint64_t ri;
    ri=_seed[i<<3];
    for(j=1;j<_nseed;j++)ri|=(uint64_t)_seed[i<<3|j]<<(j<<3);
    r[i++]^=ri;
  }
  x[0]=x[1]=x[2]=x[3]=x[4]=x[5]=x[6]=x[7]=(uint64_t)0x9E3779B97F4A7C13ULL;
  for(i=0;i<4;i++)isaac64_mix(x);
  for(i=0;i<ISAAC64_SZ;i+=8){
    for(j=0;j<8;j++)x[j]+=r[i+j];
    isaac64_mix(x);
    memcpy(m+i,x,sizeof(x));
  }
  for(i=0;i<ISAAC64_SZ;i+=8){
    for(j=0;j<8;j++)x[j]+=m[i+j];
    isaac64_mix(x);
    memcpy(m+i,x,sizeof(x));
  }
  isaac64_update(_ctx);
}

uint64_t isaac64_next_uint64(isaac64_ctx *_ctx){
  if(!_ctx->n)isaac64_update(_ctx);
  return _ctx->r[--_ctx->n];
}

uint64_t isaac64_next_uint(isaac64_ctx *_ctx,uint64_t _n){
  uint64_t r;
  uint64_t v;
  uint64_t d;
  do{
    r=isaac64_next_uint64(_ctx);
    v=r%_n;
    d=r-v;
  }
  while(((d+_n-1)&ISAAC64_MASK)<d);
  return v;
}
#endif /* USE_ISAAC64 */

/*isaac64.c*/
