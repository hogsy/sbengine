#ifndef _INC_XRBlockNavGrid_GameWorld
#define _INC_XRBlockNavGrid_GameWorld

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CXR_BlockNav_Grid inherited to support dynamic objects in a 
					CWorld_PhysState.
					
	Author:			Magnus Högdahl
					
	Copyright:		2002 Starbreeze Studios AB
					
	Contents:		CXR_BlockNav_Grid_GameWorld
					
	History:		
		020116:		Created File
\*____________________________________________________________________________________________*/


#include "WPhysState.h"
#include "../../XR/XRBlockNav.h"
#include "../../XR/XRBlockNavGrid.h"

class CXR_BlockNav_Grid_GameWorld : public CXR_BlockNav_Grid
{
	MRTC_DECLARE;

protected:
	CWorld_PhysState* m_pWPhysState;

	CXR_PhysicsModel* m_pCurrentRenderModel;	// RenderModel_r parameter, is member variable to reduce recursion overhead.
	CDecompressedTile* m_pCurrentRenderTile;	// RenderModel_r parameter, is member variable to reduce recursion overhead.
	CVec3Dfp32 m_CurrentTileWPos;				// RenderModel_r parameter, is member variable to reduce recursion overhead.

	void RenderModel_r(CXR_PhysicsContext* _pPhys, const CVec3Dint32& _Origin, int _Size) const;
	void RenderBox(const CBox3Dfp32& _Box, int _CellValue = 0);
	void RenderOBB(const CBox3Dfp32& _Box, const CMat4Dfp32& _Pos, int _CellValue = 0);

	// Override in CXR_BlockNav_Grid
	virtual void OnPostDecompressTile(CDecompressedTile& _Tile);

public:
	CXR_BlockNav_Grid_GameWorld();
	virtual void Create(int _nCachedTiles, CWorld_PhysState* _pWPhysState);

	virtual void InvalidateBox(const CBox3Dfp32& _Box);
};

//----------------------------------------------------------------------------------------------------
class CXR_BlockNavSearcher_VPUDispatcher : public CXR_BlockNavSearcher
{
	MRTC_DECLARE;
private:
	class CVPUSearchInstance : public CReferenceCount
	{
	public:
		int32 m_JobID;
		bool m_bInUse;

		int m_LastReleasedTick;
		CXBN_SearchParams m_Params;
		CXR_BlockNav_GridProvider *m_pGrid;
		CXR_BlockNav* m_pBlockNav;
		CWO_SpaceEnum_RW_NavGrid *m_pNavGridSpace;

		// result
		enum
		{
			MAX_RESULT_NODES=512,
		};

		int m_ReturnValue;
		int m_NumResultNodes;
		int m_Cost;
		TThinArrayAlign<CXBN_SearchResultNode,16> m_lResultBuffer;
		TThinArrayAlign<CWO_Hash_Settings,16> m_lSettings;

		//
		CVPUSearchInstance();
		~CVPUSearchInstance();

		bool InUse(int _GameTick)
		{
			if ((_GameTick == -1) || (m_LastReleasedTick == -1))
				return m_bInUse;
			else
				return m_bInUse || (_GameTick == m_LastReleasedTick);
		};

		void Create(CWO_SpaceEnum_RW_NavGrid *_pNavGridSpace, CXR_BlockNav_GridProvider* _Grid, CXR_BlockNav* _pBlockNav);

		void Search_Create(const CXBN_SearchParams& _Params);
		int Search_Execute(int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop = 2);
		bool Search_Get(CXBN_SearchResultCore*  _pRes, bool _ReturnPartial = false);
		int Search_GetUser();

		void Release(int _GameTick)
		{
			m_bInUse = false;
			m_LastReleasedTick = _GameTick;
		};
	};

	CIndexPool16 m_NavGridSpaceUpdates;
	CWO_SpaceEnum_RW_NavGrid m_NavGridSpace;
	TArray<TPtr<CVPUSearchInstance> > m_lspSearches;
	CXR_BlockNav *m_pBlockNav;

	void UpdateNavGrid();

public:

	CXR_BlockNavSearcher_VPUDispatcher();
	virtual ~CXR_BlockNavSearcher_VPUDispatcher();

	CWorld_PhysState *m_pPhysState;

	virtual void Create(class CXR_BlockNav *_pBlockNav, int _MaxInstances);

	virtual void RegisterObjectMovement(int _iObj);

	virtual int Search_Create(const CXBN_SearchParams& _Params, int _GameTick = -1);
	virtual int Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop = 2);
	virtual bool Search_Get(int _iSearch, class CXBN_SearchResultCore* _pRes, bool _ReturnPartial = false);
	virtual void Search_Destroy(int _iSearch, int _GameTick = -1);
	virtual int Search_GetUser(int _iSearch);
	virtual int Search_GetNum();
	
	virtual void Debug_Render(CWireContainer* _pWC, CVec3Dfp32 _Pos);
};



//----------------------------------------------------------------------------------------------------
class CXR_BlockNavSearcher_Plain : public CXR_BlockNavSearcher
{
	MRTC_DECLARE;
private:
	TArray<TPtr<CXBN_SearchInstance> > m_lspSearches;
	CXR_BlockNav *m_pBlockNav;

	void UpdateNavGrid();

public:
	virtual void Create(class CXR_BlockNav *_pBlockNav, int _MaxInstances);

	virtual void RegisterObjectMovement(int _iObj);

	virtual int Search_Create(const CXBN_SearchParams& _Params, int _GameTick = -1);
	virtual int Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop = 2);
	virtual bool Search_Get(int _iSearch, class CXBN_SearchResultCore* _pRes, bool _ReturnPartial = false);
	virtual void Search_Destroy(int _iSearch, int _GameTick = -1);
	virtual int Search_GetUser(int _iSearch);
	virtual int Search_GetNum();
	
	virtual void Debug_Render(CWireContainer* _pWC, CVec3Dfp32 _Pos);
};

#endif // _INC_XRBlockNavGrid_GameWorld

