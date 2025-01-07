#include "PCH.h"

#include "XW4Common.h"
#include "MFloat.h"

#include "../../XR/XRVBContext.h"

void CBSP4_Node::Read(CCFile* _pF, int _Ver)
{
	MAUTOSTRIP(CBSP4_Node_Read, MAUTOSTRIP_VOID);
	switch(_Ver)
	{
	case 0x0200 :
		{
			struct	// This is a copy of CBSP_Node (to remove dependancies between BSP models)
			{
				uint16 iPlane;
				uint16 iNodeFront;
				uint16 iNodeBack;
				uint16 iNodeParent;
				uint32 iiFaces;
				uint16 Flags;
				uint16 iPortalLeaf;
			} TempRead;
			_pF->ReadLE( (uint8*)&TempRead, sizeof( TempRead ) );

			::SwapLE( TempRead.iPlane );
			::SwapLE( TempRead.iNodeFront );
			::SwapLE( TempRead.iNodeBack );
			::SwapLE( TempRead.iiFaces );

#if !defined M_CONSOLE && !defined M_RTM
/*			if (TempRead.iiFaces > 0xffff)
				Error_static("CBSP4_Node::Read", "m_iiFaces > 0xffff");*/
#endif

			m_iPlane = TempRead.iPlane;
			m_iNodeFront = TempRead.iNodeFront;
			m_iNodeBack = TempRead.iNodeBack;
			m_iiFaces = TempRead.iiFaces;
//			m_Flags = Node.m_Flags;
//			m_iPortalLeaf = Node.m_iPortalLeaf;
		}
		break;

	case 0x0201:
		{
			struct
			{
				uint32 m_iNodeFront;
				uint32 m_iNodeBack;
				uint32 m_iNodeParent;
				M_BITFIELD2(uint32, m_iiFaces, 24, m_Flags, 8) m_NodeIO;
				uint16 m_iPlane;
				uint16 m_iPortalLeaf;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iNodeFront);
			::SwapLE(TempRead.m_iNodeBack);
			TempRead.m_NodeIO.SwapLE();
			::SwapLE(TempRead.m_iPlane);
			
			m_iNodeFront = TempRead.m_iNodeFront;
			m_iNodeBack = TempRead.m_iNodeBack;
			m_iiFaces = TempRead.m_NodeIO.m_iiFaces;
			m_iPlane = TempRead.m_iPlane;
		}
		break;

	case 0x0202:
		{
			struct
			{
				uint32 m_iNodeFront;
				uint32 m_iNodeBack;
				uint32 m_iNodeParent;
				M_BITFIELD2(uint32, m_iiFaces, 24, m_Flags, 8) m_NodeIO;
				uint32 m_iPlane;
				uint16 m_iPortalLeaf;
				uint16 m_Padding0;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			::SwapLE(TempRead.m_iNodeFront);
			::SwapLE(TempRead.m_iNodeBack);
			TempRead.m_NodeIO.SwapLE();
			::SwapLE(TempRead.m_iPlane);
			
			m_iNodeFront = TempRead.m_iNodeFront;
			m_iNodeBack = TempRead.m_iNodeBack;
			m_iiFaces = TempRead.m_NodeIO.m_iiFaces;
			m_iPlane = TempRead.m_iPlane;
		}
		break;

	default :
		Error_static("CBSP4_Node::Read", CStrF("Unsupported version %.4x", _Ver));
	}

}

void CBSP4_Node::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CBSP4_Node_Write, MAUTOSTRIP_VOID);
	Error_static("CBSP4_Node::Write", "Not supported!");
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4_CoreFace
|__________________________________________________________________________________________________
\*************************************************************************************************/
CBSP4_CoreFace::CBSP4_CoreFace()
{
	MAUTOSTRIP(CBSP4_CoreFace_ctor, MAUTOSTRIP_VOID);
	m_iiVertices = 0;

	m_nVertices = 0;
	m_iSurface = 0;
	m_Flags = 0;

	m_iPlane = 0;
	m_iBackMedium = 0;

}

