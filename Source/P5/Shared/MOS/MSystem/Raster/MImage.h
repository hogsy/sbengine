/*
------------------------------------------------------------------------------------------------
Name:		MOS_Imge.cpp/h
Purpose:	Bitmap management
Creation:	960901
Author:		Magnus Högdahl

Contents:
class				CPnt						960901  OK					2D Point for CImage functions
class				CRct						960901	OK					2D Rectangle - " -
class				CClipRect					960901  OK					Clipping def. for CImage func.
class				CPixel555, 565, 24, 32		960901  OK					Pixel classes.
class				CImagePalette				960901  OK					-
class				CImage						960901  work in progress	-
class				CROP						961123	?					No current purpose.
class				CROPIP						961123	?					Utility class for CROP
class				CROP						000713	Deleted				Useless
class				CROPIP						000713	Deleted				Useless
class				CImageIO_JPG				000713	OK					JPG image IO support.

------------------------------------------------------------------------------------------------
*/
#ifndef _INC_MOS_Imge
#define _INC_MOS_Imge

#include "../SysInc.h"
#include "MCC.h"
#include "MRTC_AutoStrip.h"

// -------------------------------------------------------------------
//  CImage format bits, 
// -------------------------------------------------------------------

// THESE ARE STATIC, CAN'T BE CHANGED, EVER!
// TEXTURES AND WAVES ARE STORED WITH THESE FORMATS!

enum
{
	IMAGE_FORMAT_ENUM_SHIFT		= 21,

	IMAGE_FORMAT_VOID			= 0x00000000,
	IMAGE_FORMAT_BGR10A2		= 0x00000001,
//	IMAGE_FORMAT_CLUT4			= 0x00000001,	// 16 indexed
	IMAGE_FORMAT_CLUT8			= 0x00000002,	// 256 indexed
	IMAGE_FORMAT_RGB10A2_F		= 0x00000004,
//	IMAGE_FORMAT_RGB332			= 0x00000004,	// 3-3-2				bytebits:	rrrgggbb
	IMAGE_FORMAT_BGR5			= 0x00000008,	// 5-5-5				wordbits:	xrrrrrgggggbbbbb
	IMAGE_FORMAT_B5G6R5			= 0x00000010,	// 5-6-5				wordbits:	rrrrrggggggbbbbb
	IMAGE_FORMAT_BGR8			= 0x00000020,	// 8-8-8				bytes:		b,g,r,		(was IMAGE_FORMAT_RGB24)
	IMAGE_FORMAT_BGRX8			= 0x00000040,	// 8-8-8-x				bytes:		b,g,r,x		CPixel32/Windows color order, (was IMAGE_FORMAT_RGB32)
	IMAGE_FORMAT_RGB16			= 0x00000080,	// 16-16-16				words:		r,g,b
	IMAGE_FORMAT_BGRA4			= 0x00000100,	// 4-4-4-4				wordbits:	aaaarrrrggggbbbb
	IMAGE_FORMAT_BGR5A1			= 0x00000200,	// 5-5-5-1				wordbits:	arrrrrgggggbbbbb
//	IMAGE_FORMAT_RGBA3328		= 0x00000400,	// 3-3-2-8				
	IMAGE_FORMAT_BGRA8			= 0x00000800,	// 8-8-8-8				bytes:		b,g,r,a		CPixel32/Windows color order, (was IMAGE_FORMAT_RGBA32)
	IMAGE_FORMAT_BGRA16			= 0x00001000,	// 16-16-16-16			words:		b,g,r,a
	IMAGE_FORMAT_I8				= 0x00002000,	// Intensity 8

	IMAGE_FORMAT_RGBA8			= 0x00004000,	// 8-8-8-8, GL_RGBA		bytes:		r,g,b,a
	IMAGE_FORMAT_I16			= 0x00008000,	// Intensity 16
	IMAGE_FORMAT_RGB8			= 0x00010000,	// 8-8-8, GL_RGB		bytes:		r,g,b

	IMAGE_FORMAT_I8A8			= 0x00020000,	// Intensity8, Alpha8	bytes:		i,a
	IMAGE_FORMAT_A8				= 0x00040000,	// Alpha8

	IMAGE_FORMAT_RGB32_F		= 0x00080000,	// f32-f32-f32,		12 bytes per pixel.	bytes: RRRRGGGGBBBB
	IMAGE_FORMAT_RGBA32_F		= 0x00100000,	// f32-f32-f32-f32, 16 bytes per pixel.	bytes: RRRRGGGGBBBBAAAA
	IMAGE_FORMAT_RGBA16			= 0x00200000,
	IMAGE_FORMAT_RGBA16_F		= 0x00400000,

	// Z-Buffer formats
//	IMAGE_FORMAT_ZBUFFER8		= 0x10000000,
//	IMAGE_FORMAT_ZBUFFER16		= 0x20000000,
//	IMAGE_FORMAT_ZBUFFER24		= 0x40000000,
//	IMAGE_FORMAT_ZBUFFER32		= 0x80000000,
	IMAGE_FORMAT_Z24S8			= 0x80000000,


	// Enumerated formats max 16 formats

//	IMAGE_FORMAT_V8U8			= 1 << IMAGE_FORMAT_ENUM_SHIFT,	// Signed 8-8			digits: rrrrgggg
//	IMAGE_FORMAT_GREY_F16		= 2 << IMAGE_FORMAT_ENUM_SHIFT,	// Floating point 16 bit format

//	IMAGE_FORMAT_UYVY			= 3 << IMAGE_FORMAT_ENUM_SHIFT,	// 
//	IMAGE_FORMAT_YUY2			= 4 << IMAGE_FORMAT_ENUM_SHIFT,	// 

//	IMAGE_FORMAT_V16U16			= 5 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_G16R16			= 6 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_BGR10A2	= IMAGE_FORMAT_ALPHA_MODIFIER | 7 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_A2W10V10U10	= IMAGE_FORMAT_ALPHA_MODIFIER | 8 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_R11G11B10		= 9 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_W11V11U10		= 10 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_G16R16F		= 11 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_R32F			= 12 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_RGBA16	= IMAGE_FORMAT_ALPHA_MODIFIER | 13 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_RGBA16_F	= IMAGE_FORMAT_ALPHA_MODIFIER | 14 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_G32R32F		= 15 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_A32B32G32R32F	= IMAGE_FORMAT_ALPHA_MODIFIER | 16 << IMAGE_FORMAT_ENUM_SHIFT,
//	IMAGE_FORMAT_RGB10A2_F	= IMAGE_FORMAT_ALPHA_MODIFIER | 17 << IMAGE_FORMAT_ENUM_SHIFT,


// Format combos
	IMAGE_FORMAT_ZBUFFER  = IMAGE_FORMAT_Z24S8,

	IMAGE_FORMAT_PALETTE = (IMAGE_FORMAT_CLUT8),

	IMAGE_FORMAT_ALPHA = 
		(IMAGE_FORMAT_BGRA4 + IMAGE_FORMAT_BGR5A1 + IMAGE_FORMAT_BGR10A2 + IMAGE_FORMAT_RGB10A2_F +
		 IMAGE_FORMAT_BGRA8 + IMAGE_FORMAT_BGRA16 + IMAGE_FORMAT_RGBA8 + 
		 IMAGE_FORMAT_I8A8 + IMAGE_FORMAT_A8 + IMAGE_FORMAT_RGBA32_F + IMAGE_FORMAT_RGBA16 + IMAGE_FORMAT_RGBA16_F)
};

enum 
{
	// -------------------------------------------------------------------
	// Types
	IMAGE_MEM_VOID					= 0x00000000
	,IMAGE_MEM_OFFSCREEN				= 0x00000001
	,IMAGE_MEM_PRIMARY				= 0x00000002
	,IMAGE_MEM_PRIMARYDOUBLE			= 0x00000003
	,IMAGE_MEM_TEXTURE				= 0x00000004
	,IMAGE_MEM_ZBUFFER				= 0x00000005

	,IMAGE_MEM_TYPEMASK				= 0x0000000f

	// Flags (bitar)
	,IMAGE_MEM_SYSTEM				= 0x00000010
	,IMAGE_MEM_VIDEO					= 0x00000020
	,IMAGE_MEM_3D					= 0x00000040
	,IMAGE_MEM_WINDOWED				= 0x00000080	// Surface is representing the desktop
	,IMAGE_MEM_BACKBUFFER			= 0x00000100	// Not really used for anything.

	,IMAGE_MEM_VIRTUAL				= 0x00000200	// Image does not contain data, just an image description.
	,IMAGE_MEM_REFERENCE				= 0x00000400	// Image points to data it didn't allocate itself.
	,IMAGE_MEM_LOCKABLE				= 0x00000800	// Indicates if direct access to the bitmap is allowed.
	,IMAGE_MEM_COMPRESSED			= 0x00001000	// Image data is compressed. If COMPRESSED is set, LOCKABLE is zero.
	,IMAGE_MEM_COMPRESSTYPE_JPG		= 0x00002000	// Image data is in JPG format.
	,IMAGE_MEM_COMPRESSTYPE_S3TC	= 0x00004000	// Image data is in S3TC format.
	,IMAGE_MEM_COMPRESSTYPE_SOUND	= 0x00008000	// Image data is in Vorbis sound streaming format.
	,IMAGE_MEM_COMPRESSTYPE_3DC		= 0x00010000	// ATI 3DC compressed texture
	,IMAGE_MEM_COMPRESSTYPE_CTX		= 0x00020000	// ATI CTX compressed texture

