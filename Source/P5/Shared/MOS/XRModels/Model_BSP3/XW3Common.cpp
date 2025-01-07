#include "PCH.h"

#include "XW3Common.h"
#include "MFloat.h"

#define MODEL_BSP3_USEKNITSTRIP

enum
{
	BSP3_LIGHT_VER	= 0x100
};

void CBSP3_Node::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP3_Node_Read, MAUTOSTRIP_VOID);
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

	default :
		Error_static("CBSP3_Node::Read", CStrF("Unsupported version %.4x", _Ver));
	}

}

void CBSP3_Node::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP3_Node_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
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
#endif
}

#ifndef CPU_LITTLEENDIAN
void CBSP3_Node::SwapLE()
{
	MAUTOSTRIP(CBSP3_Node_SwapLE, MAUTOSTRIP_VOID);
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
CBSP3_Portal::CBSP3_Portal()
{
	MAUTOSTRIP(CBSP3_Portal_ctor, MAUTOSTRIP_VOID);
	m_iPlane = 0;
	m_iNodeFront = 0;
	m_iNodeBack = 0;
	m_iRPortal = 0;
	m_iFogPortal = 0;
}

void CBSP3_Portal::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP3_Portal_Read, MAUTOSTRIP_VOID);
	CXR_IndexedPortal::Read(_pF);
	switch(_Version)
	{
	case 0x0000:
		{
			struct
			{
				uint16 m_iPlane;
				uint16 m_iNodeFront;
				uint16 m_iNodeBack;
				uint16 m_iRPortal;
				uint16 m_iFogPortal;
			} TempRead;
			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iNodeFront);
			::SwapLE(TempRead.m_iNodeBack);
			::SwapLE(TempRead.m_iRPortal);
			::SwapLE(TempRead.m_iFogPortal);
			m_iPlane = TempRead.m_iPlane;
			m_iNodeFront = TempRead.m_iNodeFront;
			m_iNodeBack = TempRead.m_iNodeBack;
			m_iRPortal = TempRead.m_iRPortal;
			m_iFogPortal = TempRead.m_iFogPortal;
		}
		break;

	case 0x0101:
		{
			struct
			{
				uint32 m_iNodeFront;
				uint32 m_iNodeBack;
				uint32 m_iPlane;
				uint16 m_iRPortal;
				uint16 m_iFogPortal;
			} TempRead;
			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iNodeFront);
			::SwapLE(TempRead.m_iNodeBack);
			::SwapLE(TempRead.m_iRPortal);
			::SwapLE(TempRead.m_iFogPortal);
			m_iPlane = TempRead.m_iPlane;
			m_iNodeFront = TempRead.m_iNodeFront;
			m_iNodeBack = TempRead.m_iNodeBack;
			m_iRPortal = TempRead.m_iRPortal;
			m_iFogPortal = TempRead.m_iFogPortal;
		}
		break;

	default:
		Error_static("Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CBSP3_Portal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP3_Portal_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	Error_static("Write", "Unsupported function");
#endif
}

// -------------------------------------------------------------------
void CBSP3_PortalLeaf::Read(CCFile* _pF, int _bReadVersion)
{
	MAUTOSTRIP(CBSP3_PortalLeaf_Read, MAUTOSTRIP_VOID);
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
		Error_static("CBSP3_PortalLeaf::Read", CStrF("Invalid portal-leaf version. %.4x", Version));
	}
}

void CBSP3_PortalLeaf::Write(CCFile* _pF, int _bReadVersion) const
{
	MAUTOSTRIP(CBSP3_PortalLeaf_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
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
#endif
}

// -------------------------------------------------------------------
void CBSP3_FogPortalFace::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP3_FogPortalFace_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iiVertices);
	_pF->ReadLE(m_nVertices);
}

void CBSP3_FogPortalFace::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP3_FogPortalFace_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	_pF->WriteLE(m_iiVertices);
	_pF->WriteLE(m_nVertices);
#endif
}

// -------------------------------------------------------------------
CBSP3_FogPortal::CBSP3_FogPortal()
{
	MAUTOSTRIP(CBSP3_FogPortal_ctor, MAUTOSTRIP_VOID);
	m_iVertices = 0;
	m_nVertices = 0;
	m_iFaces = 0;
	m_nFaces = 0;
}

void CBSP3_FogPortal::Read(CCFile* _pF)
{
	MAUTOSTRIP(CBSP3_FogPortal_Read, MAUTOSTRIP_VOID);
	_pF->ReadLE(m_iVertices);
	_pF->ReadLE(m_nVertices);
	_pF->ReadLE(m_iFaces);
	_pF->ReadLE(m_nFaces);
}

void CBSP3_FogPortal::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP3_FogPortal_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	_pF->WriteLE(m_iVertices);
	_pF->WriteLE(m_nVertices);
	_pF->WriteLE(m_iFaces);
	_pF->WriteLE(m_nFaces);
#endif
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| 
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CBSP3_LightMapInfo::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP3_LightMapInfo_Read, MAUTOSTRIP_VOID);
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
	
		m_Scale = Scale;
		m_LMCOffsetXHalf = LMCOffsetU >> 1;
		m_LMCOffsetYHalf = LMCOffsetV >> 1;
		m_iLMC = iLMC;

		m_LMCWidthHalf = 0;
		m_LMCHeightHalf = 0;
	}
	else if (_Version == 2)
	{
		_pF->ReadLE(m_Scale);
		_pF->ReadLE(m_LMCOffsetXHalf);
		_pF->ReadLE(m_LMCOffsetYHalf);
		_pF->ReadLE(m_iLMC);
		_pF->ReadLE(m_LMCWidthHalf);
		_pF->ReadLE(m_LMCHeightHalf);
	}
}

