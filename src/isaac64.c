/*Written by Timothy B. Terriberry (tterribe@xiph.org) 1999-2009
  CC0 (Public domain) - see LICENSE file for details
  Based on the public domain ISAAC implementation by Robert J. Jenkins Jr.*/
#include <float.h>
#include <math.h>
#include <string.h>
#include <ccan/ilog/ilog.h>
#include "isaac64.h"


#define ISAAC64_MASK ((uint64_t)0xFFFFFFFFFFFFFFFFULL)

/* Extract ISAAC64_SZ_LOG bits (starting at bit 3). */
static inline uint32_t lower_bits(uint64_t x)
{
	return (x & ((ISAAC64_SZ-1) << 3)) >>3;
}

/* Extract next ISAAC64_SZ_LOG bits (starting at bit ISAAC64_SZ_LOG+2). */
static inline uint32_t upper_bits(uint32_t y)
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

/*Returns a uniform random float.
  The expected value is within FLT_MIN (e.g., 1E-37) of 0.5.
  _bits: An initial set of random bits.
  _base: This should be -(the number of bits in _bits), up to -64.
  Return: A float uniformly distributed between 0 (inclusive) and 1
           (exclusive).
          The average value was measured over 2**32 samples to be
           0.499991407275206357.*/
static float isaac64_float_bits(isaac64_ctx *_ctx,uint64_t _bits,int _base){
  float ret;
  int   nbits_needed;
  while(!_bits){
    if(_base+FLT_MANT_DIG<FLT_MIN_EXP)return 0;
    _base-=64;
    _bits=isaac64_next_uint64(_ctx);
  }
  nbits_needed=FLT_MANT_DIG-ilog64_nz(_bits);
#if FLT_MANT_DIG>64
  ret=ldexpf((float)_bits,_base);
# if FLT_MANT_DIG>129
  while(64-nbits_needed<0){
# else
  if(64-nbits_needed<0){
# endif
    _base-=64;
    nbits_needed-=64;
    ret+=ldexpf((float)isaac64_next_uint64(_ctx),_base);
  }
  _bits=isaac64_next_uint64(_ctx)>>(64-nbits_needed);
  ret+=ldexpf((float)_bits,_base-nbits_needed);
#else
  if(nbits_needed>0){
    _bits=_bits<<nbits_needed|isaac64_next_uint64(_ctx)>>(64-nbits_needed);
  }
# if FLT_MANT_DIG<64
  else _bits>>=-nbits_needed;
# endif
  ret=ldexpf((float)_bits,_base-nbits_needed);
#endif
  return ret;
}

float isaac64_next_float(isaac64_ctx *_ctx){
  return isaac64_float_bits(_ctx,0,0);
}

float isaac64_next_signed_float(isaac64_ctx *_ctx){
  uint64_t bits;
  bits=isaac64_next_uint64(_ctx);
  return (1|-((int)bits&1))*isaac64_float_bits(_ctx,bits>>1,-63);
}

/*Returns a uniform random double.
  _bits: An initial set of random bits.
  _base: This should be -(the number of bits in _bits), up to -64.
  Return: A double uniformly distributed between 0 (inclusive) and 1
           (exclusive).
          The average value was measured over 2**32 samples to be
           0.499990992392019273.*/
static double isaac64_double_bits(isaac64_ctx *_ctx,uint64_t _bits,int _base){
  double ret;
  int    nbits_needed;
  while(!_bits){
    if(_base+DBL_MANT_DIG<DBL_MIN_EXP)return 0;
    _base-=64;
    _bits=isaac64_next_uint64(_ctx);
  }
  nbits_needed=DBL_MANT_DIG-ilog64_nz(_bits);
#if DBL_MANT_DIG>64
  ret=ldexp((double)_bits,_base);
# if DBL_MANT_DIG>129
  while(64-nbits_needed<0){
# else
  if(64-nbits_needed<0){
# endif
    _base-=64;
    nbits_needed-=64;
    ret+=ldexp((double)isaac64_next_uint64(_ctx),_base);
  }
  _bits=isaac64_next_uint64(_ctx)>>(64-nbits_needed);
  ret+=ldexp((double)_bits,_base-nbits_needed);
#else
  if(nbits_needed>0){
    _bits=_bits<<nbits_needed|isaac64_next_uint64(_ctx)>>(64-nbits_needed);
  }
# if DBL_MANT_DIG<64
  else _bits>>=-nbits_needed;
# endif
  ret=ldexp((double)_bits,_base-nbits_needed);
#endif
  return ret;
}

double isaac64_next_double(isaac64_ctx *_ctx){
  return isaac64_double_bits(_ctx,0,0);
}

double isaac64_next_signed_double(isaac64_ctx *_ctx){
  uint64_t bits;
  bits=isaac64_next_uint64(_ctx);
  return (1|-((int)bits&1))*isaac64_double_bits(_ctx,bits>>1,-63);
}
