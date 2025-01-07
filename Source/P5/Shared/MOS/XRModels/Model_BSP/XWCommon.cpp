#include "PCH.h"

#include "XWCommon.h"
#include "MFloat.h"

#define SHIFT_VALUE(v, _off, _mask) ((v >> _off) & ((1 << _mask) - 1))

void CBSP_Node::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP_Node_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0000 :
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iNodeParent);

			if (IsNode()) {
				_pF->ReadLE(m_Bound.m_iBoundNodeFront);
				_pF->ReadLE(m_Bound.m_iBoundNodeBack);
			} else {
				_pF->ReadLE(m_iiFaces);
			}

			_pF->ReadLE(m_Flags);
			_pF->ReadLE(m_iPortalLeaf);
			uint16 Dummy;
			_pF->ReadLE(Dummy);
			_pF->ReadLE(Dummy);
		}
		break;

	case 0x0200 :
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iNodeParent);

			if (IsNode()) {
				_pF->ReadLE(m_Bound.m_iBoundNodeFront);
				_pF->ReadLE(m_Bound.m_iBoundNodeBack);
			} else {
				_pF->ReadLE(m_iiFaces);
			}

			_pF->ReadLE(m_Flags);
			_pF->ReadLE(m_iPortalLeaf);
		}
		break;

	case 0x0202:
		{
			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iNodeParent);

			if (IsNode()) {
				_pF->ReadLE(m_Bound.m_iBoundNodeFront);
				_pF->ReadLE(m_Bound.m_iBoundNodeBack);
			} else {
				_pF->ReadLE(m_iiFaces);
			}

			_pF->ReadLE(m_Flags);
			_pF->ReadLE(m_iPortalLeaf);
			uint16 Padding0;
			_pF->ReadLE(Padding0);
		}
		break;

	default :
		Error_static("CBSP_Node::Read", CStrF("Unsupported version %.4x", _Ver));
	}

}

void CBSP_Node::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_Node_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iPlane);
	_pF->WriteLE(m_iNodeFront);
	_pF->WriteLE(m_iNodeBack);
	_pF->WriteLE(m_iNodeParent);

	if (IsNode()) {
		_pF->WriteLE(m_Bound.m_iBoundNodeFront);
		_pF->WriteLE(m_Bound.m_iBoundNodeBack);
	} else {
		_pF->WriteLE(m_iiFaces);
	}

	_pF->WriteLE(m_Flags);
	_pF->WriteLE(m_iPortalLeaf);
	uint16 Padding0 = 0;
	_pF->WriteLE(Padding0);
}

#ifndef CPU_LITTLEENDIAN
void CBSP_Node::SwapLE()
{
	MAUTOSTRIP(CBSP_Node_SwapLE, MAUTOSTRIP_VOID);
	::SwapLE(m_iPlane);
	::SwapLE(m_iNodeFront);
	::SwapLE(m_iNodeBack);
	::SwapLE(m_iNodeParent);

	if (IsNode())
	{
		::SwapLE(m_Bound.m_iBoundNodeFront);
		::SwapLE(m_Bound.m_iBoundNodeBack);
	}
	else
		::SwapLE(m_iiFaces);

	::SwapLE(m_Flags);
	::SwapLE(m_iPortalLeaf);
}
#endif

// -------------------------------------------------------------------
CBSP_Portal::CBSP_Portal()
{
	MAUTOSTRIP(CBSP_Portal_ctor, MAUTOSTRIP_VOID);
	m_iPlane = 0;
	m_iNodeFront = 0;
	m_iNodeBack = 0;
	m_iRPortal = 0;
	m_iFogPortal = 0;
}

void CBSP_Portal::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP_Portal_Read, MAUTOSTRIP_VOID);
	CXR_IndexedPortal::Read(_pF);
	switch(_Version)
	{
	case 0x0000:
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			uint16 iNodeFront, iNodeBack;
			_pF->ReadLE(iNodeFront);
			_pF->ReadLE(iNodeBack);
			m_iNodeFront = iNodeFront;
			m_iNodeBack = iNodeBack;
			_pF->ReadLE(m_iRPortal);
			_pF->ReadLE(m_iFogPortal);
			break;
		}

	case 0x0100:
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iRPortal);
			_pF->ReadLE(m_iFogPortal);
		}
		break;

	case 0x0101:
		{
			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iRPortal);
			_pF->ReadLE(m_iFogPortal);
		}
		break;

	default:
		Error_static("Read", CStrF("Unsupported version %.4x (current %.4x)", _Version, XW_PORTAL_VERSION));
	}
}

void CBSP_Portal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_Portal_Write, MAUTOSTRIP_VOID);
	CXR_IndexedPortal::Write(_pF);
	_pF->WriteLE(m_iPlane);
	_pF->WriteLE(m_iNodeFront);
	_pF->WriteLE(m_iNodeBack);
	_pF->WriteLE(m_iRPortal);
	_pF->WriteLE(m_iFogPortal);
}

// -------------------------------------------------------------------
void CBSP_PortalLeaf::Read(CCFile* _pF, int _bReadVersion)
{
	MAUTOSTRIP(CBSP_PortalLeaf_Read, MAUTOSTRIP_VOID);
	int16 Version = 0x0100;
	if (_bReadVersion)
		_pF->ReadLE(Version);

	switch(Version)
	{

	case 0x0201:
		{
			_pF->ReadLE(m_iOctaAABBNode);
		}

	case 0x0200:
		{
			_pF->ReadLE(m_iLightVolume);
			_pF->ReadLE(m_nLightVolume);
		}

	case 0x0100:
		{
			_pF->ReadLE(m_Flags);
			_pF->ReadLE(m_nPortals);
			_pF->ReadLE(m_iiPortals);
			m_BoundBox.Read(_pF);
			_pF->ReadLE(m_ContainsMedium);
			_pF->ReadLE(m_iMedium);
			_pF->ReadLE(m_iPVS);
			_pF->ReadLE(m_iPHS);
			_pF->ReadLE(m_PVSBitLen);
			_pF->ReadLE(m_PHSBitLen);
			_pF->Read(m_AmbientVol, sizeof(m_AmbientVol));
		}
		break;

	default:
		Error_static("CBSP_PortalLeaf::Read", CStrF("Invalid portal-leaf version. %.4x", Version));
	}
}

void CBSP_PortalLeaf::Write(CCFile* _pF, int _bReadVersion) const
{
	MAUTOSTRIP(CBSP_PortalLeaf_Write, MAUTOSTRIP_VOID);
	int16 Version = XW_PORTALLEAF_VERSION;
	_pF->WriteLE(Version);

	// >= 0x0201
	_pF->WriteLE(m_iOctaAABBNode);

	// >= 0x0200
	_pF->WriteLE(m_iLightVolume);
	_pF->WriteLE(m_nLightVolume);

	// >= 0x0100
	_pF->WriteLE(m_Flags);
	_pF->WriteLE(m_nPortals);
	_pF->WriteLE(m_iiPortals);
	m_BoundBox.Write(_pF);
	_pF->WriteLE(m_ContainsMedium);
	_pF->WriteLE(m_iMedium);
	_pF->WriteLE(m_iPVS);
	_pF->WriteLE(m_iPHS);
	_pF->WriteLE(m_PVSBitLen);
	_pF->WriteLE(m_PHSBitLen);
	_pF->Write(m_AmbientVol, sizeof(m_AmbientVol));
}