void CBSP3_LightMapInfo::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP3_LightMapInfo_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	_pF->WriteLE(m_Scale);
	_pF->WriteLE(m_LMCOffsetXHalf);
	_pF->WriteLE(m_LMCOffsetYHalf);
	_pF->WriteLE(m_iLMC);
	_pF->WriteLE(m_LMCWidthHalf);
	_pF->WriteLE(m_LMCHeightHalf);
#endif
}

#ifndef CPU_LITTLEENDIAN
void CBSP3_LightMapInfo::SwapLE()
{
	MAUTOSTRIP(CBSP3_LightMapInfo_SwapLE, MAUTOSTRIP_VOID);
	// Just bytes here..
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP3_LightMapContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CBSP3_LightMapContainer::CBSP3_LMDesc::Read(CCFile* _pFile, int _Version)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_CBSP3_LMDesc_Read, MAUTOSTRIP_VOID);
	if (_Version == 0x0100)
	{
		_pFile->ReadLE(m_Width);
		_pFile->ReadLE(m_Height);
		_pFile->ReadLE(m_DataPos);
	}
	else
		Error_static("CBSP3_LightMapContainer::CBSP3_LMDesc::Read", CStrF("Unsupported version %.8x", _Version));
}

void CBSP3_LightMapContainer::CBSP3_LMDesc::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_CBSP3_LMDesc_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_Width);
	_pFile->WriteLE(m_Height);
	_pFile->WriteLE(m_DataPos);
#endif
}

CBSP3_LightMapContainer::CBSP3_LightMapContainer()
{
	MAUTOSTRIP(CBSP3_LightMapContainer_ctor, MAUTOSTRIP_VOID);
	m_Format = 0;
}

CBSP3_LightMapContainer::~CBSP3_LightMapContainer()
{
	MAUTOSTRIP(CBSP3_LightMapContainer_dtor, MAUTOSTRIP_VOID);
}

void CBSP3_LightMapContainer::Create(TArray<spCImage> _lspLightMaps, int _Format)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_Create, MAUTOSTRIP_VOID);
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

TArray<spCImage> CBSP3_LightMapContainer::GetLightMapImages(int _Format)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_GetLightMapImages, TArray<spCImage>());
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

void CBSP3_LightMapContainer::GetLightMap(int _iLightMap, CImage& _RefImage)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_GetLightMap, MAUTOSTRIP_VOID);
	const CBSP3_LMDesc& LMDesc = m_lLMDesc[_iLightMap];
	_RefImage.CreateReference(LMDesc.m_Width, LMDesc.m_Height, m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
//	_RefImage.CreateReference(LMDesc.GetWidth(), LMDesc.GetHeight(), m_Format, IMAGE_MEM_IMAGE, LMDesc.GetWidth() * CImage::Format2PixelSize(m_Format), &m_lLMData[LMDesc.m_DataPos], NULL);
}

void CBSP3_LightMapContainer::Read(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_Read, MAUTOSTRIP_VOID);
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

