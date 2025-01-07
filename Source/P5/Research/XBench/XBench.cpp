#include "PCH.h"

#include "MRTC.h"
#include "MRTC_CallGraph.h"
#include "MFloat.h"

/*#ifndef PLATFORM_XBOX
#include <windows.h>
#endif*/

#include "..\MOS\MOS.h"
#include "..\MOS\Msystem\Raster\MRCCore.h"
#include "..\MOS\Classes\Win\MWinGrph.h"
//#include "..\MOS\Classes\Win\MWindows.h"
//#include "..\MOS\Classes\Win\MWinCtrl3D.h"

//#include "..\MOS\XRModels\Model_TriMesh\WTriMesh.h"

//#include "..\MOS\MSystem\Raster\MTextureContainerXTC2.h"

#include "..\MOS\MMain.h"

#include "..\MOS\XR\XRVBContext.h"


#ifdef PLATFORM_DOLPHIN
	#define M_FLUSHARRAY(lData) DCStoreRange(lData.GetBasePtr(), lData.ListSize())
#else
	#define M_FLUSHARRAY(lData) void(0)
#endif

// ----------------------------------------------------------------
/*

65536 triangle toruses:



								Col				Txt				Txt+Col			DualTxt			DualTxt+Col
// --------------------------------------------------------------------------------------------------------------
GameCube:
								xxx				25.5			19.4				15.6				14.2

								Col				Txt				Txt+Col			DualTxt			DualTxt+Col
// --------------------------------------------------------------------------------------------------------------
OpenGL, PentiumIII-500:

Voodoo3-3000, glVertex3fv		33.6 (2.20M)	26.9 (1.76M)	26.9 (1.76M)	22.35 (1.46M)	22.15 (1.45M)
Voodoo3-3000, CVA				18.8			15.15			15.1			13.35			13.35
Voodoo3, testy20047				11.2
Voodoo3, sbzdm2					11.9

RivaTNT2 v1.28, glVertex3fv		28.7			33.6			24.5			27.0			20.9
RivaTNT2 v1.28, CVA				15.2			11.95			11.35			10.75			10.6
RivaTNT2 v1.28, testy20047		24.6
RivaTNT2 v1.28, sbzdm2			48.5

RivaTNT2 v1.91, glVertex3fv		41.8 (2.74M)	34.9 (2.29M)	35.0 (2.29M)	27.2 (1.78M)	27.3 (1.79M)
RivaTNT2 v1.91, CVA				49.0 (3.21M)	41.9 (2.75M)	41.8 (2.74M)	38.1 (2.50M)	38.2 (2.50M)
RivaTNT2 v1.91, testy20047		26.6, CVA 25.4
RivaTNT2 v1.91, sbzdm2			51.4, CVA 49.6

GeForce256 v3.53, CVA															48.1 (3.1M)

GeForce256 v5.22, CVA															46 ? ()

// --------------------------------------------------------------------------------------------------------------
Glide3x, Pentium2-450
Voodoo3-3000,									30.1 (1.97M)	33.3 (2.18M)	30.6 (2.01M)	25.0 (1.64M)

// --------------------------------------------------------------------------------------------------------------
Glide3x, PentiumPro-233
Voodoo2											15.8 (1.04M)	15.9 (1.04M)	15.5 (1.02M)	13.2 (0.87M)


		  
// --------------------------------------------------------------------------------------------------------------
Voodoo3:
    GL_ARB_MULTITEXTURE
    GL_EXT_ABGR
    GL_EXT_BGRA
    GL_EXT_CLIP_VOLUME_HINT
    GL_EXT_COMPILED_VERTEX_ARRAY
    GL_EXT_PACKED_PIXELS
    GL_EXT_PALETTED_TEXTURE
    GL_EXT_SHARED_TEXTURE_PALETTE
    GL_EXT_VERTEX_ARRAY
    GL_EXT_POINT_PARAMETERS
    GL_SGIS_MULTITEXTURE
    WGL_EXT_SWAP_CONTROL


Tests:
	Throughput:
							Vertex3fv	ArrayElem	DrawElem
		TriStrip
		Triangles

	RenderState penalty:
		Triangles			Texture, Blend Enable, BlendMode, AlphaComp, Fog Enable, Fog Color

	Fillrate:
							32x32		64x64		256x256		512x512		1024x1024
		Opaque
		AlphaBlend
		Opaque + Z
		AlphaBlend + Z

	Texture upload:
							32x32		64x64		256x256
		Async
		Stalled

*/
// ----------------------------------------------------------------
// CMWnd_Desktop_XModel
// ----------------------------------------------------------------
class CXBenchApp;


#define WMSG_TEST_GLOBALDATAPTR		0x8001

// ----------------------------------------------------------------
// CXBenchApp
// ----------------------------------------------------------------
class CXB_Torus;
typedef TPtr<CXB_Torus> spCXB_Torus;

#define NUMFPSHIST 80

/*
*/
class CXB_Benchmark : public CReferenceCount
{
public:
	CStr m_Name;
	int m_ModelType;
	int m_nModels;
	CVec3Dint32 m_Tess;
	CVec3Dfp4 m_Size;
	int m_bCompColors : 1;
	int m_bCompNormals : 1;
	int m_bCompTV0 : 1;
	int m_bCompTV1 : 1;
	int m_bLighting : 1;
	int m_bUniqueVerts : 1;
	int m_bStaticIndices : 1;
	int m_bStaticVertices : 1;
	int m_PrimType;
	int m_StripLen;
	int m_nLights;
	fp4 m_AttrInc;
	int m_nAttr;

