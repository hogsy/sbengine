#ifndef _INC_MImageDxtLibWrapper
#define _INC_MImageDxtLibWrapper

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			MImageDxtLibWrapper.h

Author:			Anders Ekermo

Copyright:		Copyright Starbreeze Studios AB 2006

Contents:		Wrapper class for NVidia's DxtLib

Comments:		DxtLib usage in separate file to preserve separation

History:		
060124: ae, created file
060814: ae, added "IsAlphaBinary" function
\*____________________________________________________________________*/


//Uncomment to make everything disappear
#ifndef PLATFORM_CONSOLE
#define	S3TC_USE_DXTLIB
#endif

#ifdef S3TC_USE_DXTLIB


class CDXTLibCompressor
{

public:

	CDXTLibCompressor();

	~CDXTLibCompressor();

	// Encode the image
	bool Encode(uint8* _pSrcImg,uint8 * _pDst,uint16 _Width,uint16 _Height,
				bool _bAlpha,fp32 *_pRGBWeights);

protected:

	// Callback function for DXTLIB
	static long Callback(size_t _nBytes,void * _pBuf,void * _pUserData);

	//Test if alpha image can be compressed to DXT1
	static uint32 NonBinaryAlpha(const uint8 * _pSrcImage,uint32 _nPixels,uint8 _Threshold);

	size_t	m_nBytesWritten;
	uint8*	m_pTarget;


};



#endif // S3TC_USE_DXTLIB

#endif // Inclusion guard
