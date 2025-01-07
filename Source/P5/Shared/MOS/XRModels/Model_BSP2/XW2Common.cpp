#include "PCH.h"

#include "XW2Common.h"
#include "MFloat.h"

#include "../../XR/XRVBContext.h"

enum
{
	e_Platform_PC    = 0,
	e_Platform_Xbox  = 1,
	e_Platform_PS2   = 2,
	e_Platform_GC    = 3,
	e_Platform_Xenon = 4,
	e_Platform_PS3	 = 5,

	e_Platform_Default = e_Platform_PC,
};


//#pragma optimize("",off)
//#pragma inline_depth(0)

#define MODEL_BSP_TRACE 1 ? (void)0 : M_TRACEALWAYS
#define SHIFT_VALUE(v, _off, _mask) ((v >> _off) & ((1 << _mask) - 1))


void CBSP2_Node::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP2_Node_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0000 :
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			uint16 iNodeFront, iNodeBack, iNodeParent;
			_pF->ReadLE(iNodeFront);
			_pF->ReadLE(iNodeBack);
			_pF->ReadLE(iNodeParent);
			m_iNodeFront = iNodeFront;
			m_iNodeBack = iNodeBack;
			m_iNodeParent = iNodeParent;

			if (IsNode()) {
				uint16 iBoundNodeFront, iBoundNodeBack;
				_pF->ReadLE(iBoundNodeFront);
				_pF->ReadLE(iBoundNodeBack);
			} else {
				uint32 iiFaces;
				_pF->ReadLE(iiFaces);
				m_iiFaces = iiFaces;
			}

			uint16 Flags;
			_pF->ReadLE(Flags);
			m_Flags = Flags;
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
			uint16 iNodeFront, iNodeBack, iNodeParent;
			_pF->ReadLE(iNodeFront);
			_pF->ReadLE(iNodeBack);
			_pF->ReadLE(iNodeParent);
			m_iNodeFront = iNodeFront;
			m_iNodeBack = iNodeBack;
			m_iNodeParent = iNodeParent;

			if (IsNode()) {
				uint16 iBoundNodeFront, iBoundNodeBack;
				_pF->ReadLE(iBoundNodeFront);
				_pF->ReadLE(iBoundNodeBack);
			} else {
				uint32 iiFaces;
				_pF->ReadLE(iiFaces);
				m_iiFaces = iiFaces;
			}

			uint16 Flags;
			_pF->ReadLE(Flags);
			m_Flags = Flags;
			_pF->ReadLE(m_iPortalLeaf);
		}
		break;

	case 0x0201:
		{
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iNodeParent);
			uint32 tmp;
			_pF->ReadLE(tmp);
			m_iiFaces = SHIFT_VALUE(tmp, 0, 24);
			m_Flags = SHIFT_VALUE(tmp, 24, 8);
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane = iPlane;
			_pF->ReadLE(m_iPortalLeaf);
		}
		break;

	case 0x0202:
		{
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			_pF->ReadLE(m_iNodeParent);
			uint32 tmp;
			_pF->ReadLE(tmp);
			m_iiFaces = SHIFT_VALUE(tmp, 0, 24);
			m_Flags = SHIFT_VALUE(tmp, 24, 8);
			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iPortalLeaf);
			uint16 Padding;
			_pF->ReadLE(Padding);
		}
		break;

	default :
		Error_static("CBSP2_Node::Read", CStrF("Unsupported version %.4x", _Ver));
	}

}

void CBSP2_Node::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_Node_Write, MAUTOSTRIP_VOID);
	Error_static("Write", "Unsupported");
}

#ifndef CPU_LITTLEENDIAN
void CBSP2_Node::SwapLE()
{
	MAUTOSTRIP(CBSP2_Node_SwapLE, MAUTOSTRIP_VOID);
	::SwapLE(m_iNodeFront);
	::SwapLE(m_iNodeBack);
	::SwapLE(m_iNodeParent);
	uint32 tmp = m_NodeIO;
	::SwapLE(tmp);
	m_iiFaces = SHIFT_VALUE(tmp, 0, 24);
	m_Flags = SHIFT_VALUE(tmp, 24, 8);
	::SwapLE(m_iPlane);
	::SwapLE(m_iPortalLeaf);
}
#endif

// -------------------------------------------------------------------
CBSP2_Portal::CBSP2_Portal()
{
	MAUTOSTRIP(CBSP2_Portal_ctor, MAUTOSTRIP_VOID);
	m_iPlane = 0;
	m_iNodeFront = 0;
	m_iNodeBack = 0;
	m_iFogPortal = 0;
}

void CBSP2_Portal::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP2_Portal_Read, MAUTOSTRIP_VOID);
	CXR_IndexedPortal::Read(_pF);
	switch(_Version)
	{
	case 0x0000:
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane	= iPlane;
			uint16 iNodeFront, iNodeBack;
			_pF->ReadLE(iNodeFront);
			_pF->ReadLE(iNodeBack);
			m_iNodeFront = iNodeFront;
			m_iNodeBack = iNodeBack;
			uint16 iRPortal_Unused;
			_pF->ReadLE(iRPortal_Unused);
			_pF->ReadLE(m_iFogPortal);
		}
		break;

	case 0x0100:
		{
			uint16 iPlane;
			_pF->ReadLE(iPlane);
			m_iPlane	= iPlane;
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			uint16 iRPortal_Unused;
			_pF->ReadLE(iRPortal_Unused);
			_pF->ReadLE(m_iFogPortal);
		}
		break;

	case 0x0101:
		{
			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iNodeFront);
			_pF->ReadLE(m_iNodeBack);
			uint16 iRPortal_Unused;
			_pF->ReadLE(iRPortal_Unused);
			_pF->ReadLE(m_iFogPortal);
		}
		break;
	}
}

void CBSP2_Portal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_Portal_Write, MAUTOSTRIP_VOID);
	CXR_IndexedPortal::Write(_pF);
	_pF->WriteLE(m_iPlane);
	_pF->WriteLE(m_iNodeFront);
	_pF->WriteLE(m_iNodeBack);
	uint16 iRPortal_Unused = 0;
	_pF->WriteLE(iRPortal_Unused);
	_pF->WriteLE(m_iFogPortal);
}

// -------------------------------------------------------------------
void CBSP2_PortalLeaf::Read(CCFile* _pF, int _bReadVersion)
{
	MAUTOSTRIP(CBSP2_PortalLeaf_Read, MAUTOSTRIP_VOID);
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
		Error_static("CBSP2_PortalLeaf::Read", CStrF("Invalid portal-leaf version. %.4x", Version));
	}
}

void CBSP2_PortalLeaf::Write(CCFile* _pF, int _bReadVersion) const
{
	MAUTOSTRIP(CBSP2_PortalLeaf_Write, MAUTOSTRIP_VOID);
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
void CBSP2_FogPortalFace::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP2_FogPortalFace_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iiVertices);
	_pF->ReadLE(m_nVertices);
}

void CBSP2_FogPortalFace::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_FogPortalFace_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_iiVertices);
	_pF->WriteLE(m_nVertices);
}

// -------------------------------------------------------------------
CBSP2_FogPortal::CBSP2_FogPortal()
{
	MAUTOSTRIP(CBSP2_FogPortal_ctor, MAUTOSTRIP_VOID);
	m_iVertices = 0;
	m_nVertices = 0;
	m_iFaces = 0;
	m_nFaces = 0;
}