	void Create(CRegistry* _pReg)
	{
		m_Name = _pReg->GetThisValue();

		m_ModelType				= _pReg->GetValuei("MODELTYPE", 0);
		m_nModels					= _pReg->GetValuei("MODELS", 1);
		m_Tess.ParseString(_pReg->GetValue("TESSELLATE", "1,1,1"));
		m_Size.ParseString(_pReg->GetValue("SIZE", "256, 256, 256"));

		m_bCompColors			= _pReg->GetValuei("COLORS", 1);
		m_bCompNormals		= _pReg->GetValuei("NORMALS", 0);
		m_bCompTV0				= _pReg->GetValuei("TEXTURE0", 1);
		m_bCompTV1				= _pReg->GetValuei("TEXTURE1", 0);

		m_bLighting				= _pReg->GetValuei("LIGHTING", 0);
		m_bUniqueVerts		= _pReg->GetValuei("UNIQUEVERTICES", 0);
		m_bStaticIndices	= _pReg->GetValuei("STATICINDICES", 0);
		m_bStaticVertices	= _pReg->GetValuei("STATICVERTICES", 0);
		m_PrimType				= _pReg->GetValuei("PRIMTYPE", 0);
		m_StripLen				= _pReg->GetValuei("STRIPLEN", 0);
		m_nLights					= _pReg->GetValuei("LIGHTS", 1);
		m_AttrInc					= _pReg->GetValuef("ATTRIBUTEINC", 0);
		if(m_AttrInc)
			m_nAttr						= _pReg->GetValuei("ATTRIBUTES", 1) * m_nModels;
		else
			m_nAttr						= _pReg->GetValuei("ATTRIBUTES", 1);
	}
};

/*
*/
class CXB_Setup : public CReferenceCount
{
public:
	typedef TPtr<CXB_Benchmark> spCXB_Benchmark;

	TArray<spCXB_Benchmark> m_lspBenchmarks;


	void Create(CRegistry* _pReg)
	{
		for(int i = 0; i< _pReg->GetNumChildren(); i++)
		{
			spCXB_Benchmark spBM = MNew(CXB_Benchmark);
			if (!spBM) MemError("Create");
			spBM->Create(_pReg->GetChild(i));
			m_lspBenchmarks.Add(spBM);
		}
	}

	void Create(const char* _pFileName)
	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys) Error("Create", "No system.");

		CStr Path = pSys->m_ExePath + _pFileName;

		CRegistry_Dynamic Reg;

		Reg.XRG_Read(Path);

		Create(&Reg);
	};
};

/*
*/
class	CXB_Quad	:	public	CReferenceCount, public CXR_VBContainer
{
	public:

	TArray<CVec3Dfp4> m_lVertices;
	TArray<CVec2Dfp4> m_lTVertices;
	TArray<uint16>		m_lTrianglePrim;

	CVec3Dfp4	m_Rot;
	CVec3Dfp4	m_Rot2;
	fp4				m_Time;

	int m_VBID;

	CRC_Attributes	m_attrib;

					CXB_Quad()
					{
						m_VBID = 0;
					}

					~CXB_Quad()
					{
						if (m_VBID && m_pVBCtx)
							m_pVBCtx->FreeID(m_VBID);
					}

					void	Create( CTextureContainer *_pTC )
					{
						m_VBID = m_pVBCtx->AllocID( m_iVBC, 0 );
						if( !m_VBID )	Error("Create", "Unable to allocate VBID.");

						m_attrib.SetDefault();
						m_attrib.Attrib_Enable( CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE );
						m_attrib.Attrib_Disable( CRC_FLAGS_CULL );
//						m_attrib.Attrib_TexEnvColor( 0, CPixel32( 255, 0, 0 ) );
						m_attrib.Attrib_TextureID( 0, 1 );

						m_attrib.Attrib_TextureID( 1, 82 );
						m_attrib.Attrib_TexCoordSet( 1, 0 );
						m_attrib.Attrib_TexEnvMode( 1, CRC_TEXENVMODE_REPLACE ); 

						m_attrib.Attrib_TextureID( 2, 11 );
						m_attrib.Attrib_TexCoordSet( 2, 0 );
						m_attrib.Attrib_TexEnvMode( 2, CRC_TEXENVMODE_BLEND ); 


						m_lVertices.SetLen( 4 );
						m_lTVertices.SetLen( 4 );
						m_lTrianglePrim.SetLen( 6 );

						fp4 s = 400.0f;

						m_lVertices[0][0] = -s;	m_lVertices[0][1] = -s;	m_lVertices[0][2] = 0;
						m_lVertices[1][0] = s;	m_lVertices[1][1] = -s;	m_lVertices[1][2] = 0;
						m_lVertices[2][0] = s;	m_lVertices[2][1] = s;	m_lVertices[2][2] = 0;
						m_lVertices[3][0] = -s;	m_lVertices[3][1] = s;	m_lVertices[3][2] = 0;

						m_lTVertices[0][0] = 0;	m_lTVertices[0][1] = 0;
						m_lTVertices[1][0] = 1;	m_lTVertices[1][1] = 0;
						m_lTVertices[2][0] = 1;	m_lTVertices[2][1] = 1;
						m_lTVertices[3][0] = 0;	m_lTVertices[3][1] = 1;

						int iv0 = 0;
						int iv1 = 1;
						int iv2 = 2;
						int iv3 = 3;

						m_lTrianglePrim.Add(iv0);
						m_lTrianglePrim.Add(iv1);
						m_lTrianglePrim.Add(iv2);

						m_lTrianglePrim.Add(iv0);
						m_lTrianglePrim.Add(iv2);
						m_lTrianglePrim.Add(iv3);


						M_FLUSHARRAY( m_lVertices );
						M_FLUSHARRAY( m_lTVertices );
						M_FLUSHARRAY( m_lTrianglePrim );

						m_Rot = CVec3Dfp4(0);
						m_Rot2 = CVec3Dfp4(0);
						m_Time = 0;
					}

