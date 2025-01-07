
#ifndef _INC_XRBLOCKNAV
#define _INC_XRBLOCKNAV

//#include "WMapData.h"
//#include "XRBlockNavGrid.h"
#include "XRBlockNavGrid_Shared.h"
#include "XRBlockNavParam.h"
#include "XRBlockNavInst.h"

// WBN = World block navigation


// -------------------------------------------------------------------
class CXR_BlockNav : public CReferenceCount
{
	MRTC_DECLARE;
private:
	CXR_BlockNav_GridProvider *m_pGrid;							// Points to resource - strictly read-only.

	//Size constants
	enum {
		CHARSIZE = XR_TRAVERSABILITY_X,
		CHARHEIGHT = XR_TRAVERSABILITY_Z,
		CHARSIDESIZE = XR_TRAVERSABILITY_X * 2 + 1
	};

	//Helper to IsTraversableImpl
	bool Is3x3x4Traversable(bool _bClimb, int _x, int _y, int _z);

	//Helper to IsTraversableImpl for critters smaller than a normal character
	bool IsTraversableSmall(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx = 0, int _dy = 0, int _dz = 0);

	//Helper to IsTraversableImpl for critters bigger than a normal character
	bool IsTraversableBig(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx = 0, int _dy = 0, int _dz = 0);

	//Helper to IsTraversable; checks traversability for movement in xy-plane and up/down, respectively, for normal sized characters
	//Params are always the position to check to, and the diff to a previous node that already have been checked
	bool IsTraversableLevel(bool _bClimb, int16 _x, int16 _y, int16 _z, int16 _dx, int16 _dy);
	bool IsTraversableVertical(bool _bClimb, int16 _x, int16 _y, int16 _z, int16 _dz);
public:
	CXR_BlockNav();
	~CXR_BlockNav();

	void Create(class CXR_BlockNav_GridProvider *_pGrid);

	//CXBN_SearchInstance* Search_GetInstance(int _iSearch);

	
	//Helpers to reason about the navigation grid:

	//Get cell medium (see XRBlockNavGrid.h)
	int GetCellValue(int _x, int _y, int _z);

	//Check if grid position is outside of or too close to the edge of the grid
	bool IsOutsideGridBoundaries(const CVec3Dint16& _Pos, int _iBaseSize = 1, int _iHeight = 1);
	bool IsOutsideGridBoundariesImpl(const CVec3Dint16& _Pos, int _iBaseSize = 0, int _iHeight = 1);

	//Checks if the given grid position is traversable for an entity with the given basesize and height (in world units).
	//The _dx, -y and -z arguments are used for optimizing the check (see WBlockNav.cpp)
	bool IsTraversable(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx = 0, int _dy = 0, int _dz = 0);

	//Checks traversability while assuming that the given coordinates are within the grid. Base size and height are in cells.
	bool IsTraversableImpl(const CVec3Dint16& _Pos, int _iBaseSize, int _iHeight, bool _bClimb, int _dx = 0, int _dy = 0, int _dz = 0);
	
	//Checks if an entity with the given base size and height (in world units) can travel in a straight line from 
	//the start to the end position, following the ground with no steps higher than _iMaxStep units and no falls greater than _iMaxFall units
	//If the _pRes argument is supplied, this value is set to the furthest position along the line from the start to the end that
	//could be reached.
	bool IsGroundTraversable(const CVec3Dfp32& _Start, const CVec3Dfp32& _End, int _iBaseSize, int _iHeight, bool _bClimb, int _iMaxStep = 0, int _iMaxFall = 0, CVec3Dfp32* _pRes = NULL);
	//Below all params are in grid positions and "units". Base size is square radius (see implementation)
	bool IsGroundTraversableImpl(const CVec3Dint16& _Start, const CVec3Dint16& _End, int _iBaseSize, int _iHeight, bool _bClimb, int _iMaxStep = 0, int _iMaxFall = 0, CVec3Dint16* _pRes = NULL);

	//Checks if the given grid position is on the ground (i.e. if an entity can safely stand on it)
	bool IsOnGround(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb);
	bool IsOnGroundImpl(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb);
	bool IsOnGround(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb, int& _iCount);
	bool IsOnGroundImpl(const CVec3Dint16& _Pos, int _iBaseSize, bool _bClimb, int& _iCount);
	
	//Gets the corresponding grid position of the given world position
	CVec3Dint16 GetGridPosition(const CVec3Dfp32& _WorldPos);

