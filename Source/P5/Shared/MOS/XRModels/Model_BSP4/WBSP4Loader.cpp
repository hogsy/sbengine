#include "PCH.h"

#include "WBSP4Model.h"
#include "WBSP4Def.h"

#include "MFloat.h"

#include "../../MSystem/Raster/MTextureContainers.h"

// -------------------------------------------------------------------
void CXR_Model_BSP4::Create_PostRead()
{
	MAUTOSTRIP(CXR_Model_BSP_Create_PostRead, MAUTOSTRIP_VOID);
	MSCOPE(Create_PostRead, XR_BSPMODEL4);

	InitBound();
	m_spEnumContextPool	= MNew(TObjectPool<CBSP4_EnumContext>);
	m_spEnumContextPool->Create(5);
}

#define CHECK_LD(Var1, Expr, Var2, Msg)	\
	if (Var1 Expr Var2)	\
	{	M_TRACEALWAYS(CStrF("        "#Var1"(%d) "#Expr" "#Var2"(%d) \n"#Msg, Var1, Var2)); bErr = true; };
		

void CXR_Model_BSP4::InitBound()
{
	MAUTOSTRIP(CXR_Model_BSP_InitBound, MAUTOSTRIP_VOID);
	CVec3Dfp32* pV = m_lVertices.GetBasePtr();

	// Init bound-radius
	int nv = m_lVertices.Len();
	m_BoundRadius = 0;
	for(int v = 0; v < nv; v++)
	{
		fp32 l = pV[v].Length();
		if (l > m_BoundRadius) m_BoundRadius = l;
	}
	m_BoundRadius += 0.0001f;

	// Init bound-box
	CVec3Dfp32::GetMinBoundBox(pV, m_BoundBox.m_Min, m_BoundBox.m_Max, nv);

	m_BoundBox.m_Min -= CVec3Dfp32(0.0001f);
	m_BoundBox.m_Max += CVec3Dfp32(0.0001f);
}

void CXR_Model_BSP4::Clear()
{
	MAUTOSTRIP(CXR_Model_BSP_Clear, MAUTOSTRIP_VOID);
	Error("Clear", "Not implemented.");
}


#ifndef PLATFORM_CONSOLE

static int RenderBSPImage_r(CXR_Model_BSP4* _pModel, CImage* _pImage, int _iNode, int _Depth, int _Column)
{
	const CBSP4_Node& Node = _pModel->m_lNodes[_iNode];
	if (Node.IsLeaf())
	{
		if (!_iNode)
			return 0;

		CPixel32 Color = (_iNode) ? 0xffffff00 : 0xffff0000;
		_pImage->SetPixel(_pImage->GetClipRect(), CPnt(_Column, _Depth), Color);
		return 1;
	}
	else if (Node.IsNode())
	{
		CPixel32 Color = 0xff0000ff;
		if (Node.IsStructureLeaf())
			Color = 0xff00ffff;

		_pImage->SetPixel(_pImage->GetClipRect(), CPnt(_Column, _Depth), Color);

		int nColumns = 0;
		nColumns += RenderBSPImage_r(_pModel, _pImage, Node.m_iNodeFront, _Depth+1, _Column+nColumns);
		nColumns += RenderBSPImage_r(_pModel, _pImage, Node.m_iNodeBack, _Depth+1, _Column+nColumns);
		return nColumns;
	}

	return 0;
}

#endif

// -------------------------------------------------------------------
//  Read/Write
// -------------------------------------------------------------------
void CXR_Model_BSP4::Create(const char* _pParam, CDataFile* _pDFile, CCFile*, const CBSP_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP4::Create, XR_BSPMODEL4);

	if (!_pDFile) Error("Create", "Datafile expected.");
	CCFile* pFile = _pDFile->GetFile();

	// Check file-version
	if (!_pDFile->GetNext("VERSION"))
		Error("Read", "No VERSION entry.");
	const int XWVersion = _pDFile->GetUserData();
	if (XWVersion > XW_VERSION)
		Error("Read", CStrF("Invalid XW-version. %.4x should be less or equal to %.4x", _pDFile->GetUserData(), XW_VERSION));

	// Read stuff
	if (!_pDFile->GetNext("PLANES")) Error("Read", "No PLANES entry.");
	if (_pDFile->GetEntrySize() / _pDFile->GetUserData() != 16)
	{
		Error("Read", "Ouch!, THAT's an old file! (Old plane def.)");
	}
	else
	{
		MSCOPE(m_lPlanes, XR_BSPMODEL4);

		m_lPlanes.SetLen(_pDFile->GetUserData());
		pFile->Read(&m_lPlanes[0], m_lPlanes.ListSize());
		SwitchArrayLE_fp32((fp32*)m_lPlanes.GetBasePtr(), m_lPlanes.Len() * sizeof(CPlane3Dfp32)/sizeof(fp32)); //AR-ADD
	}


	// OctAABBTree
	_pDFile->PushPosition();
	{
		_pDFile->ReadArrayEntry(m_lOctaAABBNodes,"OCTAAABBTREE",BSP_OCTAAABBTREE_VERSION);
		if( _pDFile->GetNext("OCTAAABBHEADER") )
		{
			m_OctaAABBBox.Read(_pDFile->GetFile());
			CVec4Dfp32 Scale(65535.0f / (m_OctaAABBBox.m_Max[0] - m_OctaAABBBox.m_Min[0]),
				65535.0f / (m_OctaAABBBox.m_Max[1] - m_OctaAABBBox.m_Min[1]),
				65535.0f / (m_OctaAABBBox.m_Max[2] - m_OctaAABBBox.m_Min[2]), 0.0f);
			m_OctaAABBToU16.Unit();
			m_OctaAABBToU16.k[0][0] = Scale.k[0];
			m_OctaAABBToU16.k[1][1] = Scale.k[1];
			m_OctaAABBToU16.k[2][2] = Scale.k[2];
			m_OctaAABBToU16.k[3][0] = -m_OctaAABBBox.m_Min[0] * m_OctaAABBToU16.k[0][0];
			m_OctaAABBToU16.k[3][1] = -m_OctaAABBBox.m_Min[1] * m_OctaAABBToU16.k[1][1];
			m_OctaAABBToU16.k[3][2] = -m_OctaAABBBox.m_Min[2] * m_OctaAABBToU16.k[2][2];
			m_OctaAABBScale = Scale.v;
			m_OctaAABBTranslate = M_VLd_V3_Slow(&m_OctaAABBBox.m_Min);
		}
	}
	_pDFile->PopPosition();

	{
		MSCOPE(m_lVertices, XR_BSPMODEL4);
		if (!_pDFile->GetNext("VERTICES")) Error("Read", "No VERTICES entry.");
		m_lVertices.SetLen(_pDFile->GetUserData());
		pFile->Read(&m_lVertices[0], m_lVertices.ListSize());
		SwitchArrayLE_fp32((fp32*)m_lVertices.GetBasePtr(), m_lVertices.Len() * sizeof(CVec3Dfp32)/sizeof(fp32)); //AR-ADD
	}

	{
		MSCOPE(m_liVertices, XR_BSPMODEL4);
		if (!_pDFile->GetNext("VERTEXINDICES")) Error("Read", "No VERTEXINDICES entry.");
		int Len = _pDFile->GetUserData();
		m_liVertices.SetLen(Len); 
		if( 0x0123 == XWVersion )
		{
			uint32* piV = m_liVertices.GetBasePtr();
			int nRead = 0;
			while(nRead < Len)
			{
				uint16 liTemp[256];
				uint16 n = Min(256, Len-nRead);
				pFile->ReadLE(liTemp, n);
				for(int i = 0; i < n; i++)
				{
					piV[nRead + i] = liTemp[i];
				}
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
		}
	}

	{
		MSCOPE(m_lFaces, XR_BSPMODEL4);
		if (!_pDFile->GetNext("FACES")) Error("Read", "No FACES entry.");

		int nFaces = _pDFile->GetUserData();
		m_lFaces.SetLen(nFaces); 
		for (int iFace=0; iFace < nFaces; iFace++)
			m_lFaces[iFace].Read(pFile, _pDFile->GetUserData2());
	}

	{
		MSCOPE(m_liFaces, XR_BSPMODEL4);
		if (!_pDFile->GetNext("FACEINDICES")) Error("Read", "No FACEINDICES entry.");
		int Len = _pDFile->GetUserData();
		m_liFaces.SetLen(Len); 
		if( 0x0123 == XWVersion )
		{
			uint32* piF = m_liFaces.GetBasePtr();
			int nRead = 0;
			while(nRead < Len)
			{
				uint16 liTemp[256];
				uint16 n = Min(256, Len-nRead);
				pFile->ReadLE(liTemp, n);
				for(int i = 0; i < n; i++)
				{
					piF[nRead + i] = liTemp[i];
				}
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE(m_liFaces.GetBasePtr(), m_liFaces.Len());
		}
	}

	{
		MSCOPE(m_lNodes, XR_BSPMODEL4);
		if (!_pDFile->GetNext("BSPNODES")) Error("Read", "No NODES entry.");

		int nNodes = _pDFile->GetUserData();
		int Ver = _pDFile->GetUserData2();
		m_lNodes.SetLen(nNodes); 
		CBSP4_Node* pN = m_lNodes.GetBasePtr();
		{
			// Fall-back path
			for (int iNode=0; iNode < nNodes; iNode++)
				pN[iNode].Read(pFile, Ver);
		}
	}

	{
		MSCOPE(Surfaces, XR_BSPMODEL4);
		if (!_pDFile->GetNext("SURFACES")) Error("Read", "No SURFACES entry.");
		CXW_Surface::Read(pFile, m_lspSurfaces, _pDFile->GetUserData());
	}

	{
		MSCOPE(m_lMediums, XR_BSPMODEL4);
		if (!_pDFile->GetNext("MEDIUMS")) Error("Read", "No MEDIUMS entry.");
		m_lMediums.SetLen(_pDFile->GetUserData()); 
		{ for(int iMedium = 0; iMedium < m_lMediums.Len(); iMedium++) m_lMediums[iMedium].Read(pFile); }
	}


	// Light normal-indices
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTNORMALINDX"))
	{
		ConOut("WARNING: CXR_Model_BSP4 with lightnormals data");
	}
	_pDFile->PopPosition();

	// LIGHTMAPINFO
	{
		ConOut("WARNING: CXR_Model_BSP4 with lightmap data");
	}

	// Light vertices
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTVERTINFO"))
	{
		ConOut("WARNING: CXR_Model_BSP4 with LightVertInfo data");
	}
	_pDFile->PopPosition();

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Init surfaces
	{
		MSCOPE(InitTextures, XR_BSPMODEL4);
		for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		{
			CXW_Surface* pS = m_lspSurfaces[iSurf];
			pS->InitTextures(false);	// Don't report failures.
		}
	}

	InitializeListPtrs();
	Create_PostRead();
	InitializeListPtrs();

#if defined PLATFORM_CONSOLE && 0
	{
		CImage BSPImage;
		BSPImage.Create(Min(8192, m_lNodes.Len()), 64, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);
		BSPImage.Fill(BSPImage.GetClipRect(), 0);

		for(int i = 0; i < 100/4; i++)
			BSPImage.Line(BSPImage.GetClipRect(), CPnt(0, 3+4*i), CPnt(65536, 3+4*i), (i & 1) ? 0xff303030 : 0xff202020);

		RenderBSPImage_r(this, &BSPImage, 1, 0, 0);

		BSPImage.Write(CRct(0,0,-1,-1), CStrF("s:\\BSPImage_%s.tga", pFile->GetFileName().GetFilenameNoExt().Str() ) );
	}
#endif
}