void CBSP3_LightMapContainer::Write(CCFile* _pFile)
{
	MAUTOSTRIP(CBSP3_LightMapContainer_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
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
#endif
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP3_CoreFace
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP3_CoreFace::CBSP3_CoreFace()
{
	MAUTOSTRIP(CBSP3_CoreFace_ctor, MAUTOSTRIP_VOID);
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

void CBSP3_CoreFace::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP3_CoreFace_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0x0203:
		{
			struct
			{
				int32 m_iiEdges;

				union
				{
					uint32 m_VertexIO;
					struct
					{
						uint32 m_iiVertices:24;	// Index to first vertex-index
						uint32 m_nVertices:8;		// Number vertices
					};
				};
				uint32 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint32 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
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
			m_iiVertices	= TempRead.m_iiVertices;
			m_nVertices		= TempRead.m_nVertices;
			m_iMapping	= TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iLightInfo = TempRead.m_iLightInfo;
			m_nLightInfo = TempRead.m_nLightInfo;
		}
		break;

	case 0x0202:
		{
			struct
			{
				int32 m_iiEdges;

				union
				{
					uint32 m_VertexIO;
					struct
					{
						uint32 m_iiVertices:24;	// Index to first vertex-index
						uint32 m_nVertices:8;		// Number vertices
					};
				};
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
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
			m_iiVertices	= TempRead.m_iiVertices;
			m_nVertices		= TempRead.m_nVertices;
			m_iMapping	= TempRead.m_iMapping;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iiNormals = TempRead.m_iiNormals;
			m_iBackMedium = TempRead.m_iBackMedium;
			m_iLightInfo = TempRead.m_iLightInfo;
			m_nLightInfo = TempRead.m_nLightInfo;
		}
		break;

	case 0x0201:
		{
			_pF->ReadLE(m_iiEdges);

			uint32 iiVertices;
			_pF->ReadLE(iiVertices);
			m_iiVertices = iiVertices;
			uint16 nVertices;
			_pF->ReadLE(nVertices);
			m_nVertices = nVertices;

			_pF->ReadLE(m_iMapping);
			_pF->ReadLE(m_iSurface);
			_pF->ReadLE(m_Flags);

			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iiNormals);
			_pF->ReadLE(m_iBackMedium);
			_pF->ReadLE(m_iFrontMedium);

			uint32 Temp;
			_pF->ReadLE(Temp);
			m_iLightInfo	= Temp & ((1 << 28) - 1);
			m_nLightInfo	= Temp >> 28;
			break;
		};
	case 0x0200 :
			_pF->ReadLE(m_iiEdges);

	case 0x0000 : 
		{
			uint32 iiVertices;
			_pF->ReadLE(iiVertices);
			m_iiVertices = iiVertices;
			uint16 nVertices;
			_pF->ReadLE(nVertices);
			m_nVertices = nVertices;

			_pF->ReadLE(m_iMapping);
			_pF->ReadLE(m_iSurface);
			_pF->ReadLE(m_Flags);

			_pF->ReadLE(m_iPlane);
			_pF->ReadLE(m_iiNormals);
			_pF->ReadLE(m_iBackMedium);
			_pF->ReadLE(m_iFrontMedium);

			uint16 iLightInfo, nLightInfo;
			_pF->ReadLE(iLightInfo);
			_pF->ReadLE(nLightInfo);
			m_iLightInfo	= iLightInfo;
			m_nLightInfo	= nLightInfo;
		}
		break;

	default :
		Error_static("CBSP3_CoreFace::Read", CStrF("Invalid version: %.4x", _Version));
	}
}

void CBSP3_CoreFace::Write(CCFile* _pF)
{
	MAUTOSTRIP(CBSP3_CoreFace_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
#ifdef PLATFORM_DOLPHIN
	Error_static("CBSP3_CoreFace::Write", "Not supported!");
#else
	_pF->WriteLE(m_iiEdges);

	_pF->WriteLE(m_iiVertices);

	_pF->WriteLE(m_nVertices);
	_pF->WriteLE(m_iMapping);
	_pF->WriteLE(m_iSurface);
	_pF->WriteLE(m_Flags);

	_pF->WriteLE(m_iPlane);
	_pF->WriteLE(m_iiNormals);
	_pF->WriteLE(m_iBackMedium);
	_pF->WriteLE(m_iFrontMedium);

	uint32 Temp = m_iLightInfo | (m_nLightInfo << 28);
	_pF->WriteLE(Temp);
#endif
#endif
}

#ifndef CPU_LITTLEENDIAN
void CBSP3_CoreFace::SwapLE()
{
	MAUTOSTRIP(CBSP3_CoreFace_SwapLE, MAUTOSTRIP_VOID);
#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS3)
	Error_static("CBSP3_CoreFace::Write", "Not supported!");
#else
	::SwapLE(m_iiEdges);

	::SwapLE(m_iiVertices);

	::SwapLE(m_nVertices);
	::SwapLE(m_iMapping);
	::SwapLE(m_iSurface);
	::SwapLE(m_Flags);

	::SwapLE(m_iPlane);
	::SwapLE(m_iiNormals);
	::SwapLE(m_iBackMedium);
	::SwapLE(m_iFrontMedium);

	::SwapLE(m_iLightInfo);
	::SwapLE(m_nLightInfo);
#endif	
}
#endif


CBSP3_LightData::CBSP3_LightData()
{
	m_Flags	= 0;
	m_VertexBufferPos	= 0;
	m_VertexBufferCount	= 0;
	m_ClusterPos	= 0;
	m_ClusterCount	= 0;
	m_KeepData	= 0;

#ifndef	PLATFORM_CONSOLE
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	// If we're creating xdf's then data should not be loaded until requested
	if( !(bXDF && Platform != 0))
		m_KeepData	= 1;
#endif
}

void CBSP3_LightData::Read(CDataFile* _pDFile)
{
	MSCOPESHORT(CBSP3_LightData::Read);
	// FACELIGHTS
//	{
//		MSCOPESHORT(FaceLights);
//		if (!_pDFile->GetNext("FACELIGHTS")) 
//			Error("Read", "No FACELIGHTS entry.");
//
//		m_liFaceLights.SetLen(_pDFile->GetUserData()); 
//		_pDFile->GetFile()->Read(m_liFaceLights.GetBasePtr(), m_liFaceLights.ListSize());
//		SwitchArrayLE_uint16(m_liFaceLights.GetBasePtr(), m_liFaceLights.Len()); //AR-ADD, byte ordering..
//	}

	// LIGHT
	{
		MSCOPESHORT(Lights);
		if (!_pDFile->GetNext("LIGHTS"))
			Error("Read", "No LIGHTS entry.");

		m_LightsCount = _pDFile->GetUserData();
		m_LightsVer = _pDFile->GetUserData2();
		m_LightsPos = _pDFile->GetFile()->Pos();

		if( m_KeepData )
		{
			ReadLights( _pDFile->GetFile() );
		}
	}

	// SV
	{
		MSCOPESHORT(ShadowVolume);
		if (!_pDFile->GetNext("SV")) 
			Error("Read", "No SV entry.");

		int nSV = _pDFile->GetUserData();
		m_lSV.SetLen(nSV); 

		if (m_lSV.ListSize() != _pDFile->GetEntrySize())
			Error("Read", "SV entry size missmatch.");

		_pDFile->GetFile()->Read(m_lSV.GetBasePtr(), m_lSV.ListSize());

#ifdef CPU_BIGENDIAN
		CBSP3_ShadowVolume* pSV = m_lSV.GetBasePtr();
		for(int i = 0; i < nSV; i++)
			pSV[i].SwapLE();
#endif

		m_plSV	= m_lSV.GetBasePtr();
	}
	// SVFIRSTPL
	{
		MSCOPESHORT(SVFIRSTPL);
		if (!_pDFile->GetNext("SVFIRSTPL")) 
			Error("Read", "No SVFIRSTPL entry.");

		m_liSVFirstPL.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSVFirstPL.GetBasePtr(), m_liSVFirstPL.ListSize());
		SwitchArrayLE_uint16(m_liSVFirstPL.GetBasePtr(), m_liSVFirstPL.Len());
	}

	// SVPLCOUNT
	{
		MSCOPESHORT(SVPLCOUNT);
		if (!_pDFile->GetNext("SVPLCOUNT")) 
			Error("Read", "No SVPLCOUNT entry.");

		m_lnSVPL.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_lnSVPL.GetBasePtr(), m_lnSVPL.ListSize());
		SwitchArrayLE_uint16(m_lnSVPL.GetBasePtr(), m_lnSVPL.Len());
	}

	// SVFIRSTLIGHT
	{
		MSCOPESHORT(SVFIRSTLIGHT);
		if (!_pDFile->GetNext("SVFIRSTLIGHT")) 
			Error("Read", "No SVFIRSTLIGHT entry.");

		m_liSVFirstLight.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSVFirstLight.GetBasePtr(), m_liSVFirstLight.ListSize());
		SwitchArrayLE_uint16(m_liSVFirstLight.GetBasePtr(), m_liSVFirstLight.Len());
	}
/*
	// SVVERTICES
	{
		MSCOPESHORT(SVVERTICES);
		if (!_pDFile->GetNext("SVVERTICES")) 
			Error("Read", "No SVVERTICES entry.");

		m_lSVVertices.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_lSVVertices.GetBasePtr(), m_lSVVertices.ListSize());
		SwitchArrayLE_uint32((uint32*)m_lSVVertices.GetBasePtr(), m_lSVVertices.Len()*3);
	}

	// SVPRIM
	{
		MSCOPESHORT(SVPRIM);
		if (!_pDFile->GetNext("SVPRIM")) 
			Error("Read", "No SVPRIM entry.");

		m_liSVPrim.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSVPrim.GetBasePtr(), m_liSVPrim.ListSize());
		SwitchArrayLE_uint16(m_liSVPrim.GetBasePtr(), m_liSVPrim.Len());
	}
*/
/*
	// SLC
	{
		MSCOPESHORT(SLC);
		if (!_pDFile->GetNext("SLC")) 
			Error("Read", "No SLC entry.");

		int nSLC = _pDFile->GetUserData();
		m_lSLC.SetLen(nSLC); 

		if (m_lSLC.ListSize() != _pDFile->GetEntrySize())
			Error("Read", "SV entry size missmatch.");

		_pDFile->GetFile()->Read(m_lSLC.GetBasePtr(), m_lSLC.ListSize());

#ifdef CPU_BIGENDIAN
		CBSP3_ShadowVolume* pSLC = m_lSLC.GetBasePtr();
		for(int i = 0; i < nSLC; i++)
			pSLC[i].SwitchLE();
#endif
	}

	// SLCFACES
	{
		MSCOPESHORT(SLCFACES);
		if (!_pDFile->GetNext("SLCFACES")) 
			Error("Read", "No SLCFACES entry.");

		m_liSLCFaces.SetLen(_pDFile->GetUserData()); 
		_pDFile->GetFile()->Read(m_liSLCFaces.GetBasePtr(), m_liSLCFaces.ListSize());
		SwitchArrayLE_uint16(m_liSLCFaces.GetBasePtr(), m_liSLCFaces.Len());
	}
*/
/*
	// SHADERQUEUE
	{
		MSCOPESHORT(SHADERQUEUE);
		if (!_pDFile->GetNext("SHADERQUEUE")) 
			Error("Read", "No SHADERQUEUE entry.");

		int nSQ = _pDFile->GetUserData();
		m_lShaderQueue.SetLen(nSQ); 

		if (m_lShaderQueue.ListSize() != _pDFile->GetEntrySize())
			Error("Read", "SV entry size missmatch.");

		_pDFile->GetFile()->Read(m_lShaderQueue.GetBasePtr(), m_lShaderQueue.ListSize());

#ifdef CPU_BIGENDIAN
		CBSP3_ShadowVolume* pSQ = m_lShaderQueue.GetBasePtr();
		for(int i = 0; i < nSQ; i++)
			pSQ[i].SwitchLE();
#endif

	}
*/
	// PLANES
	{
		MSCOPESHORT(PLANES);
		if (!_pDFile->GetNext("PLANES")) 
			Error("Read", "No PLANES entry.");

		int nPlanes = _pDFile->GetUserData();
		m_lPlanes.SetLen(nPlanes); 
		_pDFile->GetFile()->Read(m_lPlanes.GetBasePtr(), m_lPlanes.ListSize());

#ifndef CPU_LITTLEENDIAN
		for(int i = 0; i < nPlanes; i++)
			m_lPlanes[i].SwapLE();
#endif
		m_plPlanes	= m_lPlanes.GetBasePtr();
	}

}

