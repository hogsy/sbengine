
#ifndef __INC_IMAGEIO
#define __INC_IMAGEIO

#include "MImage.h"

// -------------------------------------------------------------------
//  CImageIO, virtual image IO class
// -------------------------------------------------------------------
//
//  NOTE:
//		Clipping is not properly supported by all image IO classes.
//		Therefore, clipping should either be removed from the 
//		specification or emulated by the base-class.

class CImageIO : public CReferenceCount
{
public:
	virtual spCImage ReadInfo(const CStr& filename) pure;
	virtual void Read(const CStr& filename, CImage* img, int _MemModel) pure;
	virtual void Write(const CStr& filename, CImage* img) pure;
	virtual void ReadPalette(const CStr& filename, CImagePalette* destpal) pure;

	static TPtr<CImageIO> GetImageIO(const CStr& filename);
	static TPtr<CImageIO> CreateImageIO(const CStr& _Type);
};

// -------------------------------------------------------------------
class CImageIO_JPG : public CImageIO
{
public:
	virtual spCImage ReadInfo(const CStr& filename);

	// These statics are used by CImage
	static void ReadJPG(CCFile* _pFile, CImage* _pImg, bool _bAlpha, int _MemModel, int _Fmt = 0);
	static void WriteJPG(CCFile* _pFile, CImage* _pImg, bool _bAlpha, fp4 _Quality);

	virtual void Read(const CStr& filename, CImage* _pImg, int _MemModel);
	virtual void Write(const CStr& filename, CImage* _pImg);
	virtual void ReadPalette(const CStr& filename, CImagePalette* destpal);
};


#endif // __INC_IMAGEIO
