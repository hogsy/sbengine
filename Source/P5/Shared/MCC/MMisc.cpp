
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			-
					
	Author:			Erik Olofsson
					
	Copyright:		Starbreeze Studios, 2003
					
	Contents:		-
					
	Comments:		-
					
	History:		
		030505:		Added Comments
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "../Platform/Platform.h"
#include "MFloat.h"
#include "MMisc.h"
#include "MFile.h"


#ifdef TARGET_DREAMCAST_SHINOBI

	#include <shinobi.h>
	#include <sg_tmr.h>

#elif defined TARGET_WIN32_X86
	// These are needed for timeGetTime() only.
	#include <windows.h>
	#include <mmsystem.h>
#endif

// -------------------------------------------------------------------
//  CTypeDescriptor
// -------------------------------------------------------------------
#if 0
const fp4 CTypeDesc_fp4::ms_cMin(_FP4_MIN);
const fp4 CTypeDesc_fp4::ms_cMax(_FP4_MAX);
const fp4 CTypeDesc_fp4::ms_cEpsilon(_FP4_EPSILON);

const fp8 CTypeDesc_fp8::ms_cMin(_FP8_MIN);
const fp8 CTypeDesc_fp8::ms_cMax(_FP8_MAX);
const fp8 CTypeDesc_fp8::ms_cEpsilon(_FP8_EPSILON);

const int CTypeDesc_int8::ms_cMin = -0x80;
const int CTypeDesc_int8::ms_cMax = 0x7f;
const int CTypeDesc_int8::ms_cEpsilon = 1;

const int CTypeDesc_int16::ms_cMin = -(int)0x8000;
const int CTypeDesc_int16::ms_cMax = 0x7fff;
const int CTypeDesc_int16::ms_cEpsilon = 1;

const int CTypeDesc_int32::ms_cMin = -(int)0x80000000;
const int CTypeDesc_int32::ms_cMax = 0x7fffffff;
const int CTypeDesc_int32::ms_cEpsilon = 1;

const unsigned int CTypeDesc_uint8::ms_cMin = 0;
const unsigned int CTypeDesc_uint8::ms_cMax = 0xff;
const unsigned int CTypeDesc_uint8::ms_cEpsilon = 1;

const unsigned int CTypeDesc_uint16::ms_cMin = 0;
const unsigned int CTypeDesc_uint16::ms_cMax = 0xffff;
const unsigned int CTypeDesc_uint16::ms_cEpsilon = 1;

const unsigned int CTypeDesc_uint32::ms_cMin = 0;
const unsigned int CTypeDesc_uint32::ms_cMax = 0xffffffff;
const unsigned int CTypeDesc_uint32::ms_cEpsilon = 1;

#ifdef CPU_INT32
	const int CTypeDesc_int::ms_cMin = -(int)0x80000000;	// Tiny :(
	const int CTypeDesc_int::ms_cMax = 0x7fffffff;
#else
	#ifdef CPU_INT64
		const int CTypeDesc_int::ms_cMin = -0x8000000000000000;	// Fet!!!
		const int CTypeDesc_int::ms_cMax = 0x7fffffffffffffff;
	#else
		#ifdef CPU_INT128
			const int CTypeDesc_int::ms_cMin = -0x80000000000000000000000000000000;	// As-fet!!!!!
			const int CTypeDesc_int::ms_cMax = 0x7fffffffffffffffffffffffffffffff;
		#else
			"256-bit????"
		#endif
	#endif
#endif
const int CTypeDesc_int::ms_cEpsilon = 1;

#endif