// -------------------------------------------------------------------
void CBSP_FogPortalFace::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP_FogPortalFace_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iiVertices);
	_pF->ReadLE(m_nVertices);
}

void CBSP_FogPortalFace::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_FogPortalFace_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iiVertices);
	_pF->WriteLE(m_nVertices);
}

// -------------------------------------------------------------------
CBSP_FogPortal::CBSP_FogPortal()
{
	MAUTOSTRIP(CBSP_FogPortal_ctor, MAUTOSTRIP_VOID);
	m_iVertices = 0;
	m_nVertices = 0;
	m_iFaces = 0;
	m_nFaces = 0;
}

void CBSP_FogPortal::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP_FogPortal_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iVertices);
	_pF->ReadLE(m_nVertices);
	_pF->ReadLE(m_iFaces);
	_pF->ReadLE(m_nFaces);
}

void CBSP_FogPortal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_FogPortal_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iVertices);
	_pF->WriteLE(m_nVertices);
	_pF->WriteLE(m_iFaces);
	_pF->WriteLE(m_nFaces);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| 
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CBSP_LightMapInfo::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP_LightMapInfo_Read, MAUTOSTRIP_VOID);
	if (_Version == 1)
	{
		int32 LightID;
		int32 Scale;
		fp32 OffsetU;
		fp32 OffsetV;
		uint16 LMCOffsetU;
		uint16 LMCOffsetV;
		uint16 iLMC;
		uint16 Padd;
		_pF->ReadLE(LightID);
		_pF->ReadLE(Scale);
		_pF->ReadLE(OffsetU);
		_pF->ReadLE(OffsetV);
		_pF->ReadLE(LMCOffsetU);
		_pF->ReadLE(LMCOffsetV);
		_pF->ReadLE(iLMC);
		_pF->ReadLE(Padd);
	
#ifndef M_RTM
		if(!IsPow2(Scale))
			LogFile("ERROR: This map needs to have lightmaps re-rendered");
#endif
		m_ScaleShift = Log2(Scale);
		m_LMCOffsetXHalf = LMCOffsetU >> 1;
		m_LMCOffsetYHalf = LMCOffsetV >> 1;
		m_iLMC = iLMC;

		m_LMCWidthHalf = 0;
		m_LMCHeightHalf = 0;
		m_IntensityScale	= 1.0f;
	}
	else if (_Version == 2)
	{
		m_IntensityScale	= 1.0f;
		uint8 Scale;
		_pF->ReadLE(Scale);
		_pF->ReadLE(m_LMCOffsetXHalf);
		_pF->ReadLE(m_LMCOffsetYHalf);
		_pF->ReadLE(m_iLMC);
		_pF->ReadLE(m_LMCWidthHalf);
		_pF->ReadLE(m_LMCHeightHalf);
#ifndef M_RTM
		if(!IsPow2(Scale))
			LogFile("ERROR: This map needs to have lightmaps re-rendered");
#endif
		m_ScaleShift = Log2(Scale);
	}
	else if((_Version == 3) || (_Version == 4))
	{
		m_IntensityScale.Read(_pF);
		_pF->ReadLE(m_ScaleShift);
		_pF->ReadLE(m_LMCOffsetXHalf);
		_pF->ReadLE(m_LMCOffsetYHalf);
		_pF->ReadLE(m_iLMC);
		_pF->ReadLE(m_LMCWidthHalf);
		_pF->ReadLE(m_LMCHeightHalf);

		if(_Version == 3)
		{
#ifndef M_RTM
			if(!IsPow2(m_ScaleShift))
				LogFile("ERROR: This map needs to have lightmaps re-rendered");
#endif
			m_ScaleShift = Log2(m_ScaleShift);
		}
	}
}

void CBSP_LightMapInfo::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_LightMapInfo_Write, MAUTOSTRIP_VOID);
	m_IntensityScale.Write(_pF);
	_pF->WriteLE(m_ScaleShift);
	_pF->WriteLE(m_LMCOffsetXHalf);
	_pF->WriteLE(m_LMCOffsetYHalf);
	_pF->WriteLE(m_iLMC);
	_pF->WriteLE(m_LMCWidthHalf);
	_pF->WriteLE(m_LMCHeightHalf);
}

#ifndef CPU_LITTLEENDIAN
void CBSP_LightMapInfo::SwapLE()
{
	MAUTOSTRIP(CBSP_LightMapInfo_SwapLE, MAUTOSTRIP_VOID);
	m_IntensityScale.SwapLE();
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP_LightMapContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP_LightMapContainer::CBSP_LMDesc::Read(CCFile* _pFile, int _Version)
{
	MAUTOSTRIP(CBSP_LightMapContainer_CBSP_LMDesc_Read, MAUTOSTRIP_VOID);
	if (_Version == 0x0100)
	{
		_pFile->ReadLE(m_Width);
		_pFile->ReadLE(m_Height);
		_pFile->ReadLE(m_DataPos);
	}
	else
		Error_static("CBSP_LightMapContainer::CBSP_LMDesc::Read", CStrF("Unsupported version %.8x", _Version));
}

void CBSP_LightMapContainer::CBSP_LMDesc::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP_LightMapContainer_CBSP_LMDesc_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_Width);
	_pFile->WriteLE(m_Height);
	_pFile->WriteLE(m_DataPos);
}

CBSP_LightMapContainer::CBSP_LightMapContainer()
{
	MAUTOSTRIP(CBSP_LightMapContainer_ctor, MAUTOSTRIP_VOID);
	m_Format = 0;
}

CBSP_LightMapContainer::~CBSP_LightMapContainer()
{
	MAUTOSTRIP(CBSP_LightMapContainer_dtor, MAUTOSTRIP_VOID);
}