void CBSP3_LightData::Write(CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	_pDFile->BeginEntry("LIGHTMAP3");
	_pDFile->EndEntry(BSP3_LIGHT_VER);
	_pDFile->BeginSubDir();
		if( m_spLMTC )
		{
			_pDFile->BeginEntry("IMAGELIST");
			_pDFile->EndEntry(0);
			_pDFile->BeginSubDir();
			{
				CTextureContainer_Plain TextureContainer;
				TextureContainer.Add(m_spLMTC);
				TextureContainer.WriteImageList(_pDFile);
			}
			_pDFile->EndSubDir();
		}

		_pDFile->BeginEntry("VERTEXBUFFERS");
		_pDFile->GetFile()->Write( m_lVertexBufferSize.GetBasePtr(), m_lVertexBufferSize.ListSize() );
		for( int iVB = 0; iVB < m_lLightVB.Len(); iVB++ )
		{
			_pDFile->GetFile()->Write( m_lLightVB[iVB].m_lPosition.GetBasePtr(), m_lLightVB[iVB].m_lPosition.ListSize() );
			_pDFile->GetFile()->Write( m_lLightVB[iVB].m_lTexCoord.GetBasePtr(), m_lLightVB[iVB].m_lTexCoord.ListSize() );
		}
		_pDFile->EndEntry(m_lLightVB.Len());

		_pDFile->BeginEntry("CLUSTERS");
		_pDFile->GetFile()->Write( m_lLMCLightmap.GetBasePtr(), m_lLMCLightmap.ListSize() );
		_pDFile->GetFile()->Write( m_lLMCGameData.GetBasePtr(),  m_lLMCGameData.ListSize() );
		_pDFile->GetFile()->Write( m_lLightStrips.GetBasePtr(),  m_lLightStrips.ListSize() );
		_pDFile->EndEntry(m_lLMCGameData.Len());

	_pDFile->EndSubDir();
#endif
}