/*
// -------------------------------------------------------------------
//  CMathAccel
// -------------------------------------------------------------------
CMathAccel g_MathAccel;

CMathAccel* GetMathAccel()
{
	return &g_MathAccel;
}

CMathAccel::CMathAccel()
{
	InitInverseTable_fp4();
	InitInverseTable_Fixed16();
	InitSqrtTable();
}

void CMathAccel::InitInverseTable_Fixed16()
{
	if (m_pIntInverseTab == NULL)
	{
		m_pIntInverseTab = DNew(int) int[CONST_INVERSEMAX+0x80];
		if (m_pIntInverseTab == NULL) Error_static("GetInverse_Fixed16", "Out of memory.");
		m_pIntInverseTab[0] = 0x3fffffff;
		for (int i = 1; i < CONST_INVERSEMAX; i++)
		{
			m_pIntInverseTab[i] = 65536/i;
		}
	};
};

void CMathAccel::InitInverseTable_fp4()
{
	if (m_pFp4InverseTab == NULL)
	{
		m_pFp4InverseTab = DNew(fp4) fp4[CONST_INVERSEMAX];
		if (m_pFp4InverseTab == NULL) Error_static("GetInverse_fp4", "Out of memory.");
		m_pFp4InverseTab[0] = _FP4_MAX/2;
		for (int i = 1; i < CONST_INVERSEMAX; i++)
			m_pFp4InverseTab[i] = 1.0f/i;
	};
};

void CMathAccel::InitSqrtTable()
{
	unsigned short i;
	float f;
	unsigned int *fi = (unsigned int *)&f;
				// to access the bits of a float in
				// C quickly we must misuse pointers

	for(i=0; i<= 0x7f; i++) {
		*fi = 0;

		//
		// Build a float with the bit pattern i as mantissa
		// and an exponent of 0, stored as 127
  		//

		*fi = (i << 16) | (127 << 23);
		f = M_Sqrt(f);

		//
		// Take the square root then strip the first 7 bits of
		// the mantissa into the table
		//

		m_sqrttab[i] = (*fi & 0x7fffff) >> 16;

		//
		// Repeat the process, this time with an exponent of
		// 1, stored as 128
		//

		*fi = 0;
		*fi = (i << 16) | (128 << 23);
		f = M_Sqrt(f);
		m_sqrttab[i+0x80] = (*fi & 0x7fffff) >> 16;
	}
}

#if !defined(PLATFORM_DOLPHIN) && !defined(PLATFORM_PS3)
float CMathAccel::fsqrt(fp4 n) const
{
	unsigned int *num = (unsigned int *)&n;
				// to access the bits of a float in C
				// we must misuse pointers
							
	short e;		// the exponent
	if (n == 0) return (0);	// check for square root of 0
	e = (*num >> 23) - 127;	// get the exponent - on a SPARC the
				// exponent is stored with 127 added
	*num &= 0x7fffff;	// leave only the mantissa
	if (e & 0x01) *num |= 0x800000;
				// the exponent is odd so we have to
				// look it up in the second half of
				// the lookup table, so we set the
				// high bit
	e >>= 1;	// divide the exponent by two
				// note that in C the shift
				// operators are sign preserving
				// for signed operands
	// Do the table lookup, based on the quaternary mantissa,
	// then reconstruct the result back into a float
	//
	*num = ((m_sqrttab[*num >> 16]) << 16) | ((e + 127) << 23);
	return(n);
}
#endif

//
//	FastDivInt,		int32 / int32
//
//	16 c. Inc. loop.
//	39 c. Inc loop, Kåd med vanlig division.
//
int CMathAccel::FastDivInt(int taljare, int namnare)
{
	if ((namnare < CONST_INVERSEMAX) && (namnare >= 0))
	{
		if (taljare > 32767)
			return ((int64)m_pIntInverseTab[(namnare)] * (taljare)) >> 16;
		else
			return (m_pIntInverseTab[(namnare)] * (taljare)) >> 16;
	}
	else
		return (taljare)/(namnare);
}

//-------------------------------------------------------------------
//FastMulDiv64,	(int32 * int32) / int32
//
//	26 c. Inc. loop. 119 om det inte går att optimera.
//	106 c. Inc loop, Kåd med vanlig 64-bit division.
//
int CMathAccel::FastMulDiv64(int factor1, int factor2, int namnare)
{
	if ((namnare < CONST_INVERSEMAX) && (namnare >= 0))
	{
		if ((factor1) > (factor2))
			return (((int64)(factor1) * m_pIntInverseTab[(namnare)]) >> 16) * (factor2);
		else
			return (((int64)(factor2) * m_pIntInverseTab[(namnare)]) >> 16) * (factor1);
	}
	else
		return ((int64)factor1 * factor2)/(namnare);
}

//-------------------------------------------------------------------
//FastDivFp4Int,  fp4 / int32
//
//	13 c. Inc. loop. 30 om ingen opt.
//	30 c. Inc loop, Kåd med vanlig division.

int CMathAccel::FastDivFp4Int(fp4 taljare, int namnare)
{
	if ((namnare < CONST_INVERSEMAX) && (namnare >= 0))
	{
		return TruncToInt(m_pFp4InverseTab[(namnare)] * (taljare));
	}
	else
		return TruncToInt((taljare)/(namnare));
}
*/

