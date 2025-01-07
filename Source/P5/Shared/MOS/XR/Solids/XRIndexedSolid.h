
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Indexed solid

	Author:			Magnus Högdahl

	Copyright:		Starbreeze AB 2005
					
	Contents:		

	History:		
		050916:		Created File. 


	Space for a cube est. (excluding surfaces)

							"CStaticSolid"		IndexedSolid (32-bit)	IndexedSolid (local 16-bit)		BSP-Shared (32-bit index)			BSP-Shared Plane Only (32-bit index)
	8 * Vert				4+12*8 = 100		12*8 = 96				12*8 = 96															12*8 = 96
	12 * Edge				4+4*12 = 52			8*12 = 96				4*12 = 48						8*12 = 96							4*12 = 48
	24 * iVert				4+2*24 = 52			4*24 = 96				2*24 = 48						2*24 = 48 (tripple deref)			2*24 = 48
																										4*8 = 32							
	6 * Face				4+8*6 = 52			8*6 = 48				8*6 = 48						12*6 = 48 (incl plane index)		8*6 = 48 (incl plane index)
	6 * Plane				4+16*6 = 100		16*6 = 96				16*6 = 96
	Array Mem Overhead		64 (?)
	Array Indices								24						24								4									8

	Total bytes				420					456						360								228									244

\*____________________________________________________________________________________________*/

#ifndef __INC_XRIndexedSolid
#define __INC_XRIndexedSolid

#include "../XRClass.h"
#ifndef PLATFORM_CONSOLE
	#include "XWSolid.h"
#endif

enum
{
	XR_INDEXEDSOLID_VERSION	=		0x0100,
	XR_INDEXEDSOLIDFACE_VERSION	=	0x0100,
};
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_IndexedSolidContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_IndexedSolidFace
{
public:
	uint8 m_Flags;
	uint8 m_nV;
	uint16 m_iiV;			// Relative solid base index
	uint16 m_iSurface;

	void Read(CCFile* _pFile, int _Version);
	void Write(CCFile* _pFile);
	void SwapLE();
};

// -------------------------------------------------------------------
class CXR_IndexedSolid
{
public:
	CBox3Dfp32 m_BoundBox;

	uint32 m_iVertices;
	uint32 m_iEdges;
	uint32 m_iFaces;
	uint32 m_iiVertices;
	uint16 m_nVertices;
	uint16 m_nEdges;
	uint16 m_nFaces;
	uint16 m_niVertices;

	uint16 m_iMedium;


	void Read(CCFile* _pFile, int _Version);
	void Write(CCFile* _pFile);
	void SwapLE();
};

// -------------------------------------------------------------------
class CXR_IndexedSolidDesc
{
public:
	TAP<const CVec3Dfp32> m_pV;
	TAP<const CIndexedEdge16> m_pE;
	TAP<const CXR_IndexedSolidFace> m_pF;
	const CPlane3Dfp32* m_pP;
	const uint16* m_piV;
};

// -------------------------------------------------------------------
class CXR_IndexedSolidContainer : public CReferenceCount
{
public:
	TThinArray<CXR_IndexedSolid> m_lSolids;
	TThinArray<CPlane3Dfp32> m_lPlanes;
	TThinArray<CXR_IndexedSolidFace> m_lFaces;
	TThinArray<CIndexedEdge16> m_lEdges;		// Relative solid base index
	TThinArray<CVec3Dfp32> m_lVertices;
	TThinArray<uint16> m_liVertices;			// Relative solid base index
	TThinArray<CXR_MediumDesc> m_lMediums;
	TThinArray<spCXW_Surface> m_lspSurfaces;

#ifndef PLATFORM_CONSOLE
	void Create(TArray<spCSolid> _lspSolids);
#endif

	void GetSolid(int _iSolid, CXR_IndexedSolidDesc& _Desc) const;

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile);
};