					void	Render( CXR_VBManager* _pVBM )
					{
						CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
						CXR_VBChain* pChain = pVB->GetVBChain();
						pVB->m_pAttrib = &m_attrib;
//						_pRC->Attrib_Push();
//						_pRC->Attrib_Set( m_attrib );

//						_pRC->Matrix_Push();
//						_pRC->Geometry_Clear();

						//_pRC->Geometry_VertexBuffer( m_VBID );
//						_pRC->Geometry_VertexArray( m_lVertices.GetBasePtr(), m_lVertices.Len() );
//						_pRC->Geometry_TVertexArray( m_lTVertices.GetBasePtr(), 0 );
						pChain->m_pV = m_lVertices.GetBasePtr();
						pChain->m_nV = m_lVertices.Len();
						pChain->m_pTV[0] = (fp4*)m_lTVertices.GetBasePtr();

						m_Time += 0.0005f;

						//m_Rot		+= CVec3Dfp4( 0.002f, -0.003f, 0.004f ) * t;
						//m_Rot2	+= CVec3Dfp4( 0.003f, 0.002f, -0.003f ) * t;

						CVec3Dfp4 Rot = m_Rot;
						CVec3Dfp4 Rot2 = m_Rot2;

						CMat4Dfp4 Mat;
						CMat4Dfp4 Mat2;

						Rot.CreateMatrixFromAngles(3, Mat);
						CVec3Dfp4(0, 0, 1024.0f + sinf(m_Time) * 900.0f ).SetMatrixRow(Mat, 3);
						Rot2.CreateMatrixFromAngles(3, Mat2);
						CVec3Dfp4 v(CVec3Dfp4::GetMatrixRow(Mat2, 2));
						v *= 266.0f * 1.1;
						v = CVec3Dfp4::GetMatrixRow(Mat2, 1);
						v *= 180.0f * 1.1;
						v = CVec3Dfp4::GetMatrixRow(Mat2, 0);
						v *= 200.0f * 1.1;

//						_pRC->Matrix_Set( Mat );
						pVB->m_pTransform = _pVBM->Alloc_M4(Mat);

//						_pRC->Render_IndexedTriangles( m_lTrianglePrim.GetBasePtr(), m_lTrianglePrim.Len() / 3 );
						pVB->Render_IndexedTriangles( m_lTrianglePrim.GetBasePtr(), m_lTrianglePrim.Len() / 3 );

//						_pRC->Matrix_Pop();
//						_pRC->Attrib_Pop();
					}
					
					virtual int GetNumLocal()					{	return(1);	};
					virtual int GetID( int _iLocal )	{	return( m_VBID );	};

					virtual void Get( int _iLocal, CRC_BuildVertexBuffer &_VB, int _Flags )
					{
						_VB.m_nV		= m_lVertices.Len();
						_VB.Geometry_VertexArray(m_lVertices.GetBasePtr());
						_VB.Geometry_TVertexArray(m_lTVertices.GetBasePtr(), 0);

						_VB.m_PrimType	= CRC_RIP_TRIANGLES;
						_VB.m_piPrim		= m_lTrianglePrim.GetBasePtr();
						_VB.m_nPrim			= m_lTrianglePrim.Len() / 3;
					}
};
typedef TPtr<CXB_Quad> spCXB_Quad;

// ----------------------------------------------------------------
// 
// ----------------------------------------------------------------
#define CXB_NUMTEXTURES	8

class CXB_Torus : public CReferenceCount, public CXR_VBContainer
{
public:
	int m_VBID;

	TArray<CVec3Dfp4> m_lVertices;
	TArray<CVec2Dfp4> m_lTVertices1;
	TArray<CVec2Dfp4> m_lTVertices2;
	TArray<CPixel32>	m_lColors;
	TArray<CVec3Dfp4> m_lNormals;

	TArray<uint16> m_lStripPrim;
	TArray<uint16> m_lTrianglePrim;

	TArray<CRC_Attributes>	m_lAttributes;

	int m_TextureID[CXB_NUMTEXTURES];

	CVec3Dfp4 m_Rot;
	CVec3Dfp4 m_Rot2;
	int m_xSeg;
	int m_ySeg;

	CXB_Torus()
	{
		m_VBID = 0;
		m_Rot = 0;
		m_Rot2 = 0;
		FillChar(&m_TextureID, sizeof(m_TextureID), 0);
	}

	~CXB_Torus()
	{
		if (m_VBID && m_pVBCtx)
			m_pVBCtx->FreeID(m_VBID);
	}


	void GetTorusCrd(fp4 rx, fp4 ry, int x, int y, int xSeg, int ySeg, CVec3Dfp4* pV)
	{
		fp4 v = (fp4)x/(fp4)xSeg*2*3.141596;
		fp4 w = (fp4)y/(fp4)ySeg*2*3.141596;

		pV->k[1] = M_Cos(w)*ry;
		fp4 r = rx-M_Sin(w)*ry;
		pV->k[0] = M_Cos(v)*r;
		pV->k[2] = M_Sin(v)*r;
	};

	void GetTorusNormal(fp4 rx, fp4 ry, int x, int y, int xSeg, int ySeg, CVec3Dfp4* pN)
	{
		fp4 v = (fp4)x/(fp4)xSeg*2*3.141596;
		fp4 w = (fp4)y/(fp4)ySeg*2*3.141596;

		pN->k[1] = M_Cos(w);
		pN->k[0] = -M_Cos(v)*M_Sin(w);
		pN->k[2] = -M_Sin(v)*M_Sin(w);
	};