TPtr<CBSP3_LightData> CBSP3_LightData::FindAndRead(CDataFile* _pDFile)
{
	MAUTOSTRIP(CBSP3_LightData_FindAndRead, NULL);
	MSCOPESHORT(CBSP3_LightData::FindAndRead);

	_pDFile->PushPosition();
	if (_pDFile->GetNext("UNIFIEDLIGHTDATA"))
	{
		if (!_pDFile->GetSubDir())
			Error_static("CBSP3_LightData::FindAndRead", "Invalid entry.");

		TPtr<CBSP3_LightData> spLightData = MNew(CBSP3_LightData);
		if (!spLightData)
			Error_static("CBSP3_LightOcttree::FindAndRead", "Out of memory.");

		spLightData->Read(_pDFile);
		_pDFile->PopPosition();
		return spLightData;
	}

	_pDFile->PopPosition();

	return NULL;
}

void CBSP3_LightData::ReadLightmap(CDataFile* _pDFile, bool _bTools)
{
	MSCOPESHORT(CBSP3_LightData::ReadLightmap);
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTMAP3"))
	{
		if( _pDFile->GetUserData() != BSP3_LIGHT_VER )
			return;

		if (!_pDFile->GetSubDir())
			Error_static("CXR_Model_BSP3::ReadLightmap", "Invalid entry.");
		m_Filename	= _pDFile->GetFile()->GetFileName();

		{
			MSCOPESHORT(Textures);
			_pDFile->PushPosition();

#ifndef PLATFORM_CONSOLE
			if(_bTools)
			{
				spCTextureContainer_Plain spLMTC;
				spLMTC	= MNew(CTextureContainer_Plain);
				spLMTC->AddFromXTC(_pDFile);

				m_spLMTC	= spLMTC;
			}
			else
#endif	// PLATFORM_CONSOLE
			{
				spCTextureContainer_VirtualXTC spLMTC;
				spLMTC	= MNew(CTextureContainer_VirtualXTC);
				spLMTC->Create(_pDFile);

				m_spLMTC	= spLMTC;
			}
			_pDFile->PopPosition();
		}

		{
			MSCOPESHORT(Attributes);
			int Count = m_spLMTC->GetNumLocal();
			m_lAttributes.SetLen( Count );
			for( int i = 0; i < Count; i++ )
			{
				CRC_Attributes*pA = &m_lAttributes[i];
				pA->SetDefault();
			#ifdef	PLATFORM_PS2
				pA->Attrib_TexEnvMode( 0, CRC_PS2_TEXENVMODE_ADD | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE );
			#else
				pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			#endif
				pA->Attrib_Enable(CRC_FLAGS_CULL);
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
				int TextureID = m_spLMTC->GetTextureID( i );
				pA->Attrib_TextureID( 0, TextureID );

			#ifndef PLATFORM_PS2
				pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			#endif
			}
			
			m_plAttributes = m_lAttributes.GetBasePtr();
		}
		_pDFile->PushPosition();
		if( _pDFile->GetNext("VERTEXBUFFERS") )
		{
			MSCOPESHORT(VertexBuffers);
			m_VertexBufferCount	= _pDFile->GetUserData();
			m_VertexBufferPos	= _pDFile->GetFile()->Pos();
			if( m_KeepData )
				ReadVertexBuffers( _pDFile->GetFile() );
		}
		_pDFile->PopPosition();

		_pDFile->PushPosition();
		if( _pDFile->GetNext("CLUSTERS") )
		{
			MSCOPESHORT(Clusters);
			m_ClusterCount	= _pDFile->GetUserData();
			m_lLMCLightmap.SetLen( m_ClusterCount );
			_pDFile->GetFile()->Read( m_lLMCLightmap.GetBasePtr(), m_lLMCLightmap.ListSize() );
			m_ClusterPos	= _pDFile->GetFile()->Pos();
			if( m_KeepData )
				ReadClusters( _pDFile->GetFile() );
				
			m_plLMCLightmap = m_lLMCLightmap.GetBasePtr();
		}
		_pDFile->PopPosition();
	}

	_pDFile->PopPosition();
}

int CBSP3_LightData::GetPrimStreamCount( int _iLMC )
{
	MAUTOSTRIP( CBSP3_LightData_GetPrimStreamCount, 0 );
	CBSP3_LMCGameData& LMC = m_lLMCGameData[_iLMC];

	int nP = 0;
	{
		int StripCount	= LMC.m_nStrip;

		for( int i = 0; i < StripCount; i++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[i + LMC.m_iStrip];

			nP += Strip.m_nLen + 2;
		}
	}

	return nP;
}