	,IMAGE_MEM_ALIGN16				= 0x00100000	// Bitmap data must be 16-byte aligned
	,IMAGE_MEM_ALIGN32				= 0x00200000	// Bitmap data must be 32-byte aligned

	,IMAGE_MEM_COMPRESSTYPE_VQ		= 0x00800000	// Image data is in VQ format

	,IMAGE_MEM_COMPRESSTYPE_QUANT	= 0x40000000	// This is only used as an argument for Compress to tell it to convert textures to CLUT8
	,IMAGE_MEM_COMPRESSTYPE_ALWAYS	= 0x80000000	// This is only used as an argument for Compress to tell it to always compress

	// Combos
	,IMAGE_MEM_IMAGE = (IMAGE_MEM_SYSTEM | IMAGE_MEM_OFFSCREEN)

	,IMAGE_MEM_COMPRESS_ALLFLAGS	= (IMAGE_MEM_COMPRESSED | IMAGE_MEM_COMPRESSTYPE_JPG | IMAGE_MEM_COMPRESSTYPE_S3TC | IMAGE_MEM_COMPRESSTYPE_3DC | IMAGE_MEM_COMPRESSTYPE_CTX | IMAGE_MEM_COMPRESSTYPE_VQ | IMAGE_MEM_COMPRESSTYPE_SOUND)
};

// Other special flags that should be written to file
enum EImageFlags
{
	IMAGE_FLAGS_SWIZZLED			= 0x00000001,	// Image data is swizzled
};

enum EImageLock
{
	IMAGE_LOCK_COMPRESSED		= 1,
	IMAGE_LOCK_RECT		= 2,
};

enum EImageGetPixel
{
	IMAGE_GETPIXEL_EDGECLAMP		= M_Bit(0),
};

// -------------------------------------------------------------------
//  JPG Compression specific
// -------------------------------------------------------------------
class CImage_CompressHeader_JPG
{
public:
	uint32 m_OffsetRGB;
	uint32 m_OffsetAlpha;
};

// -------------------------------------------------------------------
//  S3TC Compression specific
// -------------------------------------------------------------------
class CImage_CompressHeader_S3TC
{
	uint32	m_CompressType;				// IMAGE_COMPRESSTYPE_S3TC_xxxx enum
	uint32	m_SizeData;
	uint32	m_Reserved;
	uint32	m_OffsetData;					// Offset from address returned by Lock();

public:
	uint32	getCompressType( void )	const
	{
		MAUTOSTRIP(CImage_CompressHeader_S3TC_getCompressType, 0);
#ifdef	CPU_LITTLEENDIAN
		return( m_CompressType );
#else
		uint32	tmp = m_CompressType;
		ByteSwap_uint32( tmp );
		return( tmp );
#endif
	};

	uint32	getSizeData( void )	const
	{
		MAUTOSTRIP(CImage_CompressHeader_S3TC_getSizeData, 0);
#ifdef	CPU_LITTLEENDIAN
		return( m_SizeData );
#else
		uint32	tmp = m_SizeData;
		ByteSwap_uint32( tmp );
		return( tmp );
#endif
	};

	uint32	getReserved( void )	const
	{
		MAUTOSTRIP(CImage_CompressHeader_S3TC_getReserved, 0);
#ifdef	CPU_LITTLEENDIAN
		return( m_Reserved );
#else
		uint32	tmp = m_Reserved;
		ByteSwap_uint32( tmp );
		return( tmp );
#endif
	};

	uint32	getOffsetData( void )	const
	{
		MAUTOSTRIP(CImage_CompressHeader_S3TC_getOffsetData, 0);
#ifdef	CPU_LITTLEENDIAN
		return( m_OffsetData );
#else
		uint32	tmp = m_OffsetData;
		ByteSwap_uint32( tmp );
		return( tmp );
#endif
	};

	void setCompressType( uint32 v ) { m_CompressType = v; };
	void setSizeData( uint32 v ) { m_SizeData = v; };
	void setReserved( uint32 v ) { m_Reserved = v; };
	void setOffsetData( uint32 v ) { m_OffsetData = v; };
};

enum
{
	IMAGE_COMPRESSTYPE_S3TC_DXT1 = 0,
	IMAGE_COMPRESSTYPE_S3TC_DXT2,
	IMAGE_COMPRESSTYPE_S3TC_DXT3,
	IMAGE_COMPRESSTYPE_S3TC_DXT4,
	IMAGE_COMPRESSTYPE_S3TC_DXT5,
	IMAGE_COMPRESSTYPE_VQ,

	IMAGE_TRANSFORM_FLIPV		= M_Bit(0),
	IMAGE_TRANSFORM_FLIPH		= M_Bit(1),
	IMAGE_TRANSFORM_ROTATECW	= M_Bit(2),
	IMAGE_TRANSFORM_ROTATECCW	= M_Bit(3)
};

// -------------------------------------------------------------------
#define IMAGE_OPER_COPY				0
#define IMAGE_OPER_ADDSATURATE		1
#define IMAGE_OPER_ADDWRAP			2
#define IMAGE_OPER_SUBSATURATE		3
#define IMAGE_OPER_SUBWRAP			4
#define IMAGE_OPER_MULADDSATURATE	5
#define IMAGE_OPER_MULADDWRAP		6
#define IMAGE_OPER_MULSUBSATURATE	7
#define IMAGE_OPER_MULSUBWRAP		8
#define IMAGE_OPER_MULSATURATE		9
#define IMAGE_OPER_MULWRAP			10
#define IMAGE_OPER_ALPHABLEND		11

// -------------------------------------------------------------------
#define IMAGE_CONVERT_RGB			1	// Note:	Destination alpha channel is set to 1, if present.

#define IMAGE_CONVERT_RGBA			2	// Note:	Destination alpha channel set to source-alpha, Source-alpha is considered to be 1 if not present, 
										//			Only RGBA-Formats can be destination. (this should be changed)

#define IMAGE_CONVERT_RGBA_AFROMRGB	3	// Note:	Destination alpha channel is set to Length(r,g,b), 
										//			Only RGBA-Formats can be destination.

#define	IMAGE_CONVERT_RGB_GB2IA		4	// Note:	Green+Blue to Intensity+Alpha
#define IMAGE_CONVERT_IA2NORMAL		5	// Intensity-alpha to Normal. r = sqrt(i^2 + a^2), g = i, b = a;
#define IMAGE_CONVERT_RGB_GB2GA		6	// Note:	Green+Blue to 0G0A

#define IMAGE_CONVERT_ALPHA			10
#define IMAGE_CONVERT_INVERSEALPHA	11
#define IMAGE_CONVERT_ALPHA_TO_RGB	12
#define IMAGE_CONVERT_INVERSEALPHA_TO_RGB 13
#define IMAGE_CONVERT_CANNOTCONVERT	14

// -------------------------------------------------------------------
//  [Un]Lock macros used internally.
// -------------------------------------------------------------------
#if M_EXCEPTIONS

#define MLock(img, pSurfMem)					\
	pSurfMem = (uint8*) img->__Lock(0);					\