	void	Create( const CXB_Benchmark &_BM, CTextureContainer *_pTC )
	{
		int i;

		m_VBID = m_pVBCtx->AllocID(m_iVBC, 0);
		if (!m_VBID) Error("Create", "Unable to allocate VBID.");

		int nTextures = _pTC->GetNumLocal();

		for(i = 0; i < CXB_NUMTEXTURES; i++)
		{
			m_TextureID[i] = _pTC->GetTextureID(i % nTextures);
			//OSReport("Texture ID (%d) = %d\n", i, m_TextureID[i]);
		}


		m_xSeg = _BM.m_Tess[0];
		m_ySeg = _BM.m_Tess[1];
		fp4 xRadius = _BM.m_Size[0];//470
		fp4 yRadius = _BM.m_Size[1]*2;//256

		// Init arrays
		int nVert = m_ySeg*m_xSeg;
		m_lVertices.SetLen(nVert);
		m_lTVertices1.SetLen(nVert);
		m_lTVertices2.SetLen(nVert);
		m_lColors.SetLen(nVert);
		m_lNormals.SetLen(nVert);

		m_lAttributes.SetLen(_BM.m_nAttr);
		for(i = 0; i < _BM.m_nAttr; i++)
		{
			CRC_Attributes &Attr = m_lAttributes[i];
			Attr.SetDefault();
			Attr.Attrib_Enable( CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE );
			Attr.Attrib_Enable( CRC_FLAGS_CULL );
			if( _BM.m_bCompTV0 )	Attr.Attrib_TextureID( 0, m_TextureID[i % CXB_NUMTEXTURES] );
			if( _BM.m_bCompTV1 )
			{
				Attr.Attrib_TextureID( 1, 82 );
				Attr.Attrib_TexEnvMode( 1, CRC_TEXENVMODE_BLEND ); 
			}

			Attr.Attrib_RasterMode( i % (CRC_RASTERMODE_ONE_INVALPHA+1) );
		}

		/*	Build torus geometry, normals, texturecoords & colors.	*/
		for( int y=0; y < m_ySeg; y++ )
		{
			for( int x=0; x < m_xSeg; x++ )
			{
				int iv = x + y*m_xSeg;

				GetTorusCrd(xRadius, yRadius, x, y, m_xSeg, m_ySeg, &m_lVertices[iv]);
				GetTorusNormal(xRadius, yRadius, x, y, m_xSeg, m_ySeg, &m_lNormals[iv]);

				fp4 u = fp4(x) / fp4(m_xSeg);
				fp4 v = fp4(y) / fp4(m_ySeg);

				/*	Texturecoordinates.	*/	
				m_lTVertices1[iv][0] = u;
				m_lTVertices1[iv][1] = v;
				m_lTVertices2[iv][0] = u * 2;
				m_lTVertices2[iv][1] = v * 2;

				/*	Some fucked up colors, yet enough to show variation across the surface of the torus.	*/	
				fp4 s = 0.9f + fp4((x ^ y) & 1) / 2.0f;

				uint8	r = (uint8)((0.5f + m_lNormals[iv][0] * 0.5f ) * 255);
				uint8	g = (uint8)((0.5f + m_lNormals[iv][1] * 0.5f ) * 255);
				uint8	b = (uint8)((0.5f + m_lNormals[iv][2] * 0.5f ) * 255);

				m_lColors[iv] = CPixel32( r, g, b, 255 );/*128.0f*s*M_Sin(fp4(x)/fp4(m_xSeg)*2*3.15)/1.0 + 0.5, 
																	128.0f*s*M_Sin(fp4(y)/fp4(m_ySeg)*2*3.15)/1.0 + 0.5,
																	128.0f*s*cos(fp4(x)/fp4(m_xSeg)*2*3.15)/1.0 + 0.5, 128 );*/
			}
		}

/*		{
			int nStripPrim = _ySeg*((_xSeg+1)*2+2) + 1;
			m_lStripPrim.SetLen(nStripPrim);

			// Build strip primitive-batch
			uint16* pPrim = m_lStripPrim.GetBasePtr();
			int iP = 0;
			{
				for(int s = 0; s < _ySeg; s++)
				{
					if (iP > nStripPrim-(_xSeg+1)*2-2) Error("Create", "Misscalculated primitive space.");
					pPrim[iP++] = CRC_RIP_TRISTRIP + (((_xSeg+1)*2+2) << 8);
					pPrim[iP++] = (_xSeg+1)*2;
					for(int t = 0; t < _xSeg+1; t++)
					{
						pPrim[iP++] = ((s+1) % _ySeg)*_xSeg + (t % _xSeg);
						pPrim[iP++] = s*_xSeg + (t % _xSeg);

						if (pPrim[iP-2] >= m_lVertices.Len()) Error("Create", "Vertex out of range. (1)");
						if (pPrim[iP-1] >= m_lVertices.Len()) Error("Create", "Vertex out of range. (2)");
					}
				}
				if (iP > nStripPrim) Error("Create", "Misscalculated primitive space. (2)");
				pPrim[iP++] = CRC_RIP_END + (1 << 8);
			}

			if (iP != nStripPrim)
				Error("Create", "Misscalculated primitive space. (3)");
		}*/

		{
			m_lStripPrim.SetGrow(4096);
			m_lTrianglePrim.SetGrow(4096);

			// Build strip primitive-batch
//			m_lStripPrim.GetBasePtr();
			{
				const int StripLen = (_BM.m_StripLen > 0) ? _BM.m_StripLen : m_xSeg;
				for(int b = 0; b < m_xSeg/StripLen; b++)
				{
					for(int s = 0; s < m_ySeg; s++)
					{
						if( _BM.m_bUniqueVerts && (s & 1) )	continue;

//						if (iP > nStripPrim-(_xSeg+1)*2-2) Error("Create", "Misscalculated primitive space.");
						if( !_BM.m_bUniqueVerts && _BM.m_PrimType == 1 )
						{
							m_lStripPrim.Add(CRC_RIP_TRISTRIP + (((StripLen+1)*2+2) << 8));
							m_lStripPrim.Add((StripLen+1)*2);
						}

						int iv0 = 0, iv1 = 0;
						for(int t = 0; t < StripLen+1; t++)
						{
							int iv2 = ((s+1) % m_ySeg)*m_xSeg + ((t+b*StripLen) % m_xSeg);
							int iv3 = s*m_xSeg + ((t+b*StripLen) % m_xSeg);

							if( _BM.m_bUniqueVerts )
							{
								if( t&1 )
								{
									if( t>0 )
									{
										if( _BM.m_PrimType == 1 )
										{
											m_lStripPrim.Add(CRC_RIP_TRISTRIP + ((4+2) << 8));
											m_lStripPrim.Add(4);
											m_lStripPrim.Add( iv2 );
											m_lStripPrim.Add( iv3 );
											m_lStripPrim.Add( iv0 );
											m_lStripPrim.Add( iv1 );
										}
										else
										{
											m_lTrianglePrim.Add(iv0);
											m_lTrianglePrim.Add(iv1);
											m_lTrianglePrim.Add(iv3);

											m_lTrianglePrim.Add(iv0);
											m_lTrianglePrim.Add(iv3);
											m_lTrianglePrim.Add(iv2);
										}
									}
								}
							}
							else
							{
								if( _BM.m_PrimType == 1 )
								{
									// Strip
									m_lStripPrim.Add( iv2 );
									m_lStripPrim.Add( iv3 );
								}
								else
								{
									// Triangles
									if( t>0 )
									{
										m_lTrianglePrim.Add(iv0);
										m_lTrianglePrim.Add(iv1);
										m_lTrianglePrim.Add(iv3);

										m_lTrianglePrim.Add(iv0);
										m_lTrianglePrim.Add(iv3);
										m_lTrianglePrim.Add(iv2);
									}
								}
							}

							iv0 = iv2;
							iv1 = iv3;

//							if (pPrim[iP-2] >= m_lVertices.Len()) Error("Create", "Vertex out of range. (1)");
//							if (pPrim[iP-1] >= m_lVertices.Len()) Error("Create", "Vertex out of range. (2)");
						}
					}
				}

//				if (iP > nStripPrim) Error("Create", "Misscalculated primitive space. (2)");
				if( _BM.m_PrimType == 1 )
					m_lStripPrim.Add( CRC_RIP_END + (1 << 8) );
			}

//			if (iP != nStripPrim)
//				Error("Create", "Misscalculated primitive space. (3)");
		}


		{
			// Build triangle primitive-batch
/*			int nTriPrim = _ySeg*_xSeg*3*2;
			m_lTrianglePrim.SetLen(nTriPrim);
			uint16* pPrim = m_lTrianglePrim.GetBasePtr();
			int iP = 0;

			{
				for (int y=0; y < _ySeg; y++)
				{
					for (int x=0; x < _xSeg; x++)
					{
						int iv = y*_xSeg*6 + x*6;
						int iv0 = y*_xSeg+x;
						int iv1 = ((y+1) % _ySeg)*_xSeg+x;
						int iv2 = y*_xSeg+((x+1) % _xSeg);
						int iv3 = ((y+1) % _ySeg)*_xSeg + ((x+1) % _xSeg);
						pPrim[iP++] = iv0;
						pPrim[iP++] = iv1;
						pPrim[iP++] = iv2;
						pPrim[iP++] = iv2;
						pPrim[iP++] = iv1;
						pPrim[iP++] = iv3;
					}
				}
			}

			if (iP != nTriPrim)
				Error("Create", "Misscalculated primitive space. (4)");*/
		}

		M_FLUSHARRAY(m_lVertices);
		M_FLUSHARRAY(m_lTVertices1);
		M_FLUSHARRAY(m_lTVertices2);
		M_FLUSHARRAY(m_lColors);
		M_FLUSHARRAY(m_lNormals);
		M_FLUSHARRAY(m_lStripPrim);
		M_FLUSHARRAY(m_lTrianglePrim);
	}