	//Gets the corresponding world position of the given grid position
	CVec3Dfp32 GetWorldPosition(const CVec3Dint16& _GridPos);

	//Returns the side length of a cell in world units
	fp32 UnitsPerCell();

	//Returns the "square radius" in cells given base size in world units. This is the number of cells outside 
	//the center cell which the base will reach
	int GridSquareRadius(fp32 _BaseSize);

	//Returns the height in cells given height in world units, rounded up or down, respectively
	int GridHeightCeil(fp32 _Height);
	int GridHeightFloor(fp32 _Height);

	//Grid dimensions of navgrid
	CVec3Dint16 GridDimensions();

	//Get grid pointer (ugly, shouldn't be used but I have no other choice when builing navgraph)
	CXR_BlockNav_GridProvider *GetGrid();	

	//virtual void Debug_Render(CWireContainer* _pWC) {};

	// void BuildNavGraph(const CVec3Dfp32& _Pos, int _iMode);//DEBUG
};


// TODO: Remove virtuals?
class CXR_BlockNavSearcher : public CReferenceCount
{
	MRTC_DECLARE;
	CXR_BlockNav *m_pBlockNav;
protected:
	uint32 m_IsAsync:1;
public:

	//The results that Search_Execute can return 
	enum 
	{
		SEARCH_DONE = 0, 
		SEARCH_IN_PROGRESS,
		SEARCH_SOURCE_INTRAVERSABLE,
		SEARCH_DESTINATION_INTRAVERSABLE,
		SEARCH_SOURCE_OUTOFBOUNDS,
		SEARCH_DESTINATION_OUTOFBOUNDS,
		SEARCH_NO_PATH,
		SEARCH_NODE_ALLOC_FAIL,
		SEARCH_INSTANCE_INVALID,
		SEARCH_INSTANCE_DESTROYED,
		SEARCH_NO_NAVGRID,
	};	

	CXR_BlockNavSearcher() : m_pBlockNav(0), m_IsAsync(0)
	{
	}

	virtual ~CXR_BlockNavSearcher()
	{
	}

	virtual void Create(CXR_BlockNav *_pBlockNav, int _MaxInstances)
	{
		m_pBlockNav = _pBlockNav;
	}

	virtual void RegisterObjectMovement(int _iObj) {}

	bool Search_IsAsync() const {return m_IsAsync;}

	//Create search with the given params, at the given game tick. If game tick is unspecified we can 
	//create a search even if it was previously used this frame
	virtual int Search_Create(const CXBN_SearchParams& _Params, int _GameTick = -1) {return 0;}

	//Perform search, expanding no more than _nNodeExpansions nodes in one go and reducing this param 
	//by the number of nodes expanded. If the _nMaxExpansions param is positiove, search fails when more
	//than this number of nodes have been expanded in total. Returns one of the above results.
	//_nMaxSlop is the number of cells away we treat as on target
	virtual int Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop = 2) { return 0; }
	
	//Get result of search
	virtual bool Search_Get(int _iSearch, class CXBN_SearchResultCore* _pRes, bool _ReturnPartial = false) { return false; }

	//Release search instance. If a gametick is specified, this instance cannot normally 
	//be re-allocated the same game tick
	virtual void Search_Destroy(int _iSearch, int _GameTick = -1) { }

	//Get user of search instance
	virtual int Search_GetUser(int _iSearch) { return 0; }

	//Get number of search instances
	virtual int Search_GetNum() { return 0; }

	virtual void Debug_Render(CWireContainer* _pWC, CVec3Dfp32 _Pos) {};


	CXR_BlockNav *GetBlockNav() const { return m_pBlockNav; }
};

/*
#ifndef PLATFORM_VPU

class CXR_BlockNavSearcher_Plain : public CXR_BlockNavSearcher
{
	TArray<TPtr<CXBN_SearchInstance> > m_lspSearches;
public:
	virtual void Create(CXR_BlockNav *_pBlockNav, int _MaxInstances);
	virtual int Search_Create(const CXBN_SearchParams& _Params, int _GameTick = -1);
	virtual int Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions,int _nMaxSlop = 2);
	virtual bool Search_Get(int _iSearch, CXBN_SearchResultCore* _pRes, bool _ReturnPartial = false);
	virtual void Search_Destroy(int _iSearch, int _GameTick = -1);
	virtual int Search_GetUser(int _iSearch);
	virtual int Search_GetNum();
};

#endif
*/


#endif // _INC_XRBLOCKNAV
