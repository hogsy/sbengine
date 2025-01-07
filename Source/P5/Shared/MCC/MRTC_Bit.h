#ifndef MACRO_INC_MRTC_BIT_H
#define MACRO_INC_MRTC_BIT_H

#if defined(CPU_POWERPC) && !defined(PLATFORM_SPU)
extern const uint16 g_lBits16[16];
extern const uint32 g_lBits32[32];
extern const uint64 g_lBits64[64];
#else
const uint16 g_lBits16[16] = {
	uint16(1)<<0,  uint16(1)<<1,  uint16(1)<<2,  uint16(1)<<3,  uint16(1)<<4,  uint16(1)<<5,  uint16(1)<<6,  uint16(1)<<7,  uint16(1)<<8,  uint16(1)<<9, 
	uint16(1)<<10,uint16(1)<<11, uint16(1)<<12, uint16(1)<<13, uint16(1)<<14, uint16(1)<<15
};

const uint32 g_lBits32[32] = {
	uint32(1)<<0,  uint32(1)<<1,  uint32(1)<<2,  uint32(1)<<3,  uint32(1)<<4,  uint32(1)<<5,  uint32(1)<<6,  uint32(1)<<7,  uint32(1)<<8,  uint32(1)<<9, 
	uint32(1)<<10,uint32(1)<<11, uint32(1)<<12, uint32(1)<<13, uint32(1)<<14, uint32(1)<<15, uint32(1)<<16, uint32(1)<<17, uint32(1)<<18, uint32(1)<<19, 
	uint32(1)<<20,uint32(1)<<21, uint32(1)<<22, uint32(1)<<23, uint32(1)<<24, uint32(1)<<25, uint32(1)<<26, uint32(1)<<27, uint32(1)<<28, uint32(1)<<29,
	uint32(1)<<30,uint32(1)<<31
};

const uint64 g_lBits64[64] = {
	uint64(1)<<0, uint64(1)<<1,  uint64(1)<<2,  uint64(1)<<3,  uint64(1)<<4,  uint64(1)<<5,  uint64(1)<<6,  uint64(1)<<7,  uint64(1)<<8,  uint64(1)<<9, 
	uint64(1)<<10,uint64(1)<<11, uint64(1)<<12, uint64(1)<<13, uint64(1)<<14, uint64(1)<<15, uint64(1)<<16, uint64(1)<<17, uint64(1)<<18, uint64(1)<<19, 
	uint64(1)<<20,uint64(1)<<21, uint64(1)<<22, uint64(1)<<23, uint64(1)<<24, uint64(1)<<25, uint64(1)<<26, uint64(1)<<27, uint64(1)<<28, uint64(1)<<29,
	uint64(1)<<30,uint64(1)<<31, uint64(1)<<32, uint64(1)<<33, uint64(1)<<34, uint64(1)<<35, uint64(1)<<36, uint64(1)<<37, uint64(1)<<38, uint64(1)<<39,
	uint64(1)<<40,uint64(1)<<41, uint64(1)<<42, uint64(1)<<43, uint64(1)<<44, uint64(1)<<45, uint64(1)<<46, uint64(1)<<47, uint64(1)<<48, uint64(1)<<49,
	uint64(1)<<50,uint64(1)<<51, uint64(1)<<52, uint64(1)<<53, uint64(1)<<54, uint64(1)<<55, uint64(1)<<56, uint64(1)<<57, uint64(1)<<58, uint64(1)<<59,
	uint64(1)<<60,uint64(1)<<61, uint64(1)<<62, uint64(1)<<63
};

#endif


template<class T>
T M_BitTD(int index);

template<>
uint16 M_FORCEINLINE M_BitTD<uint16>(int index) { return g_lBits16[index]; }

template<>
uint32 M_FORCEINLINE M_BitTD<uint32>(int index) { return g_lBits32[index]; }

template<>
uint64 M_FORCEINLINE M_BitTD<uint64>(int index) { return g_lBits64[index]; }



#ifndef M_Bit
template <uint TBitNr> struct TBit { enum { BITVALUE = 1 << TBitNr }; };
#define M_Bit(_iBit) TBit<_iBit>::BITVALUE 
#endif

#ifndef M_BitD
//#define M_BitD(_iBit) getBitD(_iBit)
#ifdef CPU_POWERPC
#define M_BitD(_iBit) g_lBits32[_iBit]
#else
#define M_BitD(_iBit) (1<<(_iBit)) 
#endif
#endif

#endif //MACRO_INC_MRTC_Bit_h

