#include "PCH.h"
#include "MBitString.h"
#include "MMisc.h"

// -------------------------------------------------------------------
//  Compressed Bit-String methods.
// -------------------------------------------------------------------

#define CBS_FILL	0
#define CBS_COPY	1

/*
0 : Fill
	3 bits, Len
	1 bits, data
1 : Copy
	3 bits, Len
	x bits, data
*/

static int GetBit(const uint8* _pBits, int _Bit)
{
	return (_pBits[_Bit >> 3] & (1 << (_Bit & 7))) ? 1 : 0;
}

static void SetBit(uint8* _pBits, int _Pos, int _Bit)
{
	if (_Bit & 1)
		_pBits[_Pos >> 3] |= 1 << (_Pos & 7);
	else
		_pBits[_Pos >> 3] &= ~(1 << (_Pos & 7));
}


static int CBS_GetFillLen(const uint8* _pBits, int _Pos, int _Len)
{
	int Bit = GetBit(_pBits, _Pos);
	int i = _Pos;
	while((i < _Len) && (i-_Pos < 16) && GetBit(_pBits, i) == Bit) i++;
	return i-_Pos;
}

static int CBS_GetCopyLen(const uint8* _pBits, int _Pos, int _Len)
{
	int i = _Pos;
	while(i < _Len && CBS_GetFillLen(_pBits, i, _Len) < 6)
	{
		i++;
	}
	return Min(8, i - _Pos);
}

int CBS_Compress(const uint8* _pBits, int _Len, uint8* _pOut)
{
	int Pos = 0;
	int Out = 0;
	while(Pos < _Len)
	{
		int nCopy = CBS_GetCopyLen(_pBits, Pos, _Len);
		if (!nCopy)
		{
			int nFill = CBS_GetFillLen(_pBits, Pos, _Len);
			if (!nFill)
				Error_static("CBS_Compress", "Internal error.");
			nFill >>= 1;
			SetBit(_pOut, Out++, CBS_FILL);
			nFill--;
			SetBit(_pOut, Out++, nFill);
			SetBit(_pOut, Out++, nFill >> 1);
			SetBit(_pOut, Out++, nFill >> 2);
			SetBit(_pOut, Out++, GetBit(_pBits, Pos));
			Pos += (nFill+1)*2;
//		LogFile(CStrF("Fill %d, %d, %d", (nFill+1)*2, Pos, Out));
		}
		else
		{
			SetBit(_pOut, Out++, CBS_COPY);
/*			nCopy--;
			SetBit(_pOut, Out++, nCopy);
			SetBit(_pOut, Out++, nCopy >> 1);
			SetBit(_pOut, Out++, nCopy >> 2);
			for(int i = 0; i < nCopy+1; i++)
				SetBit(_pOut, Out++, GetBit(_pBits, Pos++));*/
//		LogFile(CStrF("Copy %d, %d, %d", nCopy+1, Pos, Out));
			SetBit(_pOut, Out++, GetBit(_pBits, Pos++));
			SetBit(_pOut, Out++, GetBit(_pBits, Pos++));
			SetBit(_pOut, Out++, GetBit(_pBits, Pos++));
		}
	}

	return Out;
}


int CBS_Uncompress(const uint8* _pBits, int _Len, uint8* _pOut)
{
//Error_static("CBS_Uncompress", "Not today.");
	int Pos = 0;
	int nOut = 0;
	while(Pos < _Len)
	{
		int Cmd = GetBit(_pBits, Pos++);
		if (Cmd == CBS_FILL)
		{
			int nFill = GetBit(_pBits, Pos++);
			nFill += GetBit(_pBits, Pos++) << 1;
			nFill += GetBit(_pBits, Pos++) << 2;
			nFill++;
			nFill *= 2;
			int Bit = GetBit(_pBits, Pos++);
			for(int i = 0; i < nFill; i++)
				SetBit(_pOut, nOut++, Bit);
//		LogFile(CStrF("Fill %d, %d, %d", nFill, Pos, nOut));
		}
		else
		{
			SetBit(_pOut, nOut++, GetBit(_pBits, Pos++));
			SetBit(_pOut, nOut++, GetBit(_pBits, Pos++));
			SetBit(_pOut, nOut++, GetBit(_pBits, Pos++));
/*			int nCopy = GetBit(_pBits, Pos++);
			nCopy += GetBit(_pBits, Pos++) << 1;
			nCopy += GetBit(_pBits, Pos++) << 2;
			nCopy++;
			for(int i = 0; i < nCopy; i++)
				SetBit(_pOut, nOut++, GetBit(_pBits, Pos++));*/
//		LogFile(CStrF("Copy %d, %d, %d", nCopy, Pos, nOut));
		}
	}

	return nOut;
}

void CBS_ConvertByteString(const uint8* _pBytes, int _Len, uint8* _pOut)
{
	for(int i = 0; i < _Len; i++)
		SetBit(_pOut, i, _pBytes[i]);
}


CStr CBS_GetString(const uint8* _pBits, int _Len)
{
	CStr s;
	for(int i = 0; i < _Len; i++)
		s += CStrF("%d", GetBit(_pBits, i));
	return s;
}