typedef TPtr<CXR_IndexedSolidContainer> spCXR_IndexedSolidContainer;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_IndexedSolidContainer32
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_IndexedSolidFace32
{
public:
	uint8 m_Flags;
	uint8 m_nV;
	uint16 m_iSurface;
	uint16 m_iiV;
	uint16 m_iPlane;

	void Read(CCFile* _pFile, int _Version);
	void Write(CCFile* _pFile);
	void SwapLE();
};

// -------------------------------------------------------------------
class CXR_IndexedSolid32
{
public:
	CBox3Dfp32 m_BoundBox;

	uint32 m_iVertices;
	uint32 m_iEdges;
	uint32 m_iFaces;
	uint32 m_iiVertices;
	uint16 m_nVertices;
	uint16 m_nEdges;
	uint16 m_nFaces;
	uint16 m_niVertices;

	uint32 m_iColinearPlanes;
	uint16 m_nColinearPlanes;

	uint32 m_iColinearEdges;
	uint16 m_nColinearEdges;

	uint16 m_iMedium;

	fp32 m_Friction;

	void Read(CCFile* _pFile, int _Version);
	void Write(CCFile* _pFile);
	void SwapLE();
};

// -------------------------------------------------------------------
class CXR_IndexedSolidDesc32
{
public:
	TAP_RCD<const CVec3Dfp32> m_pV;
	TAP_RCD<const CIndexedEdge16> m_pE;
	TAP_RCD<const CXR_IndexedSolidFace32> m_pF;
	TAP_RCD<const CPlane3Dfp32> m_pP;
	TAP_RCD<const uint16> m_piV;

	TAP_RCD<const uint16> m_piColinearP;
	TAP_RCD<const CIndexedEdge16> m_pColinearE;

	fp32 m_Friction;

	CBox3Dfp32 m_BoundBox;
};

// -------------------------------------------------------------------
class CXR_IndexedSolidContainer32 : public CReferenceCount
{
public:
	// Shared data
	TArray<spCXW_Surface> m_lspSurfaces;
	TArray<CXR_MediumDesc> m_lMediums;
	TAP_RCD<const CPlane3Dfp32> m_pPlanes;

	// Owned data
	TThinArray<CVec3Dfp32> m_lVertices;
	TThinArray<uint16> m_liVertices;
	TThinArray<CIndexedEdge16> m_lEdges;
	TThinArray<CXR_IndexedSolidFace32> m_lFaces;

	TThinArray<uint16> m_liColinearPlanes;
	TThinArray<CIndexedEdge16> m_lColinearEdges;

	TThinArray<CXR_IndexedSolid32> m_lSolids;

	TThinArray<CPhysOBB> m_lObbs;
	TThinArray<bool> m_lIsObbs;

	static int FindPlane(TArray<CPlane3Dfp64>& _lPlanes, const CPlane3Dfp64& _Plane);

#ifndef PLATFORM_CONSOLE
	void Create(TArray<spCSolid> _lspSolids, TArray<spCXW_Surface>& _lspSurfaces, TArray<CXR_MediumDesc>& _lMediums, TArray<CPlane3Dfp64>& _lPlanes);	// For creating compiled data from source data
#endif
	void Create(TArray<spCXW_Surface>& _lspSurfaces, TArray<CXR_MediumDesc>& _lMediums, TAP_RCD<const CPlane3Dfp32> _pPlanes);				// For creating from compiled data

	TAP<const CXR_IndexedSolid32> GetSolids() const { return m_lSolids; };
	void GetSolid(int _iSolid, CXR_IndexedSolidDesc32& _Desc) const;

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile);

	void RelocateMediums(const uint16* _piMapping, int _nMapping);

protected:
	void UpdateColinearDirections();
	void UpdateObbs();
	void UpdateFriction();
};

typedef TPtr<CXR_IndexedSolidContainer32> spCXR_IndexedSolidContainer32;




#endif // #ifndef __INC_XRIndexedSolid


