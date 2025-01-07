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

#include "MGLGlobalFunctions.h"

extern const char* aGLESErrorMessages[];
extern const char* aGLCGErrorMessages[];

// -------------------------------------------------------------------
//  OpenGLES exception class
// -------------------------------------------------------------------
class CCExceptionGLES : public CCExceptionGraphicsHAL
{
public:
	CCExceptionGLES(const CObj* pObj, char* _functionname, char* _sourceposstr, int GLESerr);

	static CStr GLESErrToStr(int GLESErr);
};

// -------------------------------------------------------------------
#define GLESThrowErr(err, _functionname) \
throw CCExceptionGLES(this, _functionname, MACRO_EXCEPT_SOURCE_POS, err);

#ifdef M_RTM
#define GLESNoErrorChecking
#endif

#ifdef GLESNoErrorChecking
#define GLErr(_functionname) ((void)(0))
#define GLESErr(_functionname) ((void)(0))
#define GLESErrStatic(_functionname) ((void)(0))
#else
	__inline void GLESBreakPoint()
	{
//		M_BREAKPOINT;
	}

	#define GLCGErr(_functionname)									\
	{																\
		CGerror err = cgGetError();								\
		if (err != CG_NO_ERROR)										\
		{															\
			MRTC_SystemInfo::OS_Trace((CStrF("GLCGERROR: 0x%x(%s) - ", err, aGLCGErrorMessages[err]) + _functionname + "\n").Str());\
			GLESBreakPoint();											\
		}															\
	}

	#define GLESErr(_functionname)									\
	{																\
		GLenum err = glnGetError();									\
		if (err != GL_NO_ERROR)										\
		{															\
			MRTC_SystemInfo::OS_Trace((CStrF("GLESERROR: 0x%x(%s) - ", err, (err >= 0x0500 && err <= 0x0506)?aGLESErrorMessages[err - 0x500]:"Unknown") + _functionname + "\n").Str());\
			GLESBreakPoint();											\
		}															\
	}

	#define GLESErrStatic(_functionname)								\
	{																\
		GLenum err = glnGetError();									\
		if (err != GL_NO_ERROR)										\
		{															\
			MRTC_SystemInfo::OS_Trace((CStrF("GLESERROR: 0x%x(%s) - ", err, (err >= 0x0500 && err <= 0x0506)?aGLESErrorMessages[err - 0x500]:"Unknown") + _functionname + "\n").Str());\
			GLESBreakPoint();											\
		}															\
	}

	#define GLErr(_functionname)\
	{\
		/*glFlush();*/\
		GLESErr(_functionname);\
		GLCGErr(_functionname);\
	}
#endif

// -------------------------------------------------------------------
//  CImageGLES
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

#endif // __MRENDERPS3_IMAGE_H_INCLUDED
