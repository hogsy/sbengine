/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			S3TC image compression class
					
	Author:			Jim Kjellin
					
	Copyright:		Starbreeze Studios, 2004
					
	Contents:		A_list_of_classes_functions_etc_defined_in_file
					
	Comments:		Longer_description_not_mandatory
					
	History:		
		041110:		Created File
\*_____________________________________________________________________________________________*/

#include "PCH.h"
#include "MImage.h"


#ifndef IMAGE_IO_NOS3TC

#include "MImageDxtLibWrapper.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Template Function:  Clamps a value between a min and max value
						
	Template Params:		
		T:				Type of value
						
	Parameters:			
		_Val:			Value to clamp
		_Min:			Floor value for clamping
		_Max:			Roof value for clamping
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
template <class T>
T Clamp(const T& _Val, const T& _Min, const T& _Max)
{
	if(_Val < _Min) return _Min;
	if(_Val > _Max) return _Max;
	return _Val;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Expands a 565 RGB triplet into a 8888 RGBA quad
						
	Parameters:			
		_Color:			Input 565 triplet
		_Alpha:			Amount of alpha in pixel
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static CPixel32 ExpandRGB565( uint16 _Color, int _Alpha = 255 )
{
	int Blue = _Color & 0x001f;
	int Green = (_Color>>5) & 0x003f;
	int Red = (_Color>>11) & 0x001f;
	Red = (Red << 3) | (Red >> 2);
	Green = (Green << 2) | (Green >> 4);
	Blue = (Blue << 3) | (Blue >> 2);
	return CPixel32( Red, Green, Blue, _Alpha);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes a single 4x4 DXT1 block
						
	Parameters:			
		_pBlock:		Pointer to DXT1 block
		_pDest:			Pointer to destination buffer
		_Stride:		Stride of destination image (in 32-bit pixels)
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT1Block(const uint8* _pBlock, uint32* _pDest, int _Stride)
{
	CPixel32 aColors[4];

	uint16 Color0 = ((uint16*)_pBlock)[0];
	uint16 Color1 = ((uint16*)_pBlock)[1];

	if( Color0 > Color1 )
	{
		CPixel32 Pixel0 = ExpandRGB565(Color0);
		CPixel32 Pixel1 = ExpandRGB565(Color1);

		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGBA( Pixel0, Pixel1, 85 );
		aColors[3]	= CPixel32::LinearRGBA( Pixel1, Pixel0, 85 );
	}
	else
	{
		CPixel32 Pixel0 = ExpandRGB565(Color0);
		CPixel32 Pixel1 = ExpandRGB565(Color1);

		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGBA( Pixel0, Pixel1, 128 );
		aColors[3]	= 0;
	}
	_pBlock	+= 4;

	for( int y = 0; y < 4; y++ )
	{
		int Pixel0 = (_pBlock[y] & 0x03)>>0;
		int Pixel1 = (_pBlock[y] & 0x0c)>>2;
		int Pixel2 = (_pBlock[y] & 0x30)>>4;
		int Pixel3 = (_pBlock[y] & 0xc0)>>6;
		_pDest[y*_Stride+0]	= aColors[Pixel0];
		_pDest[y*_Stride+1]	= aColors[Pixel1];
		_pDest[y*_Stride+2]	= aColors[Pixel2];
		_pDest[y*_Stride+3]	= aColors[Pixel3];
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes an entire DXT1 image
						
	Parameters:			
		_pBlock:		Pointer to DXT1 data
		_Width:			Width of destination image
		_Height:		Height of destination image
		_Stride:		Stride of destination image (in 32-bit pixels)
		_pDestData:		Pointer to destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT1(const uint8* _pBlock, int _Width, int _Height, int _Stride, uint32* _pDestData)
{
	for( int y = 0; y < _Height; y += 4 )
	{
		for( int x = 0; x < _Width; x += 4 )
		{
			if((_Width - x) < 4 || (_Height - y) < 4 )
			{
				uint32	aTempBlock[4*4];
				DecodeDXT1Block( _pBlock, aTempBlock, 4 );
				int h = Min(4, _Height - y);
				int w = Min(4, _Width - x);
				for(int i = 0; i < h; i++)
					memcpy(_pDestData + (y+i) * _Stride + x, aTempBlock + i * 4, w * 4);
			}
			else
			{
				DecodeDXT1Block( _pBlock, _pDestData + y * _Stride + x, _Stride );
			}
			_pBlock	+= 8;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes a single 4x4 DXT3 block
						
	Parameters:			
		_pBlock:		Pointer to DXT3 block
		_pDest:			Pointer to destination block
		_Stride:		Stride of destination image (in 32-bit pixels)
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT3Block(const uint8* _pBlock, uint32* _pDest, int _Stride)
{
	const uint8* pAlpha = _pBlock;
	const uint8* pColor = _pBlock + 8;
	CPixel32 aColors[4];

	uint16 Color0 = ((uint16*)pColor)[0];
	uint16 Color1 = ((uint16*)pColor)[1];
	CPixel32 Pixel0 = ExpandRGB565(Color0, 0);
	CPixel32 Pixel1 = ExpandRGB565(Color1, 0);
	if(Color0 > Color1)
	{
		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGB( Pixel0, Pixel1, 85 );
		aColors[3]	= CPixel32::LinearRGB( Pixel1, Pixel0, 85 );
	}
	else
	{
		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGB( Pixel0, Pixel1, 128 );
		aColors[3]	= 0;
	}
	pColor	+= 4;

	for( int y = 0; y < 4; y++ )
	{
		int Pixel0 = (pColor[y] & 0x03)>>0;
		int Pixel1 = (pColor[y] & 0x0c)>>2;
		int Pixel2 = (pColor[y] & 0x30)>>4;
		int Pixel3 = (pColor[y] & 0xc0)>>6;
		int Alpha0 = (pAlpha[y*2+0] & 0x0f)>>0;
		int Alpha1 = (pAlpha[y*2+0] & 0xf0)>>4;
		int Alpha2 = (pAlpha[y*2+1] & 0x0f)>>0;
		int Alpha3 = (pAlpha[y*2+1] & 0xf0)>>4;
		_pDest[y*_Stride+0]	= aColors[Pixel0] + CPixel32(0,0,0,Alpha0 | (Alpha0<<4));
		_pDest[y*_Stride+1]	= aColors[Pixel1] + CPixel32(0,0,0,Alpha1 | (Alpha1<<4));
		_pDest[y*_Stride+2]	= aColors[Pixel2] + CPixel32(0,0,0,Alpha2 | (Alpha2<<4));
		_pDest[y*_Stride+3]	= aColors[Pixel3] + CPixel32(0,0,0,Alpha3 | (Alpha3<<4));
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes an entire DXT3 image
						
	Parameters:			
		_pBlock:		Pointer to DXT3 data
		_Width:			Width of destination image
		_Height:		Height of destination image
		_Stride:		Stride of destination image (in 32-bit pixels)
		_pDestData:		Pointer to destination data
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT3(const uint8* _pBlock, int _Width, int _Height, int _Stride, uint32* _pDestData)
{
	for( int y = 0; y < _Height; y += 4 )
	{
		for( int x = 0; x < _Width; x += 4 )
		{
			if((_Width - x) < 4 || (_Height - y) < 4 )
			{
				uint32	aTempBlock[4*4];
				DecodeDXT3Block( _pBlock, aTempBlock, 4 );
				int h = Min(4, _Height - y);
				int w = Min(4, _Width - x);
				for(int i = 0; i < h; i++)
					memcpy(_pDestData + (y+i) * _Stride + x, aTempBlock + i * 4, w * 4);
			}
			else
			{
				DecodeDXT3Block( _pBlock, _pDestData + y * _Stride + x, _Stride );
			}
			_pBlock	+= 16;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes a single 4x4 DXT5 block
						
	Parameters:			
		_pBlock:		Pointer to DXT5 block
		_pDest:			Pointer to destination image
		_Stride:		Stride of destination image (in 32-bit pixels)
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT5Block(const uint8* _pBlock, uint32* _pDest, int _Stride)
{
	const uint8* pAlpha = _pBlock;
	const uint8* pColor = _pBlock + 8;
	int aAlphaLookup[8];

	if(pAlpha[0]>pAlpha[1])
	{
		aAlphaLookup[0]	= pAlpha[0];
		aAlphaLookup[1]	= pAlpha[1];

		aAlphaLookup[2]	= (6 * pAlpha[0] + 1 * pAlpha[1] + 3) / 7;
		aAlphaLookup[3]	= (5 * pAlpha[0] + 2 * pAlpha[1] + 3) / 7;
		aAlphaLookup[4]	= (4 * pAlpha[0] + 3 * pAlpha[1] + 3) / 7;
		aAlphaLookup[5]	= (3 * pAlpha[0] + 4 * pAlpha[1] + 3) / 7;
		aAlphaLookup[6]	= (2 * pAlpha[0] + 5 * pAlpha[1] + 3) / 7;
		aAlphaLookup[7]	= (1 * pAlpha[0] + 6 * pAlpha[1] + 3) / 7;
	}
	else
	{
		aAlphaLookup[0]	= pAlpha[0];
		aAlphaLookup[1]	= pAlpha[1];

		aAlphaLookup[2]	= (4 * pAlpha[0] + 1 * pAlpha[1] + 2) / 5;
		aAlphaLookup[3]	= (3 * pAlpha[0] + 2 * pAlpha[1] + 2) / 5;
		aAlphaLookup[4]	= (2 * pAlpha[0] + 3 * pAlpha[1] + 2) / 5;
		aAlphaLookup[5]	= (1 * pAlpha[0] + 4 * pAlpha[1] + 2) / 5;

		aAlphaLookup[6]	= 0;
		aAlphaLookup[7]	= 255;
	}
	pAlpha	+= 2;

	int aAlpha[4*4];
	int Shift = 0;
	for( int i = 0; i < 16; i++ )
	{
		int iAlpha = (*pAlpha >> Shift) & 7;
		Shift	+= 3;
		if( Shift >= 8 )
		{
			pAlpha++;

			if( Shift > 8 )
			{
				// Got 1 or 2 bits from previous operation
				if( Shift == 9 )
					iAlpha	|= (*pAlpha & 1) << 2;
				else if( Shift == 10 )
					iAlpha	|= (*pAlpha & 3) << 1;
			}

			Shift	-= 8;
		}


		aAlpha[i]	= aAlphaLookup[iAlpha];
	}

	CPixel32 aColors[4];
	uint16 Color0 = ((uint16*)pColor)[0];
	uint16 Color1 = ((uint16*)pColor)[1];
	CPixel32 Pixel0 = ExpandRGB565(Color0, 0);
	CPixel32 Pixel1 = ExpandRGB565(Color1, 0);
	if(Color0 > Color1)
	{
		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGB( Pixel0, Pixel1, 85 );
		aColors[3]	= CPixel32::LinearRGB( Pixel1, Pixel0, 85 );
	}
	else
	{
		aColors[0]	= Pixel0;
		aColors[1]	= Pixel1;
		aColors[2]	= CPixel32::LinearRGB( Pixel0, Pixel1, 128 );
		aColors[3]	= 0;
	}
	pColor	+= 4;

	for( int y = 0; y < 4; y++ )
	{
		int iPixel0 = (pColor[y] & 0x03)>>0;
		int iPixel1 = (pColor[y] & 0x0c)>>2;
		int iPixel2 = (pColor[y] & 0x30)>>4;
		int iPixel3 = (pColor[y] & 0xc0)>>6;
		_pDest[y*_Stride+0]	= aColors[iPixel0] + CPixel32(0, 0, 0, aAlpha[y*4 + 0]);
		_pDest[y*_Stride+1]	= aColors[iPixel1] + CPixel32(0, 0, 0, aAlpha[y*4 + 1]);
		_pDest[y*_Stride+2]	= aColors[iPixel2] + CPixel32(0, 0, 0, aAlpha[y*4 + 2]);
		_pDest[y*_Stride+3]	= aColors[iPixel3] + CPixel32(0, 0, 0, aAlpha[y*4 + 3]);
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Decodes an entire DXT5 image
						
	Parameters:			
		_pBlock:		Pointer to DXT5 data
		_Width:			Width of destination image
		_Height:		Height of destination image
		_Stride:		Stride of destination image (in 32-bit pixels)
		_pDestData:		Pointer to destination image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void DecodeDXT5(const uint8* _pBlock, int _Width, int _Height, int _Stride, uint32* _pDestData)
{
	for( int y = 0; y < _Height; y += 4 )
	{
		for( int x = 0; x < _Width; x += 4 )
		{
			if((_Width - x) < 4 || (_Height - y) < 4 )
			{
				uint32	aTempBlock[4*4];
				DecodeDXT5Block( _pBlock, aTempBlock, 4 );
				int h = Min(4, _Height - y);
				int w = Min(4, _Width - x);
				for(int i = 0; i < h; i++)
					memcpy(_pDestData + (y+i) * _Stride + x, aTempBlock + i * 4, w * 4);
			}
			else
			{
				DecodeDXT5Block( _pBlock, _pDestData + y * _Stride + x, _Stride );
			}
			_pBlock	+= 16;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Converts an 888 RGB triplet into a 565 RGB triplet
						
	Parameters:			
		_Pixel:			Input 888 RGB triplet
						
	Returns:			Output 565 triplet
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static uint16 ShrinkTo565(const CPixel32& _Pixel)
{
	int R = (_Pixel.R() >> 3) & 0x1f;
	int G = (_Pixel.G() >> 2) & 0x3f;
	int B = (_Pixel.B() >> 3) & 0x1f;

	return B | (G<<5) | (R<<11);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Converts an 888 RGB triplet into a 565 RGB triplet
						
	Parameters:			
		_Pixel:			Input 888 RGB triplet (represented in floats)
						
	Returns:			Output 565 triplet
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static uint16 ShrinkTo565(const CVec3Dfp4& _Pixel)
{
	int R = (int)Clamp(_Pixel.k[0] * (31.0f / 255.0f), 0.0f, 31.0f);
	int G = (int)Clamp(_Pixel.k[1] * (63.0f / 255.0f), 0.0f, 63.0f);
	int B = (int)Clamp(_Pixel.k[2] * (31.0f / 255.0f), 0.0f, 31.0f);

	return B | (G<<5) | (R<<11);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates the distance between 2 RGBA values
						
	Parameters:			
		_Pixel0:		
		_Pixel1:		
						
	Returns:			Squared distance between pixels with alpha weighed in
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int Distance(const CPixel32& _Pixel0, const CPixel32& _Pixel1)
{
	int deltaR = _Pixel1.R() - _Pixel0.R();
	int deltaG = _Pixel1.G() - _Pixel0.G();
	int deltaB = _Pixel1.B() - _Pixel0.B();
	int deltaA = Abs(_Pixel1.A() - _Pixel0.A());

	return (deltaA << 23) | (deltaR * deltaR + deltaG * deltaG + deltaB * deltaB);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates the distance between 2 RGBA values
						
	Parameters:			
		_Pixel0:		
		_Pixel1:		
						
	Returns:			Squared distance between pixels with alpha weighed in
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 Distance(const CPixel32& _Pixel0, const CVec3Dfp4& _Pixel2)
{
	CVec3Dfp4 Pixel(_Pixel0.R(), _Pixel0.G(), _Pixel0.B());

	return Pixel.DistanceSqr(_Pixel2);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates the distance between 2 RGBA values
						
	Parameters:			
		_Pixel0:		
		_Pixel1:		
						
	Returns:			Squared distance between pixels with alpha weighed in
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 Distance(const CPixel32& _Pixel0, const uint16& _Pixel2)
{
	CVec3Dfp4 Pixel0(_Pixel0.R(), _Pixel0.G(), _Pixel0.B());
	CVec3Dfp4 Pixel1(ExpandRGB565(_Pixel2).R(), ExpandRGB565(_Pixel2).G(), ExpandRGB565(_Pixel2).B() );

	return Pixel1.DistanceSqr(Pixel0);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 4 entries large)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest(const CPixel32& _Color, CVec3Dfp4* _pCodebook)
{
	CVec3Dfp4 Color(_Color.R(), _Color.G(), _Color.B());

	int iClosest = 0;
	fp4 BestDist = Color.DistanceSqr(_pCodebook[0]);
	for( int i = 1; i < 4; i++)
	{
		fp4 fDist = Color.DistanceSqr(_pCodebook[i]);
		if(fDist < BestDist)
		{
			iClosest = i;
			BestDist = fDist;
		}
	}

	return iClosest;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 4 entries large)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest(const CPixel32& _Color, CPixel32* _pCodebook)
{
	int iClosest = 0;
	int BestDist = Distance( _Color, _pCodebook[0] );
	for( int i = 1; i < 4; i++ )
	{
		int d = Distance( _Color, _pCodebook[i] );
		if( d < BestDist )
		{
			BestDist = d;
			iClosest = i;
		}
	}

	return iClosest;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search, first 2 element decides if it's 3 or 4 elements large (S3TC wise)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest(const CPixel32& _Color, uint16* _pCodebook)
{
	int iClosest = 0;
	int BestDist = Distance( _Color, ExpandRGB565(_pCodebook[0]) );
	if(_pCodebook[0] > _pCodebook[1])
	{
		for( int i = 1; i < 4; i++ )
		{
			int d = Distance( _Color, ExpandRGB565(_pCodebook[i]) );
			if( d < BestDist )
			{
				BestDist = d;
				iClosest = i;
			}
		}
	}
	else
	{
		for( int i = 1; i < 3; i++ )
		{
			int d = Distance( _Color, ExpandRGB565(_pCodebook[i]) );
			if( d < BestDist )
			{
				BestDist = d;
				iClosest = i;
			}
		}
	}

	return iClosest;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 4 entries large)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest4(const CPixel32& _Color, uint16* _pCodebook)
{
	int iClosest = 0;
	int BestDist = Distance( _Color, ExpandRGB565(_pCodebook[0]) );
	for( int i = 1; i < 4; i++ )
	{
		int d = Distance( _Color, ExpandRGB565(_pCodebook[i]) );
		if( d < BestDist )
		{
			BestDist = d;
			iClosest = i;
		}
	}

	return iClosest;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 3 entries large)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest3(const CPixel32& _Color, uint16* _pCodebook)
{
	int iClosest = 0;
	int BestDist = Distance( _Color, ExpandRGB565(_pCodebook[0]) );
	for( int i = 1; i < 3; i++ )
	{
		int d = Distance( _Color, ExpandRGB565(_pCodebook[i]) );
		if( d < BestDist )
		{
			BestDist = d;
			iClosest = i;
		}
	}

	return iClosest;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Finds the best match for a color in a codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to search (always 4 entries large)
						
	Returns:			Closest match
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int FindClosest( uint16 _Color, uint16* _pCodebook )
{
	CPixel32 Color = ExpandRGB565(_Color);
	int nMax = ( _pCodebook[0] <= _pCodebook[1] )?3:4;
	int iClosest = 0;
	int BestDist = Distance( Color, ExpandRGB565(_pCodebook[0]) );
	for( int i = 1; i < nMax; i++ )
	{
		int d = Distance( Color, ExpandRGB565(_pCodebook[i]) );
		if( d < BestDist )
		{
			BestDist = d;
			iClosest = i;
		}
	}

	return iClosest;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for color when compared to codebook
						
	Parameters:			
		_Color:			Color to match
		_pCodebook:		Codebook to use (always 4 entries large)
						
	Returns:			Smallest possible error for _Color
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 FindClosestError4(const CVec3Dfp4& _Color, const CVec3Dfp4* _pCodebook)
{
	fp4 fError0 = (_pCodebook[0] - _Color).LengthSqr();
	fp4 fError1 = (_pCodebook[1] - _Color).LengthSqr();
	fp4 fError2 = (_pCodebook[2] - _Color).LengthSqr();
	fp4 fError3 = (_pCodebook[3] - _Color).LengthSqr();

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
static fp4 FindClosestError3(const CVec3Dfp4& _Color, const CVec3Dfp4* _pCodebook)
{
	fp4 fError0 = (_pCodebook[0] - _Color).LengthSqr();
	fp4 fError1 = (_pCodebook[1] - _Color).LengthSqr();
	fp4 fError2 = (_pCodebook[2] - _Color).LengthSqr();

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
static fp4 GetDXT1Error1(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	CVec3Dfp4 aCodebook[4];
	aCodebook[0]	= _Start;
	aCodebook[1]	= _Stop;
	aCodebook[2]	= (_Start * (2.0f / 3.0f)) + (_Stop * (1.0f / 3.0f));
	aCodebook[3]	= (_Start * (1.0f / 3.0f)) + (_Stop * (2.0f / 3.0f));

	fp4 fError = FindClosestError4(_pColors[0], aCodebook);
	for(int i = 1; i < 16; i++)
	{
		fError += FindClosestError4(_pColors[i], aCodebook);
	}

	return fError;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
	                    of a start and stop value. (SSE optimized version)
						
	Parameters:			
		_Start:			Value0 to use when interpolating the 4 entry codebook
		_Stop:			Value1 to use when interpolating the 4 entry codebook
		_pColors:		The 16 colors to match to codebook
						
	Returns:			The error for the specified parameters
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 GetDXT1Error1_SSE(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	CVec3Dfp4 M_ALIGN(16) aCodebook[4];
	aCodebook[0]	= _Start;
	aCodebook[1]	= _Stop;
	aCodebook[2]	= (_Start * (2.0f / 3.0f)) + (_Stop * (1.0f / 3.0f));
	aCodebook[3]	= (_Start * (1.0f / 3.0f)) + (_Stop * (2.0f / 3.0f));

	fp4 fError;
	__asm
	{
		mov		esi, _pColors
		lea		eax, aCodebook
		mov		ecx, 16

		subss	xmm7, xmm7

LoopLabel:
		movss	xmm3, dword ptr [esi]
		movss	xmm4, dword ptr [esi+4]
		movss	xmm5, dword ptr [esi+8]
		add		esi, 12

		movss	xmm0, dword ptr [eax]
		movss	xmm1, dword ptr [eax+4]
		movss	xmm2, dword ptr [eax+8]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		movss	xmm6,xmm0

		movss	xmm0, dword ptr [eax+12]
		movss	xmm1, dword ptr [eax+16]
		movss	xmm2, dword ptr [eax+20]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		minss	xmm6,xmm0

		movss	xmm0, dword ptr [eax+24]
		movss	xmm1, dword ptr [eax+28]
		movss	xmm2, dword ptr [eax+32]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		minss	xmm6,xmm0

		movss	xmm0, dword ptr [eax+36]
		movss	xmm1, dword ptr [eax+40]
		movss	xmm2, dword ptr [eax+44]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		minss	xmm6,xmm0

		addss	xmm7, xmm6

		dec		ecx
		cmp		ecx, 0
		jne		LoopLabel

		movss	dword ptr [fError], xmm7
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
static fp4 GetDXT1Error2(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	CVec3Dfp4 aCodebook[3];
	aCodebook[0]	= _Stop;
	aCodebook[1]	= _Start;
	aCodebook[2]	= (_Start * (1.0f / 2.0f)) + (_Stop * (1.0f / 2.0f));

	fp4 fError = FindClosestError3(_pColors[0], aCodebook);;
	for(int i = 1; i < 16; i++)
	{
		fError += FindClosestError3(_pColors[i], aCodebook);
	}

	return fError;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
	                    of a start and stop value. (SSE optimized version)
						
	Parameters:			
		_Start:			Value0 to use when interpolating the 3 entry codebook
		_Stop:			Value1 to use when interpolating the 3 entry codebook
		_pColors:		The 16 colors to match to codebook
						
	Returns:			The error for the specified parameters
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 GetDXT1Error2_SSE(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	CVec3Dfp4 M_ALIGN(16) aCodebook[3];
	aCodebook[0]	= _Stop;
	aCodebook[1]	= _Start;
	aCodebook[2]	= (_Start * (1.0f / 2.0f)) + (_Stop * (1.0f / 2.0f));

	fp4 fError;
	__asm
	{
		mov		esi, _pColors
		lea		eax, aCodebook
		mov		ecx, 16

		subss	xmm7, xmm7

LoopLabel:
		movss	xmm3, dword ptr [esi]
		movss	xmm4, dword ptr [esi+4]
		movss	xmm5, dword ptr [esi+8]
		add		esi, 12

		movss	xmm0, dword ptr [eax]
		movss	xmm1, dword ptr [eax+4]
		movss	xmm2, dword ptr [eax+8]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		movss	xmm6,xmm0

		movss	xmm0, dword ptr [eax+12]
		movss	xmm1, dword ptr [eax+16]
		movss	xmm2, dword ptr [eax+20]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		minss	xmm6,xmm0

		movss	xmm0, dword ptr [eax+24]
		movss	xmm1, dword ptr [eax+28]
		movss	xmm2, dword ptr [eax+32]
		subss	xmm0, xmm3
		subss	xmm1, xmm4
		subss	xmm2, xmm5
		mulss	xmm0, xmm0
		mulss	xmm1, xmm1
		mulss	xmm2, xmm2
		addss	xmm0, xmm1
		addss	xmm0, xmm2
		minss	xmm6,xmm0

		addss	xmm7, xmm6

		dec		ecx
		cmp		ecx, 0
		jne		LoopLabel

		movss	dword ptr [fError], xmm7
	}

	return fError;
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
	                    of a start and stop value. It uses both 3 and 4 entry codebooks
						and selects the best one.
						
	Parameters:			
		_Start:			Value0 to use when interpolating the codebooks
		_Stop:			Value1 to use when interpolating the codebooks
		_pColors:		The 16 colors to match to codebooks
						
	Returns:			The error for the specified parameters
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 GetDXT1Error(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	return Min(GetDXT1Error1(_Start, _Stop, _pColors), GetDXT1Error2(_Start, _Stop, _pColors));
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Calculates smallest error for 16 colors using S3TC interpolation
	                    of a start and stop value. It uses both 3 and 4 entry codebooks
						and selects the best one. (SSE optimized)
						
	Parameters:			
		_Start:			Value0 to use when interpolating the codebooks
		_Stop:			Value1 to use when interpolating the codebooks
		_pColors:		The 16 colors to match to codebooks
						
	Returns:			The error for the specified parameters
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static fp4 GetDXT1Error_SSE(const CVec3Dfp4& _Start, const CVec3Dfp4& _Stop, const CVec3Dfp4* _pColors)
{
	return Min(GetDXT1Error1_SSE(_Start, _Stop, _pColors), GetDXT1Error2_SSE(_Start, _Stop, _pColors));
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Compresses an array of 16 colors into a s3tc codebook
						
	Parameters:			
		_pFinalCodebook:	Pointer to destination codebook
		_pColors:			Source colors to compress
						
	Comments:			Compression uses octtrees and halfing of intervalls to
	                    find the points with smallest error.
\*_____________________________________________________________________________*/
static void Quantizer( uint16* _pFinalCodebook, CPixel32* _pColors )
{
	CVec3Dfp4 M_ALIGN(16) aColors[16];
	for(int i = 0; i < 16; i++)
		aColors[i]	= CVec3Dfp4(_pColors[i].R(), _pColors[i].G(), _pColors[i].B());
	CBox3Dfp4 BoundBox( aColors[0], aColors[0] );
	for(int i = 1; i < 16; i++)
		BoundBox.Expand(aColors[i]);

	int nIterations = 0;
	CVec3Dfp4 StartMinVal = BoundBox.m_Min;
	CVec3Dfp4 StopMinVal = BoundBox.m_Min;
	CVec3Dfp4 aStartCenters[8], aStopCenters[8];
	CVec3Dfp4 BoxOffset = (StartMinVal + BoundBox.m_Max) * 0.5f - StartMinVal;
	while(BoxOffset.LengthSqr() > Sqr(0.5f))
	{
		CVec3Dfp4 StartMin = StartMinVal + BoxOffset * 0.5f;
		CVec3Dfp4 StartMax = StartMin + BoxOffset;
		aStartCenters[0]	= StartMin;
		aStartCenters[1]	= CVec3Dfp4(StartMax.k[0], StartMin.k[1], StartMin.k[2]);
		aStartCenters[2]	= CVec3Dfp4(StartMin.k[0], StartMax.k[1], StartMin.k[2]);
		aStartCenters[3]	= CVec3Dfp4(StartMax.k[0], StartMax.k[1], StartMin.k[2]);
		aStartCenters[4]	= CVec3Dfp4(StartMin.k[0], StartMin.k[1], StartMax.k[2]);
		aStartCenters[5]	= CVec3Dfp4(StartMax.k[0], StartMin.k[1], StartMax.k[2]);
		aStartCenters[6]	= CVec3Dfp4(StartMin.k[0], StartMax.k[1], StartMax.k[2]);
		aStartCenters[7]	= CVec3Dfp4(StartMax.k[0], StartMax.k[1], StartMax.k[2]);

		CVec3Dfp4 StopMin = StopMinVal + BoxOffset * 0.5f;
		CVec3Dfp4 StopMax = StopMin + BoxOffset;
		aStopCenters[0]	= StopMin;
		aStopCenters[1]	= CVec3Dfp4(StopMax.k[0], StopMin.k[1], StopMin.k[2]);
		aStopCenters[2]	= CVec3Dfp4(StopMin.k[0], StopMax.k[1], StopMin.k[2]);
		aStopCenters[3]	= CVec3Dfp4(StopMax.k[0], StopMax.k[1], StopMin.k[2]);
		aStopCenters[4]	= CVec3Dfp4(StopMin.k[0], StopMin.k[1], StopMax.k[2]);
		aStopCenters[5]	= CVec3Dfp4(StopMax.k[0], StopMin.k[1], StopMax.k[2]);
		aStopCenters[6]	= CVec3Dfp4(StopMin.k[0], StopMax.k[1], StopMax.k[2]);
		aStopCenters[7]	= CVec3Dfp4(StopMax.k[0], StopMax.k[1], StopMax.k[2]);

		int iBestStart = 0, iBestStop = 0;
		fp4 fBestError = GetDXT1Error1_SSE(aStartCenters[0], aStopCenters[0], aColors);

		for(int i = 0; i < 8; i++)
		{
			for(int j = 0; j < 8; j++)
			{
				fp4 fError = GetDXT1Error1_SSE(aStartCenters[i], aStopCenters[j], aColors);
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

	CVec3Dfp4 StartCenter = StartMinVal + BoxOffset;
	CVec3Dfp4 StopCenter = StopMinVal + BoxOffset;

	fp4 fError1 = GetDXT1Error1_SSE(StartCenter, StopCenter, aColors);
	fp4 fError2 = GetDXT1Error2_SSE(StartCenter, StopCenter, aColors);

	if(fError1 < fError2)
	{
		CPixel32 StartPixel = CPixel32(StartCenter.k[0], StartCenter.k[1], StartCenter.k[2], 255);
		CPixel32 StopPixel = CPixel32(StopCenter.k[0], StopCenter.k[1], StopCenter.k[2], 255);
		if(ShrinkTo565(StartPixel) > ShrinkTo565(StopPixel))
		{
			_pFinalCodebook[0]	= ShrinkTo565(StartPixel);
			_pFinalCodebook[1]	= ShrinkTo565(StopPixel);
			_pFinalCodebook[2]	= ShrinkTo565(CPixel32::LinearRGBA(StartPixel, StopPixel, 85));
			_pFinalCodebook[3]	= ShrinkTo565(CPixel32::LinearRGBA(StopPixel, StartPixel, 85));
		}
		else
		{
			_pFinalCodebook[0]	= ShrinkTo565(StopPixel);
			_pFinalCodebook[1]	= ShrinkTo565(StartPixel);
			_pFinalCodebook[2]	= ShrinkTo565(CPixel32::LinearRGBA(StopPixel, StartPixel, 85));
			_pFinalCodebook[3]	= ShrinkTo565(CPixel32::LinearRGBA(StartPixel, StopPixel, 85));
		}
	}
	else
	{
		CPixel32 StartPixel = CPixel32(StartCenter.k[0], StartCenter.k[1], StartCenter.k[2], 255);
		CPixel32 StopPixel = CPixel32(StopCenter.k[0], StopCenter.k[1], StopCenter.k[2], 255);
		if(ShrinkTo565(StartPixel) > ShrinkTo565(StopPixel))
		{
			_pFinalCodebook[0]	= ShrinkTo565(StopPixel);
			_pFinalCodebook[1]	= ShrinkTo565(StartPixel);
		}
		else
		{
			_pFinalCodebook[0]	= ShrinkTo565(StartPixel);
			_pFinalCodebook[1]	= ShrinkTo565(StopPixel);
		}
		_pFinalCodebook[2]	= ShrinkTo565(CPixel32::LinearRGBA(StartPixel, StopPixel, 128));
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
static void EncodeDXT1(CPixel32* _pSrcColors, uint8* _pDestBlock)
{
	memset( _pDestBlock, 0, 8 );

	uint16 aCodebook[4];
	Quantizer( aCodebook, _pSrcColors );

	// Use 32-bit colors for lookup, gives much better quality
	CPixel32 aCodebookColors[4];
	if(aCodebook[0] > aCodebook[1])
	{
		aCodebookColors[0]	= ExpandRGB565(aCodebook[0]);
		aCodebookColors[1]	= ExpandRGB565(aCodebook[1]);
		aCodebookColors[2]	= CPixel32::LinearRGBA(aCodebookColors[0], aCodebookColors[1], 85);
		aCodebookColors[3]	= CPixel32::LinearRGBA(aCodebookColors[1], aCodebookColors[0], 85);
	}
	else
	{
		aCodebookColors[0]	= ExpandRGB565(aCodebook[0]);
		aCodebookColors[1]	= ExpandRGB565(aCodebook[1]);
		aCodebookColors[2]	= CPixel32::LinearRGBA(aCodebookColors[0], aCodebookColors[1], 128);
		aCodebookColors[3]	= CPixel32(0, 0, 0, 0);
	}

	((uint16*)_pDestBlock)[0]	= aCodebook[0];
	((uint16*)_pDestBlock)[1]	= aCodebook[1];
	_pDestBlock	+= 4;
	for( int y = 0; y < 4; y++ )
	{
		int iPixel0 = FindClosest( _pSrcColors[0], aCodebookColors );
		int iPixel1 = FindClosest( _pSrcColors[1], aCodebookColors );
		int iPixel2 = FindClosest( _pSrcColors[2], aCodebookColors );
		int iPixel3 = FindClosest( _pSrcColors[3], aCodebookColors );

		_pDestBlock[y]	= iPixel0 | (iPixel1 << 2) | (iPixel2 << 4) | (iPixel3 << 6);

		_pSrcColors	+= 4;
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Encodes alpha for a 4x4 pixelblock using interpolated alpha
						
	Parameters:			
		_pSrcImage:		Pointer to source pixels
		_pDestBlock:	Pointer to destination buffer
		_SrcStride:		Row stride for source pixels
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void EncodeDXT5(CPixel32* _pSrcImage, uint8* _pDestBlock)
{
	memset( _pDestBlock, 0, 8 );

	// Extract alpha
	int aAlpha[16];
	for(int i = 0; i < 16; i++)
		aAlpha[i]	= _pSrcImage[i].GetA();

	int nMin = 256;
	int nMax = -1;
	bool bUse6 = false;
	for( int i = 0; i < 16; i++ )
	{
		if( (aAlpha[i] == 0) || (aAlpha[i] == 255) )
			bUse6 = true;
		else
		{
			nMin = Min( nMin, aAlpha[i] );
			nMax = Max( nMax, aAlpha[i] );
		}
	}

	int aAlphaLookup[8];
	if( bUse6 )
	{
		aAlphaLookup[0] = nMin;
		aAlphaLookup[1] = nMax;

		aAlphaLookup[2] = ( 4 * nMin + 1 * nMax + 2 ) / 5;
		aAlphaLookup[3] = ( 3 * nMin + 2 * nMax + 2 ) / 5;
		aAlphaLookup[4] = ( 2 * nMin + 3 * nMax + 2 ) / 5;
		aAlphaLookup[5] = ( 1 * nMin + 4 * nMax + 2 ) / 5;
		aAlphaLookup[6] = 0;
		aAlphaLookup[7] = 255;
	}
	else
	{
		aAlphaLookup[0] = nMax;
		aAlphaLookup[1] = nMin;
		aAlphaLookup[2] = ( 6 * nMax + 1 * nMin + 3 ) / 7;
		aAlphaLookup[3] = ( 5 * nMax + 2 * nMin + 3 ) / 7;
		aAlphaLookup[4] = ( 4 * nMax + 3 * nMin + 3 ) / 7;
		aAlphaLookup[5] = ( 3 * nMax + 4 * nMin + 3 ) / 7;
		aAlphaLookup[6] = ( 2 * nMax + 5 * nMin + 3 ) / 7;
		aAlphaLookup[7] = ( 1 * nMax + 6 * nMin + 3 ) / 7;
	}

	_pDestBlock[0]	= aAlphaLookup[0] & 0xff;
	_pDestBlock[1]	= aAlphaLookup[1] & 0xff;
	_pDestBlock	+= 2;
	int Shift = 0;
	for( int i = 0; i < 16; i++ )
	{
		int iBest = 0;
		int BestDiff = Abs(aAlpha[i] - aAlphaLookup[0]);

		for( int j = 1; j < 8; j++ )
		{
			int Diff = Abs(aAlpha[i] - aAlphaLookup[j]);
			if( Diff < BestDiff )
			{
				iBest = j;
				BestDiff = Diff;
				if(BestDiff == 0)
					break;	// Exact match, no point in searching
			}
		}

		*_pDestBlock	|= (iBest << Shift) & 0xff;
		Shift	+= 3;
		if( Shift >= 8 )
		{
			_pDestBlock++;
			if( Shift > 8 )
			{
				if( Shift == 9 )
					*_pDestBlock |= (iBest >> 2) & 0xff;
				else if( Shift == 10 )
					*_pDestBlock |= (iBest >> 1) & 0xff;
			}
			Shift	-= 8;
		}
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Compress image using S3TC algorithm
						
	Parameters:			
		_pImage:			Source image
		_pDest:				Destination memory pointer
		_bImageAlpha:		Does source image have alpha?
		_pRGBWeight:		Weights (not used by algorithm in this file)
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static void S3TCEncode(CImage* _pImage, void* _pDest, bool _bImageAlpha, fp4* _pRGBWeight )
{
	uint8* pBlock = (uint8*)_pDest;
	uint32* pSrcImage = (uint32*)_pImage->Lock();
	int Width = _pImage->GetWidth();
	int Height = _pImage->GetHeight();

#ifdef S3TC_USE_DXTLIB

	CDXTLibCompressor Compressor;
	Compressor.Encode((uint8*)pSrcImage,pBlock,Width,Height,_bImageAlpha,_pRGBWeight);

#else

	int SrcStride = Width;
	for( int y = 0; y < Height; y += 4 )
	{
		for( int x = 0; x < Width; x += 4 )
		{
			CPixel32 aColors[16];
			{
				if( (Width - x) >= 4 && (Height - y) >= 4)
				{
					for( int i = 0; i < 4; i++ )
					{
						memcpy(aColors + i * 4, pSrcImage + x + i * SrcStride, sizeof(CPixel32) * 4);
					}
				}
				else
				{
					// This will fuck up weighting but does that matter?
					int h = Min(4, Height - y);
					int w = Min(4, Width - x);
					for(int by = 0; by < h; by++)
					{
						memcpy(aColors + by * 4, pSrcImage + x + by * SrcStride, w * sizeof(CPixel32));
					}

					for(int by = 0; by < 4; by++)
					{
						for(int bx = 0; bx < 4; bx++)
						{
							if( (bx < w) && (by < h) )
								continue;
							aColors[by*4+bx]	= aColors[0];
						}
					}
				}
			}

			if(_bImageAlpha)
			{
				EncodeDXT5(aColors, pBlock);
				pBlock	+= 8;

				for(int i = 0; i < 16; i++)
					aColors[i].A() = 255;
			}
			EncodeDXT1(aColors, pBlock);
			pBlock	+= 8;
		}

		pSrcImage	+= SrcStride * 4;
	}
#endif // S3TC_USE_DXTLIB

	_pImage->Unlock();
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			Gets required amount of memory for image
						
	Parameters:			
		_Width:			Width of image
		_Height:		Height of image
		_bAlpha:		Does image include alpha?
						
	Returns:			Amount of memory required for compressed image
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
static int S3TCGetEncodeSize(int _Width, int _Height, bool _bAlpha)
{
	// Make sure both are evenly divisable by 4
	int Width = (_Width + 3) & ~3;
	int Height = (_Height + 3) & ~3;

	int nSize = Width * Height;
	if(!_bAlpha)
		nSize	>>= 1;

	return nSize;
}
#endif // IMAGE_IO_NOS3TC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			S3TC image compression
						
	Parameters:			
		_Quality:		Requested compression quality (0.0-1.0 range)
		_pDestImg:		Pointer to image to placed compressed data in
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Compress_S3TC(fp4 _Quality, CImage* _pDestImg)
{
#ifdef IMAGE_IO_NOS3TC
	Error("Compress_S3TC", "S3TC support disabled in this build.");
#else
	if (!CanCompress(IMAGE_MEM_COMPRESSTYPE_S3TC))
		Error("Compress", CStrF("Image format not supported by S3TC compressed format. (%s)", (char*)CImage::GetFormatName(GetFormat())));

	bool bImageAlpha = false;
	int CompressType = IMAGE_COMPRESSTYPE_S3TC_DXT1;

	switch(GetFormat())
	{
	case IMAGE_FORMAT_BGRA8 :
		bImageAlpha	= true;
		CompressType = IMAGE_COMPRESSTYPE_S3TC_DXT5;
		break;
	default :;
	}

	CImage *ConvImage = this;

	CImage ConvImageTemp;

	if (GetFormat() != IMAGE_FORMAT_BGRA8)
	{
		ConvImage = &ConvImageTemp;

		//Duplicate(ConvImage);

		Convert(ConvImage, IMAGE_FORMAT_BGRA8, IMAGE_CONVERT_RGBA);
	}

	uint8* pSurfMem = NULL;
	MLock(ConvImage, pSurfMem);

	uint32 CompressedSize = S3TCGetEncodeSize(ConvImage->GetWidth(), ConvImage->GetHeight(), bImageAlpha);
	uint32 AllocSize = CompressedSize + sizeof(CImage_CompressHeader_S3TC);

	_pDestImg->Destroy();
	_pDestImg->m_pBitmap = DNew(uint8) uint8[AllocSize];
	if (!_pDestImg->m_pBitmap) MemError("Compress_S3TC");
	_pDestImg->m_AllocSize = AllocSize;
	_pDestImg->m_Width = GetWidth();
	_pDestImg->m_Height = GetHeight();
	_pDestImg->m_Format = ConvImage->GetFormat();
	_pDestImg->m_Memmodel = GetMemModel() | IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_S3TC;
	_pDestImg->m_Memmodel &= ~IMAGE_MEM_LOCKABLE;
	_pDestImg->UpdateFormat();

//	float weight[3] = {0.3086f, 0.6094f, 0.0820f};
	float weight[3] = {0.33333f, 0.33333f, 0.33333f};
	S3TCEncode(ConvImage, (uint8*)_pDestImg->m_pBitmap + sizeof(CImage_CompressHeader_S3TC), bImageAlpha, NULL);

	CImage_CompressHeader_S3TC& Header = *(CImage_CompressHeader_S3TC*)_pDestImg->m_pBitmap;
	Header.setOffsetData( sizeof(CImage_CompressHeader_S3TC) );
	Header.setCompressType( CompressType );
	Header.setSizeData( CompressedSize );
	Header.setReserved( 0 );

	MUnlock(ConvImage);
#endif
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:			S3TC image decompression
						
	Parameters:			
		_pDestImg:		description
						
	Comments:			Longer_description_not_mandatory
\*_____________________________________________________________________________*/
void CImage::Decompress_S3TC(CImage* _pDestImg)
{
#ifdef IMAGE_IO_NOS3TC
	Error("Decompress_S3TC", "S3TC support disabled in this build.");
#else
	uint8* pSurfMem = (uint8* )LockCompressed();

	const CImage_CompressHeader_S3TC& Header = *(CImage_CompressHeader_S3TC*)pSurfMem;

	int Width = GetWidth();
	int Height = GetHeight();
	int Format = 0;
	int Size = Width * Height * 4;	// Destination format is always 32-bit (either RGB32 or RGBA32)
	_pDestImg->Destroy();
	_pDestImg->m_pBitmap	= DNew(uint8 ) uint8 [Size];
	if (!_pDestImg->m_pBitmap) MemError("Decompress_S3TC");
	switch(Header.getCompressType() )
	{
	case IMAGE_COMPRESSTYPE_S3TC_DXT1 : 
		Format = IMAGE_FORMAT_BGRA8;
		DecodeDXT1( pSurfMem + Header.getOffsetData(), Width, Height, Width, (uint32*)_pDestImg->m_pBitmap );
		break;

	case IMAGE_COMPRESSTYPE_S3TC_DXT3 : 
		Format = IMAGE_FORMAT_BGRA8; 
		DecodeDXT3( pSurfMem + Header.getOffsetData(), Width, Height, Width, (uint32*)_pDestImg->m_pBitmap );
		break;

	case IMAGE_COMPRESSTYPE_S3TC_DXT5 : 
		Format = IMAGE_FORMAT_BGRA8; 
		DecodeDXT5( pSurfMem + Header.getOffsetData(), Width, Height, Width, (uint32*)_pDestImg->m_pBitmap );
		break;

	default : Error("Decompress_S3TC", "Unsupported format.");
	}

	_pDestImg->m_AllocSize = Size;
	_pDestImg->m_Width = Width;
	_pDestImg->m_Height = Height;
	_pDestImg->m_Modulo = Width * 4;
	_pDestImg->m_Format = Format;
	_pDestImg->m_Memmodel = IMAGE_MEM_IMAGE | IMAGE_MEM_LOCKABLE;
	_pDestImg->UpdateFormat();

	Unlock();
#endif
}