void CBSP_LightMapContainer::Create(TArray<spCImage> _lspLightMaps, int _Format)
{
	MAUTOSTRIP(CBSP_LightMapContainer_Create, MAUTOSTRIP_VOID);
	int nLM = _lspLightMaps.Len();
//	TArray<spCImage> lspLMTemp;
	m_lLMDesc.SetLen(nLM);
//	lspLMTemp.SetLen(nLM);

	int PixelSize = CImage::Format2PixelSize(_Format);
	// Convert images and calculate total size of image data.
	int SizeTotal = 0;
	{
		for(int i = 0; i < nLM; i++)
		{
//			lspLMTemp[i] = _lspLightMaps[i]->Convert(_Format);
//			SizeTotal += lspLMTemp[i]->GetSize();
			SizeTotal += PixelSize * _lspLightMaps[i]->GetWidth() * _lspLightMaps[i]->GetHeight();
		}
	}

	// Stuff all images into the one data block
	m_lLMData.SetLen(SizeTotal);
	{
		int Pos = 0;
		for(int i = 0; i < nLM; i++)
		{
			// We do it this way because it will work even if the source's modulo != width*pixelsize
			CImage* pLM = _lspLightMaps[i];
			CImage Ref; Ref.CreateReference(pLM->GetWidth(), pLM->GetHeight(), _Format, IMAGE_MEM_IMAGE, pLM->GetWidth() * PixelSize, &m_lLMData[Pos], NULL);
//			Ref.Blt(Ref.GetClipRect(), *lspLMTemp[i], 0, CPnt(0,0));
			Ref.Blt(Ref.GetClipRect(), *pLM, 0, CPnt(0,0));

			if (Ref.GetWidth() > 255 || Ref.GetHeight() > 255)
				Error("Create", "Invalid lightmap dimension: " + Ref.IDString());

/*			if (Ref.GetWidth() & 1)
				Error("Create", "Lightmap width was not even: " + Ref.IDString());
			if (Ref.GetHeight() & 1)
				Error("Create", "Lightmap height was not even: " + Ref.IDString());
*/
			m_lLMDesc[i].m_DataPos = Pos;
			m_lLMDesc[i].m_Width = Ref.GetWidth()/* >> 1*/;
			m_lLMDesc[i].m_Height = Ref.GetHeight()/* >> 1*/;

			Pos += Ref.GetSize();
		}

		if (Pos != m_lLMData.Len())
			Error("Create", CStrF("Internal error. %d != %d", Pos, m_lLMData.Len()));
	}

	m_Format = _Format;
}

TArray<spCImage> CBSP_LightMapContainer::GetLightMapImages(int _Format)
{
	MAUTOSTRIP(CBSP_LightMapContainer_GetLightMapImages, TArray<spCImage>());
	TArray<spCImage> lspLM;
	lspLM.SetLen(m_lLMDesc.Len());

	for(int i = 0; i < lspLM.Len(); i++)
	{
		CImage Ref;
		GetLightMap(i, Ref);
		lspLM[i] = Ref.Convert(_Format);
	}

	return lspLM;
}

void CBSP_LightMapContainer::GetLightMap(int _iLightMap, CImage& _RefImage)
{
	MAUTOSTRIP(CBSP_LightMapContainer_GetLightMap, MAUTOSTRIP_VOID);
	const CBSP_LMDesc& LMDesc = m_lLMDesc[_iLightMap];
	_RefImage.CreateReference(LMDesc.m_Width, LMDesc.m_Height, m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
//	_RefImage.CreateReference(LMDesc.GetWidth(), LMDesc.GetHeight(), m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
}

void CBSP_LightMapContainer::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP_LightMapContainer_Read, MAUTOSTRIP_VOID);
	uint32 Version = 0;
	_pFile->ReadLE(Version);

	if (Version == 0x0100)
	{
		uint32 nLMDesc = 0;
		uint32 nLMData = 0;
		_pFile->ReadLE(m_Format);
		_pFile->ReadLE(nLMDesc);
		_pFile->ReadLE(nLMData);

		m_lLMDesc.SetLen(nLMDesc);
		for(int i = 0; i < nLMDesc; i++)
			m_lLMDesc[i].Read(_pFile, Version);

		m_lLMData.SetLen(nLMData);
		_pFile->Read(m_lLMData.GetBasePtr(), nLMData);
	}
	else
		Error("Read", CStrF("Unsupported version %.8x", Version));
}

void CBSP_LightMapContainer::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP_LightMapContainer_Write, MAUTOSTRIP_VOID);
	uint32 Version = XW_LIGHTMAPCONTAINER_VERSION;
	uint32 nLMDesc = m_lLMDesc.Len();
	uint32 nLMData = m_lLMData.Len();
	_pFile->WriteLE(Version);
	_pFile->WriteLE(m_Format);
	_pFile->WriteLE(nLMDesc);
	_pFile->WriteLE(nLMData);

	for(int i = 0; i < nLMDesc; i++)
		m_lLMDesc[i].Write(_pFile);

	_pFile->Write(m_lLMData.GetBasePtr(), nLMData);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP_LightVolumeInfo
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP_LightVolumeInfo::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP_LightVolumeInfo_Read, MAUTOSTRIP_VOID);
	int16 Version;
	_pF->ReadLE(Version);

	switch(Version)
	{
	case 0x0100:
		{
			_pF->ReadLE(m_LightID);
			m_VolumeSize.Read(_pF);
			m_CellSize.Read(_pF);
			m_CellSizeInv.Read(_pF);
			m_GridSize.Read(_pF);
			_pF->ReadLE(m_nGrids);
			_pF->ReadLE(m_CellsPerGrid);
			_pF->ReadLE(m_iCells);
			_pF->ReadLE(m_iPortalLeaf);
		}
		break;

	default:
		Error_static("CBSP_LightVolumeInfo::Read", CStrF("Invalid light-volume version. %.4x", Version));
	}
}

void CBSP_LightVolumeInfo::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_LightVolumeInfo_Write, MAUTOSTRIP_VOID);
	int16 Version = XW_LIGHTVOLUME_VERSION;
	_pF->WriteLE(Version);

	_pF->WriteLE(m_LightID);
	m_VolumeSize.Write(_pF);
	m_CellSize.Write(_pF);
	m_CellSizeInv.Write(_pF);
	m_GridSize.Write(_pF);
	_pF->WriteLE(m_nGrids);
	_pF->WriteLE(m_CellsPerGrid);
	_pF->WriteLE(m_iCells);
	_pF->WriteLE(m_iPortalLeaf);
}

// -------------------------------------------------------------------
#define LERPI2(c0, c1, t) (int(c0) + (((int(c1)-int(c0))*int(t)) >> 8))

