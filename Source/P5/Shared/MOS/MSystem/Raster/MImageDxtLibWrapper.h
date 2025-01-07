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
\*____________________________________________________________________*/


//Uncomment to make everything disappear
#ifndef PLATFORM_CONSOLE
#define	S3TC_USE_DXTLIB
#endif

#ifdef S3TC_USE_DXTLIB


class CDXTLibCompressor
{

public:

	// Encode the image
	void Encode(uint8* _pSrcImg,uint8 * _pDst,uint16 _Width,uint16 _Height,
				bool _bAlpha,fp4 *_pRGBWeights);

protected:

	// Callback function for DXTLIB
	static long Callback(size_t _nBytes,void * _pBuf,void * _pUserData);

	size_t	m_nBytesWritten;
	uint8*	m_pTarget;

};



#endif // S3TC_USE_DXTLIB

#endif // Inclusion guard
