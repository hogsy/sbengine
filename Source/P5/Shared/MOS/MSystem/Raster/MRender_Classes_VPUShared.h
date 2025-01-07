
#ifndef _INC_MRender_Classes_VPUShared
#define _INC_MRender_Classes_VPUShared


#if defined( PLATFORM_XBOX1 ) || defined(PLATFORM_PS2)
#define	DEF_CRC_MAXTEXTURES	4
#define	DEF_CRC_MAXTEXCOORDS 4
#else
#define	DEF_CRC_MAXTEXTURES	8
#define	DEF_CRC_MAXTEXCOORDS 8
#endif

#ifndef PLATFORM_CONSOLE
#define CRC_SUPPORTVERTEXLIGHTING
#endif

enum
{
	CRC_MAXMRT					= 4,
	CRC_MAXTEXTUREENV			= 4,	// Maximum number of supported textures coordinates.
#if	(DEF_CRC_MAXTEXTURES==4)
	CRC_MAXTEXCOORDS			= 4,	// Maximum number of supported textures coordinates.
	CRC_MAXTEXTURES				= 4,	// Maximum number of supported textures channels. (Multitexture) Affects CRC_MATRIXSTACKS
#else
	CRC_MAXTEXCOORDS			= DEF_CRC_MAXTEXCOORDS,	// Maximum number of supported textures coordinates.
	CRC_MAXTEXTURES				= 16,	// Maximum number of supported textures channels. (Multitexture) Affects CRC_MATRIXSTACKS
#endif
	CRC_MAXLIGHTS				= 8,	// Maximum simultaneous lights.
	CRC_NUMTEXCOORDCOMP			= 4,	// U,V,W,?  ehum...   S,T,R,Q..

	CRC_ATTRIBSTACKDEPTH		= 48,
	CRC_MATRIXSTACKDEPTH		= 12,
	CRC_VIEWPORTSTACKDEPTH		= 4,
	CRC_MATRIXSTACKS			= 2 + CRC_MAXTEXCOORDS,

	CRC_MAXPOLYGONVERTICES		= 32,

	CRC_DEFAULT_FRONTPLANE		= 4,
	CRC_DEFAULT_BACKPLANE		= 2048,
	CRC_DEFAULT_FOV				= 90,

	CRC_MAXPICMIPS				= 16,
};

//----------------------------------------------------------------
// Instructions for RenderIndexedPrimitives()
//----------------------------------------------------------------
enum
{
	CRC_RIP_END				= 0,
	CRC_RIP_TRIANGLES		= 1,
	CRC_RIP_QUADS			= 2,
	CRC_RIP_TRISTRIP		= 3,
	CRC_RIP_TRIFAN			= 4,
	CRC_RIP_QUADSTRIP		= 5,
	CRC_RIP_POLYGON			= 6,
	CRC_RIP_PRIMLIST		= 7,		// A primitive list in a prim list
	CRC_RIP_TEXT			= 8,
	CRC_RIP_PARTICLES		= 9,
	CRC_RIP_TYPEMASK		= 0x0f,


	CRC_RIP_COPLANAR		= 0x40,		// All faces in primitive lay in the same plane.
	CRC_RIP_COPLANARLAST	= 0x80,		// All faces in primitive lay in last primitive's plane.
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CRC_VertexBuffer
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CRC_VertexBuffer
{
public:
	union {
		uint32 m_DummyOffset;
		struct {
			uint32 m_nV:16;
			uint32 m_nMWComp:16;		// Weight components per vertex, allowed range is [0..4] (works as m_nTVComp)
		};
	};
	CVec3Dfp32* m_pV;
	fp32* m_pTV[CRC_MAXTEXCOORDS];
	uint8  m_nTVComp[CRC_MAXTEXCOORDS];
	CVec3Dfp32* m_pN;
	CPixel32* m_pCol;

	CPixel32* m_pSpec;
	/*	fp32* m_pFog;
	*/

	uint32* m_pMI;		// Matrix index array, Points to an array of uint32[nV] (uint8[nV][4])  256 matrices _should_ be enough for a while, but I'll probably regrett this much sooner than I think.  /mh
	fp32* m_pMW;			// Matrix weight array, Points to an array of fp32[nV][m_nMatricesPerVertex];

	// Disable the color because it does not belong here
	//	CPixel32 m_Color;	// Constant color, ignored if m_pCol != NULL;

	uint16* m_piPrim;
	union {
		uint32 m_PrimData;
		struct {
			uint16 m_nPrim;
			int16 m_PrimType;
		};
	};

	void Clear()
	{
		m_nV = 0;
		m_pV = NULL;
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++) { m_pTV[i] = NULL; m_nTVComp[i] = 0; }
		m_pN = NULL;
		m_pCol = NULL;
		m_pSpec = NULL;
		//		m_pFog = NULL;
		m_pMI = NULL;
		m_pMW = NULL;
		m_nMWComp = 0;

		//		m_Color = 0xffffffff;

		m_piPrim = NULL;
		m_nPrim = NULL;
		m_PrimType = NULL;
	}

	CRC_VertexBuffer()
	{
		Clear();
	}
};


#endif // _INC_MRender_Classes