fp32 CBSP_LightVolumeInfo::Light_EvalVertex(const CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor)
{
	MAUTOSTRIP(CBSP_LightVolumeInfo_Light_EvalVertex, 0.0f);
	CVec3Dfp32 v;
	_V.Sub(m_VolumeSize.m_Min, v);
	v.CompMul(m_CellSizeInv, v);
	v *= 256.0f;

	int ymod = m_GridSize.k[0];
	int zmod = m_GridSize.k[0]*m_GridSize.k[1];

	int vx = RoundToInt(v[0]);
	int vy = RoundToInt(v[1]);
	int vz = RoundToInt(v[2]);
	int fx = vx & 255;
	int fy = vy & 255;
	int fz = vz & 255;
	vx /= 256;
	vy /= 256;
	vz /= 256;
	if (vx < 0) { vx = 0; fx = 0; }
	if (vy < 0) { vy = 0; fy = 0; }
	if (vz < 0) { vz = 0; fz = 0; }
	if (vx >= m_GridSize[0]-1) { vx = m_GridSize[0]-2; fx = 255; };
	if (vy >= m_GridSize[1]-1) { vy = m_GridSize[1]-2; fy = 255; };
	if (vz >= m_GridSize[2]-1) { vz = m_GridSize[2]-2; fz = 255; };

	int iG = vx + vy*ymod + vz*zmod;

//ConOut(CStrF("EvalVertex %s, %d, %d, %d, iG %d", (char*) v.GetString(), vx, vy, vz, iG));

	// In theory, we should be completely within bounds of the cell data.


	const CXR_LightGridPoint* pG = m_pCells;
/*ConOut(CStrF("GridPoint %d, %d, %d, %d, %d, %d, %d", 
	   pG[iG].m_Light.GetR(), pG[iG].m_Light.GetG(), pG[iG].m_Light.GetB(), 
	   pG[iG].m_LightDir[0], pG[iG].m_LightDir[1], pG[iG].m_LightDir[2], pG[iG].m_LightBias));*/
//	CPixel32 Light = pG[iG].m_Light;

	CPixel32 Light = CPixel32::TrilinearRGB(
		pG[iG].GetLight(), pG[iG+1].GetLight(), pG[iG+ymod].GetLight(), pG[iG+1+ymod].GetLight(), 
		pG[iG+zmod].GetLight(), pG[iG+1+zmod].GetLight(), pG[iG+ymod+zmod].GetLight(), pG[iG+1+ymod+zmod].GetLight(),
		fx, fy, fz);

	_LColor[0] = fp32(Light.GetR()) / 256.0f * 2.0f;
	_LColor[1] = fp32(Light.GetG()) / 256.0f * 2.0f;
	_LColor[2] = fp32(Light.GetB()) / 256.0f * 2.0f;

	int xx0 = LERPI2(pG[iG].GetLightDirX(), pG[iG+1].GetLightDirX(), fx);
	int xy0 = LERPI2(pG[iG].GetLightDirY(), pG[iG+1].GetLightDirY(), fx);
	int xz0 = LERPI2(pG[iG].GetLightDirZ(), pG[iG+1].GetLightDirZ(), fx);
	int xb0 = LERPI2(pG[iG].GetLightBias(), pG[iG+1].GetLightBias(), fx);
	int xx1 = LERPI2(pG[iG+ymod].GetLightDirX(), pG[iG+1+ymod].GetLightDirX(), fx);
	int xy1 = LERPI2(pG[iG+ymod].GetLightDirY(), pG[iG+1+ymod].GetLightDirY(), fx);
	int xz1 = LERPI2(pG[iG+ymod].GetLightDirZ(), pG[iG+1+ymod].GetLightDirZ(), fx);
	int xb1 = LERPI2(pG[iG+ymod].GetLightBias(), pG[iG+1+ymod].GetLightBias(), fx);

	int yx0 = LERPI2(xx0, xx1, fy);
	int yy0 = LERPI2(xy0, xy1, fy);
	int yz0 = LERPI2(xz0, xz1, fy);
	int yb0 = LERPI2(xb0, xb1, fy);

	int xx2 = LERPI2(pG[iG+zmod].GetLightDirX(), pG[iG+1+zmod].GetLightDirX(), fx);
	int xy2 = LERPI2(pG[iG+zmod].GetLightDirY(), pG[iG+1+zmod].GetLightDirY(), fx);
	int xz2 = LERPI2(pG[iG+zmod].GetLightDirZ(), pG[iG+1+zmod].GetLightDirZ(), fx);
	int xb2 = LERPI2(pG[iG+zmod].GetLightBias(), pG[iG+1+zmod].GetLightBias(), fx);
	int xx3 = LERPI2(pG[iG+ymod+zmod].GetLightDirX(), pG[iG+1+ymod+zmod].GetLightDirX(), fx);
	int xy3 = LERPI2(pG[iG+ymod+zmod].GetLightDirY(), pG[iG+1+ymod+zmod].GetLightDirY(), fx);
	int xz3 = LERPI2(pG[iG+ymod+zmod].GetLightDirZ(), pG[iG+1+ymod+zmod].GetLightDirZ(), fx);
	int xb3 = LERPI2(pG[iG+ymod+zmod].GetLightBias(), pG[iG+1+ymod+zmod].GetLightBias(), fx);

	int yx1 = LERPI2(xx2, xx3, fy);
	int yy1 = LERPI2(xy2, xy3, fy);
	int yz1 = LERPI2(xz2, xz3, fy);
	int yb1 = LERPI2(xb2, xb3, fy);

	_LDir[0] = fp32(LERPI2(yx0, yx1, fz)) / 127.0f;
	_LDir[1] = fp32(LERPI2(yy0, yy1, fz)) / 127.0f;
	_LDir[2] = fp32(LERPI2(yz0, yz1, fz)) / 127.0f;
	return fp32(LERPI2(yb0, yb1, fz)) / 256.0f;

	// TODO: Trilinear interpolate these:
/*	_LDir[0] = fp32(pG[iG].m_LightDir[0]) / 127.0f;
	_LDir[1] = fp32(pG[iG].m_LightDir[1]) / 127.0f;
	_LDir[2] = fp32(pG[iG].m_LightDir[2]) / 127.0f;
	fp32 Bias = fp32(pG[iG].m_LightBias) / 256.0f * 2.0f;
	return Bias;*/
}

CPixel32 CBSP_LightVolumeInfo::Light_Vertex(const CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN)
{
	MAUTOSTRIP(CBSP_LightVolumeInfo_Light_Vertex, CPixel32());
	Error_static("CBSP_LightVolumeInfo::Light_Vertex", "Not implemented.");
	return 0;
}

void CBSP_LightVolumeInfo::Light_Array(const CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CMat4Dfp32* _pWMat, int _Flags)
{
	MAUTOSTRIP(CBSP_LightVolumeInfo_Light_Array, MAUTOSTRIP_VOID);
	Error_static("CBSP_LightVolumeInfo::Light_Array", "Not implemented.");
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP_LightOcttreeNode
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP_LightOcttreeNode::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP_LightOcttreeNode_Read, MAUTOSTRIP_VOID);
	if (_Ver == 0x0100)
	{
		uint8  SubNodeTypes;
		uint16 iSubNodes;
		_pF->ReadLE(SubNodeTypes);
		_pF->ReadLE(iSubNodes);
		SetSubNodeTypes(SubNodeTypes);
		SetSubNodes(iSubNodes);
	}
	else
		Error_static("CBSP_LightOcttreeNode::Read", CStrF("Unsupported version %.4x", _Ver));
}