void CBSP3_LightData::CreatePrimStream( int _iLMC, uint16** _ppDest )
{
	MAUTOSTRIP( CBSP3_LightData_CreatePrimStream, MAUTOSTRIP_VOID );
	CBSP3_LMCGameData& LMC = m_lLMCGameData[_iLMC];
	uint16 *piPrim = *_ppDest;
	int StripCount = LMC.m_nStrip;

	{
		int nP = 0;
		
		for( int s = 0; s < StripCount; s++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[LMC.m_iStrip + s];

			int iv = Strip.m_iStart;
			int nv = Strip.m_nLen;
			int odd = nv & 1;

			M_ASSERT( nv != 0, "!" );
			
			
			piPrim[nP++]	= CRC_RIP_TRISTRIP | ( ( nv + 2 ) << 8 );
			piPrim[nP++]	= nv;
			
			int iv0 = iv;
			int iv1 = iv + nv - 1;
			nv -= odd;
			
			for( int j = 0; j < nv; j += 2 )
			{
				// This fudgy code is to fool the compiler so it won't unroll since it will unroll way more than is needed
				--nP;
				piPrim[++nP]	= iv1--;
				piPrim[++nP]	= iv0++;
				++nP;
			}
			
			if( odd )
				piPrim[nP++]	= iv1;
			
		}
		*_ppDest	+= nP;
	}
}

int CBSP3_LightData::GetPrimStreamCount_KnitStrip( int _iLMC )
{
	MAUTOSTRIP( CBSP3_LightData_GetPrimStreamCount_KnitStrip, MAUTOSTRIP_VOID );
	CBSP3_LMCGameData& LMC = m_lLMCGameData[_iLMC];

#ifdef	PLATFORM_PS2
	int i;
	int nP = 2;
	int nvStrip = 0;

	// Count primstream to get size
	{

		const uint16 ADC = 0x8000;

		int StripCount	= LMC.m_nStrip;

		for( i = 0; i < StripCount; i++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[LMC.m_iStrip + i];
			
			int nv = Strip.m_nLen;
			
			int iv = Strip.m_iStart;

			nv	+= nv & 1;

			if( ( nvStrip + nv ) > MODEL_BSP3_MAXKNITSTRIP )
			{
				nP += nvStrip + 2;
				
				nvStrip = 0;
			}
			
			nvStrip	+= nv;
		}
	}
	nP += nvStrip;
#else
	int i;
	int nP = 2;
	{
		int StripCount	= LMC.m_nStrip;
		int nvStrip = 0;

		for( i = 0; i < StripCount; i++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[LMC.m_iStrip + i];

			int odd = Strip.m_nLen & 1;

			int StripLen = Strip.m_nLen - odd + 2 + 2;	// odd is compensated for when stitching

			if( ( nvStrip + StripLen ) > MODEL_BSP3_MAXKNITSTRIP )
			{
				nP	+= nvStrip + 2;
				nvStrip	= 0;
			}

			// Might not need odd thingy here.. let stiching take care of it?
			nvStrip	+= StripLen;
		}

		nP	+= nvStrip;
	}
#endif

	return nP;
}

void CBSP3_LightData::CreatePrimStream_KnitStrip( int _iLMC, uint16** _ppDest )
{
	MAUTOSTRIP( CBSP3_LightData_CreatePrimStream_KnitStrip, MAUTOSTRIP_VOID );
	CBSP3_LMCGameData& LMC = m_lLMCGameData[_iLMC];
	uint16 *piPrim = *_ppDest;
	int StripCount = LMC.m_nStrip;

#ifdef	PLATFORM_PS2
	{
		int i;
		int iPrimStart = 0;
		int nP = 2;
		int nvStrip = 0;

		const uint16 ADC = 0x8000;

		for( i = 0; i < StripCount; i++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[LMC.m_iStrip + i];
			
			int nv = Strip.m_nLen;
			
			int iv = Strip.m_iStart;

			int odd = nv & 1;

			if( ( nvStrip + nv + odd ) > MODEL_BSP3_MAXKNITSTRIP )
			{
				piPrim[iPrimStart+0]	= CRC_RIP_TRISTRIP | ( ( nvStrip + 2 ) << 8 );
				piPrim[iPrimStart+1]	= nvStrip;
				
				nvStrip = 0;
				iPrimStart	= nP;
				nP			+= 2;
			}

			int iFirst = nP;

			int iv0 = iv;
			int iv1 = iv + nv - 1;
			nv -= odd;

			for( int j = 0; j < nv; j += 2 )
			{
				--nP;
				piPrim[++nP]	= iv1--;
				piPrim[++nP]	= iv0++;
				++nP;
			}
			
			nvStrip	+= nv;

			if( odd )
			{
				piPrim[nP++]	= iv1;
				piPrim[nP++]	= ADC | iv1;
				
				nvStrip	+= 2;
			}
			
			piPrim[iFirst+0]	|= ADC;
			piPrim[iFirst+1]	|= ADC;
		}

		piPrim[iPrimStart+0]	= CRC_RIP_TRISTRIP | ( ( nvStrip + 2 ) << 8 );
		piPrim[iPrimStart+1]	= nvStrip;

		*_ppDest	+= nP;
	}
#else
	{
		int iPrimStart = 0;
		int nP = 2;
		int nvStrip = 0;

		for( int i = 0; i < StripCount; i++ )
		{
			const CBSP3_LightStrip& Strip = m_lLightStrips[LMC.m_iStrip + i];
			
			int nv = Strip.m_nLen;
			
			int iv = Strip.m_iStart;

			int odd = nv & 1;

			if( ( nvStrip + nv + odd + 2 ) > MODEL_BSP3_MAXKNITSTRIP )
			{
				piPrim[iPrimStart+0]	= CRC_RIP_TRISTRIP | ( ( nvStrip + 2 ) << 8 );
				piPrim[iPrimStart+1]	= nvStrip;
				
				nvStrip = 0;
				iPrimStart	= nP;
				nP			+= 2;
			}

			int iv0 = iv;
			int iv1 = iv + nv - 1;
			nv -= odd;

			{
				// Stich-patch
				piPrim[nP++]	= iv1;
				piPrim[nP++]	= iv1;
				nvStrip += 2;
			}

			for( int j = 0; j < nv; j += 2 )
			{
				piPrim[nP++]	= iv1--;
				piPrim[nP++]	= iv0++;
			}
			
			nvStrip	+= nv;

			if( odd )
			{
				piPrim[nP++]	= iv1;
				piPrim[nP++]	= iv1;
				
				nvStrip	+= 2;
			}
			else
			{
				// Stich-patch
				piPrim[nP++]	= iv1;
				piPrim[nP++]	= iv1;
				nvStrip += 2;
			}

		}

		piPrim[iPrimStart+0]	= CRC_RIP_TRISTRIP | ( ( nvStrip + 2 ) << 8 );
		piPrim[iPrimStart+1]	= nvStrip;

		*_ppDest	+= nP;
	}
#endif
}