	CMTime m_LastFrameTime;

	bool	m_bTex1, m_bTex2, m_bNorm, m_bCols;

//	void Render( CRenderContext *_pRC, const CXB_Benchmark &_BM )
	void Render(CXR_VBManager* _pVBM, const CXB_Benchmark &_BM )
	{
		m_bTex1 = _BM.m_bCompTV0 != 0;
		m_bTex2 = _BM.m_bCompTV1 != 0;
		m_bNorm	=	_BM.m_bCompNormals != 0;
		m_bCols	=	_BM.m_bCompColors != 0;

//		_pRC->Attrib_Push();
//		_pRC->Matrix_Push();

//		_pRC->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
//		_pRC->Attrib_Enable(CRC_FLAGS_CULL);

		//_pRC->Attrib_Disable( CRC_FLAGS_BLEND );

		CMTime FrameTime = CMTime::GetCPU();
		CMTime FrameTime2 = FrameTime;

		FrameTime -= m_LastFrameTime;

		m_LastFrameTime = FrameTime2;

		fp4 DeltaTime = FrameTime.GetTime();
		DeltaTime *= 30.0f;

		//FrameTime = 1;
		DeltaTime = 0.1f;

		m_Rot		+= CVec3Dfp4( 0.002f, -0.003f, 0.004f ) * DeltaTime;
		m_Rot2	+= CVec3Dfp4( 0.003f, 0.002f, -0.003f ) * DeltaTime;

		CVec3Dfp4 Rot = m_Rot;
		CVec3Dfp4 Rot2 = m_Rot2;

		CRC_Light Lights[2];

		int LastAttr = -1;
		fp4 Attr = 0;

		CMat4Dfp4 Mat;
		CMat4Dfp4 Mat2;

		for(int i = 0; i < _BM.m_nModels; i++)
		{
			CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
//			_pRC->Geometry_Clear();

			Rot.CreateMatrixFromAngles(3, Mat);
			//CVec3Dfp4(-250 + ((fp4)i * 500), 0, 1224).SetMatrixRow(Mat, 3);
			CVec3Dfp4(0, 0, 1024).SetMatrixRow(Mat, 3);
			Rot2.CreateMatrixFromAngles(3, Mat2);
			CVec3Dfp4 v(CVec3Dfp4::GetMatrixRow(Mat2, 2));
			v *= 266.0f * 1.1;
			CVec3Dfp4::GetMatrixRow(Mat, 3) += v;

			v = CVec3Dfp4::GetMatrixRow(Mat2, 1);
			v *= 180.0f * 1.1;
//			CVec3Dfp4::GetMatrixRow(Mat, 3) += v;

			v = CVec3Dfp4::GetMatrixRow(Mat2, 0);
			v *= 200.0f * 1.1;
//			CVec3Dfp4::GetMatrixRow(Mat, 3) += v;

//			_pRC->Matrix_Set(Mat);
			pVB->m_pTransform = _pVBM->Alloc_M4(Mat);

			int iAttr = TruncToInt(Attr);
			if (iAttr != LastAttr)
			{
//				_pRC->Attrib_Set(m_lAttributes[iAttr % m_lAttributes.Len()]);
//				pVB->m_pAttrib = &m_lAttributes[iAttr % m_lAttributes.Len()];
				LastAttr = iAttr;
			}
			pVB->m_pAttrib = &m_lAttributes[LastAttr];

			Attr += _BM.m_AttrInc;

			/*	_pRC->Attrib_Enable(CRC_FLAGS_BLEND);
				_pRC->Attrib_SourceBlend(CRC_BLEND_ONE);
				_pRC->Attrib_DestBlend(CRC_BLEND_ONE);
 
				_pRC->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_CULLCCW);
	
				_pRC->Attrib_TextureID(0, m_TextureID[0]);
				_pRC->Geometry_TVertexArray(m_lTVertices1.GetBasePtr(), 0);*/

			//			_pRC->Attrib_Lights(&Lights[0], 1);

			/*if (m_TextureID[1])
			{
				_pRC->Attrib_TextureID(1, m_TextureID[1]);
				_pRC->Geometry_TVertexArray(m_lTVertices2.GetBasePtr(), 1);
			}*/

			if( _BM.m_bStaticIndices && _BM.m_bStaticVertices )
			{
				pVB->SetVBChain(_pVBM, true);
//				_pRC->Render_VertexBuffer( m_VBID );
				pVB->Render_VertexBuffer( m_VBID );
			}
			else
			{
				if( _BM.m_bStaticVertices )
				{
					pVB->SetVBChain(_pVBM, true);
//					_pRC->Geometry_VertexBuffer( m_VBID, true );
					pVB->Geometry_VertexBuffer( m_VBID, true );
				}
				else
				{
					pVB->SetVBChain(_pVBM, false);
//					_pRC->Geometry_VertexArray( m_lVertices.GetBasePtr(), m_lVertices.Len() );
					pVB->Geometry_VertexArray( m_lVertices.GetBasePtr(), m_lVertices.Len(), true);

					if( _BM.m_bCompTV0 )
					{
		//				_pRC->Geometry_TVertexArray( m_lTVertices1.GetBasePtr(), 0 );
						pVB->Geometry_TVertexArray( m_lTVertices1.GetBasePtr(), 0 );

						if( _BM.m_bCompTV1 )
						{
		//					_pRC->Geometry_TVertexArray( m_lTVertices2.GetBasePtr(), 1 );
							pVB->Geometry_TVertexArray( m_lTVertices2.GetBasePtr(), 1 );
						}
					}

					if( _BM.m_bCompColors )
					{
		//				_pRC->Geometry_ColorArray( m_lColors.GetBasePtr() );
						pVB->Geometry_ColorArray( m_lColors.GetBasePtr() );
					}

					if( _BM.m_bCompNormals )
					{
		//				_pRC->Geometry_NormalArray( m_lNormals.GetBasePtr() );
						pVB->Geometry_Normal( m_lNormals.GetBasePtr() );
					}

				}

				if (m_lStripPrim.Len())
				{
//					_pRC->Render_IndexedPrimitives(m_lStripPrim.GetBasePtr(), m_lStripPrim.Len());
					pVB->Render_IndexedPrimitives(m_lStripPrim.GetBasePtr(), m_lStripPrim.Len());
				}
				else
				{
//					_pRC->Render_IndexedTriangles(m_lTrianglePrim.GetBasePtr(), m_lTrianglePrim.Len() / 3);
					pVB->Render_IndexedTriangles(m_lTrianglePrim.GetBasePtr(), m_lTrianglePrim.Len() / 3);
				}
			}
			_pVBM->AddVB(pVB);


//			_pRC->Attrib_Lights(NULL, 0);
//			_pRC->Matrix_Pop();

			Rot		+= CVec3Dfp4( -0.02f, 0.03f, -0.04f ) * 0.75f;
			Rot2	+= CVec3Dfp4( 0.07f, -0.05f, 0.045f ) * 0.75f;
		}

//		_pRC->Geometry_Clear();

//		_pRC->Matrix_Pop();
//		_pRC->Attrib_Pop();
	}

/*
*/
	virtual int GetNumLocal()
	{
		return(1);
	};

/*
*/
	virtual int GetID( int _iLocal )
	{
		return( m_VBID );
	};

/*
*/
	virtual void Get( int _iLocal, CRC_BuildVertexBuffer &_VB, int _Flags)
	{
		_VB.m_nV		= m_lVertices.Len();
		_VB.Geometry_VertexArray(m_lVertices.GetBasePtr());

		if( m_bCols )	_VB.Geometry_ColorArray(m_lColors.GetBasePtr());
		if( m_bNorm )	_VB.Geometry_NormalArray(m_lNormals.GetBasePtr());

		if( m_bTex1 )
		{
			_VB.Geometry_TVertexArray(m_lTVertices1.GetBasePtr(), 0);

			if( m_bTex2 )
			{
				_VB.Geometry_TVertexArray(m_lTVertices2.GetBasePtr(), 1);
			}
		}

		if( m_lTrianglePrim.Len() )
		{
			_VB.m_PrimType	= CRC_RIP_TRIANGLES;
			_VB.m_piPrim		= m_lTrianglePrim.GetBasePtr();
			_VB.m_nPrim			= m_lTrianglePrim.Len() / 3;
		}
		else
		{
			_VB.m_PrimType	= CRC_RIP_STREAM;
			_VB.m_piPrim		= m_lStripPrim.GetBasePtr();
			_VB.m_nPrim			= m_lStripPrim.Len();
		}
	}
};