// -------------------------------------------------------------------
void MemSetW(void* _pDest, int _Value, int _Count)
{
#ifdef CPU_INTELP5
	__asm {
		mov ecx,[_Count]
		or ecx,ecx
		jz MSW_NoSet
		js MSW_NoSet
		mov edi,[_pDest]
		mov eax,[_Value]
		mov edx,eax
		shl eax,16
		mov ax,dx

		test edi, 2
		jz MSW_NoWordAlign

			// Align to dword if word-only aligned.
			mov [edi],ax
			dec ecx
			jz MSW_NoSet
			add edi,2
		
	MSW_NoWordAlign:
		mov edx,ecx
		shr edx,1
		jz MSW_NoSetD
		push edx

	MSW_LoopD:
			mov [edi],eax
			add edi,4
			dec edx
		jnz MSW_LoopD

		pop edx
	MSW_NoSetD:
		shl edx,1
		sub ecx,edx
		jz MSW_NoSet
		rep stosw

	MSW_NoSet:
	};
#else
	for (int i = 0; i < _Count; ((int16*) _pDest)[i++] = _Value) {};
#endif
};

void MemSetD(void* _pDest, int _Value, int _Count)
{
#ifdef CPU_INTELP5
	__asm {
		mov ecx,[_Count]
		or ecx,ecx
		jz MSD_NoSet
		js MSD_NoSet
		mov edi,[_pDest]
		mov eax,[_Value]

		mov edx,ecx
		shr edx,1
		jz MSD_NoSetQ
		push edx

	MSD_LoopQ:
			mov [edi],eax
			mov [edi+4],eax
			add edi,8
			dec edx
		jnz MSD_LoopQ

		pop edx
	MSD_NoSetQ:
		shl edx,1
		sub ecx,edx
		jz MSD_NoSet
			mov [edi],eax

	MSD_NoSet:
	};
#else
	for (int i = 0; i < _Count; ((int32*) _pDest)[i++] = _Value) {};
#endif
};


#include "PCH.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMDA_CRC32Checker
|__________________________________________________________________________________________________
\*************************************************************************************************/

CMDA_CRC32CheckerTable g_CRCTable;

CMDA_CRC32CheckerTable::CMDA_CRC32CheckerTable()
{	
	uint32 c;
	int n, k;

	for (n = 0; n < 256; n++) 
	{
		c = (uint32)n;
	
		for (k = 0; k < 8; k++)
		{
			if (c & 1) 
				c = 0xEDB88320 ^ (c >> 1);
			else
				c = c >> 1;
		}

		m_Entry[n] = c;
	}
}

CMDA_CRC32Checker::CMDA_CRC32Checker()
{
	m_CurrentCRC = 0;
}

CMDA_CRC32Checker::~CMDA_CRC32Checker()
{
}