void CBSP_LightOcttreeNode::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP_LightOcttreeNode_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(GetSubNodeTypes());
	_pF->WriteLE(GetSubNodes());
}

#ifndef CPU_LITTLEENDIAN
void CBSP_LightOcttreeNode::SwapLE()
{
	MAUTOSTRIP(CBSP_LightOcttreeNode_SwapLE, MAUTOSTRIP_VOID);
	uint8  SubNodeTypes = GetSubNodeTypes();
	uint16 iSubNodes = GetSubNodes();
	::SwapLE(SubNodeTypes);
	::SwapLE(iSubNodes);
	SetSubNodeTypes(SubNodeTypes);
	SetSubNodes(iSubNodes);
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP_LightOcttree
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP_LightOcttree::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP_LightOcttree_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	if (!_pDFile->GetNext("LIGHTOCTTREE"))
		Error("Read", "LIGHTOCTTREE entry not found.");

	if (!_pDFile->GetSubDir())
		Error("Read", "Invalid LIGHTOCTTREE entry.");

	// TREEINFO
	if (!_pDFile->GetNext("TREEINFO"))
		Error("Read", "TREEINFO entry not found.");

	if (_pDFile->GetUserData() == 0x0100)
	{
		pF->ReadLE(m_RootSize);
		m_RootPos.Read(pF);
		m_MinLeafSize = 64;
	}
	else if (_pDFile->GetUserData() == 0x0101)
	{
		pF->ReadLE(m_RootSize);
		pF->ReadLE(m_MinLeafSize);
		m_RootPos.Read(pF);
	}
	else
		Error("Read", CStrF("Unsupported TREEINFO version %.4x", _pDFile->GetUserData()));

	// NODES
	if (!_pDFile->GetNext("NODES"))
		Error("Read", "TREEINFO entry not found.");

	{
//		int NodeVer = _pDFile->GetUserData2();
		m_lNodes.SetLen(_pDFile->GetUserData());

		M_ASSERT(m_lNodes.ListSize() == _pDFile->GetEntrySize(), CStrF("%d != %d", m_lNodes.ListSize(), _pDFile->GetEntrySize()));
		pF->Read(m_lNodes.GetBasePtr(), m_lNodes.ListSize());
#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < m_lNodes.Len(); i++)
			m_lNodes[i].SwapLE();
#endif

//		for(int i = 0; i < m_lNodes.Len(); i++)
//			m_lNodes[i].Read(pF, NodeVer);
	}

	// POINTS
	if (!_pDFile->GetNext("POINTS"))
		Error("Read", "TREEINFO entry not found.");

	{
//		int PointVer = _pDFile->GetUserData2();
		m_lPoints.SetLen(_pDFile->GetUserData());

#ifdef PLATFORM_DOLPHIN
		for (int i=0; i<m_lPoints.Len(); i++)
			m_lPoints[i].Read(pF, 0x0100);
#else
		M_ASSERT(m_lPoints.ListSize() == _pDFile->GetEntrySize(), "!");
		pF->Read(m_lPoints.GetBasePtr(), m_lPoints.ListSize());
#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < m_lPoints.Len(); i++)
			m_lPoints[i].SwapLE();
#endif
#endif

//		for(int i = 0; i < m_lPoints.Len(); i++)
//			m_lPoints[i].Read(pF, PointVer);
	}

	_pDFile->GetParent();
}

void CBSP_LightOcttree::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CBSP_LightOcttree_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginEntry("LIGHTOCTTREE");
	_pDFile->EndEntry(XR_LIGHTOCTTREE_VERSION);
	_pDFile->BeginSubDir();
	
	{
		// TREEINFO
		_pDFile->BeginEntry("TREEINFO");
		{
			pF->WriteLE(m_RootSize);
			pF->WriteLE(m_MinLeafSize);
			m_RootPos.Write(pF);
		}
		_pDFile->EndEntry(XR_LIGHTOCTTREE_VERSION);

		// NODES
		_pDFile->BeginEntry("NODES");
		{
			for(int i = 0; i < m_lNodes.Len(); i++)
				m_lNodes[i].Write(pF);
		}
		_pDFile->EndEntry(m_lNodes.Len(), XR_LIGHTOCTTREE_NODEVERSION);

		// POINTS
		_pDFile->BeginEntry("POINTS");
		{
			for(int i = 0; i < m_lPoints.Len(); i++)
				m_lPoints[i].Write(pF, XR_LIGHTOCTTREE_POINTVERSION);
		}
		_pDFile->EndEntry(m_lPoints.Len(), XR_LIGHTOCTTREE_POINTVERSION);
	}

	_pDFile->EndSubDir();
}

TPtr<CBSP_LightOcttree> CBSP_LightOcttree::FindAndRead(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP_LightOcttree_FindAndRead, NULL);
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTOCTTREE"))
	{
		_pDFile->PopPosition();

		TPtr<CBSP_LightOcttree> spLightTree = MNew(CBSP_LightOcttree);
		if (!spLightTree)
			Error_static("CBSP_LightOcttree::FindAndRead", "Out of memory.");

		spLightTree->Read(_pDFile);
		return spLightTree;
	}
	else
		_pDFile->PopPosition();

	return NULL;
}