void CBSP2_FogPortal::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP2_FogPortal_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iVertices);
	_pF->ReadLE(m_nVertices);
	_pF->ReadLE(m_iFaces);
	_pF->ReadLE(m_nFaces);
}

void CBSP2_FogPortal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_FogPortal_Write, MAUTOSTRIP_VOID);
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

void CBSP2_LightMapInfo::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP2_LightMapInfo_Read, MAUTOSTRIP_VOID);
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
			LogFile(CStrF("ERROR: This map needs the lightmaps re-rendered, %d is not pow2", m_ScaleShift));
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
			LogFile(CStrF("ERROR: This map needs the lightmaps re-rendered, %d is not pow2", m_ScaleShift));
#endif
		m_ScaleShift = Log2(Scale);
	}
	else if ((_Version == 3) || (_Version == 4))
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
				LogFile(CStrF("ERROR: This map needs the lightmaps re-rendered, %d is not pow2", m_ScaleShift));
#endif
			m_ScaleShift = Log2(m_ScaleShift);
		}
	}
}

void CBSP2_LightMapInfo::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_LightMapInfo_Write, MAUTOSTRIP_VOID);
	m_IntensityScale.Write(_pF);
	_pF->WriteLE(m_ScaleShift);
	_pF->WriteLE(m_LMCOffsetXHalf);
	_pF->WriteLE(m_LMCOffsetYHalf);
	_pF->WriteLE(m_iLMC);
	_pF->WriteLE(m_LMCWidthHalf);
	_pF->WriteLE(m_LMCHeightHalf);
}

#ifndef CPU_LITTLEENDIAN
void CBSP2_LightMapInfo::SwapLE()
{
	MAUTOSTRIP(CBSP2_LightMapInfo_SwapLE, MAUTOSTRIP_VOID);
	m_IntensityScale.SwapLE();
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP2_LightMapContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP2_LightMapContainer::CBSP2_LMDesc::Read(CCFile* _pFile, int _Version)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_CBSP2_LMDesc_Read, MAUTOSTRIP_VOID);
	if (_Version == 0x0100)
	{
		_pFile->ReadLE(m_Width);
		_pFile->ReadLE(m_Height);
		_pFile->ReadLE(m_DataPos);
	}
	else
		Error_static("CBSP2_LightMapContainer::CBSP2_LMDesc::Read", CStrF("Unsupported version %.8x", _Version));
}

void CBSP2_LightMapContainer::CBSP2_LMDesc::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_CBSP2_LMDesc_Write, MAUTOSTRIP_VOID);
	_pFile->WriteLE(m_Width);
	_pFile->WriteLE(m_Height);
	_pFile->WriteLE(m_DataPos);
}

CBSP2_LightMapContainer::CBSP2_LightMapContainer()
{
	MAUTOSTRIP(CBSP2_LightMapContainer_ctor, MAUTOSTRIP_VOID);
	m_Format = 0;
}

CBSP2_LightMapContainer::~CBSP2_LightMapContainer()
{
	MAUTOSTRIP(CBSP2_LightMapContainer_dtor, MAUTOSTRIP_VOID);
}

void CBSP2_LightMapContainer::Create(TArray<spCImage> _lspLightMaps, int _Format)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_Create, MAUTOSTRIP_VOID);
	int nLM = _lspLightMaps.Len();
	TArray<spCImage> lspLMTemp;
	m_lLMDesc.SetLen(nLM);
	lspLMTemp.SetLen(nLM);

	// Convert images and calculate total size of image data.
	int SizeTotal = 0;
	{
		for(int i = 0; i < nLM; i++)
		{
			lspLMTemp[i] = _lspLightMaps[i]->Convert(_Format);
			SizeTotal += lspLMTemp[i]->GetSize();
		}
	}

	// Stuff all images into the one data block
	m_lLMData.SetLen(SizeTotal);
	{
		int Pos = 0;
		for(int i = 0; i < nLM; i++)
		{
			// We do it this way because it will work even if the source's modulo != width*pixelsize
			CImage Ref; Ref.CreateReference(lspLMTemp[i]->GetWidth(), lspLMTemp[i]->GetHeight(), lspLMTemp[i]->GetFormat(), IMAGE_MEM_IMAGE, lspLMTemp[i]->GetWidth() * lspLMTemp[i]->GetPixelSize(), &m_lLMData[Pos], NULL);
			Ref.Blt(Ref.GetClipRect(), *lspLMTemp[i], 0, CPnt(0,0));

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

TArray<spCImage> CBSP2_LightMapContainer::GetLightMapImages(int _Format)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_GetLightMapImages, TArray<spCImage>());
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

void CBSP2_LightMapContainer::GetLightMap(int _iLightMap, CImage& _RefImage)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_GetLightMap, MAUTOSTRIP_VOID);
	const CBSP2_LMDesc& LMDesc = m_lLMDesc[_iLightMap];
	_RefImage.CreateReference(LMDesc.m_Width, LMDesc.m_Height, m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
//	_RefImage.CreateReference(LMDesc.GetWidth(), LMDesc.GetHeight(), m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
}

void CBSP2_LightMapContainer::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_Read, MAUTOSTRIP_VOID);
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

void CBSP2_LightMapContainer::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP2_LightMapContainer_Write, MAUTOSTRIP_VOID);
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
| CBSP2_CoreFace
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP2_CoreFace::CBSP2_CoreFace()
{
	MAUTOSTRIP(CBSP2_CoreFace_ctor, MAUTOSTRIP_VOID);
	m_iiVertices = 0;

	m_nVertices = 0;
	m_iMapping = 0;
	m_iSurface = 0;
	m_Flags = 0;

	m_iPlane = 0;
	m_iBackMedium = 0;
	m_iiNormals = 0;

#ifndef PLATFORM_DOLPHIN
	m_iiEdges = ~0;
#endif
}

