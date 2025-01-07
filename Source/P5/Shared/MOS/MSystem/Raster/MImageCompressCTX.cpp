/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CTX image compression class

	Author:			Jim Kjellin

	Copyright:		Starbreeze Studios, 2004

	Contents:		A_list_of_classes_functions_etc_defined_in_file

	Comments:		Longer_description_not_mandatory

	History:		
		041110:		Created File
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "MImage.h"
#include "MImageIO.h"

#ifdef	IMAGE_IO_CTX
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates required memory for CTX compressed image

	Parameters:			
		_pImg:			Pointer to image

	Returns:			Amount of memory required for compressed image.

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int ATICTX_GetEncodedSize(CImage* _pImg)
{
	// Make sure texture is evenly divisable by 4 (for memory allocation purposes)
	int Width = (_pImg->GetWidth() + 3) & ~3;
	int Height = (_pImg->GetHeight() + 3) & ~3;

	return Width * Height;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for color when compared to codebook

	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to use (always 4 entries large)

	Returns:			Smallest possible error for _Color

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp32 FindClosestError4(const CVec2Dfp32& _Color, const CVec2Dfp32* _pCodebook)
{
	fp32 fError0 = (_pCodebook[0] - _Color).LengthSqr();
	fp32 fError1 = (_pCodebook[1] - _Color).LengthSqr();
	fp32 fError2 = (_pCodebook[2] - _Color).LengthSqr();
	fp32 fError3 = (_pCodebook[3] - _Color).LengthSqr();

	return Min(fError0, Min(fError1, Min(fError2, fError3)));
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for color when compared to codebook

	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to use (always 3 entries large)

	Returns:			Smallest possible error for _Color

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp32 FindClosestError3(const CVec2Dfp32& _Color, const CVec2Dfp32* _pCodebook)
{
	fp32 fError0 = (_pCodebook[0] - _Color).LengthSqr();
	fp32 fError1 = (_pCodebook[1] - _Color).LengthSqr();
	fp32 fError2 = (_pCodebook[2] - _Color).LengthSqr();

	return Min(fError0, Min(fError1, fError2));
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
						of a start and stop value.

	Parameters:			
		_Start:			Value0 to use when interpolating the 4 entry codebook
		_Stop:			Value1 to use when interpolating the 4 entry codebook
		_pColors:		The 16 colors to match to codebook

	Returns:			The error for the specified parameters

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp32 GetCTXError1(const CVec2Dfp32& _Start, const CVec2Dfp32& _Stop, const CVec2Dfp32* _pColors)
{
	CVec2Dfp32 aCodebook[4];
	aCodebook[0]	= _Start;
	aCodebook[1]	= _Stop;
	aCodebook[2]	= (_Start * (2.0f / 3.0f)) + (_Stop * (1.0f / 3.0f));
	aCodebook[3]	= (_Start * (1.0f / 3.0f)) + (_Stop * (2.0f / 3.0f));

	fp32 fError = FindClosestError4(_pColors[0], aCodebook);
	for(int i = 1; i < 16; i++)
	{
		fError += FindClosestError4(_pColors[i], aCodebook);
	}

	return fError;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
						of a start and stop value.

	Parameters:			
		_Start:			Value0 to use when interpolating the 3 entry codebook
		_Stop:			Value1 to use when interpolating the 3 entry codebook
		_pColors:		The 16 colors to match to codebook

	Returns:			The error for the specified parameters

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp32 GetCTXError2(const CVec2Dfp32& _Start, const CVec2Dfp32& _Stop, const CVec2Dfp32* _pColors)
{
	CVec2Dfp32 aCodebook[3];
	aCodebook[0]	= _Stop;
	aCodebook[1]	= _Start;
	aCodebook[2]	= (_Start * (1.0f / 2.0f)) + (_Stop * (1.0f / 2.0f));

	fp32 fError = FindClosestError3(_pColors[0], aCodebook);;
	for(int i = 1; i < 16; i++)
	{
		fError += FindClosestError3(_pColors[i], aCodebook);
	}

	return fError;
}

static uint16 InterpolateLA8(uint16 _a, uint16 _b, uint32 _Value)
{
	uint16 L = (((_a & 0xff) * (255 - _Value)) + ((_b & 0xff) * _Value)) / 255;
	uint16 A = ((((_a>>8) & 0xff) * (255 - _Value)) + (((_b>>8) & 0xff) * _Value)) / 255;

	return L | (A << 8);
}
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook

	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 4 entries large)

	Returns:			Closest match

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest(const uint16& _Color, CVec2Dfp32* _pCodebook)
{
	CVec2Dfp32 Color(_Color & 0xff, (_Color >> 8) & 0xff);

	int iClosest = 0;
	fp32 BestDist = Color.DistanceSqr(_pCodebook[0]);
	for( int i = 1; i < 4; i++)
	{
		fp32 fDist = Color.DistanceSqr(_pCodebook[i]);
		if(fDist < BestDist)
		{
			iClosest = i;
			BestDist = fDist;
		}
	}

	return iClosest;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:				Compresses an array of 16 colors into a s3tc codebook

	Parameters:			
		_pFinalCodebook:	Pointer to destination codebook
		_pColors:			Source colors to compress
		_SrcModulo:			Source modulo in pixels

	Comments:				Compression uses octtrees and halfing of intervalls to
							find the points with smallest error.
\*_____________________________________________________________________________*/
static void CTXQuantizer( uint16* _pFinalCodebook, uint16* _pColors, uint32 _SrcModulo )
{
	CVec2Dfp32 M_ALIGN(16) aColors[16];
	for(int i = 0; i < 4; i++)
	{
		aColors[i*4 + 0]	= CVec2Dfp32(_pColors[i * _SrcModulo + 0] & 0xff, (_pColors[i * _SrcModulo + 0] >> 8) & 0xff);
		aColors[i*4 + 1]	= CVec2Dfp32(_pColors[i * _SrcModulo + 1] & 0xff, (_pColors[i * _SrcModulo + 1] >> 8) & 0xff);
		aColors[i*4 + 2]	= CVec2Dfp32(_pColors[i * _SrcModulo + 2] & 0xff, (_pColors[i * _SrcModulo + 2] >> 8) & 0xff);
		aColors[i*4 + 3]	= CVec2Dfp32(_pColors[i * _SrcModulo + 3] & 0xff, (_pColors[i * _SrcModulo + 3] >> 8) & 0xff);
	}

	CRect2Dfp32 BoundRect( aColors[0], aColors[0] );
	for(int i = 1; i < 16; i++)
		BoundRect.Expand(aColors[i]);

	int nIterations = 0;
	CVec2Dfp32 StartMinVal = BoundRect.m_Min;
	CVec2Dfp32 StopMinVal = BoundRect.m_Min;
	CVec2Dfp32 aStartCenters[4], aStopCenters[4];
	CVec2Dfp32 BoxOffset = (StartMinVal + BoundRect.m_Max) * 0.5f - StartMinVal;
	while(BoxOffset.LengthSqr() > Sqr(0.5f))
	{
		CVec2Dfp32 StartMin = StartMinVal + BoxOffset * 0.5f;
		CVec2Dfp32 StartMax = StartMin + BoxOffset;
		aStartCenters[0]	= StartMin;
		aStartCenters[1]	= CVec2Dfp32(StartMax.k[0], StartMin.k[1]);
		aStartCenters[2]	= CVec2Dfp32(StartMin.k[0], StartMax.k[1]);
		aStartCenters[3]	= CVec2Dfp32(StartMax.k[0], StartMax.k[1]);

		CVec2Dfp32 StopMin = StopMinVal + BoxOffset * 0.5f;
		CVec2Dfp32 StopMax = StopMin + BoxOffset;
		aStopCenters[0]	= StopMin;
		aStopCenters[1]	= CVec2Dfp32(StopMax.k[0], StopMin.k[1]);
		aStopCenters[2]	= CVec2Dfp32(StopMin.k[0], StopMax.k[1]);
		aStopCenters[3]	= CVec2Dfp32(StopMax.k[0], StopMax.k[1]);

		int iBestStart = 0, iBestStop = 0;
		fp32 fBestError = GetCTXError1(aStartCenters[0], aStopCenters[0], aColors);

		for(int i = 0; i < 4; i++)
		{
			for(int j = 0; j < 4; j++)
			{
				fp32 fError = GetCTXError1(aStartCenters[i], aStopCenters[j], aColors);
				if(fError < fBestError)
				{
					fBestError	= fError;
					iBestStart	= i;
					iBestStop	= j;
				}
			}
		}

		BoxOffset = BoxOffset * 0.5f;
		StartMinVal	= aStartCenters[iBestStart] - BoxOffset;
		StopMinVal	= aStopCenters[iBestStop] - BoxOffset;

		nIterations++;
	}

	CVec2Dfp32 StartCenter = StartMinVal + BoxOffset;
	CVec2Dfp32 StopCenter = StopMinVal + BoxOffset;

	fp32 fError1 = GetCTXError1(StartCenter, StopCenter, aColors);
	fp32 fError2 = GetCTXError2(StartCenter, StopCenter, aColors);

	if(fError1 < fError2)
	{
		uint16 StartPixel = (uint16)Clamp(StartCenter.k[0], 0, 255) | ((uint16)Clamp(StartCenter.k[1], 0, 255) << 8);
		uint16 StopPixel = (uint16)Clamp(StopCenter.k[0], 0, 255) | ((uint16)Clamp(StopCenter.k[1], 0, 255) << 8);
		if(StartPixel > StopPixel)
		{
			_pFinalCodebook[0]	= StartPixel;
			_pFinalCodebook[1]	= StopPixel;
			_pFinalCodebook[2]	= InterpolateLA8(StartPixel, StopPixel, 85);
			_pFinalCodebook[3]	= InterpolateLA8(StopPixel, StartPixel, 85);
		}
		else
		{
			_pFinalCodebook[0]	= StopPixel;
			_pFinalCodebook[1]	= StartPixel;
			_pFinalCodebook[2]	= InterpolateLA8(StopPixel, StartPixel, 85);
			_pFinalCodebook[3]	= InterpolateLA8(StartPixel, StopPixel, 85);
		}
	}
	else
	{
		uint16 StartPixel = (uint16)Clamp(StartCenter.k[0], 0, 255) | ((uint16)Clamp(StartCenter.k[1], 0, 255) << 8);
		uint16 StopPixel = (uint16)Clamp(StopCenter.k[0], 0, 255) | ((uint16)Clamp(StopCenter.k[1], 0, 255) << 8);
		if(StartPixel > StopPixel)
		{
			_pFinalCodebook[0]	= StopPixel;
			_pFinalCodebook[1]	= StartPixel;
		}
		else
		{
			_pFinalCodebook[0]	= StartPixel;
			_pFinalCodebook[1]	= StopPixel;
		}
		_pFinalCodebook[2]	= InterpolateLA8(StartPixel, StopPixel, 128);
		_pFinalCodebook[3]	= 0;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Encodes pixels for a 4x4 block

	Parameters:			
		_pSrcImage:		Pointer to source pixels
		_pDestBlock:	Pointer to destination buffer
		_SrcStride:		Row stride for source pixels

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATICTX_EncodeBlock(uint16* _pSrcColors, uint8* _pDestBlock, uint32 _SrcModulo)
{
	memset( _pDestBlock, 0, 8 );

	uint16 aCodebook[4];
	CTXQuantizer( aCodebook, _pSrcColors, _SrcModulo );

	CVec2Dfp32 aCodebookColors[4];
	if(aCodebook[0] > aCodebook[1])
	{
		aCodebookColors[0]	= CVec2Dfp32(aCodebook[0] & 0xff, (aCodebook[0] >> 8 ) & 0xff);
		aCodebookColors[1]	= CVec2Dfp32(aCodebook[1] & 0xff, (aCodebook[1] >> 8 ) & 0xff);
		aCodebookColors[0].Combine(aCodebookColors[1], 1.0f / 3.0f, aCodebookColors[2]);
		aCodebookColors[0].Combine(aCodebookColors[1], 2.0f / 3.0f, aCodebookColors[3]);
	}
	else
	{
		aCodebookColors[0]	= CVec2Dfp32(aCodebook[0] & 0xff, (aCodebook[0] >> 8 ) & 0xff);
		aCodebookColors[1]	= CVec2Dfp32(aCodebook[1] & 0xff, (aCodebook[1] >> 8 ) & 0xff);
		aCodebookColors[0].Combine(aCodebookColors[1], 0.5f, aCodebookColors[2]);
		aCodebookColors[3]	= CVec2Dfp32(0);
	}

	((uint16*)_pDestBlock)[0]	= aCodebook[0];
	((uint16*)_pDestBlock)[1]	= aCodebook[1];
	_pDestBlock	+= 4;
	for( int y = 0; y < 4; y++ )
	{
		int iPixel0 = FindClosest( _pSrcColors[y * _SrcModulo + 0], aCodebookColors );
		int iPixel1 = FindClosest( _pSrcColors[y * _SrcModulo + 1], aCodebookColors );
		int iPixel2 = FindClosest( _pSrcColors[y * _SrcModulo + 2], aCodebookColors );
		int iPixel3 = FindClosest( _pSrcColors[y * _SrcModulo + 3], aCodebookColors );

		_pDestBlock[y]	= iPixel0 | (iPixel1 << 2) | (iPixel2 << 4) | (iPixel3 << 6);
	}
}



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Encode an entire image using CTX algorithm

	Parameters:			
		_pSrcImage:		Pointer to source image
		_pDest:			Pointer to destination buffer

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATICTX_Encode(CImage* _pSrcImage, uint8* _pDest)
{
	uint8* pSrcData = (uint8*)_pSrcImage->Lock();

	int SrcModulo = _pSrcImage->GetModulo();
	int Width = _pSrcImage->GetWidth();
	int Height = _pSrcImage->GetHeight();
	for(int y = 0; y < Height; y += 4)
	{
		for(int x = 0; x < Width; x	+= 4)
		{
			if( ((Width - x) >= 4) && ((Height - y) >= 4))
				ATICTX_EncodeBlock((uint16*)(pSrcData + x*2), _pDest, SrcModulo / 2);	// Modulo is in bytes, convert to pixels
			else
			{
				int w = Min(4, Width - x);
				int h = Min(4, Height - y);
				uint8 aBlock[4*4*2];

				for(int iy = 0; iy < h; iy++)
				{
					for(int ix = 0; ix < w; ix++)
					{
						aBlock[(iy*4+ix)*2+0]	= pSrcData[(iy * SrcModulo) + (x + ix) * 2 + 0];
						aBlock[(iy*4+ix)*2+1]	= pSrcData[(iy * SrcModulo) + (x + ix) * 2 + 1];
					}
				}

				// Replicate first pixel into all "unused" pixels of block
				for(int iy = 0; iy < 4; iy++)
				{
					for(int ix = 0; ix < 4; ix++)
					{
						if((ix < w) && (iy < h))
							continue;
						aBlock[(iy*4+ix)*2+0]	= aBlock[0];
						aBlock[(iy*4+ix)*2+1]	= aBlock[1];
					}
				}

				ATICTX_EncodeBlock((uint16*)aBlock, _pDest, 4);
			}
			_pDest	+= 8;
		}
		pSrcData	+= SrcModulo * 4;
	}

	_pSrcImage->Unlock();
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decompress a 4x4 CTX block

	Parameters:			
		_pSrcData:		Pointer to CTX block
		_pDest:			Pointer to destination image
		_DestModulo:	Stride of destination image

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATICTX_DecompressBlock(uint8* _pSrcData, uint16* _pDest, int _DestModulo)
{
	uint16 aColors[4];

	uint16 Color0 = ((uint16*)_pSrcData)[0];
	uint16 Color1 = ((uint16*)_pSrcData)[1];

	if( Color0 > Color1 )
	{
		aColors[0]	= Color0;
		aColors[1]	= Color1;
		aColors[2]	= InterpolateLA8(Color0, Color1, 85);
		aColors[3]	= InterpolateLA8(Color1, Color0, 85);
	}
	else
	{
		aColors[0]	= Color0;
		aColors[1]	= Color1;
		aColors[2]	= InterpolateLA8(Color0, Color1, 128);
		aColors[3]	= 0;
	}
	_pSrcData	+= 4;

	for( int y = 0; y < 4; y++ )
	{
		int Pixel0 = (_pSrcData[y] & 0x03)>>0;
		int Pixel1 = (_pSrcData[y] & 0x0c)>>2;
		int Pixel2 = (_pSrcData[y] & 0x30)>>4;
		int Pixel3 = (_pSrcData[y] & 0xc0)>>6;
		_pDest[y*_DestModulo+0]	= aColors[Pixel0];
		_pDest[y*_DestModulo+1]	= aColors[Pixel1];
		_pDest[y*_DestModulo+2]	= aColors[Pixel2];
		_pDest[y*_DestModulo+3]	= aColors[Pixel3];
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decompress and entire CTX image

	Parameters:			
		_pSrc:			Pointer to CTX data
		_pDest:			Pointer to destination image
		_DestModulo:	Stride of destination image

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATICTX_Decompress(CImage* _pSrc, uint8* _pDest, int _DestModulo)
{
	int Width = _pSrc->GetWidth();
	int Height = _pSrc->GetHeight();

	uint8* pSrcData = (uint8*)_pSrc->LockCompressed();

	for(int y = 0; y < Height; y += 4)
	{
		for(int x = 0; x < Width; x+= 4)
		{
			if( ((Width - x) >= 4) && ((Height - y) >= 4))
				ATICTX_DecompressBlock(pSrcData, (uint16*)(_pDest + x * 2), _DestModulo / 2);
			else
			{
				uint8 aBlock[4*4*2];
				ATICTX_DecompressBlock(pSrcData, (uint16*)aBlock, 4);
				int w = Min(4, Width - x);
				int h = Min(4, Height - y);
				for(int iy = 0; iy < h; iy++)
					memcpy(_pDest + x * 2 + iy * _DestModulo, aBlock + iy * 8, w * 2);
			}
			pSrcData	+= 8;
		}
		_pDest	+= _DestModulo * 4;
	}

	_pSrc->Unlock();
}


#endif	// IMAGE_IO_CTX

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			CImage interface function for compressing images with CTX

	Parameters:			
		_Quality:		Requested quality (not used)
		_pDestImage:	Pointer to destination image

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Compress_CTX(fp32 _Quality, CImage* _pDestImage)
{
#ifndef IMAGE_IO_CTX
	Error("Compress_CTX", "CTX support disabled in this build.");
#else
	CImage* pConvImage = this;

	CImage ConvTempImage;

	if(GetFormat() != IMAGE_FORMAT_I8A8)
	{
		pConvImage	= &ConvTempImage;

		Convert(pConvImage,IMAGE_FORMAT_I8A8, IMAGE_CONVERT_RGB_GB2IA);
	}

	int AllocSize = ATICTX_GetEncodedSize(this);

	_pDestImage->Destroy();
	_pDestImage->m_pBitmap	= DNew(uint8 ) uint8 [AllocSize];
	if(!_pDestImage->m_pBitmap)	MemError("Compress_CTX");
	_pDestImage->m_AllocSize	= AllocSize;
	_pDestImage->m_Width		= GetWidth();
	_pDestImage->m_Height		= GetHeight();
	_pDestImage->m_Format		= IMAGE_FORMAT_I8A8;
	_pDestImage->m_Memmodel		= GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_CTX;
	_pDestImage->m_Memmodel		&= ~IMAGE_MEM_LOCKABLE;
	_pDestImage->UpdateFormat();
	_pDestImage->m_Modulo		= _pDestImage->m_Width;

	ATICTX_Encode(pConvImage, (uint8*)_pDestImage->m_pBitmap);
#endif
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			CImage interface function for decompressing a CTX image

	Parameters:			
		_pDestImage:	Pointer to destination image

	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Decompress_CTX(CImage* _pDestImage)
{
#ifndef	IMAGE_IO_CTX
	Error("Compress_CTX", "CTX support disabled in this build.");
#else
	_pDestImage->Destroy();
	_pDestImage->Create(GetWidth(), GetHeight(), IMAGE_FORMAT_I8A8, IMAGE_MEM_IMAGE);

	uint8* pDestPtr = (uint8*)_pDestImage->Lock();

	ATICTX_Decompress(this, pDestPtr, _pDestImage->GetModulo() );

	_pDestImage->Unlock();
#endif	// IMAGE_IO_CTX
}
