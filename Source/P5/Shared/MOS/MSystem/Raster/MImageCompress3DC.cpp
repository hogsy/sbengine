/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			3DC image compression class
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2004
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		041110:		Created File
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "MImage.h"

#define	IMAGE_IO_3DC

#ifdef	IMAGE_IO_3DC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates required memory for 3DC compressed image
						
	Parameters:			
		_pImg:			Pointer to image
						
	Returns:			Amount of memory required for compressed image.
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int ATI3DC_GetEncodedSize(CImage* _pImg)
{
	// Make sure texture is evenly divisable by 4 (for memory allocation purposes)
	int Width = (_pImg->GetWidth() + 3) & ~3;
	int Height = (_pImg->GetHeight() + 3) & ~3;

	return Width * Height;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Find closest match to requested value in a LUT
						
	Parameters:			
		_Value:			Value to match
		_pTable:		8 entry LUT to search
						
	Returns:			Index of best match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest(uint8 _Value, int* _pTable)
{
	int Value = _Value;
	int iClosest = 0;
	int Diff = Abs(_pTable[0] - Value);
	for(int i = 1; i < 8; i++)
	{
		int d = Abs(_pTable[i] - Value);
		if(d < Diff)
		{
			iClosest = i;
			Diff	= d;
		}
	}

	return iClosest;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Adds 3 bits to 3DC bytestream
						
	Parameters:			
		_pDest:			Pointer to destination buffer
		_Val:			Value to insert
		_Shift:			Amount of shifting required to place it at the correct position
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void Add3Bits(uint8* &_pDest, int _Val, int& _Shift)
{
	*_pDest	|= _Val << _Shift;
	_Shift	+= 3;
	if(_Shift >= 8)
	{
		_pDest++;
		_Shift	-= 8;
		if(_Shift == 1)
			*_pDest	= _Val >> 2;
		else if(_Shift == 2)
			*_pDest	= _Val >> 1;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Encodes a single 4x4 3DC block
						
	Parameters:			
		_pSrcData:		Pointer to source image
		_pDest:			Pointer to destination buffer
		_SrcModulo:		Stride of source image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATI3DC_EncodeBlock(uint8* _pSrcData, uint8* _pDest, int _SrcModulo)
{
	memset(_pDest, 0, 16);
	uint8	xMin = 255, xMax = 0;
	uint8	yMin = 255, yMax = 0;

	for(int y = 0; y < 4; y++)
	{
		xMin = Min(xMin, _pSrcData[y*_SrcModulo + 0]);
		xMax = Max(xMax, _pSrcData[y*_SrcModulo + 0]);
		yMin = Min(yMin, _pSrcData[y*_SrcModulo + 1]);
		yMax = Max(yMax, _pSrcData[y*_SrcModulo + 1]);

		xMin = Min(xMin, _pSrcData[y*_SrcModulo + 2]);
		xMax = Max(xMax, _pSrcData[y*_SrcModulo + 2]);
		yMin = Min(yMin, _pSrcData[y*_SrcModulo + 3]);
		yMax = Max(yMax, _pSrcData[y*_SrcModulo + 3]);

		xMin = Min(xMin, _pSrcData[y*_SrcModulo + 4]);
		xMax = Max(xMax, _pSrcData[y*_SrcModulo + 4]);
		yMin = Min(yMin, _pSrcData[y*_SrcModulo + 5]);
		yMax = Max(yMax, _pSrcData[y*_SrcModulo + 5]);

		xMin = Min(xMin, _pSrcData[y*_SrcModulo + 6]);
		xMax = Max(xMax, _pSrcData[y*_SrcModulo + 6]);
		yMin = Min(yMin, _pSrcData[y*_SrcModulo + 7]);
		yMax = Max(yMax, _pSrcData[y*_SrcModulo + 7]);
	}

	int aXVal[8];
	int aYVal[8];
	aXVal[0]	= xMax;
	aXVal[1]	= xMin;
	aYVal[0]	= yMax;
	aYVal[1]	= yMin;

	for(int i = 0; i < 6; i++)
	{
		aXVal[2+i]	= ((int)xMax * (6-i) + (int)xMin * (1+i) + 3) / 7;
		aYVal[2+i]	= ((int)yMax * (6-i) + (int)yMin * (1+i) + 3) / 7;
	}

	int ShiftX = 0;
	int ShiftY = 0;

	uint8* pDestX = _pDest;
	uint8* pDestY = _pDest + 8;

	*pDestX++	= xMax;
	*pDestX++	= xMin;
	*pDestY++	= yMax;
	*pDestY++	= yMin;
	for(int y = 0; y < 4; y++)
	{
		for(int x = 0; x < 4; x++)
		{
			int xVal = FindClosest(_pSrcData[x*2 + 0], aXVal);
			int yVal = FindClosest(_pSrcData[x*2 + 1], aYVal);

			Add3Bits(pDestX, xVal, ShiftX);
			Add3Bits(pDestY, yVal, ShiftY);
		}

		_pSrcData	+= _SrcModulo;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Encode an entire image using 3DC algorithm
						
	Parameters:			
		_pSrcImage:		Pointer to source image
		_pDest:			Pointer to destination buffer
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATI3DC_Encode(CImage* _pSrcImage, uint8* _pDest)
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
				ATI3DC_EncodeBlock(pSrcData + x*2, _pDest, SrcModulo);
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

				ATI3DC_EncodeBlock(aBlock, _pDest, 8);
			}
			_pDest	+= 16;
		}
		pSrcData	+= SrcModulo * 4;
	}

	_pSrcImage->Unlock();
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Retrieves 3 bits from 3DC bytestream
						
	Parameters:			
		_pSrc:			Pointer to 3DC bytestream
		_Shift:			Amount of shifting required to get correct data
						
	Returns:			Value of 3 bits
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int Get3Bits(uint8* &_pSrc, int& _Shift)
{
	int Val = (*_pSrc >> _Shift) & 7;
	_Shift	+= 3;
	if(_Shift >= 8)
	{
		_Shift	-= 8;
		_pSrc++;
		if( _Shift == 1 )
			Val	|= (*_pSrc << 2) & 7;
		else if( _Shift == 2 )
			Val	|= (*_pSrc << 1) & 7;
	}

	return Val;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decompress a 4x4 3DC block
						
	Parameters:			
		_pSrcData:		Pointer to 3DC block
		_pDest:			Pointer to destination image
		_DestModulo:	Stride of destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATI3DC_DecompressBlock(uint8* _pSrcData, uint8* _pDest, int _DestModulo)
{
	uint8 xMin, xMax, yMin, yMax;

	uint8* pXData = _pSrcData;
	uint8* pYData = _pSrcData + 8;
	xMax	= *pXData++;
	xMin	= *pXData++;
	yMax	= *pYData++;
	yMin	= *pYData++;

	int aXVal[8];
	int aYVal[8];
	aXVal[0]	= xMax;
	aXVal[1]	= xMin;
	aYVal[0]	= yMax;
	aYVal[1]	= yMin;

	for(int i = 0; i < 6; i++)
	{
		aXVal[2+i]	= ((int)xMax * (6-i) + (int)xMin * (1+i) + 3) / 7;
		aYVal[2+i]	= ((int)yMax * (6-i) + (int)yMin * (1+i) + 3) / 7;
	}

	int ShiftX = 0;
	int ShiftY = 0;
	for(int y = 0; y < 4; y++)
	{
		for(int x = 0; x < 4; x++)
		{
			int xVal = Get3Bits(pXData, ShiftX);
			int yVal = Get3Bits(pYData, ShiftY);

			_pDest[x*2 + 0]	= aXVal[xVal];
			_pDest[x*2 + 1]	= aYVal[yVal];
		}

		_pDest	+= _DestModulo;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decompress and entire 3DC image
						
	Parameters:			
		_pSrc:			Pointer to 3DC data
		_pDest:			Pointer to destination image
		_DestModulo:	Stride of destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void ATI3DC_Decompress(CImage* _pSrc, uint8* _pDest, int _DestModulo)
{
	int Width = _pSrc->GetWidth();
	int Height = _pSrc->GetHeight();

	uint8* pSrcData = (uint8*)_pSrc->LockCompressed();

	for(int y = 0; y < Height; y += 4)
	{
		for(int x = 0; x < Width; x+= 4)
		{
			if( ((Width - x) >= 4) && ((Height - y) >= 4))
				ATI3DC_DecompressBlock(pSrcData, _pDest + x * 2, _DestModulo);
			else
			{
				uint8 aBlock[4*4*2];
				ATI3DC_DecompressBlock(pSrcData, aBlock, 8);
				int w = Min(4, Width - x);
				int h = Min(4, Height - y);
				for(int iy = 0; iy < h; iy++)
					memcpy(_pDest + x * 2 + iy * _DestModulo, aBlock + iy * 8, w * 2);
			}
			pSrcData	+= 16;
		}
		_pDest	+= _DestModulo * 4;
	}

	_pSrc->Unlock();
}

#endif	// IMAGE_IO_3DC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			CImage interface function for compressing images with 3DC
						
	Parameters:			
		_Quality:		Requested quality (not used)
		_pDestImage:	Pointer to destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Compress_3DC(fp4 _Quality, CImage* _pDestImage)
{
#ifndef	IMAGE_IO_3DC
	Error("Compress_3DC", "3DC support disabled in this build.");
#else
	CImage* pConvImage = this;
	
	CImage ConvTempImage;

	if(GetFormat() != IMAGE_FORMAT_I8A8)
	{
		pConvImage	= &ConvTempImage;

		Convert(pConvImage,IMAGE_FORMAT_I8A8, IMAGE_CONVERT_RGB_GB2IA);
	}

	int AllocSize = ATI3DC_GetEncodedSize(this);

	_pDestImage->Destroy();
	_pDestImage->m_pBitmap	= DNew(uint8 ) uint8 [AllocSize];
	if(!_pDestImage->m_pBitmap)	MemError("Compress_3DC");
	_pDestImage->m_AllocSize	= AllocSize;
	_pDestImage->m_Width		= GetWidth();
	_pDestImage->m_Height		= GetHeight();
	_pDestImage->m_Format		= IMAGE_FORMAT_I8A8;
	_pDestImage->m_Memmodel		= GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_3DC;
	_pDestImage->m_Memmodel		&= ~IMAGE_MEM_LOCKABLE;
	_pDestImage->UpdateFormat();
	_pDestImage->m_Modulo		= _pDestImage->m_Width;

	ATI3DC_Encode(pConvImage, (uint8*)_pDestImage->m_pBitmap);
#endif	// IMAGE_IO_3DC
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			CImage interface function for decompressing a 3DC image
						
	Parameters:			
		_pDestImage:	Pointer to destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Decompress_3DC(CImage* _pDestImage)
{
#ifndef	IMAGE_IO_3DC
	Error("Compress_3DC", "3DC support disabled in this build.");
#else
	_pDestImage->Destroy();
	_pDestImage->Create(GetWidth(), GetHeight(), IMAGE_FORMAT_I8A8, IMAGE_MEM_IMAGE);

	uint8* pDestPtr = (uint8*)_pDestImage->Lock();

	ATI3DC_Decompress(this, pDestPtr, _pDestImage->GetModulo() );

	_pDestImage->Unlock();
#endif	// IMAGE_IO_3DC
}