void CBSP2_CoreFace::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP2_CoreFace_Read, MAUTOSTRIP_VOID);
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

			m_iiEdges	= TempRead.m_iiEdges;
			m_iDiameterClass	 = 0;
			m_iiVertices = SHIFT_VALUE(TempRead.m_VertexIO, 0, 24);
			m_nVertices = SHIFT_VALUE(TempRead.m_VertexIO, 24, 8);
			m_iMapping	= TempRead.m_iMapping;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iiNormals = TempRead.m_iiNormals;
			m_iLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 0, 28);
			m_nLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 28, 4);
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
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

			m_iiEdges	= TempRead.m_iiEdges;
			m_iDiameterClass	 = 0;
			m_iiVertices = SHIFT_VALUE(TempRead.m_VertexIO, 0, 24);
			m_nVertices = SHIFT_VALUE(TempRead.m_VertexIO, 24, 8);
			m_iMapping	= TempRead.m_iMapping;
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
				int32 m_iiEdges;

				uint32 m_iiVertices;	// Index to first vertex-index
				uint16 m_nVertices;		// Number vertices
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint16 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				union
				{
					struct
					{
						uint32 m_iLightInfo:28;	// Index of first lightmap/lightvertices
						uint32 m_nLightInfo:4;
					};
					uint32 m_LightInfoIO;
				};
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));

			::SwapLE(TempRead.m_iiEdges);
			m_iiEdges = TempRead.m_iiEdges;
			m_iDiameterClass = 0;
			::SwapLE(TempRead.m_iiVertices);
			::SwapLE(TempRead.m_nVertices);
			m_iiVertices = TempRead.m_iiVertices;
			m_nVertices = TempRead.m_nVertices;
			::SwapLE(TempRead.m_iMapping);
			m_iMapping = TempRead.m_iMapping;
			::SwapLE(TempRead.m_iSurface);
			m_iSurface = TempRead.m_iSurface;
			::SwapLE(TempRead.m_Flags);
			m_Flags = TempRead.m_Flags;

			::SwapLE(TempRead.m_iPlane);
			m_iPlane = TempRead.m_iPlane;
			::SwapLE(TempRead.m_iiNormals);
			m_iiNormals = TempRead.m_iiNormals;
			::SwapLE(TempRead.m_iBackMedium);
			m_iBackMedium = TempRead.m_iBackMedium;
			::SwapLE(TempRead.m_LightInfoIO);
			m_iLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 0, 28);
			m_nLightInfo = SHIFT_VALUE(TempRead.m_LightInfoIO, 28, 4);
			break;
		};
	case 0x0200 :
		{
			struct
			{
				int32 m_iiEdges;

				uint32 m_iiVertices;	// Index to first vertex-index
				uint16 m_nVertices;		// Number vertices
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint16 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				uint16 m_iLightInfo;	// Index of first lightmap/lightvertices
				uint16 m_nLightInfo;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));

			::SwapLE(TempRead.m_iiEdges);
			m_iiEdges = TempRead.m_iiEdges;
			m_iDiameterClass = 0;
			::SwapLE(TempRead.m_iiVertices);
			::SwapLE(TempRead.m_nVertices);
			m_iiVertices = TempRead.m_iiVertices;
			m_nVertices = TempRead.m_nVertices;
			::SwapLE(TempRead.m_iMapping);
			m_iMapping = TempRead.m_iMapping;
			::SwapLE(TempRead.m_iSurface);
			m_iSurface = TempRead.m_iSurface;
			::SwapLE(TempRead.m_Flags);
			m_Flags = TempRead.m_Flags;

			::SwapLE(TempRead.m_iPlane);
			m_iPlane = TempRead.m_iPlane;
			::SwapLE(TempRead.m_iiNormals);
			m_iiNormals = TempRead.m_iiNormals;
			::SwapLE(TempRead.m_iBackMedium);
			m_iBackMedium = TempRead.m_iBackMedium;
		}
		break;

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
			m_iLightInfo = TempRead.m_iLightInfo;
			m_nLightInfo = TempRead.m_nLightInfo;
		}
		break;

	default :
		Error_static("CBSP2_CoreFace::Read", CStrF("Invalid version: %.4x", _Version));
	}
}

void CBSP2_CoreFace::Write(CCFile* _pF)
{
	MAUTOSTRIP(CBSP2_CoreFace_Write, MAUTOSTRIP_VOID);
	Error_static("CBSP2_CoreFace::Write", "Not supported!");
}

#ifndef CPU_LITTLEENDIAN
void CBSP2_CoreFace::SwapLE()
{
	MAUTOSTRIP(CBSP2_CoreFace_SwapLE, MAUTOSTRIP_VOID);
	Error_static("CBSP2_CoreFace::Write", "Not supported!");
}
#endif

CBSP2_LightData::~CBSP2_LightData()
{
	m_spSLCIBC = NULL;
}

void CBSP2_LightData::Read(CDataFile* _pDFile)
{
	// FACELIGHTS
	{
		if (!_pDFile->GetNext("FACELIGHTS")) 
			Error("Read", "No FACELIGHTS entry.");

		m_liFaceLights.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->ReadLE(m_liFaceLights.GetBasePtr(), m_liFaceLights.Len());
	}

	// LIGHT
	{
		if (!_pDFile->GetNext("LIGHTS"))
			Error("Read", "No LIGHTS entry.");

		int nLights = _pDFile->GetUserData();
		int Ver = _pDFile->GetUserData2();
		m_lLights.SetLen(nLights); 
		CXR_Light* pL = m_lLights.GetBasePtr();

		// Fall-back path
		CCFile* pF = _pDFile->GetFile();
		for (int iL=0; iL < nLights; iL++)
		{
			pL[iL].Read(pF, Ver);
		}
	}

	// SV
	{
		if (!_pDFile->GetNext("SV")) 
			Error("Read", "No SV entry.");

		int nSV = _pDFile->GetUserData();
		m_lSV.SetLen(nSV); 
		int nVer = _pDFile->GetUserData2();

		if(nVer == XW2_SHADOWVOLUME_VER)
		{
			if (m_lSV.ListSize() != _pDFile->GetEntrySize())
				Error("Read", "SV entry size missmatch.");

			_pDFile->GetFile()->Read(m_lSV.GetBasePtr(), m_lSV.ListSize());

#ifdef CPU_BIGENDIAN
			CBSP2_ShadowVolume* pSV = m_lSV.GetBasePtr();
			for(int i = 0; i < nSV; i++)
				pSV[i].SwapLE();
#endif
		}
		else
		{
			CBSP2_ShadowVolume* pSV = m_lSV.GetBasePtr();
			for(int i = 0; i < nSV; i++)
			{
				pSV[i].Read(_pDFile->GetFile(), nVer);
			}
		}
	}

	// SVFIRSTPL
	{
		if (!_pDFile->GetNext("SVFIRSTPL")) 
			Error("Read", "No SVFIRSTPL entry.");

		m_liSVFirstPL.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSVFirstPL.GetBasePtr(), m_liSVFirstPL.ListSize());
		SwitchArrayLE_uint16(m_liSVFirstPL.GetBasePtr(), m_liSVFirstPL.Len());
	}

	// SVPLCOUNT
	{
		if (!_pDFile->GetNext("SVPLCOUNT")) 
			Error("Read", "No SVPLCOUNT entry.");

		m_lnSVPL.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_lnSVPL.GetBasePtr(), m_lnSVPL.ListSize());
		SwitchArrayLE_uint16(m_lnSVPL.GetBasePtr(), m_lnSVPL.Len());
	}

	// SVFIRSTLIGHT
	{
		if (!_pDFile->GetNext("SVFIRSTLIGHT")) 
			Error("Read", "No SVFIRSTLIGHT entry.");

		m_liSVFirstLight.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSVFirstLight.GetBasePtr(), m_liSVFirstLight.ListSize());
		SwitchArrayLE_uint16(m_liSVFirstLight.GetBasePtr(), m_liSVFirstLight.Len());
	}

#ifdef PLATFORM_CONSOLE
	bool bDelayLoad = true;