TThinArray<uint16>	lPrimStream;

void CBSP3_LightData::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP( CBSP3_LightData_Get, MAUTOSTRIP_VOID );
	MSCOPESHORT(CBSP3_LightData::Get);
	M_ASSERT( m_lLMCGameData.ValidPos(_iLocal), "!" );

	CBSP3_LMCGameData& LMC = m_lLMCGameData[_iLocal];
	if( lPrimStream.Len() == 0 )
	{
#ifdef	MODEL_BSP3_USEKNITSTRIP
		int Len = GetPrimStreamCount_KnitStrip( _iLocal );
		lPrimStream.SetLen( Len );
		uint16*pData = lPrimStream.GetBasePtr();
		CreatePrimStream_KnitStrip( _iLocal, &pData );
#else
		int Len = GetPrimStreamCount( _iLocal );
		lPrimStream.SetLen( Len );
		uint16*pData = lPrimStream.GetBasePtr();
		CreatePrimStream( _iLocal, &pData );
#endif
	}

	int iVB = m_lLightStrips[LMC.m_iStrip].m_iVB;

	_VB.Clear();
	_VB.m_piPrim		= lPrimStream.GetBasePtr();
	_VB.m_nPrim			= lPrimStream.Len();
	_VB.m_PrimType		= CRC_RIP_STREAM;

	if (m_lLightVB[iVB].m_lPosition.GetBasePtr())
		_VB.Geometry_VertexArray(m_lLightVB[iVB].m_lPosition.GetBasePtr());
	_VB.m_nV			= m_lLightVB[iVB].m_lPosition.Len();
	if (m_lLightVB[iVB].m_lTexCoord.GetBasePtr())
		_VB.Geometry_TVertexArray(m_lLightVB[iVB].m_lTexCoord.GetBasePtr(), 0);
}

void CBSP3_LightData::Release( int _iLocal )
{
	lPrimStream.Destroy();
}

void CBSP3_LightData::OnPostPrecache()
{
	MAUTOSTRIP( CBSP3_LightData_OnPostPrecache, MAUTOSTRIP_VOID );

	if( !m_KeepData )
	{
		m_lLights.Destroy();
		m_lLightStrips.Destroy();
		m_lVertexBufferSize.Destroy();
		m_lLightVB.Destroy();
		m_lLMCGameData.Destroy();
		m_lLightStrips.Destroy();
	}
}

void CBSP3_LightGenData::Write(CDataFile* _pDFile)
{
#ifndef	PLATFORM_CONSOLE
	_pDFile->BeginEntry("LIGHTMAP3");
	_pDFile->EndEntry(BSP3_LIGHT_VER);
	_pDFile->BeginSubDir();
		if( m_spLMTC )
		{
			_pDFile->BeginEntry("IMAGELIST");
			_pDFile->EndEntry(0);
			_pDFile->BeginSubDir();
				m_spLMTC->WriteImageList( _pDFile );
			_pDFile->EndSubDir();
		}

		_pDFile->BeginEntry("VERTEXBUFFERS");
		for( int iVB = 0; iVB < m_lLightVB.Len(); iVB++ )
		{
			uint16 Size = m_lLightVB[iVB]->m_lPosition.Len();
			_pDFile->GetFile()->Write( &Size, 2 );
		}
		for( int iVB = 0; iVB < m_lLightVB.Len(); iVB++ )
		{
			_pDFile->GetFile()->Write( m_lLightVB[iVB]->m_lPosition.GetBasePtr(), m_lLightVB[iVB]->m_lPosition.ListSize() );
			_pDFile->GetFile()->Write( m_lLightVB[iVB]->m_lTexCoord.GetBasePtr(), m_lLightVB[iVB]->m_lTexCoord.ListSize() );
		}
		_pDFile->EndEntry(m_lLightVB.Len());

		_pDFile->BeginEntry("CLUSTERS");
		for( int iLMC = 0; iLMC < m_lLMCData.Len(); iLMC++ )
		{
			CBSP3_LMCRenderData Data;
			Data.m_iTex	= 0;
			Data.m_iVB	= 0;
			if( m_lLMCData[iLMC].m_lStrips.Len() > 0 )
			{
				Data.m_iTex = m_lLMCData[iLMC].m_lStrips[0].m_iLMC;
				Data.m_iVB	= m_lLMCData[iLMC].m_lStrips[0].m_iVB;
			}
			_pDFile->GetFile()->Write( &Data, 1 );
		}

		int nStrip = 0;
		TThinArray<CBSP3_LMCGameData>	lLMCGameData;
		lLMCGameData.SetLen( m_lLMCData.Len() );
		for( int iLMC = 0; iLMC < m_lLMCData.Len(); iLMC++ )
		{
			uint16 Size = m_lLMCData[iLMC].m_lStrips.Len();
			lLMCGameData[iLMC].m_iStrip	= nStrip;
			lLMCGameData[iLMC].m_nStrip	= Size;
			nStrip	+= Size;
		}
		_pDFile->GetFile()->Write( lLMCGameData.GetBasePtr(), lLMCGameData.ListSize() );
		for( int iLMC = 0; iLMC < m_lLMCData.Len(); iLMC++ )
		{
			_pDFile->GetFile()->Write( m_lLMCData[iLMC].m_lStrips.GetBasePtr(), m_lLMCData[iLMC].m_lStrips.ListSize() );
		}
		_pDFile->EndEntry(m_lLMCData.Len());

	_pDFile->EndSubDir();
#endif
}