void CBSP4_CoreFace::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CBSP4_CoreFace_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0x0203:
		{
			struct
			{
				int32 m_iiEdges;


				M_BITFIELD2(uint32, m_iiVertices, 24, m_nVertices, 8) m_VertexIO;

				uint32 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint32 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				M_BITFIELD2(uint32, m_iLightInfo, 28, m_nLightInfo, 4) m_LightInfoIO;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			TempRead.m_VertexIO.SwapLE();
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iBackMedium);

			m_iiVertices	= TempRead.m_VertexIO.m_iiVertices;
			m_nVertices		= TempRead.m_VertexIO.m_nVertices;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iBackMedium = TempRead.m_iBackMedium;
		}
		break;

	case 0x0202:
		{
			struct
			{
				int32 m_iiEdges;


				M_BITFIELD2(uint32, m_iiVertices, 24, m_nVertices, 8) m_VertexIO;

				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint32 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				M_BITFIELD2(uint32, m_iLightInfo, 28, m_nLightInfo, 4) m_LightInfoIO;
			} TempRead;

			_pF->Read(&TempRead, sizeof(TempRead));
			TempRead.m_VertexIO.SwapLE();
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iBackMedium);

			m_iiVertices	= TempRead.m_VertexIO.m_iiVertices;
			m_nVertices		= TempRead.m_VertexIO.m_nVertices;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iBackMedium = TempRead.m_iBackMedium;
		}
		break;

	case 0x0201:
		{
			struct CIOFormat
			{
				uint32 m_iiEdges;

				uint32 m_iiVertices;	// Index to first vertex-index
				uint16 m_nVertices;		// Number vertices
				uint16 m_iMapping;
				uint16 m_iSurface;
				uint16 m_Flags;

				uint16 m_iPlane;
				uint16 m_iiNormals;		// Index to first normal-index
				uint16 m_iBackMedium;	// What's behind the face.
				uint16 m_iFrontMedium;	// What's in front of the face.

				M_BITFIELD2(uint32, m_iLightInfo, 28, m_nLightInfo, 4) m_LightInfoIO;
			};

			CIOFormat Face;
			_pF->Read(&Face, sizeof(Face));

			::SwapLE(Face.m_iiVertices);
			::SwapLE(Face.m_nVertices);
			m_iiVertices = Face.m_iiVertices;
			m_nVertices = Face.m_nVertices;

			::SwapLE(Face.m_iSurface);
			m_iSurface = Face.m_iSurface;
			::SwapLE(Face.m_Flags);
			m_Flags = Face.m_Flags;

			::SwapLE(Face.m_iPlane);
			m_iPlane = Face.m_iPlane;

			::SwapLE(Face.m_iBackMedium);
			m_iBackMedium = Face.m_iBackMedium;
			break;
		};
	case 0x0200 :
			uint32 iiEdges;
			_pF->ReadLE(iiEdges);

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
			::SwapLE(TempRead.m_iSurface);
			::SwapLE(TempRead.m_Flags);
			::SwapLE(TempRead.m_iPlane);
			::SwapLE(TempRead.m_iBackMedium);

			m_iiVertices = TempRead.m_iiVertices;
			m_nVertices = TempRead.m_nVertices;
			m_iSurface = TempRead.m_iSurface;
			m_Flags = TempRead.m_Flags;
			m_iPlane = TempRead.m_iPlane;
			m_iBackMedium = TempRead.m_iBackMedium;
		}
		break;
	default :
		Error_static("CBSP4_CoreFace::Read", CStrF("Invalid version: %.4x", _Version));
	}
}

void CBSP4_CoreFace::Write(CCFile* _pF)
{
	MAUTOSTRIP(CBSP4_CoreFace_Write, MAUTOSTRIP_VOID);
	Error_static("CBSP4_CoreFace::Write", "Not supported!");
}

#ifndef CPU_LITTLEENDIAN
void CBSP4_CoreFace::SwapLE()
{
	MAUTOSTRIP(CBSP4_CoreFace_SwapLE, MAUTOSTRIP_VOID);
	Error_static("CBSP4_CoreFace::Write", "Not supported!");
}
#endif