fp32 CBSP_LightOcttree::Light_EvalVertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor)
{
	MAUTOSTRIP(CBSP_LightOcttree_Light_EvalVertex, 0.0f);
	CVec3Dfp32 VLocal;
	_V.Sub(m_RootPos, VLocal);

	int MinNodeSize = 64;
	int MinNodeSizeAnd = MinNodeSize-1;
	int MinNodeSizeHalf = MinNodeSize >> 1;

	CVec3Dint32 VLocalInt;
	VLocalInt[0] = RoundToInt(VLocal[0]) + MinNodeSizeHalf;
	VLocalInt[1] = RoundToInt(VLocal[1]) + MinNodeSizeHalf;
	VLocalInt[2] = RoundToInt(VLocal[2]) + MinNodeSizeHalf;

	CVec3Dint32 VGrid;
	VGrid[0] = VLocalInt[0] & ~MinNodeSizeAnd;
	VGrid[1] = VLocalInt[1] & ~MinNodeSizeAnd;
	VGrid[2] = VLocalInt[2] & ~MinNodeSizeAnd;

	CXR_LightGridPoint lSamples[8];

	for(int iSample = 0; iSample < 8; iSample++)
	{
		int NodeSize = m_RootSize;
		int iNode = 0;

		CVec3Dint32 VSample(VGrid);
		if (iSample & 1) VSample[0] += MinNodeSize;
		if (iSample & 2) VSample[1] += MinNodeSize;
		if (iSample & 4) VSample[2] += MinNodeSize;

		while(1)
		{
			NodeSize >>= 1;

			const CBSP_LightOcttreeNode& Node = m_lNodes[iNode];

			int iSubNode = 0;
			if (VSample[0] > NodeSize) { iSubNode |= 1; VSample[0] -= NodeSize; }
			if (VSample[1] > NodeSize) { iSubNode |= 2; VSample[1] -= NodeSize; }
			if (VSample[2] > NodeSize) { iSubNode |= 4; VSample[2] -= NodeSize; }
			if (!(Node.GetSubNodeTypes() & (1 << iSubNode)))
			{
				const CBSP_LightOcttreeNode& Leaf = m_lNodes[iSubNode + Node.GetSubNodes()];
				lSamples[iSample] = m_lPoints[Leaf.GetSubNodes()];
				break;
			}
			iNode = iSubNode + Node.GetSubNodes();
		}
	}

	// Do trilinear interpolation using the 8 samples we got.
	const int ymod = 2;
	const int zmod = 4;
	int fx = (VLocalInt[0] & MinNodeSizeAnd) * 256 / MinNodeSize;
	int fy = (VLocalInt[1] & MinNodeSizeAnd) * 256 / MinNodeSize;
	int fz = (VLocalInt[2] & MinNodeSizeAnd) * 256 / MinNodeSize;

	CPixel32 Light = CPixel32::TrilinearRGB(
		lSamples[0].GetLight(), lSamples[1].GetLight(), lSamples[ymod].GetLight(), lSamples[1+ymod].GetLight(), 
		lSamples[zmod].GetLight(), lSamples[1+zmod].GetLight(), lSamples[ymod+zmod].GetLight(), lSamples[1+ymod+zmod].GetLight(),
		fx, fy, fz);

	_LColor[0] = fp32(Light.GetR()) / 256.0f;
	_LColor[1] = fp32(Light.GetG()) / 256.0f;
	_LColor[2] = fp32(Light.GetB()) / 256.0f;

	int xx0 = LERPI2(lSamples[0].GetLightDirX(), lSamples[1].GetLightDirX(), fx);
	int xy0 = LERPI2(lSamples[0].GetLightDirY(), lSamples[1].GetLightDirY(), fx);
	int xz0 = LERPI2(lSamples[0].GetLightDirZ(), lSamples[1].GetLightDirZ(), fx);
	int xb0 = LERPI2(lSamples[0].GetLightBias(), lSamples[1].GetLightBias(), fx);
	int xx1 = LERPI2(lSamples[ymod].GetLightDirX(), lSamples[1+ymod].GetLightDirX(), fx);
	int xy1 = LERPI2(lSamples[ymod].GetLightDirY(), lSamples[1+ymod].GetLightDirY(), fx);
	int xz1 = LERPI2(lSamples[ymod].GetLightDirZ(), lSamples[1+ymod].GetLightDirZ(), fx);
	int xb1 = LERPI2(lSamples[ymod].GetLightBias(), lSamples[1+ymod].GetLightBias(), fx);

	int yx0 = LERPI2(xx0, xx1, fy);
	int yy0 = LERPI2(xy0, xy1, fy);
	int yz0 = LERPI2(xz0, xz1, fy);
	int yb0 = LERPI2(xb0, xb1, fy);

	int xx2 = LERPI2(lSamples[zmod].GetLightDirX(), lSamples[1+zmod].GetLightDirX(), fx);
	int xy2 = LERPI2(lSamples[zmod].GetLightDirY(), lSamples[1+zmod].GetLightDirY(), fx);
	int xz2 = LERPI2(lSamples[zmod].GetLightDirZ(), lSamples[1+zmod].GetLightDirZ(), fx);
	int xb2 = LERPI2(lSamples[zmod].GetLightBias(), lSamples[1+zmod].GetLightBias(), fx);
	int xx3 = LERPI2(lSamples[ymod+zmod].GetLightDirX(), lSamples[1+ymod+zmod].GetLightDirX(), fx);
	int xy3 = LERPI2(lSamples[ymod+zmod].GetLightDirY(), lSamples[1+ymod+zmod].GetLightDirY(), fx);
	int xz3 = LERPI2(lSamples[ymod+zmod].GetLightDirZ(), lSamples[1+ymod+zmod].GetLightDirZ(), fx);
	int xb3 = LERPI2(lSamples[ymod+zmod].GetLightBias(), lSamples[1+ymod+zmod].GetLightBias(), fx);

	int yx1 = LERPI2(xx2, xx3, fy);
	int yy1 = LERPI2(xy2, xy3, fy);
	int yz1 = LERPI2(xz2, xz3, fy);
	int yb1 = LERPI2(xb2, xb3, fy);

	_LDir[0] = fp32(LERPI2(yx0, yx1, fz)) / 127.0f;
	_LDir[1] = fp32(LERPI2(yy0, yy1, fz)) / 127.0f;
	_LDir[2] = fp32(LERPI2(yz0, yz1, fz)) / 127.0f;
	return fp32(LERPI2(yb0, yb1, fz)) / 256.0f;

/*
				_LDir[0] = fp32(LGP.m_LightDir[0]) / 127.0f;
				_LDir[1] = fp32(LGP.m_LightDir[1]) / 127.0f;
				_LDir[2] = fp32(LGP.m_LightDir[2]) / 127.0f;
				_LColor[0] = fp32(LGP.m_Light.GetR()) / 256.0f * 2.0;
				_LColor[1] = fp32(LGP.m_Light.GetG()) / 256.0f * 2.0;
				_LColor[2] = fp32(LGP.m_Light.GetB()) / 256.0f * 2.0;
		ConOut(CStrF("%s (%d, %d, %d) %d, Node %d, iLPG %d, Color %.8x", VLocal.GetString().Str(), 
			VLocalInt[0], VLocalInt[1], VLocalInt[2], NodeSize, 
			iSubNode + Node.m_iSubNodes, Leaf.m_iSubNodes, LGP.m_Light));
				return fp32(LGP.m_LightBias) / 256.0 * 2.0;

//	M_ASSERT(0, "ToDo");
	return 0;*/
}

CPixel32 CBSP_LightOcttree::Light_Vertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN)
{
	MAUTOSTRIP(CBSP_LightOcttree_Light_Vertex, CPixel32());
	M_ASSERT(0, "!");	// Should not be used
	return 0;
}