#else
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif
	// SVVERTICES
	{
		if (!_pDFile->GetNext("SVVERTICES")) 
			Error("Read", "No SVVERTICES entry.");

		if (bDelayLoad)
		{
			m_FileName = _pDFile->GetFile()->GetFileName();
			m_FileOffsetVertices = _pDFile->GetFile()->Pos();
		}
#ifndef PLATFORM_CONSOLE
		else
		{
			m_lSVVertices.SetLen(_pDFile->GetUserData()); 
			_pDFile->GetFile()->Read(m_lSVVertices.GetBasePtr(), m_lSVVertices.ListSize());
			SwitchArrayLE_uint32((uint32*)m_lSVVertices.GetBasePtr(), m_lSVVertices.Len()*3);
		}
#endif
	}

	// SVPRIM
	{
		if (!_pDFile->GetNext("SVPRIM")) 
			Error("Read", "No SVPRIM entry.");

		if (bDelayLoad)
		{
			m_FileOffsetPrim = _pDFile->GetFile()->Pos();
		}
#ifndef PLATFORM_CONSOLE
		else
		{
			m_liSVPrim.SetLen(_pDFile->GetUserData()); 
			_pDFile->GetFile()->Read(m_liSVPrim.GetBasePtr(), m_liSVPrim.ListSize());
			SwitchArrayLE_uint16(m_liSVPrim.GetBasePtr(), m_liSVPrim.Len());
		}
#endif
	}

	// SLC
	{
		if (!_pDFile->GetNext("SLC")) 
			Error("Read", "No SLC entry.");

		int nVersion = _pDFile->GetUserData2();
		int nSLC = _pDFile->GetUserData();
		m_lSLC.SetLen(nSLC); 

		if(nVersion == XW_SLC_VERSION)
		{
			if (m_lSLC.ListSize() != _pDFile->GetEntrySize())
				Error("Read", "SLC entry size missmatch.");

			_pDFile->GetFile()->Read(m_lSLC.GetBasePtr(), m_lSLC.ListSize());

#ifdef CPU_BIGENDIAN
			CBSP2_SLCluster* pSLC = m_lSLC.GetBasePtr();
			for(int i = 0; i < nSLC; i++)
				pSLC[i].SwapLE();
#endif
		}
		else
		{
			// Not latest version, read instance by instance
			CBSP2_SLCluster* pSLC = m_lSLC.GetBasePtr();
			for(int i = 0; i < nSLC; i++)
				pSLC[i].Read(_pDFile->GetFile(), nVersion);
		}
	}

	// SLCFACES
	{
		if (!_pDFile->GetNext("SLCFACES")) 
			Error("Read", "No SLCFACES entry.");

		m_liSLCFaces.SetLen(_pDFile->GetUserData());
		int XWSLCVersion = _pDFile->GetUserData2();
		if(XWSLCVersion == 0x0100)
		{
			_pDFile->GetFile()->ReadLE(m_liSLCFaces.GetBasePtr(), m_liSLCFaces.Len());
		}
		else
		{
			int nLen = _pDFile->GetUserData();

			uint32* pF = m_liSLCFaces.GetBasePtr();
			for(int i = 0; i < nLen; i += 256)
			{
				uint16 liTemp[256];
				int nRead = Min(256, nLen - i);
				_pDFile->GetFile()->ReadLE(liTemp, nRead);
				for(int j = 0; j < nRead; j++)
					pF[i + j] = liTemp[j];
			}
		}
	}

	// SHADERQUEUE
	{
		if (!_pDFile->GetNext("SHADERQUEUE")) 
			Error("Read", "No SHADERQUEUE entry.");

		int nSQ = _pDFile->GetUserData();
		m_lShaderQueue.SetLen(nSQ); 

		if (m_lShaderQueue.ListSize() != _pDFile->GetEntrySize())
			Error("Read", "ShaderQueue entry size missmatch.");

		_pDFile->GetFile()->Read(m_lShaderQueue.GetBasePtr(), m_lShaderQueue.ListSize());

#ifdef CPU_BIGENDIAN
		CBSP2_ShaderQueueElement* pSQ = m_lShaderQueue.GetBasePtr();
		for(int i = 0; i < nSQ; i++)
			pSQ[i].SwapLE();
#endif

	}

	// PLANES
	{
		if (!_pDFile->GetNext("PLANES")) 
			Error("Read", "No PLANES entry.");

		int nPlanes = _pDFile->GetUserData();
		m_lPlanes.SetLen(nPlanes); 
		_pDFile->GetFile()->Read(m_lPlanes.GetBasePtr(), m_lPlanes.ListSize());

#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < nPlanes; i++)
			m_lPlanes[i].SwapLE();
#endif
	}

}

void CBSP2_LightData::Write(CDataFile* _pDFile) const
{
#ifdef PLATFORM_CONSOLE
	Error("Write", "Not supported on console");
#else
	// FACELIGHTS
	_pDFile->BeginEntry("UNIFIEDLIGHTDATA");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();
	{
		// FACELIGHTS
		_pDFile->BeginEntry("FACELIGHTS");
		_pDFile->GetFile()->WriteLE(m_liFaceLights.GetBasePtr(), m_liFaceLights.Len());
		_pDFile->EndEntry(m_liFaceLights.Len());

		// LIGHTS
		{
			_pDFile->BeginEntry("LIGHTS");
			for(int i = 0; i < m_lLights.Len(); i++)
				m_lLights[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lLights.Len(), CXR_LIGHT_VERSION);
		}

		// SV
		{
			_pDFile->BeginEntry("SV");
			for(int i = 0; i < m_lSV.Len(); i++)
				m_lSV[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lSV.Len(), XW2_SHADOWVOLUME_VER);
		}

		// SVFIRSTPL
		{
#ifdef CPU_BIGENDIAN
#error "FixMyByteOrderWrite"
#endif
			_pDFile->BeginEntry("SVFIRSTPL");
			_pDFile->GetFile()->Write(m_liSVFirstPL.GetBasePtr(), m_liSVFirstPL.ListSize());
			_pDFile->EndEntry(m_liSVFirstPL.Len(), 0);
		}
		// SVPLCOUNT
		{
#ifdef CPU_BIGENDIAN
#error "FixMyByteOrderWrite"
#endif
			_pDFile->BeginEntry("SVPLCOUNT");
			_pDFile->GetFile()->Write(m_lnSVPL.GetBasePtr(), m_lnSVPL.ListSize());
			_pDFile->EndEntry(m_lnSVPL.Len(), 0);
		}
		// SVFIRSTLIGHT
		{
#ifdef CPU_BIGENDIAN
#error "FixMyByteOrderWrite"
#endif
			_pDFile->BeginEntry("SVFIRSTLIGHT");
			_pDFile->GetFile()->Write(m_liSVFirstLight.GetBasePtr(), m_liSVFirstLight.ListSize());
			_pDFile->EndEntry(m_liSVFirstLight.Len(), 0);
		}
		// SVVERTICES
		{
			_pDFile->BeginEntry("SVVERTICES");
			for(int i = 0; i < m_lSVVertices.Len(); i++)
				m_lSVVertices[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lSVVertices.Len(), 0);
		}

		// SVPRIM
		{
#ifdef CPU_BIGENDIAN
#error "FixMyByteOrderWrite"
#endif
			_pDFile->BeginEntry("SVPRIM");
			_pDFile->GetFile()->Write(m_liSVPrim.GetBasePtr(), m_liSVPrim.ListSize());
			_pDFile->EndEntry(m_liSVPrim.Len(), 0);
		}

		// SLC
		{
			_pDFile->BeginEntry("SLC");
			for(int i = 0; i < m_lSLC.Len(); i++)
				m_lSLC[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lSLC.Len(), XW_SLC_VERSION);
		}

		// SLCFACES
		{
#ifdef CPU_BIGENDIAN
#error "FixMyByteOrderWrite"
#endif
			_pDFile->BeginEntry("SLCFACES");
			_pDFile->GetFile()->Write(m_liSLCFaces.GetBasePtr(), m_liSLCFaces.ListSize());
			_pDFile->EndEntry(m_liSLCFaces.Len(), XW_SLCFACE_VERSION);
		}

		// SHADERQUEUE
		{
			_pDFile->BeginEntry("SHADERQUEUE");
			for(int i = 0; i < m_lShaderQueue.Len(); i++)
				m_lShaderQueue[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lShaderQueue.Len(), 0);
		}

		// PLANES
		{
			_pDFile->BeginEntry("PLANES");
			for(int i = 0; i < m_lPlanes.Len(); i++)
				m_lPlanes[i].Write(_pDFile->GetFile());
			_pDFile->EndEntry(m_lPlanes.Len(), 0);
		}

	}
	_pDFile->EndSubDir();
#endif
}

