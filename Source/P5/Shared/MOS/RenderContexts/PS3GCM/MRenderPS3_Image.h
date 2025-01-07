/*
------------------------------------------------------------------------------------------------
Name:		MImageGLES.cpp/h
Purpose:	Bitmap management
Creation:	9703??

Contents:
class				CImageGLES							960901  -				CImage for OpenGLES

------------------------------------------------------------------------------------------------
*/
#ifndef __MRENDERPS3_IMAGE_H_INCLUDED
#define __MRENDERPS3_IMAGE_H_INCLUDED

#include "../../MSystem/MSystem.h"

namespace NRenderPS3GCM
{

extern const char* aGLESErrorMessages[];
extern const char* aGLCGErrorMessages[];

// -------------------------------------------------------------------
//  CImagePS3
// -------------------------------------------------------------------
class CImagePS3 : public CImage
{
protected:
	// Internal lock/unlock
	virtual void* __Lock(int ExtLockMode);
	virtual void __Unlock();

public:
	// Construction & Destruction
	CImagePS3();
	~CImagePS3();
	virtual void Destroy();
	virtual void Create(int _w, int _h, int _format, int _memmodel, spCImagePalette _spPalette = spCImagePalette(NULL));

	// Drawing operations
//	virtual void SetRAWData(CPnt pos, int bytecount, uint8* data);
	virtual void GetRAWData(CPnt pos, int bytecount, uint8* data);

//	virtual void SetPixel(const CClipRect& cr, CPnt p, CPixel32 _Color);
//	virtual void Fill(const CClipRect& cr, int32 color);
//	virtual void FillZStencil(const CClipRect& cr, int32 color);
//	virtual void Line(const CClipRect& cr, CPnt p0, CPnt p1, int32 color);
//	virtual void Blt(const CClipRect& cr, CImage& src, int _flags, CPnt destp, int _EffectValue = 0);
//	virtual void DebugText(const CClipRect cr, CPnt pos, const char* _pStr, int32 color);

	static uint8 DebugFont[128][8];
	static uint8 DebugFontWidth[128];
};

typedef TPtr<CImagePS3> spCImagePS3;

};

#endif // __MRENDERPS3_IMAGE_H_INCLUDED