void CBSP_LightOcttree::Light_Array(const class CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CMat4Dfp32* _pWMat, int _Flags)
{
	MAUTOSTRIP(CBSP_LightOcttree_Light_Array, MAUTOSTRIP_VOID);
	M_ASSERT(0, "!");	// Should not be used
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP_CoreFace
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP_CoreFace::CBSP_CoreFace()
{
	MAUTOSTRIP(CBSP_CoreFace_ctor, MAUTOSTRIP_VOID);
	m_iiVertices = 0;

	m_nVertices = 0;
	m_iMapping = 0;
	m_iSurface = 0;
	m_Flags = 0;

	m_iPlane = 0;
	m_iBackMedium = 0;
	m_iiNormals = 0;

	m_iLightInfo = 0;
	m_nLightInfo = 0;

#ifndef PLATFORM_DOLPHIN
	m_iFrontMedium = 0;
	m_iiEdges = -1;
#endif
}

void CBSP_CoreFace::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP_CoreFace_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0x0203:
		{
			struct
			{
				int32 m_iiEdges;

				uint32 m_VertexIO;
				uint32 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint32 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				uint32 m_LightInfoIO;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iiEdges);
			::SwapLE(TempRead.m_VertexIO);
			::SwapLE(TempRead.m_iMapping);
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iiNormals);
			::SwapLE(TempRead.m_iBackMedium);
			::SwapLE(TempRead.m_iFrontMedium);
			::SwapLE(TempRead.m_LightInfoIO);

			m_iiEdges = TempRead.m_iiEdges;
			m_iiVertices = SHIFT_VALUE(TempRead.m_VertexIO, 0, 24);
			m_nVertices = SHIFT_VALUE(TempRead.m_VertexIO, 24, 8);
			m_iMapping = TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 0, 28);
			m_nLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 28, 4);
		}
		break;

	case 0x0202:
		{
			struct
			{
				int32 m_iiEdges;

				uint32 m_VertexIO;
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				uint32 m_LightInfoIO;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iiEdges);
			::SwapLE(TempRead.m_VertexIO);
			::SwapLE(TempRead.m_iMapping);
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iiNormals);
			::SwapLE(TempRead.m_iBackMedium);
			::SwapLE(TempRead.m_iFrontMedium);
			::SwapLE(TempRead.m_LightInfoIO);

			m_iiEdges = TempRead.m_iiEdges;
			m_iiVertices = SHIFT_VALUE(TempRead.m_VertexIO, 0, 24);
			m_nVertices = SHIFT_VALUE(TempRead.m_VertexIO, 24, 8);
			m_iMapping = TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 0, 28);
			m_nLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 28, 4);
		}
		break;

	case 0x0201:
		{
			struct
			{
				uint32 m_iiEdges;
				uint32 m_iiVertices;
				uint16 m_nVertices;
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;
				uint16 m_iPlane;
				uint16 m_iiNormals;
				uint16 m_iBackMedium;
				uint16 m_iFrontMedium;
				uint32 m_LightInfoIO;
			} TempRead;
			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iiEdges);
			::SwapLE(TempRead.m_iiVertices);
			::SwapLE(TempRead.m_nVertices);
			::SwapLE(TempRead.m_iMapping);
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iiNormals);
			::SwapLE(TempRead.m_iBackMedium);
			::SwapLE(TempRead.m_iFrontMedium);
			::SwapLE(TempRead.m_LightInfoIO);

			m_iiVertices = TempRead.m_iiVertices;
			m_nVertices = TempRead.m_nVertices;
			m_iMapping = TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iFrontMedium = TempRead.m_iFrontMedium;
			m_iLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 0, 28);
			m_nLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 28, 4);
			break;
		};
	case 0x0200 :
			_pF->ReadLE(m_iiEdges);

	case 0x0000 : 
		{
			struct
			{
				uint32 m_iiVertices;
				uint16 m_nVertices;
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;
				uint16 m_iPlane;
				uint16 m_iiNormals;
				uint16 m_iBackMedium;
				uint16 m_iFrontMedium;
				uint16 m_iLightInfo;
				uint16 m_nLightInfo;
			} TempRead;
			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iiVertices);
			::SwapLE(TempRead.m_nVertices);
			::SwapLE(TempRead.m_iMapping);
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iiNormals);
			::SwapLE(TempRead.m_iBackMedium);
			::SwapLE(TempRead.m_iFrontMedium);
			::SwapLE(TempRead.m_iLightInfo);
			::SwapLE(TempRead.m_nLightInfo);

			m_iiVertices = TempRead.m_iiVertices;
			m_nVertices = TempRead.m_nVertices;
			m_iMapping = TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iFrontMedium = TempRead.m_iFrontMedium;
			m_iLightInfo = TempRead.m_iLightInfo;
			m_nLightInfo = TempRead.m_nLightInfo;
		}
		break;

	default :
		Error_static("CBSP_CoreFace::Read", CStrF("Invalid version: %.4x", _Version));
	}
}

void CBSP_CoreFace::Write(CCFile* _pF)
{
	MAUTOSTRIP(CBSP_CoreFace_Write, MAUTOSTRIP_VOID);
#if defined( PLATFORM_DOLPHIN ) || defined( PLATFORM_PS2 )
	Error_static("CBSP_CoreFace::Write", "Not supported!");
#else
	_pF->WriteLE(m_iiEdges);

	uint32 Temp = m_iiVertices | (m_nVertices << 24);
	_pF->WriteLE(Temp);
	_pF->WriteLE(m_iMapping);
	_pF->WriteLE(m_iSurface);
	_pF->WriteLE(m_Flags);

	_pF->WriteLE(m_iPlane);
	_pF->WriteLE(m_iiNormals);
	_pF->WriteLE(m_iBackMedium);
	_pF->WriteLE(m_iFrontMedium);

	Temp = m_iLightInfo | (m_nLightInfo << 28);
	_pF->WriteLE(Temp);
#endif
}

#ifndef CPU_LITTLEENDIAN
void CBSP_CoreFace::SwapLE()
{
	MAUTOSTRIP(CBSP_CoreFace_SwapLE, MAUTOSTRIP_VOID);
#ifdef PLATFORM_DOLPHIN
	Error_static("CBSP_CoreFace::Write", "Not supported!");
#else
	::SwapLE(m_iiEdges);

	uint32 tmp = m_VertexIO;
	::SwapLE(tmp);
	m_iiVertices = SHIFT_VALUE(tmp, 0, 24);
	m_nVertices = SHIFT_VALUE(tmp, 24, 8);

	::SwapLE(m_iMapping);
	::SwapLE(m_iSurface);
	::SwapLE(m_Flags);

	::SwapLE(m_iPlane);
	::SwapLE(m_iiNormals);
	::SwapLE(m_iBackMedium);
	::SwapLE(m_iFrontMedium);

	tmp = m_LightInfoIO;
	::SwapLE(tmp);
	m_iLightInfo = SHIFT_VALUE(tmp, 0, 28);
	m_nLightInfo = SHIFT_VALUE(tmp, 28, 4);
#endif	
}
#endif