void CMDA_CRC32Checker::ResetCRC()
{
	m_CurrentCRC = 0;
}

void CMDA_CRC32Checker::AddBlock(const void *block, int size)
{
	uint32 c = m_CurrentCRC ^ 0xFFFFFFFF;
	uint8 *bufend = ((uint8 *)block) + size;

	for (; bufend - ((uint8 *)block) ; ++(*((uint8 **)&block)) )
		c = g_CRCTable.m_Entry[(c ^ (*((uint8*)block))) & 0xFF] ^ (c >> 8);

	m_CurrentCRC = c ^ 0xFFFFFFFF;
}

void CMDA_CRC32Checker::AddStr(const char *block)
{
	uint32 c = m_CurrentCRC ^ 0xFFFFFFFF;

	for (; *((uint8 *)block); ++block )
		c = g_CRCTable.m_Entry[(c ^ (*((uint8*)block))) & 0xFF] ^ (c >> 8);

	m_CurrentCRC = c ^ 0xFFFFFFFF;
}


void CMDA_CRC32Checker::AddFile(CCFile* _pFile)
{
	const int BufferSize = 4096;
	uint8 lBuffer[BufferSize];

	int Len = _pFile->Length();
	int Pos = 0;

	while(!_pFile->EndOfFile() && (Pos < Len))
	{
		int Chunk = Min(BufferSize, Len-Pos);
		_pFile->Read(lBuffer, Chunk);
		Pos += Chunk;

		AddBlock(lBuffer, Chunk);
	}
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	InternalRadixSort. (internal helper to RadixSort<T>)

	Comments:	Sorts an array..

	Arguments:	_pBuf           - input data.
				_pTempBuf       - temporary buffer, same size as _pBuf.
				_nNumItems      - num elements in _pBuf.
				_nSortkeyOffset - offset to sortkey in elements.
				_nSortkeySize   - size of sortkey in elements.

	Author:		Anton Ragnarsson
\*____________________________________________________________________*/
void InternalRadixSort(void *_pBuf, void *_pTempBuf, int _nNumItems, 
                       int _nSizeofItem, int _nSortkeyOffset, int _nSortkeySize)
{
	static uint32 s_iHashTable[257];

	uint8* pTempBuf = _pTempBuf ? (uint8*)_pTempBuf : DNew(uint8) uint8[_nNumItems * _nSizeofItem];
	uint8* pDst = pTempBuf;
	uint8* pSrc = (uint8*)_pBuf;

	for (int r = 0; r < _nSortkeySize; r++)
	{
#ifdef CPU_BIGENDIAN
		int h = _nSortkeyOffset + (_nSortkeySize - 1) - r;
#else
		int h = _nSortkeyOffset + r;
#endif
		int i;
		for (i=0; i<257; i++)
			s_iHashTable[i] = 0;

		for (i=0; i<_nNumItems; i++)
		{
			int j = pSrc[i*_nSizeofItem + h] + 1;
			s_iHashTable[j]++;
		}

		for (i=0; i<256; i++)
			s_iHashTable[i+1] += s_iHashTable[i];

		for (i=0; i<_nNumItems; i++)
		{
			int j = pSrc[i*_nSizeofItem + h];
			uint8* pA = pSrc + i * _nSizeofItem;
			uint8* pB = pDst + (s_iHashTable[j]++) * _nSizeofItem;
			for (int k=0; k<_nSizeofItem; k++)
				pB[k] = pA[k];
		}

		uint8 *pTmp = pSrc; 
		pSrc = pDst; 
		pDst = pTmp;
	}

	// Need to move data back to source buffer?
	if (_nSortkeySize & 1)
		memcpy(_pBuf, pTempBuf, _nNumItems * _nSizeofItem);

	// Did we allocate our own temp buffer?
	if (!_pTempBuf)
		delete[] pTempBuf;
}