/*
TPtr<CBSP2_LightData> CBSP2_LightData::FindAndRead(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP2_LightData_FindAndRead, NULL);

	_pDFile->PushPosition();
	if (_pDFile->GetNext("UNIFIEDLIGHTDATA"))
	{
		if (!_pDFile->GetSubDir())
			Error_static("CBSP2_LightData::FindAndRead", "Invalid entry.");

		TPtr<CBSP2_LightData> spLightData = MNew(CBSP2_LightData);
		if (!spLightData)
			Error_static("CBSP2_LightOcttree::FindAndRead", "Out of memory.");

		spLightData->Read(_pDFile);
		_pDFile->PopPosition();
		return spLightData;
	}

	_pDFile->PopPosition();

	return NULL;
}
*/

template <int tPlatform>
static void SetWantTransform(CRC_BuildVertexBuffer& _VB, CBSP2_LightData* _pLD, int _iLocal);

template <>
static void SetWantTransform<e_Platform_Default>(CRC_BuildVertexBuffer& _VB, CBSP2_LightData* _pLD, int _iLocal)
{
}

template <>
static void SetWantTransform<e_Platform_Xenon>(CRC_BuildVertexBuffer& _VB, CBSP2_LightData* _pLD, int _iLocal)
{
	if(_VB.m_lpVReg[CRC_VREG_POS])
	{
			CVec3Dfp32 Min3D;
			CVec3Dfp32 Max3D;
			CVec3Dfp32::GetMinBoundBox((CVec3Dfp32 *)_pLD->m_lVBIds[_iLocal].m_pData, Min3D, Max3D, _VB.m_nV);
			fp32 MaxRange = Max3D.k[0] - Min3D.k[0];
			MaxRange = Max(MaxRange, Max3D.k[1] - Min3D.k[1]);
			MaxRange = Max(MaxRange, Max3D.k[2] - Min3D.k[2]);
			fp32 MinUnitAccurency = 8.0f;
			CRC_VRegTransform Transform;
			Transform.m_Offset = Min3D;
			Transform.m_Offset.k[3] = 0.0f;
			Transform.m_Scale = (Max3D - Min3D);

			if (MaxRange <= 1024.0f / MinUnitAccurency)
			{
				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f, 1.0f, 1.0f, 1.0f), Transform.m_Scale);
				_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NU3_P32);
				_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
				MODEL_BSP_TRACE("Shadow VB Position using NU3_P32\n");
			}
			else if (MaxRange <= 65535.0f / MinUnitAccurency)
			{
				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
				_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_V3_U16);
				_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
				MODEL_BSP_TRACE("Shadow VB Position using V3_I16\n");
			}
			else
				MODEL_BSP_TRACE("Shadow VB Position using V3_F32\n");
	}
}

template <>
static void SetWantTransform<e_Platform_PS3>(CRC_BuildVertexBuffer& _VB, CBSP2_LightData* _pLD, int _iLocal)
{
	if(_VB.m_lpVReg[CRC_VREG_POS])
	{
			CVec3Dfp32 Min3D;
			CVec3Dfp32 Max3D;
			CVec3Dfp32::GetMinBoundBox((CVec3Dfp32 *)_pLD->m_lVBIds[_iLocal].m_pData, Min3D, Max3D, _VB.m_nV);
			fp32 MaxRange = Max3D.k[0] - Min3D.k[0];
			MaxRange = Max(MaxRange, Max3D.k[1] - Min3D.k[1]);
			MaxRange = Max(MaxRange, Max3D.k[2] - Min3D.k[2]);
			fp32 MinUnitAccurency = 8.0f;
			CRC_VRegTransform Transform;
			Transform.m_Offset = Min3D;
			Transform.m_Offset.k[3] = 0.0f;
			Transform.m_Scale = (Max3D - Min3D);

			if (MaxRange <= 1024.0f / MinUnitAccurency)
			{
				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f, 1.0f, 1.0f, 1.0f), Transform.m_Scale);
				_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NU3_P32);
				_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
				MODEL_BSP_TRACE("Shadow VB Position using NU3_P32\n");
			}
			else if (MaxRange <= 65535.0f / MinUnitAccurency)
			{
				Transform.m_Scale.CompMul(CVec4Dfp32(1.0f/65535.0f, 1.0f/65535.0f, 1.0f/65535.0f, 1.0f), Transform.m_Scale);
				_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_V3_U16);
				_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
				MODEL_BSP_TRACE("Shadow VB Position using V3_I16\n");
			}
			else
				MODEL_BSP_TRACE("Shadow VB Position using V3_F32\n");
	}
}


void CBSP2_LightData::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{	
#ifndef PLATFORM_CONSOLE
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
	if (!bDelayLoad)
	{
		_VB.Clear();
		_VB.m_nV = m_lSV[_iLocal].m_nVertices;
		_VB.Geometry_VertexArray(&m_lSVVertices[m_lSV[_iLocal].m_iVBase]);

		_VB.m_PrimType = CRC_RIP_TRIANGLES;
		_VB.m_nPrim = m_lSV[_iLocal].m_nTriangles;
		_VB.m_piPrim = &m_liSVPrim[m_lSV[_iLocal].m_iTriBase];
	}
	else
#endif
	{
		if (_Flags & VB_GETFLAGS_FALLBACK)
			m_lVBIds[_iLocal].m_bNoRelease = 1;

		if (!m_lVBIds[_iLocal].m_pData)
		{
			void* pData = M_ALLOC(m_lSV[_iLocal].m_nVertices * 12 + m_lSV[_iLocal].m_nTriangles * 6);
			if(!pData)
				MemError("CBSP2_LightData::Get");
			m_lVBIds[_iLocal].m_pData = pData;

			CCFile File;
			File.Open(m_FileName, CFILE_READ | CFILE_BINARY);
			File.Seek(m_FileOffsetVertices + m_lSV[_iLocal].m_iVBase * 12);
			File.ReadLE((fp32 *)m_lVBIds[_iLocal].m_pData, m_lSV[_iLocal].m_nVertices * 3);
			File.Seek(m_FileOffsetPrim + m_lSV[_iLocal].m_iTriBase * 2);
			File.ReadLE((uint16 *)((uint8 *)m_lVBIds[_iLocal].m_pData + m_lSV[_iLocal].m_nVertices * 12), m_lSV[_iLocal].m_nTriangles * 3);
		}

		_VB.Clear();
		_VB.m_nV = m_lSV[_iLocal].m_nVertices;
		if ((CVec3Dfp32 *)m_lVBIds[_iLocal].m_pData)
		{
			_VB.Geometry_VertexArray((CVec3Dfp32 *)m_lVBIds[_iLocal].m_pData);
		}
	//	MODEL_BSP_TRACE("%d Shadow\n",_VB.m_nV);

		_VB.m_PrimType = CRC_RIP_TRIANGLES;
		_VB.m_nPrim = m_lSV[_iLocal].m_nTriangles;
		_VB.m_piPrim = (uint16 *)((uint8 *)m_lVBIds[_iLocal].m_pData + m_lSV[_iLocal].m_nVertices * 12);
	}

	switch (D_MPLATFORM)
	{
	case e_Platform_Xenon: SetWantTransform<e_Platform_Xenon>(_VB, this, _iLocal); break;
	case e_Platform_PS3:   SetWantTransform<e_Platform_PS3>(_VB, this, _iLocal); break;
	default:               SetWantTransform<e_Platform_Default>(_VB, this, _iLocal); break;
	}
}