try {

// -------------------------------------------------------------------
#define MUnlock(img) }						\
catch(CCException)							\
{											\
	img->__Unlock();						\
	throw;									\
}											\
catch( ... ) {								\
	img->__Unlock();						\
	throw;									\
};											\
img->__Unlock();

#else
#define MLock(img, pSurfMem) pSurfMem = (uint8*) img->__Lock(0);

#define MUnlock(img) img->__Unlock();

#endif


// -------------------------------------------------------------------
//  CPnt
// -------------------------------------------------------------------
class CClipRect;

class CPnt
{
public:
	int x;
	int y;

	CPnt() : x(0), y(0) {}
	CPnt(int _x0, int _y0) : x(_x0), y(_y0) {}

	M_INLINE static CPnt From_fp32(fp32 _x0, fp32 _p0)
	{
		return CPnt(int(_x0), int(_p0));
	}

	bool operator== (const CPnt& p) const { return ((p.x == x) && (p.y == y)); };
//	const CPnt& operator= (CPnt p) { x = p.x; y = p.y; return *this; };
	void operator+= (const CPnt& p) { x += p.x;  y += p.y; };
	void operator-= (const CPnt& p) { x -= p.x;  y -= p.y; };
	const CPnt operator+ (const CPnt& p) const { return CPnt(x + p.x, y + p.y); }
	const CPnt operator- (const CPnt& p) const { return CPnt(x - p.x, y - p.y); }

	const CPnt& operator= (const CPnt& p) { x = p.x; y = p.y; return *this; };

	void Bound(const CPnt& p0, const CPnt& p1) 
	{
		MAUTOSTRIP(CPnt_Bound, MAUTOSTRIP_VOID);
		x = Min(Max(x, p0.x), p1.x);
		y = Min(Max(y, p0.y), p1.y);
	};
};

// -------------------------------------------------------------------
//  CRct
// -------------------------------------------------------------------
class CRct
{
public:
	CPnt p0;
	CPnt p1;

	CRct() {} // p0 & p1 will construct themselves..

	CRct(int x0, int y0, int x1, int y1) :
		p0(x0, y0),
		p1(Max(x0, x1), Max(y0, y1)) 
	{ }

	M_INLINE CRct static From_fp32(fp32 x0, fp32 y0, fp32 x1, fp32 y1)
	{
		return CRct(int(x0), int(y0), int(x1), int(y1));
	}

	CRct(const CPnt& a, const CPnt& b) :
		p0(a),
		p1(Max(a.x, b.x), Max(a.y, b.y))
	{ }

	void operator+= (const CPnt& p) { p0 += p;  p1 += p; };
	void operator-= (const CPnt& p) { p0 -= p;  p1 -= p; };
	
	void Bound(const CRct& boundr) 
	{
		MAUTOSTRIP(CRct_Bound, MAUTOSTRIP_VOID);
		p0.Bound(boundr.p0, boundr.p1);
		p1.Bound(boundr.p0, boundr.p1);
	};

	bool Valid() const
	{
		MAUTOSTRIP(CRct_Valid, false);
		return ((p1.x > p0.x) && (p1.y > p0.y));
	};

	bool Inside(const CPnt& p) const
	{
		MAUTOSTRIP(CRct_Inside_CPnt, false);
		return ((p.x >= p0.x) && (p.x < p1.x) && (p.y >= p0.y) && (p.y < p1.y));
	};

	bool Inside(const CVec3Dfp32& p) const
	{
		MAUTOSTRIP(CRct_Inside_CVec3Dfp32, false);
		return ((p.k[0]-1 >= p0.x) && (p.k[0]+2 < p1.x) && (p.k[1]-1 >= p0.y) && (p.k[1]+2 < p1.y));
	};

	int GetWidth() const { return (p1.x - p0.x); }
	int GetHeight() const { return (p1.y - p0.y); }
};

M_FORCEINLINE void operator<< (CRect2Duint16& _Dst, CRct _Src)
{
	_Dst.m_Min[0] = _Src.p0.x;
	_Dst.m_Min[1] = _Src.p0.y;
	_Dst.m_Max[0] = _Src.p1.x;
	_Dst.m_Max[1] = _Src.p1.y;
}


// -------------------------------------------------------------------
//  CClipRect
// -------------------------------------------------------------------
class CClipRect
{
public:
	CPnt ofs;		// Abs origo. (screen crd, to be added to whatever is beeing drawn in CClipRect.)
	CRct clip;		// Abs clip rect. (screen crd)

	CClipRect() {};
	
	CClipRect(int x0, int y0, int x1, int y1)
	{
		MAUTOSTRIP(CClipRect_ctor_x0_y0_x1_y1, MAUTOSTRIP_VOID);
		ofs = CPnt(0, 0);
		clip.p0 = CPnt(x0, y0);
		clip.p1 = CPnt(x1, y1);
	};

	CClipRect(const CPnt& p0, const CPnt& p1)
	{
		MAUTOSTRIP(CClipRect_ctor_p0_p1, MAUTOSTRIP_VOID);
		ofs = CPnt(0, 0);
		clip.p0 = p0;
		clip.p1 = p1;
	};

	CClipRect(const CPnt& p0, const CPnt& p1, const CPnt& _ofs)
	{
		MAUTOSTRIP(CClipRect_ctor_p0_p1_ofs, MAUTOSTRIP_VOID);
		ofs = _ofs;
		clip.p0 = p0;
		clip.p1 = p1;
	};

	CClipRect(const CRct& rect)
	{
		MAUTOSTRIP(CClipRect_ctor_rect, MAUTOSTRIP_VOID);
		ofs = CPnt(0, 0);
		clip = rect;
	};

	CClipRect(const CRct& rect, CPnt _ofs)
	{
		MAUTOSTRIP(CClipRect_ctor_rect_ofs, MAUTOSTRIP_VOID);
		ofs = _ofs;
		clip = rect;
	};

	friend void operator+= (CRct& r, const CClipRect cr)		// Offset a rectangle.
	{
		MAUTOSTRIP(CClipRect_operator_add, MAUTOSTRIP_VOID);
		r += cr.ofs;
	};

	friend void operator+= (CPnt& p, const CClipRect cr)	// Offset a point
	{
		MAUTOSTRIP(CClipRect_operator_addme_p_cr, MAUTOSTRIP_VOID);
		p += cr.ofs;
	};

	CClipRect operator+= (const CClipRect& cr)					// Add global clipper to local clipper
	{
		MAUTOSTRIP(CClipRect_operator_addme_cr, CClipRect());
		clip += cr.ofs;
		ofs += cr.ofs;
		clip.Bound(cr.clip);
		return *this;
	};

	void operator= (const CRct& cr )
	{
		MAUTOSTRIP(CClipRect_operator_eq, MAUTOSTRIP_VOID);
		clip = cr;
	};


	bool Visible(CPnt p) const						// Check if CPnt is visible.
	{
		MAUTOSTRIP(CClipRect_Visible_CPnt, false);
		p += ofs;
		return clip.Inside(p);
	};

	bool Visible(const CVec3Dfp32& p) const
	{
		MAUTOSTRIP(CClipRect_Visible_CVec3Dfp32, false);
		return ((p.k[0] >= clip.p0.x-ofs.x) && (p.k[0]+1.0f < clip.p1.x-ofs.x) && (p.k[1] >= clip.p0.y-ofs.y) && (p.k[1]+1.0f < clip.p1.y-ofs.y));
	};

	bool Visible(CRct r) const							// Check if CRct is visible.
	{
		MAUTOSTRIP(CClipRect_Visible_CRct, false);
		r += ofs;
		r.Bound(clip);
		return r.Valid();
	};

	bool Visible(CClipRect cr) const					// Check if CClipRect is visible.
	{
		MAUTOSTRIP(CClipRect_Visible_CClipRect, false);
		cr.clip += ofs;
		cr.clip.Bound(clip);
		return cr.clip.Valid();
	};

	bool VisibleLine(int y) const
	{
		MAUTOSTRIP(CClipRect_VisibleLine, false);
		y += ofs.y;
		return ((y >= clip.p0.y) && (y < clip.p1.y));
	};

	bool VisibleColumn(int x) const
	{
		MAUTOSTRIP(CClipRect_VisibleColumn, false);
		x += ofs.x;
		return ((x >= clip.p0.x) && (x < clip.p1.x));
	};

	const int GetWidth() const { return clip.GetWidth(); };
	const int GetHeight() const { return clip.GetHeight(); };
};

// -------------------------------------------------------------------
//  CPixels
// -------------------------------------------------------------------

struct CPixel24			// Junk, don't bother with it
{
	uint8 c8[3];

	CPixel24(int32 c)	// Röv kod!
	{   
		MAUTOSTRIP(CPixel24_ctor, MAUTOSTRIP_VOID);
		c8[0] = (uint8) c;
		c8[1] = (uint8) (c >> 8);
		c8[2] = (uint8) (c >> 16);
	};
};

// -------------------------------------------------------------------
struct CPixel565		// Junk, don't bother with it
{
	uint16 p16;
	CPixel565(uint16 c) { p16 = c; };

	operator int32 () const { return p16; };
};

// -------------------------------------------------------------------
struct CPixel555		// Junk, don't bother with it
{
	uint16 p16;
	CPixel555(uint16 c) { p16 = c; };

	operator int32 () const { return p16; };
};

// -------------------------------------------------------------------
#define LERPI(c0, c1, t) ((c0) + ((((c1)-(c0))*(t)) >> 8))

struct CPixel32Aggr
{
protected:
	union
	{
		uint32 p32;
		uint8 b[4]; // PC/XBox:  b,g,r,a (0xaarrggbb)
                // GameCube: r,g,b,a (0xrrggbbaa)
	};
public:

#ifdef CPU_BIGENDIAN
	enum 
	{
		INDEX_A = 0,
		INDEX_R = 1,
		INDEX_G = 2,
		INDEX_B = 3,
		INDEX_OFFSET = 1
	};
#else
	enum 
	{
		INDEX_A = 3,
		INDEX_R = 2,
		INDEX_G = 1,
		INDEX_B = 0,
		INDEX_OFFSET = 0
	};
#endif

	M_INLINE const uint8& A() const { return b[INDEX_A]; }
	M_INLINE       uint8& A()       { return b[INDEX_A]; }
	M_INLINE const uint8& R() const { return b[INDEX_R]; }
	M_INLINE       uint8& R()       { return b[INDEX_R]; }
	M_INLINE const uint8& G() const { return b[INDEX_G]; }
	M_INLINE       uint8& G()       { return b[INDEX_G]; }
	M_INLINE const uint8& B() const { return b[INDEX_B]; }
	M_INLINE       uint8& B()       { return b[INDEX_B]; }

	M_INLINE int GetR() const { return R(); }
	M_INLINE int GetG() const { return G(); }
	M_INLINE int GetB() const { return B(); }
	M_INLINE int GetA() const { return A(); }

	M_INLINE bool operator > (const CPixel32Aggr& _Other) const { return p32 > _Other.p32; }
	M_INLINE bool operator < (const CPixel32Aggr& _Other) const { return p32 < _Other.p32; }
	M_INLINE bool operator == (const CPixel32Aggr& _Other) const { return p32 == _Other.p32; }
	M_INLINE bool operator != (const CPixel32Aggr& _Other) const { return p32 != _Other.p32; }

	M_INLINE operator uint32 () const { return p32; }
};

struct CPixel32 : public CPixel32Aggr
{
public:

	//----------------------------------------
	M_FORCEINLINE void operator = (const CPixel32Aggr& _p)
	{
		p32 = _p;
	}

	M_INLINE CPixel32()
	{
	}

	M_INLINE CPixel32( int _x ) // input is in PC-format (0xaarrggbb)
	{p32 = _x;}

	M_INLINE CPixel32(const CPixel32Aggr& _c)
	{p32 = _c;}

	M_FORCEINLINE CPixel32(const CVec4Dfp32& _v)
	{
		M_VSt_V4f32_Pixel32(_v.v, &p32);
/*		R() = TruncToInt(ClampRange(_v.k[0], 255));
		G() = TruncToInt(ClampRange(_v.k[1], 255));
		B() = TruncToInt(ClampRange(_v.k[2], 255));
		A() = TruncToInt(ClampRange(_v.k[3], 255));*/
	}

	M_INLINE CPixel32( int r, int g, int b )
	{
		p32 = Min(b, 255) + (Min(g, 255)<<8) + (Min(r,255)<<16);
	}

	M_INLINE CPixel32(int r, int g, int b, int a)
	{
		p32 = Min(b, 255) + (Min(g, 255)<<8) + (Min(r,255)<<16) + (Min(a,255)<<24);
	}

	M_FORCEINLINE static CPixel32 From_fp32( fp32 r, fp32 g, fp32 b )
	{
		vec128 c = M_VLd(r, g, b, 255.0f);
		CPixel32 Ret;
		M_VSt_V4f32_Pixel32(c, &Ret.p32);
		return Ret;
//		return CPixel32(int(r), int(g), int(b));
	}

	M_FORCEINLINE static CPixel32 From_fp32( fp32 r, fp32 g, fp32 b, fp32 a)
	{
		vec128 c = M_VLd(r, g, b, a);
		CPixel32 Ret;
		M_VSt_V4f32_Pixel32(c, &Ret.p32);
		return Ret;
//		return CPixel32(int(r), int(g), int(b), int(a));
	}


	M_FORCEINLINE operator CVec4Dfp32 () const
	{
		CVec4Dfp32 v;
		v.v = M_VLd_Pixel32_f32(&p32);
		return v;
//return CVec4Dfp32(R(), G(), B(), A());
	}

//	M_INLINE void Assign(CVec4Dfp32& _Vec) { _Vec[0] = R(); _Vec[1] = G(); _Vec[2] = B(); _Vec[3] = A(); };
//	M_INLINE void AssignNormalized(CVec4Dfp32& _Vec) { _Vec[0] = fp32(R()) / 255.0f; _Vec[1] = fp32(G()) / 255.0f; _Vec[2] = fp32(B()) / 255.0f; _Vec[3] = fp32(A()) / 255.0f; };
	M_FORCEINLINE void Assign(CVec4Dfp32& _Vec) { _Vec.v = M_VLd_Pixel32_f32(&p32); };
	M_FORCEINLINE void AssignNormalized(CVec4Dfp32& _Vec) { _Vec.v = M_VMul(M_VScalar(1.0f / 255.0f),  M_VLd_Pixel32_f32(&p32)); };

	M_INLINE operator CPixel24 () const { return CPixel24(p32); }

	M_INLINE operator CPixel565 () const  
	{
		MAUTOSTRIP(CPixel32_operator_CPixel565, 0);
		return CPixel565(((p32 & 0xff) >> 3) | ((p32 & 0xfc00) >> (2+8-5)) | ((p32 & 0xf80000) >> (3+8+8-5-6)));
	}

	M_INLINE operator CPixel555 () const 
	{
		MAUTOSTRIP(CPixel32_operator_CPixel555, 0);
		return CPixel555(((p32 & 0xff) >> 3) | ((p32 & 0xf800) >> (3+8-5)) | ((p32 & 0xf80000) >> (3+8+8-5-5)));
	}

	CPixel32 operator* (fp32 _s) const
	{
		MAUTOSTRIP(CPixel32_operator_mul_fp32, 0);
		vec128 s = M_VLdScalar(_s);
		vec128 c = M_VMul(s, M_VLd_Pixel32_f32(&p32));
		CPixel32 Ret;
		M_VSt_V4f32_Pixel32(c, &Ret.p32);
		return Ret;
//		return CPixel32(TruncToInt(GetR()*s), TruncToInt(GetG()*s), TruncToInt(GetB()*s), TruncToInt(GetA()*s));
	}

	CPixel32 operator* (int s) const
	{
		MAUTOSTRIP(CPixel32_operator_mul_fp32, 0);
		return CPixel32(GetR()*s >> 8, GetG()*s >> 8, GetB()*s >> 8, GetA()*s >> 8);
	}

	CPixel32 operator* (const CPixel32& s) const			// Resultat alltid mindre, mul med 1 omöjlig.
	{
		MAUTOSTRIP(CPixel32_operator_mul_CPixel32, 0);
		return CPixel32(GetR()*s.GetR()>>8, GetG()*s.GetG()>>8, GetB()*s.GetB()>>8, GetA()*s.GetA()>>8);
	}

	M_FORCEINLINE void operator*= (fp32 _s)
	{
		MAUTOSTRIP(CPixel32_operator_mulme_fp32, MAUTOSTRIP_VOID);
		vec128 s = M_VLdScalar(_s);
		vec128 c = M_VLd_V4u8_f32(&p32);
		M_VSt_V4f32_u8(M_VMul(s, c), &p32);

/*		b[0] = uint8(fp32(b[0]) * s);
		b[1] = uint8(fp32(b[1]) * s);
		b[2] = uint8(fp32(b[2]) * s);
		b[3] = uint8(fp32(b[3]) * s);*/
	}

	void operator*= (const CPixel32& s)			// Resultat alltid mindre, mul med 1 omöjlig.
	{
		MAUTOSTRIP(CPixel32_operator_mulme_CPixel32, MAUTOSTRIP_VOID);
		b[0] = b[0] * s.b[0] >> 8;
		b[1] = b[1] * s.b[1] >> 8;
		b[2] = b[2] * s.b[2] >> 8;
		b[3] = b[3] * s.b[3] >> 8;
	}

	CPixel32 operator+ (const CPixel32& s)
	{
		MAUTOSTRIP(CPixel32_operator_add_CPixel32, 0);
		return CPixel32(Min(255, GetR() + s.GetR()),
						Min(255, GetG() + s.GetG()),
						Min(255, GetB() + s.GetB()),
						Min(255, GetA() + s.GetA()));
	}

	void operator+= (const CPixel32& s)
	{
		MAUTOSTRIP(CPixel32_operator_addme_CPixel32, MAUTOSTRIP_VOID);
		b[0] = Min(255, b[0] + s.b[0]);
		b[1] = Min(255, b[1] + s.b[1]);
		b[2] = Min(255, b[2] + s.b[2]);
		b[3] = Min(255, b[3] + s.b[3]);
	}

/*	void MultiplyRGB_Fixed_NoClip(int _s)		// Note: "1.0" == 256, inte 255
	{
		MAUTOSTRIP(CPixel32_MultiplyRGB_Fixed_NoClip, MAUTOSTRIP_VOID);
		b[INDEX_OFFSET + 0] = (_s * b[INDEX_OFFSET + 0]) >> 8;
		b[INDEX_OFFSET + 1] = (_s * b[INDEX_OFFSET + 1]) >> 8;
		b[INDEX_OFFSET + 2] = (_s * b[INDEX_OFFSET + 2]) >> 8;
	}

	void MultiplyRGB_Fixed_Clip(int _s)			// Note: "1.0" == 256, inte 255
	{
		MAUTOSTRIP(CPixel32_MultiplyRGB_Fixed_Clip, MAUTOSTRIP_VOID);
		b[INDEX_OFFSET + 0] = Min(0xff00, Max(0, _s * b[INDEX_OFFSET + 0])) >> 8;
		b[INDEX_OFFSET + 1] = Min(0xff00, Max(0, _s * b[INDEX_OFFSET + 1])) >> 8;
		b[INDEX_OFFSET + 2] = Min(0xff00, Max(0, _s * b[INDEX_OFFSET + 2])) >> 8;
	}

	void MultiplyRGBA_Fixed_NoClip(int _s)		// Note: "1.0" == 256, inte 255
	{
		MAUTOSTRIP(CPixel32_MultiplyRGBA_Fixed_NoClip, MAUTOSTRIP_VOID);
		b[0] = (_s * b[0]) >> 8;
		b[1] = (_s * b[1]) >> 8;
		b[2] = (_s * b[2]) >> 8;
		b[3] = (_s * b[3]) >> 8;
	}

	void MultiplyRGBA_Fixed_Clip(int _s)			// Note: "1.0" == 256, inte 255
	{
		MAUTOSTRIP(CPixel32_MultiplyRGBA_Fixed_Clip, MAUTOSTRIP_VOID);
		b[0] = Min(0xff00, Max(0, _s * b[0])) >> 8;
		b[1] = Min(0xff00, Max(0, _s * b[1])) >> 8;
		b[2] = Min(0xff00, Max(0, _s * b[2])) >> 8;
		b[3] = Min(0xff00, Max(0, _s * b[3])) >> 8;
	}

	void Multiply_NoClip(fp32 _s)
	{
		MAUTOSTRIP(CPixel32_Multiply_NoClip, MAUTOSTRIP_VOID);
		b[0] = TruncToInt(((fp32)b[0])*_s);
		b[1] = TruncToInt(((fp32)b[1])*_s);
		b[2] = TruncToInt(((fp32)b[2])*_s);
		b[3] = TruncToInt(((fp32)b[3])*_s);
	}

	void Multiply_Clip(fp32 _s)
	{
		MAUTOSTRIP(CPixel32_Multiply_Clip, MAUTOSTRIP_VOID);
		b[0] = Min(255, Max(0, int(((fp32)b[0])*_s)));
		b[1] = Min(255, Max(0, int(((fp32)b[1])*_s)));
		b[2] = Min(255, Max(0, int(((fp32)b[2])*_s)));
		b[3] = Min(255, Max(0, int(((fp32)b[3])*_s)));
	}
*/
	int GetGray() const
	{
		MAUTOSTRIP(CPixel32_GetGray, 0);
		//Rougly weighted.
		return (GetR() + 2*GetG() + GetB()) >> 2;
	}

	CPixel32 AlphaBlendRGBA(const CPixel32& s, int _Alpha)
	{
		MAUTOSTRIP(CPixel32_AlphaBlendRGBA, 0);
		return CPixel32((GetR() * _Alpha + s.GetR() * (256 - _Alpha)) >> 8,
		                (GetG() * _Alpha + s.GetG() * (256 - _Alpha)) >> 8,
		                (GetB() * _Alpha + s.GetB() * (256 - _Alpha)) >> 8,
		                (GetA() * _Alpha + s.GetA() * (256 - _Alpha)) >> 8);
	}

	CPixel32 AlphaBlendRGB(const CPixel32& s, int _Alpha)
	{
		MAUTOSTRIP(CPixel32_AlphaBlendRGB_s_Alpha, 0);
		return CPixel32((GetR() * _Alpha + s.GetR() * (256 - _Alpha)) >> 8,
		                (GetG() * _Alpha + s.GetG() * (256 - _Alpha)) >> 8,
		                (GetB() * _Alpha + s.GetB() * (256 - _Alpha)) >> 8);
	}

	CPixel32 AlphaBlendRGB(const CPixel32& s)
	{
		MAUTOSTRIP(CPixel32_AlphaBlendRGB_s, 0);
		int Alpha = (GetA() * s.GetA()) >> 8;
		return CPixel32((GetR() * Alpha + s.GetR() * (256 - Alpha)) >> 8,
		                (GetG() * Alpha + s.GetG() * (256 - Alpha)) >> 8,
		                (GetB() * Alpha + s.GetB() * (256 - Alpha)) >> 8,
		                255);
	}

	void Parse(CStr _s)
	{
		MAUTOSTRIP(CPixel32_Parse, MAUTOSTRIP_VOID);
		CVec4Dfp32 v;
		v.ParseColor(_s);
		*this = v;
	}
	
	static CPixel32 FromStr(CStr _s)
	{
		MAUTOSTRIP(CPixel32_FromStr, 0);
		CVec4Dfp32 v;
		v.ParseColor(_s);
		return CPixel32(v);
	}

	// -------------------------------------------------------------------
	//  Interpolation
	// -------------------------------------------------------------------
	static CPixel32 LinearRGB(const CPixel32& c0, const CPixel32& c1, int f)
	{
		MAUTOSTRIP(CPixel32_LinearRGB, 0);
		// f == [0..256]
		int xr0 = LERPI(c0.GetR(), c1.GetR(), f);
		int xg0 = LERPI(c0.GetG(), c1.GetG(), f);
		int xb0 = LERPI(c0.GetB(), c1.GetB(), f);
		return (xr0 << 16) + (xg0 << 8) + xb0;
	}

	static CPixel32 LinearRGBA(const CPixel32& c0, const CPixel32& c1, int f)
	{
		MAUTOSTRIP(CPixel32_LinearRGBA, 0);
		// f == [0..256]
		int xr0 = LERPI(c0.GetR(), c1.GetR(), f);
		int xg0 = LERPI(c0.GetG(), c1.GetG(), f);
		int xb0 = LERPI(c0.GetB(), c1.GetB(), f);
		int xa0 = LERPI(c0.GetA(), c1.GetA(), f);
		return (xa0 << 24) + (xr0 << 16) + (xg0 << 8) + xb0;
	}

	static CPixel32 BilinearRGB(
		const CPixel32& c000, const CPixel32& c001, const CPixel32& c010, const CPixel32& c011,
		int fx, int fy)
	{
		MAUTOSTRIP(CPixel32_BilinearRGB, 0);
		// fx, fy == [0..256]
		int xr0 = LERPI(c000.GetR(), c001.GetR(), fx);
		int xg0 = LERPI(c000.GetG(), c001.GetG(), fx);
		int xb0 = LERPI(c000.GetB(), c001.GetB(), fx);
		int xr1 = LERPI(c010.GetR(), c011.GetR(), fx);
		int xg1 = LERPI(c010.GetG(), c011.GetG(), fx);
		int xb1 = LERPI(c010.GetB(), c011.GetB(), fx);

		int yr0 = LERPI(xr0, xr1, fy);
		int yg0 = LERPI(xg0, xg1, fy);
		int yb0 = LERPI(xb0, xb1, fy);
		return (yr0 << 16) + (yg0 << 8) + yb0;
	}

	static CPixel32 BilinearRGBA(
		const CPixel32& c000, const CPixel32& c001, const CPixel32& c010, const CPixel32& c011,
		int fx, int fy)
	{
		MAUTOSTRIP(CPixel32_BilinearRGBA, 0);
		// fx, fy == [0..256]
		int xr0 = LERPI(c000.GetR(), c001.GetR(), fx);
		int xg0 = LERPI(c000.GetG(), c001.GetG(), fx);
		int xb0 = LERPI(c000.GetB(), c001.GetB(), fx);
		int xa0 = LERPI(c000.GetA(), c001.GetA(), fx);
		int xr1 = LERPI(c010.GetR(), c011.GetR(), fx);
		int xg1 = LERPI(c010.GetG(), c011.GetG(), fx);
		int xb1 = LERPI(c010.GetB(), c011.GetB(), fx);
		int xa1 = LERPI(c010.GetA(), c011.GetA(), fx);

		int yr0 = LERPI(xr0, xr1, fy);
		int yg0 = LERPI(xg0, xg1, fy);
		int yb0 = LERPI(xb0, xb1, fy);
		int ya0 = LERPI(xa0, xa1, fy);

		return (ya0 << 24) + (yr0 << 16) + (yg0 << 8) + yb0;
	}

	static CPixel32 TrilinearRGB(
		const CPixel32& c000, const CPixel32& c001, const CPixel32& c010, const CPixel32& c011,
		const CPixel32& c100, const CPixel32& c101, const CPixel32& c110, const CPixel32& c111,
		int fx, int fy, int fz)
	{
		MAUTOSTRIP(CPixel32_TrilinearRGB, 0);
		// fx, fy, fz == [0..256]
		int xr0 = LERPI(c000.GetR(), c001.GetR(), fx);
		int xg0 = LERPI(c000.GetG(), c001.GetG(), fx);
		int xb0 = LERPI(c000.GetB(), c001.GetB(), fx);
		int xr1 = LERPI(c010.GetR(), c011.GetR(), fx);
		int xg1 = LERPI(c010.GetG(), c011.GetG(), fx);
		int xb1 = LERPI(c010.GetB(), c011.GetB(), fx);

		int yr0 = LERPI(xr0, xr1, fy);
		int yg0 = LERPI(xg0, xg1, fy);
		int yb0 = LERPI(xb0, xb1, fy);

		int xr2 = LERPI(c100.GetR(), c101.GetR(), fx);
		int xg2 = LERPI(c100.GetG(), c101.GetG(), fx);
		int xb2 = LERPI(c100.GetB(), c101.GetB(), fx);
		int xr3 = LERPI(c110.GetR(), c111.GetR(), fx);
		int xg3 = LERPI(c110.GetG(), c111.GetG(), fx);
		int xb3 = LERPI(c110.GetB(), c111.GetB(), fx);

		int yr1 = LERPI(xr2, xr3, fy);
		int yg1 = LERPI(xg2, xg3, fy);
		int yb1 = LERPI(xb2, xb3, fy);

		int zr0 = LERPI(yr0, yr1, fz);
		int zg0 = LERPI(yg0, yg1, fz);
		int zb0 = LERPI(yb0, yb1, fz);
		return (zr0 << 16) + (zg0 << 8) + zb0;
	}


	static CPixel32 TrilinearRGBA(
		const CPixel32& c000, const CPixel32& c001, const CPixel32& c010, const CPixel32& c011,
		const CPixel32& c100, const CPixel32& c101, const CPixel32& c110, const CPixel32& c111,
		int fx, int fy, int fz)
	{
		MAUTOSTRIP(CPixel32_TrilinearRGBA, 0);
		// fx, fy, fz == [0..256]
		int xr0 = LERPI(c000.GetR(), c001.GetR(), fx);
		int xg0 = LERPI(c000.GetG(), c001.GetG(), fx);
		int xb0 = LERPI(c000.GetB(), c001.GetB(), fx);
		int xa0 = LERPI(c000.GetA(), c001.GetA(), fx);
		int xr1 = LERPI(c010.GetR(), c011.GetR(), fx);
		int xg1 = LERPI(c010.GetG(), c011.GetG(), fx);
		int xb1 = LERPI(c010.GetB(), c011.GetB(), fx);
		int xa1 = LERPI(c010.GetA(), c011.GetA(), fx);

		int yr0 = LERPI(xr0, xr1, fy);
		int yg0 = LERPI(xg0, xg1, fy);
		int yb0 = LERPI(xb0, xb1, fy);
		int ya0 = LERPI(xa0, xa1, fy);

		int xr2 = LERPI(c100.GetR(), c101.GetR(), fx);
		int xg2 = LERPI(c100.GetG(), c101.GetG(), fx);
		int xb2 = LERPI(c100.GetB(), c101.GetB(), fx);
		int xa2 = LERPI(c100.GetA(), c101.GetA(), fx);
		int xr3 = LERPI(c110.GetR(), c111.GetR(), fx);
		int xg3 = LERPI(c110.GetG(), c111.GetG(), fx);
		int xb3 = LERPI(c110.GetB(), c111.GetB(), fx);
		int xa3 = LERPI(c110.GetA(), c111.GetA(), fx);

		int yr1 = LERPI(xr2, xr3, fy);
		int yg1 = LERPI(xg2, xg3, fy);
		int yb1 = LERPI(xb2, xb3, fy);
		int ya1 = LERPI(xa2, xa3, fy);

		int zr0 = LERPI(yr0, yr1, fz);
		int zg0 = LERPI(yg0, yg1, fz);
		int zb0 = LERPI(yb0, yb1, fz);
		int za0 = LERPI(ya0, ya1, fz);
		return (za0 << 24) + (zr0 << 16) + (zg0 << 8) + zb0;
	}

	static CPixel32 TrilinearRGBA(const CPixel32* pC, int fx, int fy, int fz)
	{
		MAUTOSTRIP(CPixel32_TrilinearRGBA_2, 0);
		// fx, fy, fz == [0..256]
		int xr0 = LERPI(pC[0].GetR(), pC[1].GetR(), fx);
		int xg0 = LERPI(pC[0].GetG(), pC[1].GetG(), fx);
		int xb0 = LERPI(pC[0].GetB(), pC[1].GetB(), fx);
		int xa0 = LERPI(pC[0].GetA(), pC[1].GetA(), fx);
		int xr1 = LERPI(pC[2].GetR(), pC[3].GetR(), fx);
		int xg1 = LERPI(pC[2].GetG(), pC[3].GetG(), fx);
		int xb1 = LERPI(pC[2].GetB(), pC[3].GetB(), fx);
		int xa1 = LERPI(pC[2].GetA(), pC[3].GetA(), fx);

		int yr0 = LERPI(xr0, xr1, fy);
		int yg0 = LERPI(xg0, xg1, fy);
		int yb0 = LERPI(xb0, xb1, fy);
		int ya0 = LERPI(xa0, xa1, fy);

		int xr2 = LERPI(pC[4].GetR(), pC[5].GetR(), fx);
		int xg2 = LERPI(pC[4].GetG(), pC[5].GetG(), fx);
		int xb2 = LERPI(pC[4].GetB(), pC[5].GetB(), fx);
		int xa2 = LERPI(pC[4].GetA(), pC[5].GetA(), fx);
		int xr3 = LERPI(pC[6].GetR(), pC[7].GetR(), fx);
		int xg3 = LERPI(pC[6].GetG(), pC[7].GetG(), fx);
		int xb3 = LERPI(pC[6].GetB(), pC[7].GetB(), fx);
		int xa3 = LERPI(pC[6].GetA(), pC[7].GetA(), fx);

		int yr1 = LERPI(xr2, xr3, fy);
		int yg1 = LERPI(xg2, xg3, fy);
		int yb1 = LERPI(xb2, xb3, fy);
		int ya1 = LERPI(xa2, xa3, fy);

		int zr0 = LERPI(yr0, yr1, fz);
		int zg0 = LERPI(yg0, yg1, fz);
		int zb0 = LERPI(yb0, yb1, fz);
		int za0 = LERPI(ya0, ya1, fz);
		return (za0 << 24) + (zr0 << 16) + (zg0 << 8) + zb0;
	}

};

// -------------------------------------------------------------------
//  CImagePalette
// -------------------------------------------------------------------
class CImagePalette;
typedef TPtr<CImagePalette> spCImagePalette;

#define PAL_ACCEL_SHADE		1
#define PAL_ACCEL_ADD		2
#define PAL_ACCEL_SUB		3
#define PAL_ACCEL_MIX50		4
#define PAL_ACCEL_MIXINTENS	5

#define PAL_ACCEL_NSTEPS	64
#define PAL_ACCEL_NSTEPSHIFT 6

#ifndef PLATFORM_CONSOLE

class COctreeQuantize;
class CMinErrorQuantize;
typedef CMinErrorQuantize CDefaultQuantize; 
#endif
//typedef COctreeQuantize CDefaultQuantize; 


class SYSTEMDLLEXPORT CImagePalette : public CReferenceCount
{
protected:
	int mnColors;
	CPixel32* mpPal;
#ifndef PLATFORM_CONSOLE
	CDefaultQuantize* mpQuant;
	bool mQuantValid;
	uint8* mpP8_P8_Shade;
	uint16* mpP8_565_Shade;
	uint32* mpP8_32_Shade;
	uint8* mpP8_P8_Mix5050;
#endif


public:

	DECLARE_OPERATOR_NEW

	CImagePalette(int _Size = 256);
	~CImagePalette();
	spCImagePalette Duplicate();

	int GetNumColors()
	{
		return mnColors;
	}

	void SetPalette(CImagePalette* _pPal);
	void SetPalette(CPixel32* Pal, int Start, int nColors);
	void GetPalette(CPixel32* Pal, int Start, int nColors);
	CPixel32* GetPalettePtr() { return mpPal; };
	const CPixel32* GetPalettePtr() const { return mpPal; };

	void Read(const CStr& filename);
#ifndef PLATFORM_CONSOLE
	int GetIndex(CPixel32 _Color);
	void GetIndices(CPixel32* _pSrc, uint8* _pDest, int _nPixels);
	CDefaultQuantize *GetQuantizer();
	void* GetAcceleratorTable(int _SrcFmt, int _DestFmt, int _TableFmt, CImagePalette* _pDestPal);
#endif

};

typedef TPtr<CImagePalette> spCImagePalette;

// -------------------------------------------------------------------
//  CImage
// -------------------------------------------------------------------
class CImage;
class CTexture;
typedef TPtr<CImage> spCImage;
typedef TArray<spCImage> lspCImage;

class SYSTEMDLLEXPORT CImage : public CReferenceCount
{
protected:
	// Debug text data
	static uint8 ms_DebugFont[128][8];
	static uint8 ms_DebugFontWidth[128];

	void* m_pBitmap;
	int m_AllocSize;
	int m_Width;
	int m_Height;
	int m_Modulo;
	int m_Pixelsize;
	int m_Format;
	int m_Flags;
	int m_Memmodel;
	spCImagePalette m_spPalette;
	//--- 8*4 + RefCnt + vtbl = 40 bytes ---

	void UpdateFormat();
	static int Convert2Format(int _indexed, int bitspp, int rdepth, int gdepth, int bdepth, int adepth);

	// Internal lock/unlock
	virtual void* __Lock(int ExtLockMode, CRct *_pRect=NULL);
	virtual void __Unlock();

	// Drawing routines that assumes locked surface, no error checking.
	inline void Internal_SetPixel(int x, int y, int color, uint8* pSurfMem);
	inline int Internal_GetPixel(int x, int y, uint8* pSurfMem);
	void Internal_Line(CPnt p0, CPnt p1, CPixel32 col, uint8* pSurfMem);
	void Internal_FillMask(int mask, int color, int nPixels, uint8* pSurfMem, int psize);
	void Internal_TextureSpan_Clut8_Copy(int SpanLen, fp32 zinv0, fp32 zinv1,
		const CVec2Dfp32& _tc0,  const CVec2Dfp32& _tc1,
		uint8* pSurfMem, uint8* pTextureMem, int Txt_Modulo, int Txt_xAnd, int Txt_yAnd);

#ifndef PLATFORM_CONSOLE
	static void StretchHalfNonPow2(CImage* pSrc, CImage* pDest);
	static void StretchHalfXNonPow2(CImage* pSrc, CImage* pDest);
	static void StretchHalfYNonPow2(CImage* pSrc, CImage* pDest);
#endif // PLATFORM_CONSOLE
public:
	// Construction & Destruction

	DECLARE_OPERATOR_NEW

	CImage();
	CImage(int _w, int _h, int _format, int _memmodel, int _Flags);
	~CImage();
	void operator= (const CImage& _Image);

	virtual void Destroy();
	virtual void Create(int _w, int _h, int _format, int _memmodel, spCImagePalette _spPalette = spCImagePalette(NULL), int _Flags = 0);
	virtual void Create(CImage& imgdesc);
	virtual void CreateVirtual(int _w, int _h, int _format, int _memmodel, spCImagePalette _spPalette = spCImagePalette(NULL), int _Flags = 0);
	virtual void CreateReference(int _w, int _h, int _format, int _memmodel, int _modulo, void* _pBitmap, spCImagePalette _spPalette, int _Flags = 0);
	virtual spCImage Duplicate();
	virtual void Duplicate(CImage* _pTarget);

	void CreateVirtual_NoDelete(int _w, int _h, int _Format, int _MemModel, int _Flags);

protected:
	void Compress_JPG(fp32 _Quality, CImage* _pDestImg);
	void Compress_S3TC(fp32 _Quality, CImage* _pDestImg);
	void Compress_3DC(fp32 _Quality, CImage* _pDestImg);
	void Compress_CTX(fp32 _Quality, CImage* _pDestImg);

#ifndef	PLATFORM_DOLPHIN
	void Compress_VQ(fp32 _Quality, CImage* _pDestImg, CTexture *_pTexture );
	void Decompress_VQ(CImage* _pDestImg);
	void RemapFromClut(spCImagePalette _spPal, CImage *_pDestImg);
#endif

	void Compress_Sound(const char * _pFormat, CImage* _pDestImg, float _Priority, float _Quality, uint32 _Flags);
	void Decompress_JPG(CImage* _pDestImg);
	void Decompress_S3TC(CImage* _pDestImg);
	void Decompress_3DC(CImage* _pDestImg);
	void Decompress_CTX(CImage* _pDestImg);
	void Decompress_Sound(CImage* _pDestImg);

public:
	static int GetCompressFormats(int _Compression);
	bool CanCompress(int _Compression);
	void Compress(int _Compression, fp32 _Quality, CImage* _pDestImg, CTexture *_pTexture = 0);
	void CompressSound(const char * _pFormat, CImage* _pDestImg, float _Priority, float _Quality, uint32 _Flags);
	void Decompress(CImage* _pDestImg);

	virtual void Wait();
	virtual void SetPalette(spCImagePalette _spPal);
	spCImagePalette GetPalette() { return m_spPalette; };
	const spCImagePalette GetPalette() const { return m_spPalette; };

	void* Lock(CRct *_pRect=NULL) { return __Lock(_pRect ? IMAGE_LOCK_RECT : 0, _pRect); };
	void* LockCompressed() { return __Lock(IMAGE_LOCK_COMPRESSED); };
	void Unlock() { __Unlock(); };

	// Image file IO
	static spCImage ReadInfo(const CStr& filename);
	void Read(const CStr& filename, int _MemModel);
	void Write(const CStr& filename);
	void ReadHeader(CCFile* _pFile);
	void WriteHeader(CCFile* _pFile) const;
	void Read(CCFile* _pFile, int _MemModel, spCImagePalette _spPal);
	void Write(CCFile* _pFile);
	void ReadRAW(CCFile* _pFile, int _Width, int _Height, int _Format, int _MemModel, int _Flags, spCImagePalette _spPal);
	void WriteRAW(CCFile* _pFile);

	// Information services
	int IsLockable() const { return m_Memmodel & IMAGE_MEM_LOCKABLE; };
	int IsCompressed() const { return m_Memmodel & IMAGE_MEM_COMPRESSED; };

	int GetWidth() const { return m_Width; };
	int GetHeight() const { return m_Height; };
	int GetFormat() const { return m_Format; };
	int GetFlags() const { return m_Flags; };
	int GetMemModel() const { return m_Memmodel; };
	int GetModulo() const { return m_Modulo; };
	int GetPixelSize() const { return m_Pixelsize; };
	int GetSize() const { return GetWidth()*GetHeight()*GetPixelSize(); };
	int GetMemSize() const;
	static const char* GetFormatName (int _format);
	static const char* GetMemModelName(int _memmodel);
	static const char* GetMemTypeName(int _memmodel);
	CStr IDString() const;
	static int32 ConvertColor(CPixel32 color, int _format, CImagePalette* _pPal = NULL);
	static CPixel32 ConvertRAWColor(uint32 _Col, int _format, CImagePalette* _pPal = NULL);

	static int Format2PixelSize(int _format);
	static int Format2BPP(int _format);
	static int BPP2Format(int bpp);
	static int ZBufferBits2Format(int _zbits);

	CRct GetRect() { return CRct(0, 0, m_Width, m_Height); };
	CClipRect GetClipRect() { return CClipRect(0, 0, m_Width, m_Height); };

	// Geometric operations
	static bool ClipLine(const CRct& cr, int& x0, int& y0, int& x1, int& y1);	// False = Nothing left.
	static bool ClipLine(const CRct& cr, fp32& x0, fp32& y0, fp32& x1, fp32& y1);	// False = Nothing left.

	// Drawing operations
	virtual void SetRAWData(CPnt pos, int bytecount, uint8* data);
	virtual void GetRAWData(CPnt pos, int bytecount, uint8* data);
	static void ConvertPixelArray(void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal, int _ConvertType);
	static void ProcessPixelArray(int _Operation, int _EffectValue1, int _EffectValue2, void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal = NULL);
	static void ProcessPixelArray_Stretch(int _Operation, int _EffectValue1, int _EffectValue2, 
		void* pSrc, void* pDest, int SrcFmt, int DestFmt, int nPixels, CPixel32* pSrcPal, int _nSrcPixels, fp32 _SourcePos, fp32 _SourceStep);

	static void Convert(CImage* pSrc, CImage* pDest, int _ConvertType = IMAGE_CONVERT_RGB);
	static int ConvertDestinationFormats(int _SrcFmt, int _ConvertType = IMAGE_CONVERT_RGB);
	void Convert(CImage* _pTarget, int _TargetFmt, int _ConvertType = IMAGE_CONVERT_RGB);
	spCImage Convert(int _TargetFmt, int _ConvertType = IMAGE_CONVERT_RGB);

	spCImage Transform(int _iTransform);
	static CVec2Dfp32 TransformSampleVector(int _iTransform, const CVec2Dfp32& _InVector, fp32 _PixelWidth, fp32 _PixelHeight);

#ifndef PLATFORM_CONSOLE
	static void StretchHalf(CImage* pSrc, CImage* pDest);
	static void StretchHalfX(CImage* pSrc, CImage* pDest);
	static void StretchHalfY(CImage* pSrc, CImage* pDest);
	static void Stretch(CImage* _pSrc, CImage* _pDest, int _NewWidth, int _NewHeight);
#endif

	virtual void SetPixel(const CClipRect& cr, CPnt p, CPixel32 _Color);
	virtual void SetPixel3f(const CClipRect& cr, CPnt p, const CVec3Dfp32& _Color);
	virtual void SetPixel4f(const CClipRect& cr, CPnt p, const CVec4Dfp32& _Color);
	virtual void SetRAWPixel(const CClipRect& cr, CPnt p, uint32 _Color);

	virtual CPixel32 GetPixelUV_Nearest(const CVec2Dfp32& _UV);		// Get pixel value with nearest neighbour sampling
	virtual CPixel32 GetPixelUV_Bilinear(const CVec2Dfp32& _UV, int _EdgeClampFlags = 0);		// Get pixel value with bilinear filtering sampling, UV = (0.5/w, 0.5/h) is a "pure" (unfiltered) sample of the first pixel in the image. Filtering wraps.
	virtual CPixel32 GetPixel(const CClipRect& cr, const CPnt& p);
	virtual CVec3Dfp32 GetPixel3f(const CClipRect& cr, CPnt p);
	virtual CVec4Dfp32 GetPixel4f(const CClipRect& cr, CPnt p);
	virtual uint32 GetRAWPixel(const CClipRect& cr, CPnt p);

	virtual void Fill(const CClipRect& cr, int32 color);
	virtual void FillZStencil(const CClipRect& cr, int32 color);
	virtual void Rect(const CClipRect& cr, CRct rect, int32 color);
	virtual void Line(const CClipRect& cr, CPnt p0, CPnt p1, int32 color);
	virtual void Frame(const CClipRect& cr, CRct rect, int32 color);
	virtual void Blt(const CClipRect& cr, CImage& src, int _flags, CPnt destp, int _EffectValue = 0);
	virtual void BltStretch(const CClipRect& cr, CImage& _Src, int _Flags, CVec2Dfp32 _Dest, CVec2Dfp32 _Scale, int _EffectValue = 0, CRct* _pRetDestRect = NULL);

	// Flip Functions
	virtual void FlipV();
	virtual void FlipH();

	// Single-colour triangle. sub-pixel and clipping.
	virtual void Triangle(const CClipRect& cr, const CVec3Dfp32& _p0,  const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, int32 color);

	// Textured & gouraud-shaded with perspective correction, sub-pixel, sub-texel and clipping. NOT FINISHED!
	virtual void Triangle(const CClipRect& cr, const CVec3Dfp32& _p0,  const CVec3Dfp32& _p1, const CVec3Dfp32& _p2,
	                      CPixel32 _col0, CPixel32 _col1, CPixel32 _col2, 
	                      const CVec2Dfp32 _tc0, const CVec2Dfp32 _tc1, const CVec2Dfp32 _tc2, CImage& Texture);

	virtual bool CompareColor(const CClipRect& cr, CRct rect, int32 color);
	virtual void Invert();

	virtual int DebugTextLen(const char* _pStr);
	virtual int DebugTextHeight();
	virtual void DebugText(const CClipRect cr, CPnt pos, const char* _pStr, int32 color);

	virtual void DrawPalette(const CClipRect cr, CPnt pos);

	static CPixel32 ConvFromRGBA4444(uint16 _c);
	static CPixel32 ConvFromRGB565(uint16 _c);
	static CPixel32 ConvFromRGBA5551(uint16 _c);

	static uint16 ConvToRGBA4444(CPixel32 _c);
	static uint16 ConvToRGB565(CPixel32 _c);
	static uint16 ConvToRGBA5551(CPixel32 _c);

	/*	Gamecube swizzling & texturestuff, at this point this will not work on the actual gc, but there is no reason for that anyhow.. SS.	*/	
#ifndef	PLATFORM_CONSOLE
	void	SwizzleGC( bool bCompressed, bool bClearLodForDetailTexture );
	void	SwizzlePS2( void *pGSMem );
	void	clearLodForDetailTexture( void );
	bool	RepackS3TC_Dolphin( CImage *pImage );
	void	RepackCompressed_Dolphin( CImage *pImage );
	void	Swizzle_Dolphin();
	void	mergeDXT3_Dolphin( CImage *pAlpha );
	void	AssureImageSize(int _MinWidth = 4, int _MinHeight = 4);
	void	DoubleWidth();
	void	DoubleHeight();
	void	swizzleS3Dxt1();
	void	ConvertAlphaToColor();
#endif

	static void Swizzle(void *_pDest, int _DestPitch, void *_pSrc, int _Width, int _Height, int _PixelSize);
	void        Swizzle( CImage *_pDestImg );
	spCImage    Swizzle();

	void        Unswizzle( CImage *_pDestImg );
	spCImage    Unswizzle();
	

	void ClutVQ( unsigned char *clut ); // convert truecolor code book into palette code book
	void ClutVQ( CTexture *_pTexture );						// generate a clut and convert truecolor code book into palette code book

};

#ifndef PLATFORM_CONSOLE
// -------------------------------------------------------------------
//  COctreeQuantize
// -------------------------------------------------------------------
//
// CLASS AUTHOR:	Daniel Hansson
// STARTED	   :	961214
// LAST UPDATED:	970218
// SUPPORTS	   :	Octree quantization of one or more 8/24-bit images
//					downto 8-bit images that share one palette. Dithering
//					is optional.
//
// COMMENTS	   :	Quantization based on the algorithm by Gervautz and Purgathofer.
//					Dithering based on the Floyd-Steinberg error diffusion algorithm.
//
#define CONST_OCTQUANT_DESTBPP 8

enum eDither {DITHERING,NO_DITHERING};

class OctNode
{
	uint8 Level;
	uint8 NrChildren;
	uint32 NrColors;

	bool Leaf;
	bool Reduceable;

	uint32 RSum,GSum,BSum;

	OctNode* Children[8];

	uint8 PaletteIndex;

public:

	OctNode(uint8 Level, bool Leaf)
	{
		MAUTOSTRIP(OctNode_ctor, MAUTOSTRIP_VOID);

		OctNode::Level=Level;
		OctNode::Leaf=Leaf;

		NrChildren=0;
		NrColors=0;
		Reduceable=FALSE;

		RSum=GSum=BSum=0;

		int i;
		for (i=0; i<8; i++)
			Children[i]=NULL;
	}

	~OctNode()
	{
		MAUTOSTRIP(OctNode_dtor, MAUTOSTRIP_VOID);

		if (NrChildren>0) {
			int i;
			for (i=0; i<8; i++)
				if (Children[i]!=NULL) {
					delete Children[i];
					Children[i]=NULL;
				}
		}
	}	

	OctNode* GetChild(int i) { return Children[i]; };
	void SetChild(OctNode *Child, int i)
	{ 
		MAUTOSTRIP(OctNode_SetChild, MAUTOSTRIP_VOID);

		if (Child!=NULL && Children[i]!=NULL)
			Error_static("OctNode::SetChild","Octree child already exists.");
		Children[i]=Child;
	};

	uint8 GetLevel() { return Level; };

	bool IsLeaf() { return Leaf; };
	void SetLeaf(bool Leaf) { OctNode::Leaf=Leaf; };

	void IncChildren() { NrChildren++; };
	void DecChildren() { NrChildren--; };
	uint8 GetNrChildren() { return NrChildren; };

	bool GetReduceable() { return Reduceable; };
	void SetReduceable(bool Reduceable) { OctNode::Reduceable=Reduceable; };

	void AddColor(uint8 r,uint8 g,uint8 b) { RSum+=r; GSum+=g; BSum+=b; NrColors++; };
	uint32 GetNrColors() { return NrColors; };

	void GetAverageRGB(uint8& r, uint8& g, uint8& b)
	{
		MAUTOSTRIP(OctNode_GetAverageRGB, MAUTOSTRIP_VOID);

#ifndef	CPU_SOFTWARE_FP64
		r=(uint8)((fp64)RSum/(fp64)NrColors+0.5);
		g=(uint8)((fp64)GSum/(fp64)NrColors+0.5);
		b=(uint8)((fp64)BSum/(fp64)NrColors+0.5);
#else
		r=(uint8)((fp32)RSum/(fp32)NrColors+0.5f);
		g=(uint8)((fp32)GSum/(fp32)NrColors+0.5f);
		b=(uint8)((fp32)BSum/(fp32)NrColors+0.5f);
#endif
	}

	int FindChild(uint8 r, uint8 g, uint8 b)
	{
		MAUTOSTRIP(OctNode_FindChild, 0);

		uint8 bits=(CONST_OCTQUANT_DESTBPP-1)-Level;

		return ((b >> bits) & 1) | ( (g >> (bits-1)) & 2) | ( (r >> (bits-2)) & 4);
	}

	uint8 GetPaletteIndex() { return PaletteIndex; };
	void SetPaletteIndex(uint8 Index) { PaletteIndex=Index; };
};


class SYSTEMDLLEXPORT COctreeQuantize : public CReferenceCount 
{

	OctNode* Tree;
	uint8 LeafLevel;		// Reduction level is LeafLevel-1
	int NrLeafs;

	bool AllColorsInTree;

	CPixel32 Palette[1 << CONST_OCTQUANT_DESTBPP];
	uint8 Index;

	TList_Linked<OctNode*> Reduceables[CONST_OCTQUANT_DESTBPP];

	uint8 LastIndex;
	int LastR,LastG,LastB;

	spCImagePalette mspPal;

	OctNode* InsertColor(OctNode* Node, uint8 r, uint8 g, uint8 b);
	void DeleteSubTree(OctNode* Node);
	void ReduceTree();

	void CreatePalette(OctNode* Node);

	// MapColor can only map colors that has an exact match in the tree.
	uint8 MapColor(OctNode* Node,uint8 r,uint8 g,uint8 b);

	// MapColor2 can map all colors, even those that hasn't got an exact
	// match in the tree. If can't find an exact match it will
	// call FindBestMatch which will recursively find the best
	// match in the tree.
	uint8 MapColor2(OctNode* Node,uint8 r,uint8 g,uint8 b);
	uint8 FindBestMatch(OctNode* Node,uint8 r,uint8 g,uint8 b);
	
public:

	DECLARE_OPERATOR_NEW


	COctreeQuantize()
	{ 
		Tree=NULL; 
	};
	~COctreeQuantize() { if (Tree!=NULL) delete Tree; };

	void BuildFromPalette(CPixel32* Pal);
	void BuildFromPalette(spCImagePalette spPal);

	void Begin();
	void Include(CImage* _pImage);
	void End();
	spCImage Quantize(CImage* _pSrcImage,eDither Dither=NO_DITHERING);

	uint8 GetIndex(CPixel32 col);
	void GetIndices(CPixel32* colors, uint8* dest, int NrColors);
};

class SYSTEMDLLEXPORT CMinErrorQuantize : public CReferenceCount 
{

	void *m_pWorker;
	void Init();
public:
	CMinErrorQuantize()
	{
		m_pWorker = NULL;
	}

	~CMinErrorQuantize();

	void BuildFromPalette(CPixel32* Pal);
	void BuildFromPalette(spCImagePalette spPal);

	void Begin();
	void Include(CImage* _pImage, int _Priority = 1);
	void End();
	spCImage Quantize(CImage* _pSrcImage,eDither Dither=NO_DITHERING);

	uint8 GetIndex(CPixel32 col);
	void GetIndices(CPixel32* colors, uint8* dest, int NrColors);
};

#endif

#endif






