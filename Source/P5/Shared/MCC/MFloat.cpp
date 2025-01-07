#include "PCH.h"
#include "../Platform/Platform.h"
#include "MFloat.h"

fp32 g_SinTable[MFLOAT_SINETABLESIZE+4];
fp32 g_RandTable[MFLOAT_RANDTABLESIZE+4];


void MCCDLLEXPORT MFloat_Init()
{
	// Float tables are padded in the end with 4 floats 
	// from the beginning of the table so SSE instructions 
	// can be used to read the table.

	// Init sine table
	{
		for (int i = 0; i < MFLOAT_SINETABLESIZE; i++)
			g_SinTable[i] = M_Sin(fp32(i)/fp32(MFLOAT_SINETABLESIZE)*2.0f*_PI);

		for(int k = 0; k < 4; k++)
			g_SinTable[k+MFLOAT_SINETABLESIZE] = g_SinTable[k];
	}
	
	// Init rand table
	{
		for (int i = 0; i < MFLOAT_RANDTABLESIZE; i++)
			g_RandTable[i] = Random;

		for(int k = 0; k < 4; k++)
			g_RandTable[k+MFLOAT_RANDTABLESIZE] = g_RandTable[k];
	}
}

#ifndef CPU_INTEL_P5

static fp32 g_lAsyncRecp[8];
static int g_iAsyncRecp = -1;

void MCCDLLEXPORT AsyncRecp(fp32 _v)
{
	g_lAsyncRecp[++g_iAsyncRecp] = 1.0f / _v;
}

fp32 MCCDLLEXPORT AsyncRecpGet()
{
	return g_lAsyncRecp[g_iAsyncRecp];
}

void MCCDLLEXPORT AsyncRecpFree()
{
	g_iAsyncRecp--;
}

#endif

void MCCDLLEXPORT MFloat_GetSineTable(const fp32*& _pSin)
{
	_pSin = g_SinTable;
}

/*float MCCDLLEXPORT MFloat_GetSineTable()
{
return g_SinTable;
}*/

MCCDLLEXPORT const fp32 *MFloat_GetRandTable()
{
	return g_RandTable;
}

MCCDLLEXPORT fp32 MFloat_GetRand(int _iIndex)
{
	return g_RandTable[_iIndex &(MFLOAT_RANDTABLESIZE - 1)];
}