void CBSP2_LightData::Release(int _iLocal)
{
#ifdef PLATFORM_CONSOLE
	bool bDelayLoad = true;
#else
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif
	if (bDelayLoad)
	{
		if (m_lVBIds[_iLocal].m_pData && !m_lVBIds[_iLocal].m_bNoRelease)
		{
			MRTC_MemFree(m_lVBIds[_iLocal].m_pData);
			m_lVBIds[_iLocal].m_pData = NULL;
		}
	}
}

int CBSP2_LightData::GetID(int _iLocal)
{
	return m_lVBIds[_iLocal].m_VBID;
}

CFStr CBSP2_LightData::GetName(int _iLocal)
{
	return CFStrF("BSP2:%s:Shadow:%04d", m_FileName.Str(), _iLocal);
}

int CBSP2_LightData::GetNumLocal()
{
	return m_lVBIds.Len();
}

void CBSP2_LightData::AllocVBIds(CXR_Model_BSP2* _pModel)
{
	FreeVBIds();

	{
		m_lVBIds.SetLen(m_lSV.Len());
		for (int i = 0; i < m_lSV.Len(); ++i)
		{
			if (m_lSV[i].m_nVertices && m_lSV[i].m_nTriangles)
				m_lVBIds[i].m_VBID = m_pVBCtx->AllocID(m_iVBC, i);
		}
	}

	{
		m_spSLCIBC = MNew(CBSP2_SLCIBContainer);
		if (!m_spSLCIBC)
			MemError("AllocVBIds");
		m_spSLCIBC->Create(this, _pModel);
	}
}

void CBSP2_LightData::FreeVBIds()
{
#ifdef PLATFORM_CONSOLE
	bool bDelayLoad = true;
#else
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif
	for (int i = 0; i < m_lVBIds.Len(); ++i)
	{
		if (m_lVBIds[i].m_VBID != -1)
		{
			m_pVBCtx->FreeID(m_lVBIds[i].m_VBID);
			m_lVBIds[i].m_VBID = -1;
		}
		if (bDelayLoad)
		{
			if (m_lVBIds[i].m_pData)
			{
				MRTC_GetMemoryManager()->Free(m_lVBIds[i].m_pData);
				m_lVBIds[i].m_pData = NULL;
			}
		}
	}

	m_spSLCIBC = NULL;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP2_LightFieldOcttreeNode
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP2_LightFieldOcttreeNode::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttreeNode_Read, MAUTOSTRIP_VOID);
	if (_Ver == 0x0100)
	{
		_pF->ReadLE(m_Data.m_BitUnion);
	}
	else
		Error_static("CBSP2_LightFieldOcttreeNode::Read", CStrF("Unsupported version %.4x", _Ver));
}

void CBSP2_LightFieldOcttreeNode::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP2_LightFieldOcttreeNode_Write, MAUTOSTRIP_VOID);
	_pF->WriteLE(m_Data.m_BitUnion);
}

#ifndef CPU_LITTLEENDIAN
void CBSP2_LightFieldOcttreeNode::SwapLE()
{
	MAUTOSTRIP(CBSP2_LightFieldOcttreeNode_SwapLE, MAUTOSTRIP_VOID);
	m_Data.SwapLE();
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP2_LightFieldOcttree
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP2_LightFieldOcttree::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP_LightOcttree_Read, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();
	if (!_pDFile->GetNext("LIGHTFIELDOCTTREE"))
		Error("Read", "LIGHTFIELDOCTTREE entry not found.");

	if (!_pDFile->GetSubDir())
		Error("Read", "Invalid LIGHTFIELDOCTTREE entry.");

	// TREEINFO
	if (!_pDFile->GetNext("TREEINFO"))
		Error("Read", "TREEINFO entry not found.");

	if (_pDFile->GetUserData() == 0x0100)
	{
		pF->ReadLE(m_RootSize);
		pF->ReadLE(m_MinLeafSize);
		m_RootPos.Read(pF);
	}
	else
		Error("Read", CStrF("Unsupported TREEINFO version %.4x", _pDFile->GetUserData()));

	// NODES
	if (!_pDFile->GetNext("NODES"))
		Error("Read", "NODES entry not found.");

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
	if (!_pDFile->GetNext("ELEMENTS"))
		Error("Read", "ELEMENTS entry not found.");

	{
		int ElemVer = _pDFile->GetUserData2();
		m_lElements.SetLen(_pDFile->GetUserData());
		_pDFile->ReadArray(m_lElements.GetBasePtr(), ElemVer);
	}

	_pDFile->GetParent();
}

void CBSP2_LightFieldOcttree::Write(CDataFile* _pDFile) const
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_Write, MAUTOSTRIP_VOID);
	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginEntry("LIGHTFIELDOCTTREE");
	_pDFile->EndEntry(XR_LIGHTFIELDOCTTREE_VERSION);
	_pDFile->BeginSubDir();

	{
		// TREEINFO
		_pDFile->BeginEntry("TREEINFO");
		{
			pF->WriteLE(m_RootSize);
			pF->WriteLE(m_MinLeafSize);
			m_RootPos.Write(pF);
		}
		_pDFile->EndEntry(XR_LIGHTFIELDOCTTREE_VERSION);

		// NODES
		_pDFile->BeginEntry("NODES");
		{
			for(int i = 0; i < m_lNodes.Len(); i++)
				m_lNodes[i].Write(pF);
		}
		_pDFile->EndEntry(m_lNodes.Len(), XR_LIGHTFIELDOCTTREE_NODEVERSION);

		// POINTS
		_pDFile->BeginEntry("ELEMENTS");
		{
			for(int i = 0; i < m_lElements.Len(); i++)
				m_lElements[i].Write(pF);
		}
		_pDFile->EndEntry(m_lElements.Len(), XR_LIGHTFIELDOCTTREE_ELEMENTVERSION);
	}

	_pDFile->EndSubDir();
}

TPtr<CBSP2_LightFieldOcttree> CBSP2_LightFieldOcttree::FindAndRead(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_FindAndRead, NULL);
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTFIELDOCTTREE"))
	{
		_pDFile->PopPosition();

		TPtr<CBSP2_LightFieldOcttree> spLightTree = MNew(CBSP2_LightFieldOcttree);
		if (!spLightTree)
			Error_static("CBSP2_LightFieldOcttree::FindAndRead", "Out of memory.");

		spLightTree->Read(_pDFile);
		return spLightTree;
	}
	else
		_pDFile->PopPosition();

	return NULL;
}

fp32 CBSP2_LightFieldOcttree::Light_EvalVertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, CVec3Dfp32& _LDir, CVec3Dfp32& _LColor)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_Light_EvalVertex, 1.0f);

	// Not implemented yet
	M_ASSERT(0, "!");
	return 1.0f;
}