void CBSP3_LMCData::Read(CDataFile* _pDFile)
{
	int nLen = _pDFile->GetUserData();
	m_lStrips.SetLen( nLen );
	M_ASSERT( m_lStrips.ListSize() == _pDFile->GetUserData2(), "!" );
	_pDFile->GetFile()->Read( m_lStrips.GetBasePtr(), m_lStrips.ListSize() );
}

void CBSP3_LMCData::Write(CDataFile* _pDFile) const
{
#ifndef	PLATFORM_CONSOLE
	_pDFile->BeginEntry("STRIPS");
	_pDFile->GetFile()->Write( m_lStrips.GetBasePtr(), m_lStrips.ListSize() );
	_pDFile->EndEntry(m_lStrips.Len(), m_lStrips.ListSize());
#endif
}

void CBSP3_LightVB::Read(CDataFile* _pDFile)
{
	int nLen = _pDFile->GetUserData();
	m_lPosition.SetLen( nLen );
	m_lTexCoord.SetLen( nLen );
	M_ASSERT( m_lPosition.ListSize() == _pDFile->GetUserData2(), "!" );
	_pDFile->GetFile()->Read( m_lPosition.GetBasePtr(), m_lPosition.ListSize() );
	_pDFile->GetFile()->Read( m_lTexCoord.GetBasePtr(), m_lTexCoord.ListSize() );
}

void CBSP3_LightVB::Write(CDataFile* _pDFile) const
{
#ifndef	PLATFORM_CONSOLE
	int nLen = m_lPosition.Len();
	_pDFile->BeginEntry("VERTEXBUFFER");
	_pDFile->GetFile()->Write( m_lPosition.GetBasePtr(), m_lPosition.ListSize() );
	_pDFile->GetFile()->Write( m_lTexCoord.GetBasePtr(), m_lTexCoord.ListSize() );
	_pDFile->EndEntry(nLen, m_lPosition.ListSize());
#endif
}

void CBSP3_LightData::ReadClusters( class CCFile* _pFile )
{
	m_lLMCGameData.SetLen( m_ClusterCount );
	if (m_ClusterCount)
	{
		_pFile->Seek( m_ClusterPos );
		_pFile->Read( m_lLMCGameData.GetBasePtr(), m_lLMCGameData.ListSize() );
		int nStrip = m_lLMCGameData[m_ClusterCount-1].m_iStrip + m_lLMCGameData[m_ClusterCount-1].m_nStrip;
		m_lLightStrips.SetLen( nStrip );
		_pFile->Read( m_lLightStrips.GetBasePtr(), m_lLightStrips.ListSize() );
	}
}

void CBSP3_LightData::GetClusters()
{
	MSCOPESHORT(CBSP3_LightData::GetClusters);
	if( m_lLMCGameData.Len() == 0 )
	{
		CCFile File;
		File.Open( m_Filename, CFILE_READ|CFILE_BINARY);
		ReadClusters( &File );
	}
}

void CBSP3_LightData::ReadVertexBuffers( class CCFile* _pFile)
{
	m_lLightVB.SetLen( m_VertexBufferCount );
	_pFile->Seek( m_VertexBufferPos );
	m_lVertexBufferSize.SetLen( m_VertexBufferCount );
	_pFile->Read( m_lVertexBufferSize.GetBasePtr(), m_lVertexBufferSize.ListSize() );
	for( int iVB = 0; iVB < m_VertexBufferCount; iVB++ )
	{
		int Size = m_lVertexBufferSize[iVB];
		m_lLightVB[iVB].m_lPosition.SetLen( Size );
		m_lLightVB[iVB].m_lTexCoord.SetLen( Size );
		_pFile->Read( m_lLightVB[iVB].m_lPosition.GetBasePtr(), m_lLightVB[iVB].m_lPosition.ListSize() );
		_pFile->Read( m_lLightVB[iVB].m_lTexCoord.GetBasePtr(), m_lLightVB[iVB].m_lTexCoord.ListSize() );
	}
}

void CBSP3_LightData::GetVertexBuffers()
{
	MSCOPESHORT(CBSP3_LightData::GetVertexBuffers);
	if( m_lLightVB.Len() == 0 )
	{
		CCFile File;
		File.Open( m_Filename, CFILE_READ|CFILE_BINARY);
		ReadVertexBuffers( &File );
	}
}

void CBSP3_LightData::ReadLights( class CCFile* _pFile )
{
	_pFile->Seek( m_LightsPos );
	m_lLights.SetLen(m_LightsCount); 
	CXR_Light* pL = m_lLights.GetBasePtr();

	for (int iL=0; iL < m_LightsCount; iL++)
	{
		pL[iL].Read(_pFile, m_LightsVer);
	}
}

CXR_Light* CBSP3_LightData::GetLights()
{
	MSCOPESHORT(CBSP3_LightData::GetLights);
	if( m_lLights.Len() == 0 )
	{
		CCFile File;
		File.Open( m_Filename, CFILE_READ|CFILE_BINARY);
		ReadLights( &File );
	}
	
	return m_lLights.GetBasePtr();
}
