#ifndef __WTRIMESHDC
#define __WTRIMESHDC

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CXR_TriangleMeshDecalContainer, a container class for global
					memory management of decals for all trimeshes in a world.

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios 2003

\*____________________________________________________________________________________________*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_TriangleMeshDecalContainer
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_TMDC_Decal
{
public:
//	CMTime m_TimeCreated;
	CVec3Dfp32 m_TexU;
	CVec3Dfp32 m_TexV;
	CVec3Dfp32 m_Pos;
	CVec3Dfp32 m_Normal;
	uint8 m_iBone;
	uint8 m_iCluster;
	uint16 m_SurfaceID;
	mint m_ModelGUID;
	CMTime m_SpawnTime;

	uint16 m_GUID;
	uint16 m_iIndices;
	uint16 m_nIndices;
	uint16 m_iDecalNext;
	uint16 m_iDecalPrev;
};

// -------------------------------------------------------------------
class CXR_TriangleMeshDecalContainer : public CReferenceCount
{
	MRTC_DECLARE;

protected:
	TThinArray<uint16> m_lIndices;
	TThinArray<CXR_TMDC_Decal> m_lDecals;
	int m_nIndices;		// Same as m_lIndices.Len()
	int m_nDecals;		// Same as m_lDecals.Len()
	int m_iIndexHead;
	int m_iIndexTail;
	int m_iDecalHead;
	int m_iDecalTail;
	bool m_bFreeze;

	CMap16 m_GUIDMap;

	void LinkDecal(int _iDecal);
	void UnlinkDecal(int _iDecal);

	void FreeDecal();												// Removes the oldest decal and it's indices.
	int MaxIndexPut() const;										// Returns the free space in m_lIndeces heap (queue)
	int MaxDecalPut() const;
	int AddIndices(const uint16* _piIndices, int _nIndices);		// Returns position in m_lIndices

public:
	CXR_TriangleMeshDecalContainer();
	virtual void Create(uint16 _IndexHeap, int _DecalHeap);
	virtual void Freeze(bool _bFreeze);		// Freeze is set during rendering and any AddDecal calls are ignored during this time.

	void AddDecal(uint16 _GUID, const CXR_TMDC_Decal& _Decal, const uint16* _piIndices, int _nIndices);

	const CXR_TMDC_Decal* GetDecalFirst(uint16 _GUID) const;
	const CXR_TMDC_Decal* GetDecalNext(const CXR_TMDC_Decal* _pDecal) const;

	uint16* GetIndices() { return m_lIndices.GetBasePtr(); };
};

typedef TPtr<CXR_TriangleMeshDecalContainer> spCXR_TriangleMeshDecalContainer;


#endif	// __WTRIMESHDC