// -------------------------------------------------------------------
//  CXW_LMCluster
// -------------------------------------------------------------------
/*void CXW_LMCElement::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CXW_LMCElement_Read, NULL);
	if (_Version == 0x0200)
	{
		_pF->ReadLE(m_iLightInfo);
		_pF->ReadLE(m_nLightInfo);
	}
	else
		Error_static("CXW_LMCElement::Read", CStrF("Unsupported version %.4x", _Version));
}

void CXW_LMCElement::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CXW_LMCElement_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iLightInfo);
	_pF->WriteLE(m_nLightInfo);
}
*/
void CXW_LMCElement::Read(CCFile* _pF, int _Version)
{
	switch(_Version)
	{
	case 0x0200 : 
		{
			uint16 iLightInfo;
			uint8 nLightInfo;
			_pF->ReadLE(iLightInfo);
			_pF->ReadLE(nLightInfo);
			m_iLightInfo = iLightInfo;
			m_nLightInfo = nLightInfo;
		}
		break;

	case 0x0201 :
		{
			_pF->ReadLE(m_LightInfoIO);
		}
		break;

	default :
		Error_static("CXW_LMCElement::Read", CStrF("Unsupported version. (%.4x)", _Version));
	}
}

void CXW_LMCElement::Write(CCFile* _pF) const
{
	_pF->WriteLE(m_LightInfoIO);
}


void CXW_LMCluster::Read(CCFile* _pF)
{
	MAUTOSTRIP(CXW_LMCluster_Read, MAUTOSTRIP_VOID);
	uint16 Ver;
	_pF->ReadLE(Ver);

	if (Ver == 0x0200)
	{
		uint16 ElemVersion;
		_pF->ReadLE(ElemVersion);
		_pF->ReadLE(m_Width);
		_pF->ReadLE(m_Height);

		int32 Len; _pF->ReadLE(Len);
		m_lElements.SetLen(Len);
		for(int i = 0; i < m_lElements.Len(); i++)
			m_lElements[i].Read(_pF, ElemVersion);
	}
	else
		Error("Read", CStrF("Unsupported version %.4x", Ver));
}

void CXW_LMCluster::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CXW_LMCluster_Write, MAUTOSTRIP_VOID);
	uint16 Ver = XW_LMCLUSTER_VERSION;
	_pF->WriteLE(Ver);
	uint16 ElemVersion = XW_LMCLUSTERELEMENT_VERSION;
	_pF->WriteLE(ElemVersion);

	_pF->WriteLE(m_Width);
	_pF->WriteLE(m_Height);
	int32 Len = m_lElements.Len();
	_pF->WriteLE(Len);
	for(int i = 0; i < m_lElements.Len(); i++)
		m_lElements[i].Write(_pF);
}

// -------------------------------------------------------------------
//  CWObjectAttributes
// -------------------------------------------------------------------
CWObjectAttributes::CWObjectAttributes()
{
	MAUTOSTRIP(CWObjectAttributes_ctor, MAUTOSTRIP_VOID);
	m_nKeys = 0;
	m_pKeyData = NULL;
	m_KeySize = 0;
}

CWObjectAttributes::CWObjectAttributes(const TArray<CStr>& _lKeyNames, const TArray<CStr>& _lKeyValues)
{
	MAUTOSTRIP(CWObjectAttributes_ctor_2, MAUTOSTRIP_VOID);
	m_pKeyData = NULL;
	m_KeySize = 0;
	int nKeys = _lKeyNames.Len();

	int Size = nKeys*8;
	int i;
	for(i = 0; i < nKeys; i++)
	{
		Size += _lKeyNames[i].Len()+1;
		Size += _lKeyValues[i].Len()+1;

//		LogFile(CStrF("%d  ", Size) + _lKeyNames[i] + "  " + _lKeyValues[i]);
	}

	Size = (Size + 3) & 0xfffffffc;
	m_pKeyData = (int32*) DNew(char) char[Size];
	if (!m_pKeyData) MemError("-");

	int Pos = nKeys*8;
	for(i = 0; i < nKeys; i++)
	{
		m_pKeyData[i*2] = Pos;

		if (_lKeyNames[i].Len() > 0)
		{
			strcpy(&((char*)m_pKeyData)[Pos], (const char*) _lKeyNames[i]);
		}
		else
			((char*)m_pKeyData)[Pos] = 0;
		Pos += _lKeyNames[i].Len()+1;

		m_pKeyData[i*2+1] = Pos;
		if (_lKeyValues[i].Len() > 0)
		{
			strcpy(&((char*)m_pKeyData)[Pos], (const char*) _lKeyValues[i]);
		}
		else
			((char*)m_pKeyData)[Pos] = 0;
		Pos += _lKeyValues[i].Len()+1;

//		LogFile(CStrF("%d  ", Pos) + _lKeyNames[i] + "  " + _lKeyValues[i]);
	}
	if (Pos > Size) Error("-", CStrF("Internal error %d/%d", Pos, Size));
	m_nKeys = nKeys;
	m_KeySize = Size;
}

CWObjectAttributes::~CWObjectAttributes()
{
	MAUTOSTRIP(CWObjectAttributes_dtor, MAUTOSTRIP_VOID);
	if (m_pKeyData) { delete[] m_pKeyData; m_pKeyData = NULL; } 
}

int CWObjectAttributes::GetnKeys()
{
	MAUTOSTRIP(CWObjectAttributes_GetnKeys, 0);
	return m_nKeys;
}

int CWObjectAttributes::GetKeyIndex(CStr _Key)
{
	MAUTOSTRIP(CWObjectAttributes_GetKeyIndex, 0);
	for(int i = 0; i < m_nKeys; i++)
	{
		char* p = (char*)m_pKeyData;
		p += m_pKeyData[i*2];
		if (!_Key.Compare(p)) return i;
	}
	return -1;
}

CStr CWObjectAttributes::GetKeyName(int _iKey)
{
	MAUTOSTRIP(CWObjectAttributes_GetKeyName, CStr());
	if ((_iKey < 0) || (_iKey > m_nKeys))
		Error("GetKeyName", "Invalid index.");

	char* p = (char*)m_pKeyData;
	p += m_pKeyData[_iKey*2];
	return p;
}

CStr CWObjectAttributes::GetKeyValue(int _iKey)
{
	MAUTOSTRIP(CWObjectAttributes_GetKeyValue, CStr());
	if ((_iKey < 0) || (_iKey > m_nKeys))
		Error("GetKeyValue", "Invalid index.");

	char* p = (char*)m_pKeyData;
	p += m_pKeyData[_iKey*2+1];
	return p;
}

void CWObjectAttributes::Write(CCFile* _pFile) const
{
	MAUTOSTRIP(CWObjectAttributes_Write, MAUTOSTRIP_VOID);
	_pFile->Write(&m_nKeys, 4);
	_pFile->Write(&m_KeySize, 4);
	_pFile->Write((char*)m_pKeyData, m_KeySize);
}

void CWObjectAttributes::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CWObjectAttributes_Read, MAUTOSTRIP_VOID);
	if (m_pKeyData) { delete[] m_pKeyData; m_pKeyData = NULL; }

	_pFile->Read(&m_nKeys, 4);
	_pFile->Read(&m_KeySize, 4);
	m_pKeyData = (int32*) DNew(char) char[m_KeySize];
	_pFile->Read(m_pKeyData, m_KeySize);
}