CPixel32 CBSP2_LightFieldOcttree::Light_Vertex(const class CXR_LightID* _pIDMap, const CVec3Dfp32& _V, const CVec3Dfp32* _pN)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_Light_Vertex, CPixel32());
	M_ASSERT(0, "!");	// Should not be used
	return 0;
}

void CBSP2_LightFieldOcttree::Light_Array(const class CXR_LightID* _pIDMap, int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CMat4Dfp32* _pWMat, int _Flags)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_Light_Array, MAUTOSTRIP_VOID);
	M_ASSERT(0, "!");	// Should not be used
}

void CBSP2_LightFieldOcttree::Light_EvalPoint(const CVec3Dfp32& _V, CVec4Dfp32* _pAxis)
{
	Light_EvalPointArrayMax(&_V, 1, _pAxis);
}


void CBSP2_LightFieldOcttree::Light_EvalPointArrayMax(const CVec3Dfp32* _pV, int _nV, CVec4Dfp32* _pAxis)
{
	MAUTOSTRIP(CBSP2_LightFieldOcttree_Light_EvalPoint, 0.0f);

//	CMTime T; T.Start();

	vec128 Zero = M_VZero();
	_pAxis[0].v = Zero;
	_pAxis[1].v = Zero;
	_pAxis[2].v = Zero;
	_pAxis[3].v = Zero;
	_pAxis[4].v = Zero;
	_pAxis[5].v = Zero;

	if (m_lNodes.Len() <= 1)
	{
		return;
	}

	TAP_RCD<CBSP2_LightFieldOcttreeNode> lNodes = m_lNodes;
	TAP_RCD<CXR_LightFieldElement> lElements = m_lElements;

	for(int iV = 0; iV < _nV; iV++)
	{
		CVec3Dfp32 VLocal;
		_pV[iV].Sub(m_RootPos, VLocal);

		const int MinNodeSize = 64;
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

		CXR_LightFieldElement lSamples[8+1];	// Pad with one so we can safely read outside with vector loads

		static uint lSubNodeShift[8] = { 1,2,4,8,16,32,64,128 };

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

				const CBSP2_LightFieldOcttreeNode& Node = lNodes[iNode];

				int32 Cmp0 = int32(NodeSize - VSample[0]) >> 31;
				int32 Cmp1 = int32(NodeSize - VSample[1]) >> 31;
				int32 Cmp2 = int32(NodeSize - VSample[2]) >> 31;
				uint32 iSubNode = (1 & Cmp0) + (2 & Cmp1) + (4 & Cmp2);
				int32 Sub0 = (NodeSize & Cmp0);
				int32 Sub1 = (NodeSize & Cmp1);
				int32 Sub2 = (NodeSize & Cmp2);
				VSample[0] -= Sub0;
				VSample[1] -= Sub1;
				VSample[2] -= Sub2;

/*				if (VSample[0] > NodeSize) { iSubNode |= 1; VSample[0] -= NodeSize; }
				if (VSample[1] > NodeSize) { iSubNode |= 2; VSample[1] -= NodeSize; }
				if (VSample[2] > NodeSize) { iSubNode |= 4; VSample[2] -= NodeSize; }*/
//				if (!(Node.GetSubNodeTypes() & (1 << iSubNode)))
				if (!(Node.GetSubNodeTypes() & lSubNodeShift[iSubNode]))
				{
					const CBSP2_LightFieldOcttreeNode& Leaf = lNodes[iSubNode + Node.GetSubNodes()];
					lSamples[iSample] = lElements[Leaf.GetSubNodes()];
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

		vec128 vfx = M_VLdScalar(fp32(VLocalInt[0] & MinNodeSizeAnd) * (1.0f / fp32(MinNodeSize)));
		vec128 vfy = M_VLdScalar(fp32(VLocalInt[1] & MinNodeSizeAnd) * (1.0f / fp32(MinNodeSize)));
		vec128 vfz = M_VLdScalar(fp32(VLocalInt[2] & MinNodeSizeAnd) * (1.0f / fp32(MinNodeSize)));
		vec128 scale = M_VScalar(1.0f/255.0f);

		for(int iAxis = 0; iAxis < 6; iAxis++)
		{
/*			CVec4Dfp32 v0,v1,v2,v3,v4,v5,v6,v7,v8;
			v0[0] = lSamples[0].m_Axis[iAxis][2]; v0[1] = lSamples[0].m_Axis[iAxis][1]; v0[2] = lSamples[0].m_Axis[iAxis][0]; v0[3] = 0;
			v1[0] = lSamples[1].m_Axis[iAxis][2]; v1[1] = lSamples[1].m_Axis[iAxis][1]; v1[2] = lSamples[1].m_Axis[iAxis][0]; v1[3] = 0;
			v2[0] = lSamples[ymod].m_Axis[iAxis][2]; v2[1] = lSamples[ymod].m_Axis[iAxis][1]; v2[2] = lSamples[ymod].m_Axis[iAxis][0]; v2[3] = 0;
			v3[0] = lSamples[1+ymod].m_Axis[iAxis][2]; v3[1] = lSamples[1+ymod].m_Axis[iAxis][1]; v3[2] = lSamples[1+ymod].m_Axis[iAxis][0]; v3[3] = 0;
			v4[0] = lSamples[zmod].m_Axis[iAxis][2]; v4[1] = lSamples[zmod].m_Axis[iAxis][1]; v4[2] = lSamples[zmod].m_Axis[iAxis][0]; v4[3] = 0;
			v5[0] = lSamples[1+zmod].m_Axis[iAxis][2]; v5[1] = lSamples[1+zmod].m_Axis[iAxis][1]; v5[2] = lSamples[1+zmod].m_Axis[iAxis][0]; v5[3] = 0;
			v6[0] = lSamples[ymod+zmod].m_Axis[iAxis][2]; v6[1] = lSamples[ymod+zmod].m_Axis[iAxis][1]; v6[2] = lSamples[ymod+zmod].m_Axis[iAxis][0]; v6[3] = 0;
			v7[0] = lSamples[1+ymod+zmod].m_Axis[iAxis][2]; v7[1] = lSamples[1+ymod+zmod].m_Axis[iAxis][1]; v7[2] = lSamples[1+ymod+zmod].m_Axis[iAxis][0]; v7[3] = 0;
			vec128 a = M_VLrp(M_VLrp(M_VLrp(v0.v, v1.v, vfx), M_VLrp(v2.v, v3.v, vfx), vfy), M_VLrp(M_VLrp(v4.v, v5.v, vfx), M_VLrp(v6.v, v7.v, vfx), vfy), vfz);
			_pAxis[iAxis].v = M_VMax(_pAxis[iAxis].v, M_VMul(a, scale));
*/
/*			CVec128Access vi0,vi1,vi2,vi3,vi4,vi5,vi6,vi7;
			vi0.ku32[0] = lSamples[0].m_Axis[iAxis][2]; vi0.ku32[1] = lSamples[0].m_Axis[iAxis][1]; vi0.ku32[2] = lSamples[0].m_Axis[iAxis][0]; vi0.ku32[3] = 0;
			vi1.ku32[0] = lSamples[1].m_Axis[iAxis][2]; vi1.ku32[1] = lSamples[1].m_Axis[iAxis][1]; vi1.ku32[2] = lSamples[1].m_Axis[iAxis][0]; vi1.ku32[3] = 0;
			vi2.ku32[0] = lSamples[ymod].m_Axis[iAxis][2]; vi2.ku32[1] = lSamples[ymod].m_Axis[iAxis][1]; vi2.ku32[2] = lSamples[ymod].m_Axis[iAxis][0]; vi2.ku32[3] = 0;
			vi3.ku32[0] = lSamples[1+ymod].m_Axis[iAxis][2]; vi3.ku32[1] = lSamples[1+ymod].m_Axis[iAxis][1]; vi3.ku32[2] = lSamples[1+ymod].m_Axis[iAxis][0]; vi3.ku32[3] = 0;
			vi4.ku32[0] = lSamples[zmod].m_Axis[iAxis][2]; vi4.ku32[1] = lSamples[zmod].m_Axis[iAxis][1]; vi4.ku32[2] = lSamples[zmod].m_Axis[iAxis][0]; vi4.ku32[3] = 0;
			vi5.ku32[0] = lSamples[1+zmod].m_Axis[iAxis][2]; vi5.ku32[1] = lSamples[1+zmod].m_Axis[iAxis][1]; vi5.ku32[2] = lSamples[1+zmod].m_Axis[iAxis][0]; vi5.ku32[3] = 0;
			vi6.ku32[0] = lSamples[ymod+zmod].m_Axis[iAxis][2]; vi6.ku32[1] = lSamples[ymod+zmod].m_Axis[iAxis][1]; vi6.ku32[2] = lSamples[ymod+zmod].m_Axis[iAxis][0]; vi6.ku32[3] = 0;
			vi7.ku32[0] = lSamples[1+ymod+zmod].m_Axis[iAxis][2]; vi7.ku32[1] = lSamples[1+ymod+zmod].m_Axis[iAxis][1]; vi7.ku32[2] = lSamples[1+ymod+zmod].m_Axis[iAxis][0]; vi7.ku32[3] = 0;

			M_IMPORTBARRIER;*/

			vec128 seland = M_VConst_u32(0x000000ff, 0x000000ff, 0x000000ff, 0x00000000);
			vec128 sel0 = M_VConstMsk(1,0,1,0);
			vec128 sel1 = M_VConstMsk(1,1,0,0);
			vec128 scaleand = M_VScalar_u32(0xffff0000);

			vec128 vi0 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[0].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[0].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[0].m_Axis[iAxis][0])));
			vec128 vi1 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[1].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[1].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[1].m_Axis[iAxis][0])));
			vec128 vi2 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[ymod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[ymod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[ymod].m_Axis[iAxis][0])));
			vec128 vi3 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[1+ymod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[1+ymod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[1+ymod].m_Axis[iAxis][0])));
			vec128 vi4 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[zmod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[zmod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[zmod].m_Axis[iAxis][0])));
			vec128 vi5 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[1+zmod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[1+zmod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[1+zmod].m_Axis[iAxis][0])));
			vec128 vi6 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[ymod+zmod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[ymod+zmod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[ymod+zmod].m_Axis[iAxis][0])));
			vec128 vi7 = M_VAnd(seland, M_VSelMsk(sel1, M_VSelMsk(sel0, M_VLdScalar_u8(lSamples[1+ymod+zmod].m_Axis[iAxis][2]), M_VLdScalar_u8(lSamples[1+ymod+zmod].m_Axis[iAxis][1])), M_VLdScalar_u8(lSamples[1+ymod+zmod].m_Axis[iAxis][0])));

			vec128 vs0 = M_VAnd(M_VLdScalar_u16(lSamples[0].m_Scaler.m_Half), scaleand);
			vec128 vs1 = M_VAnd(M_VLdScalar_u16(lSamples[1].m_Scaler.m_Half), scaleand);
			vec128 vs2 = M_VAnd(M_VLdScalar_u16(lSamples[0+ymod].m_Scaler.m_Half), scaleand);
			vec128 vs3 = M_VAnd(M_VLdScalar_u16(lSamples[1+ymod].m_Scaler.m_Half), scaleand);
			vec128 vs4 = M_VAnd(M_VLdScalar_u16(lSamples[0+zmod].m_Scaler.m_Half), scaleand);
			vec128 vs5 = M_VAnd(M_VLdScalar_u16(lSamples[1+zmod].m_Scaler.m_Half), scaleand);
			vec128 vs6 = M_VAnd(M_VLdScalar_u16(lSamples[0+ymod+zmod].m_Scaler.m_Half), scaleand);
			vec128 vs7 = M_VAnd(M_VLdScalar_u16(lSamples[1+ymod+zmod].m_Scaler.m_Half), scaleand);

			vec128 v0,v1,v2,v3,v4,v5,v6,v7;
			v0 = M_VMul(M_VCnv_i32_f32(vi0), vs0); v1 = M_VMul(M_VCnv_i32_f32(vi1), vs1); v2 = M_VMul(M_VCnv_i32_f32(vi2), vs2); v3 = M_VMul(M_VCnv_i32_f32(vi3), vs3);
			v4 = M_VMul(M_VCnv_i32_f32(vi4), vs4); v5 = M_VMul(M_VCnv_i32_f32(vi5), vs6); v6 = M_VMul(M_VCnv_i32_f32(vi6), vs6); v7 = M_VMul(M_VCnv_i32_f32(vi7), vs7);

			vec128 a = M_VLrp(M_VLrp(M_VLrp(v0, v1, vfx), M_VLrp(v2, v3, vfx), vfy), M_VLrp(M_VLrp(v4, v5, vfx), M_VLrp(v6, v7, vfx), vfy), vfz);
			_pAxis[iAxis].v = M_VMax(_pAxis[iAxis].v, M_VMul(a, scale));

	/*		CPixel32 Color = CPixel32::TrilinearRGB(CPixel32::From_u8(lSamples[0].m_Axis[iAxis][2], lSamples[0].m_Axis[iAxis][1], lSamples[0].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[1].m_Axis[iAxis][2], lSamples[1].m_Axis[iAxis][1], lSamples[1].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[ymod].m_Axis[iAxis][2], lSamples[ymod].m_Axis[iAxis][1], lSamples[ymod].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[1+ymod].m_Axis[iAxis][2], lSamples[1+ymod].m_Axis[iAxis][1], lSamples[1+ymod].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[zmod].m_Axis[iAxis][2], lSamples[zmod].m_Axis[iAxis][1], lSamples[zmod].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[1+zmod].m_Axis[iAxis][2], lSamples[1+zmod].m_Axis[iAxis][1], lSamples[1+zmod].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[ymod+zmod].m_Axis[iAxis][2], lSamples[ymod+zmod].m_Axis[iAxis][1], lSamples[ymod+zmod].m_Axis[iAxis][0]),
													CPixel32::From_u8(lSamples[1+ymod+zmod].m_Axis[iAxis][2], lSamples[1+ymod+zmod].m_Axis[iAxis][1], lSamples[1+ymod+zmod].m_Axis[iAxis][0]),
													fx, fy, fz);
			_pAxis[iAxis][0]	= Color.R() * (1.0f / 255.0f);
			_pAxis[iAxis][1]	= Color.G() * (1.0f / 255.0f);
			_pAxis[iAxis][2]	= Color.B() * (1.0f / 255.0f);*/
		}
	}

//	T.Stop();
//	M_TRACEALWAYS("LightEval %.4fms\n", T.GetTime() * 1000.0f);
}