// ----------------------------------------------------------------
// CXBenchApp
// ----------------------------------------------------------------
class CXBenchApp : public CApplication
{

	MRTC_DECLARE


	CRC_Util2D m_Util2D;

	bool ShowConsole;
	bool m_bShowFPS;

	CMTime m_Time_Frame;
	fp4 m_FPSHistory[NUMFPSHIST];
	int m_FPSHistPtr;
	fp4 m_AvgFPS;
	CStr m_AvgFPSStr;

	spCRC_ConsoleViewport spViewport;

	int m_MouseReleased;

	CConsoleRender m_ConRender;

//	spCTextureContainer_VirtualXTC2	m_spXBTC;
	spCTextureContainer_Plain	m_spXBTC;
	spCRC_Font m_spWinFont;

	int m_iCurrentBM;
	CXB_Setup m_Setup;

	CXR_VBMContainer m_VBMContainer;

public:
	spCXB_Torus m_spTorus;
	spCXB_Quad	m_spQuad;

	CXBenchApp();
	~CXBenchApp();
	virtual void Create();
	void Init();
	void SetBenchmark(int _iBM);

	void OnRefreshSystem();
	void OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip);
	void OnBusy(int _Context) {};
	void DoModal();

	virtual void Register(CScriptRegisterContext & _RegContext) {};
};

// ----------------------------------------------------------------
// CXBenchApp Implementation
// ----------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXBenchApp, CApplication);

CXBenchApp::CXBenchApp()
{
	m_bShowFPS = true;
	m_iCurrentBM = 0;
}

