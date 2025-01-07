#ifndef __INC_MRNDRGL_DEF
#define __INC_MRNDRGL_DEF

// #define CRCGL_EXTENSIVE_PALETTE_CHECK
//#define CRCGL_ALPHALIGHTMAPS

#define ALLWAYS

#define SHARED_TEXTURE_PALETTE_EXT		0x81FB

// #define CRCGL_AUTOPRECACHE

//#define CRCGL_CULLFACES				// Manual face-cull before gl-calls.
#define CRCGL_CULLFACESENABLE		// Let GL cull faces.

// #define CRCGL_SHAREDPALETTE		// Enable support for shared-texture palette (for 8-bit grayscale textures)

#define CRCGL_COUNTSTATES
#define CRCGL_MAXTEXTURESIZE	2048

#ifdef CRCGL_COUNTSTATES
#define MACRO_GLCOUNTSTATE(Name) Name++;
#else
#define MACRO_GLCOUNTSTATE
#endif

#define CRCGL_POLYGONENUM GL_TRIANGLE_FAN
//#define CRCGL_POLYGONENUM GL_POLYGON

#define GLTexLog(s) 
//#define GLTexLog(s) LogFile(s)

#define CRCGL_GENTEXTURES
// #define CRCGL_COUNTTEXTURES

#define CRCGL_VERTEXCONSISTENCY

#define CRCGL_DEPTHFOGENABLE

//#define CRCGL_POLYGONOFFSETPROJECTIONMATRIX
// #define CRCGL_PRIM_STRIPCOUNT


// -------------------------------------------------------------------
//  OpenGL Help Macros
// -------------------------------------------------------------------
//#define MACRO_GLCOLOR_PIXEL32(col) glnColor3ub(col.GetR(), col.GetG(), col.GetB())
//#define MACRO_GLCOLOR_INT(color) glnColor3ub((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff)
//#define MACRO_GLCOLOR_PIXEL32(col) glnColor4f((fp4)col.GetR()/255.0, (fp4)col.GetG()/255.0, (fp4)col.GetB()/255.0)
//#define MACRO_GLCOLOR_PIXEL32(col) glnColor4f((fp4)col.GetR()/255.0f, (fp4)col.GetG()/255.0f, (fp4)col.GetB()/255.0f, (fp4((uint32)col >> 24))/255.0f)

/*#define MACRO_GLCOLOR4F_INT32(_Col)				\
	glnColor4ub(									\
		(_Col >> 16) & 0xff,					\
		(_Col >> 8) & 0xff,						\
		(_Col & 0xff),							\
		(_Col >> 24) & 0xff)*/

#define MACRO_GLCOLOR4F_INT32(_Col)				\
	glnColor4f(									\
		(1.0f/255.0f)*fp4((_Col >> 16) & 0xff),	\
		(1.0f/255.0f)*fp4((_Col >> 8) & 0xff),	\
		(1.0f/255.0f)*fp4(_Col & 0xff),			\
		(1.0f/255.0f)*fp4((_Col >> 24) & 0xff))

#define MACRO_GLSECONDARYCOLOR3F_INT32(_Col)		\
	gleSecondaryColor3f(						\
		(1.0f/255.0f)*fp4((_Col >> 16) & 0xff),	\
		(1.0f/255.0f)*fp4((_Col >> 8) & 0xff),	\
		(1.0f/255.0f)*fp4(_Col & 0xff))


#define CRC_RIP_STREAM		-1
#define CRC_RIP_WIRES		-2

#endif // __INC_MRNDRGL_DEF