CXBenchApp::~CXBenchApp()
{
}

void CXBenchApp::Create()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) Error("-", "No system.");
	m_pSystem = pSys;
	Init();
}

void CXBenchApp::Init()
{
	// These net thingies are created just to cause the crash bug to occur
	CNetwork* pNet = (CNetwork*)MRTC_GOM()->GetClassRegistry()->CreateObject("CNetworkCore");
	CNetwork_Device* pDevice = (CNetwork_Device*)MRTC_GOM()->GetClassRegistry()->CreateObject("CNetwork_Device_UDP");
	pNet->Create(pDevice);

	CStr XMDName;

	spViewport = MNew(CRC_ConsoleViewport);
	if (spViewport == NULL) MemError("-");
	spViewport->Create(true);

	spViewport->SetFOV(90);

	ShowConsole = TRUE;
	m_Time_Frame.Reset();
	FillChar(m_FPSHistory, sizeof(m_FPSHistory), 0);
	m_FPSHistPtr = 1;
	m_AvgFPS = 0;

//	m_pSystem->DC_InitFromEnvironment();

	m_Setup.Create("XBenchSetup.xrg");

	LogFile(CStrF("(CXBenchApp::Init) %d benchmarks parsed.", m_Setup.m_lspBenchmarks.Len()));


	// Load XTC for GUI
/*	m_spWinTC = DNew CTextureContainer_Plain;
	if (!m_spWinTC) MemError("Init");
	m_spWinTC->AddFromXTC(m_pSystem->m_ExePath + "WINGFX.XTC");
*/

	// Load XTC for XBench
//	m_spXBTC = MNew(CTextureContainer_VirtualXTC2);
	m_spXBTC = MNew(CTextureContainer_Plain);
	if(!m_spXBTC) MemError("Init");
//	m_spXBTC->Create( m_pSystem->m_ExePath + "XBench.XTC" );
	m_spXBTC->AddFromXTC( m_pSystem->m_ExePath + "XBench.XTC" );

	// Load GUI font
	m_spWinFont = MNew(CRC_Font);
	if (!m_spWinFont) MemError("-");
//		m_spWinFont->ReadFromScript("WINFONT1.TXT");

	{
		CDataFile DFile;
		DFile.Open(m_pSystem->m_ExePath + "FONT1.XFC");
		m_spWinFont->ReadFromXFC(&DFile);
		DFile.Close();
	}

	LogFile("(CXBenchApp::Init) Done.");

	SetBenchmark(m_iCurrentBM);
};

void CXBenchApp::SetBenchmark(int _iBM)
{
	CXB_Benchmark* pBM = m_Setup.m_lspBenchmarks[_iBM];

	m_spTorus = MNew(CXB_Torus);
	if (!m_spTorus) MemError("Init");
	m_spTorus->Create( *pBM, m_spXBTC );

/*	m_spQuad = MNew(CXB_Quad);
	if (!m_spQuad) MemError("Init");
	m_spQuad->Create( m_spXBTC );
*/
	m_iCurrentBM = _iBM;

//	m_AvgFPSStr = CStrF("%s, ?, Tris: ?, ? MTris/Sec", pBM->m_Name.Str());
	m_AvgFPSStr = CStrF("%s", pBM->m_Name.Str());
	m_FPSHistPtr = 1;
}

void CXBenchApp::OnRefreshSystem()
{
}

// ----------------------------------------------------------------
void CXBenchApp::OnRender(CDisplayContext* _pDisplay, CImage* _pFrame, CClipRect _Clip)
{
	CRCLock RCLock;
	CRenderContext* pRender = _pDisplay->GetRenderContext(&RCLock);
//	if (m_spDesktop) 
//		m_spDesktop->SetPosition(0, 0, _pFrame->GetWidth(), _pFrame->GetHeight());

	if (m_pSystem->m_spInput != NULL)
		m_pSystem->m_spInput->SetMouseArea(_pFrame->GetRect());

	fp8 AvgFPS = 0; 

	CXB_Benchmark *pBM = m_Setup.m_lspBenchmarks[m_iCurrentBM];

	// -------------------------------------------------------------------
	//  FPS-History
	CMTime Time = CMTime::GetCPU();
	fp8 TmpTime_Frame = (Time - m_Time_Frame).GetTime();
	fp8 FPS = 1.0/TmpTime_Frame;
	{
		m_FPSHistory[m_FPSHistPtr] = FPS;
		AvgFPS = 0; 
		for(int i = 0; i < NUMFPSHIST; i++) AvgFPS += m_FPSHistory[i];
		AvgFPS *= 1.0/fp8(NUMFPSHIST);
		if (m_FPSHistPtr == 0)
		{
			m_AvgFPS = AvgFPS;
//			m_AvgFPSStr = CStrF("%s, %.2f, Tris: %d, %.01f MTris/Sec", pBM->m_Name.Str(), m_AvgFPS, ((CRC_Core *)pRender)->m_nTriangles, ((CRC_Core *)pRender)->m_nTriangles * AvgFPS / 1000000.0f);
			m_AvgFPSStr = CStrF("%.01f MTri/s, %.2f Fps, %d Tri, %s",	((CRC_Core *)pRender)->m_nTriangles * AvgFPS / 1000000.0f,
																																m_AvgFPS, 
																																((CRC_Core *)pRender)->m_nTriangles,
																																pBM->m_Name.Str());

#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS3)
			//LogFile(m_AvgFPSStr);
			//m_iCurrentBM = (m_iCurrentBM + 1) % m_Setup.m_lspBenchmarks.Len();
			//SetBenchmark(m_iCurrentBM);
#endif
		}
		m_FPSHistPtr = (m_FPSHistPtr + 1) % NUMFPSHIST;
	}
	m_Time_Frame = Time;

	//------------------------------------------------------
	M_TRY
	{
		_pDisplay->ClearFrameBuffer(CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER, 0xff8080c0 );

//		_pFrame->Fill(_pFrame->GetClipRect(), 0xffff0000);

		//------------------------------------------------------
		// 3D JUNK!
		//------------------------------------------------------
		spViewport->SetView(_Clip, _Clip.clip);
		M_TRY
		{

			M_TRY
			{
				TFStr<512> Stats;
				if(m_bShowFPS)
					Stats = pRender->GetRenderingStatus();
				pRender->BeginScene(spViewport);

				CXR_VBManager* pVBM = m_VBMContainer.GetAvailVBM();
				pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
				pVBM->Begin(pRender, spViewport);
				if(m_spTorus)
				{
					pVBM->ScopeBegin(false, 8192);
					m_spTorus->Render(pVBM, *pBM);
					pVBM->ScopeEnd();
				}
				if(m_spQuad)
				{
					pVBM->ScopeBegin(false);
					m_spQuad->Render(pVBM);
					pVBM->ScopeEnd();
				}

//#ifdef NEVER
				{
					if (m_bShowFPS)
					{
						m_Util2D.Begin(pRender, spViewport, pVBM);
						pVBM->ScopeBegin(false);
						m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
						m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

						if(m_spWinFont != NULL)
						{
							m_Util2D.SetCoordinateScale( CVec2Dfp4(1.5f) );
							m_Util2D.Text( _Clip, m_spWinFont, 50, 50, m_AvgFPSStr, 0xff808080 );
							{
								int Len = Stats.Len();
								int Pos = 0;
								int y = 70;
								int x = 50;

								while(Pos < Len)
								{
									const char* pS = (char*)Stats;
									char Buff[1024];
									int Pos2 = Pos;
									while(Pos2 < Len && pS[Pos2] != ',')
									{
										Buff[Pos2-Pos] = pS[Pos2];
										Pos2++;
									}
									Buff[Pos2-Pos] = 0;

									m_Util2D.Text(_Clip, m_spWinFont, x, y, Buff, 0xff3f90af);
									y += 8;
									Pos = Pos2+1;
								}
							}
						}

//						if (m_pSystem->m_spInput->GetMouseButtons())
/*						{
							CStr WStr("Tris: %d, %.01f MTris/Sec", ((CRC_Core *)pRender)->m_nTriangles, ((CRC_Core *)pRender)->m_nTriangles * AvgFPS / 1000000.0f);
							m_Util2D.Text(_Clip, spWinFont, 0, 20, WStr, 0xffffffff);
						}
						*/

						pVBM->ScopeEnd();
					}
				}
				m_Util2D.End();
//#endif

				pVBM->End();
				pVBM->Render(pRender, 0);
				m_VBMContainer.AddAvailVBM(pVBM);
				pRender->EndScene();

			}
			M_CATCH(
			catch(CCException)
			{
				pRender->EndScene();
				throw;
			}
			)

//			if (ShowConsole) _pFrame->DebugText(_Clip, CPnt(0, 32), pRender->GetRenderingStatus(), 0x3f90af);

		}
		M_CATCH(
		catch(CCException)
		{
			throw;
		}
		)


/*		if (m_pSystem->m_spCon)
		{
			m_pSystem->m_spCon->WriteExceptions();
			if (ShowConsole) m_ConRender.RenderRect(m_pSystem->m_spCon, _Clip, _pFrame, _pFrame->GetRect());
		}*/

		//------------------------------------------------------
/*		if (ShowConsole) _pFrame->DebugText(_Clip, CPnt(0,0), _pFrame->IDString(), 0xffffff);

		T_Stop(Time_Frame);
		int32 TmpTime_Frame = Time_Frame;
		T_Start(Time_Frame);
		if (ShowConsole) _pFrame->DebugText(_Clip, CPnt(0,8), T_String("Frame: ",TmpTime_Frame) + ", "+ T_xPerSec(TmpTime_Frame, 1, " fps"), 0xffffff);
//
		if (ShowConsole) _pFrame->DebugText(_Clip, CPnt(0,16), T_String("XR: ",Time_XR) , 0xffffff);*/
		{
			MSCOPE(PageFlip, Bla);
			_pDisplay->PageFlip();
		}
	}
	M_CATCH(
	catch (CCException)
	{
		throw;
	}
	)
};

void CXBenchApp::DoModal()
{
	m_pSystem->DC_InitFromEnvironment();
	m_pSystem->CreateInput();

	int i = 0;

	m_VBMContainer.Create(3, 1024 * 1024, 1024);
	while (!m_pSystem->m_bBreakRequested)
	{
		m_pSystem->Refresh();
		if (m_pSystem->m_spInput != NULL)
		M_TRY 
		{
			while (m_pSystem->m_spInput->KeyPressed()) 
			{
				CScanKey key = m_pSystem->m_spInput->ScanKey();
				
//				M_TRACE("%.8x", key.m_ScanKey32);
				
				if (key.IsRepeat())
					continue;
				if (!key.IsDown())
					continue;

				switch (key.m_ScanKey32) 
				{
					case SKEY_PARAGRAPH : 
						{
							// ToDo: Create console command for fps-render, move to display-context? or client?
							m_bShowFPS ^= TRUE;
							break;
						}
#ifndef M_RTM
					case SKEY_P + 0x0400 : 
						{
							// ToDo: Create console command for fps-render, move to display-context? or client?
							MRTC_GetObjectManager()->m_pCallGraph->Start(2.0f);
							break;
						}
#endif
					case SKEY_L :
						{
//							LogFile(m_AvgFPSStr);
							MRTC_SystemInfo::OS_Trace("%s\n",m_AvgFPSStr.Str());
							break;
						}

					case SKEY_SPACE :
#ifdef PLATFORM_CONSOLE
					case SKEY_JOY_BUTTON00 :
#endif
						{
							m_iCurrentBM = (m_iCurrentBM + 1) % m_Setup.m_lspBenchmarks.Len();
							SetBenchmark(m_iCurrentBM);
						}
						break;

					case SKEY_ESC :
						m_pSystem->m_bBreakRequested = true;

					default:
									break;
				}

			}

//			m_spDesktop->FetchInput();
			if (m_pSystem->m_bBreakRequested) break;
		}
		M_CATCH(
		catch(...) 
		{
			throw;
		}
		)

		m_pSystem->Render();
	};
}

#ifdef PLATFORM_XBOX
	#include <xtl.h>
#elif defined PLATFORM_WIN32
	#include <windows.h>
#endif

MACRO_MAIN("CXBenchApp")
