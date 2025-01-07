#ifndef _INC_AI_AUXILIARY
#define _INC_AI_AUXILIARY

#include "MFloat.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WClassCmd.h"

class CAI_Core;

///////////////////////////////
// Classes:
//	CAI_ControlHandler
//	CAI_CharacterInfo
//	CPositionGenerator
//	CPosition_Generator_HalfSPhere
//  CAI_Resource_Pathfinding
//  CAI_Resource_Activity
//	CAI_ActivityCounter
//	CAI_AuxMath
//	TSimpleDynamicList	
//	TSimpleSortedList	
//	TIntHashMap
//	TProximityHash
///////////////////////////////


//Interface class used to construct a character control frame from the current device information 
class CAI_ControlHandler
{
private:
	//The AI
	CAI_Core* m_pAI;

	//The control frame
	CControlFrame m_Ctrl;

	//Return the control state bit(s) for the given device, or 0 if device does not have a corresponding state bit
	uint32 DeviceStateBit(int _iDevice, int _iData);

	//Construct and add command to given control frame, corresponding to command from the given device.
	void AddCommand(int _iDevice, int _iData, const CVec3Dfp32& _VecData, CControlFrame * _pCtrl);

public:
	CAI_ControlHandler();
	void ReInit(CAI_Core * _pAI);
	void SetAI(CAI_Core * _pAI);

	//Build and get the AI control frame, by polling the devices
	const CControlFrame* GetControlFrame();
};


//Name-value pair of strings
class CNameValue 
{
public:
	CStr m_Name;
	CStr m_Value;
	CNameValue();
	CNameValue(CStr _Name, CStr _Value);
};


//Generates successive CVec3Dfp32 positions with certain properties (base class)
class CPositionGenerator
{
protected:	
	//Current generated position. This will of course be the last generated position if we're done.
	CVec3Dfp32 m_CurPos;

	//Is this initialized and have more positions to generate?
	bool m_bValid;

	//Reset stuff when we're done generating positions.
	void OnFinished();

public:
	//Constructor
	CPositionGenerator();

	//(Re-)Initialize generator with default arguments
	virtual void Init();

	//Retrieve current position.
	CVec3Dfp32 GetPosition();

	//Calculate and retrieve next position, or CVec3Dfp32(_FP32_MAX) if there are no more positions
	virtual CVec3Dfp32 GetNextPosition() = 0;

	//Check if this is initialized and can generate more positions
	bool IsValid();
};


//Generates positions on the surface of a half sphere, starting at the front point and continuing in 
//successive circles to the base plane of the half-circle.
class CPositionGenerator_HalfSphere : public CPositionGenerator
{
protected:	
	//The (normalized) position matrix which determines the half-spheres center and orientation
	CMat4Dfp32 m_Mat;

	//The radius of the half-sphere
	fp32 m_Radius;

	//The density and circle density
	fp32 m_Density;

	//The maximum deviation
	fp32 m_MaxDeviation;

	//The current deviation, deviation delta, radius (of current circle), angle (on circle plane) and  angle delta
	fp32 m_Dev;
	fp32 m_dDev;
	fp32 m_Rad;
	fp32 m_Angle; //In radians
	fp32 m_dAngle;//In radians

	//Set up stuff for generating positions on next circle. Fail if we're finished.
	bool OnNextCircle();

public:
	//(Re-)Initialize generator. The (optional) arguments are the center and orientation, specified as 
	//either a matrix or as two vectors (orientation vector is (Tilt,Pitch,Heading)-angles in fractions) 
	//the radius, the density of generated positions (i.e. the minimum distance between any two positions)
	//and the maximum deviation in fractions from the front point. 
	//Default initilization is a unit half circle with the yz-plane as base.
	virtual void Init();
	virtual void Init(const CMat4Dfp32& _Mat, fp32 _Radius = 1.0f, fp32 _Density = 1.0f, fp32 _MaxDeviation = 0.25f);
	virtual void Init(const CVec3Dfp32& _Center, const CVec3Dfp32& _Orientation, fp32 _Radius = 1.0f, fp32 _Density = 1.0f, fp32 _MaxDeviation = 0.25f);

	//First position is always the front point of the halfcircle. Consecutive positions will be placed
	//along the rim of circle further along the half-sphere. The distance between each circle, and the
	//number of positions on each circle will depend on the density
	virtual CVec3Dfp32 GetNextPosition();
};


//Resource handler for pathfinding instances. Pretty incomplete... just a temporary hopefully
class CAI_Resource_Pathfinding
{
	//The characters whose AIs are currently using the resource and their corresponing priority
	struct ResourceHolder
	{
		int32 m_iUser;
		uint8 m_iPrio;
		bool m_bReservedOnly;
		int m_ReleaseTick;
	};
	TArray<ResourceHolder> m_lUsers;

	//Graph or grid?
	int m_Type;

	//Does someone want access to this resource? (This is set every time a request is denied and 
	//reset) every time a request is granted.
	bool m_bWantsResource;

	//Send release order to user given object ID. Fails if order wasn't heeded.
	bool SendReleaseOrder(int _iObj, CWorld_Server * _pServer);

public:
	//Check if there is an available resource
	bool IsAvailable();

	//Request a pathfinding instance for the given object index with given priority. Fails if unavailable for this priority.
	bool Request(int _iObj, uint8 _iPriority, CWorld_Server * _pServer);

	//Release currently held resource. Unless the _bNoMessage is true, a message is sent to object to force it to release the resource.
	void Release(int _iObj, CWorld_Server * _pServer, bool _bNoMessage = false);

	//Set number of resource slots and type
	void Init(int _n, int _Type, CWorld_Server * _pServer);
	void Clean();

	//Is this resource handler initialized?
	bool IsInitialized();

	//Does someone want this search instance?
	bool IsWanted();

	//Does this user hold a resource slot?
	bool SanctionedUser(int _iObj);

	//Special priorities
	enum
	{
		PRIO_MIN = 0,
		PRIO_MAX = 255,
	};

	//Type of resource handler
	enum {
		GRID,
		GRAPH,
	};
};


//Keeps track of the bots currently allowed to perform any higher activity. Resource 
//handler must be continually polled for the allowed activity-level of a bot.
//I should change pathfinding resource holders to use this kind of resource handler
class CAI_Resource_Activity
{
public:
	//Special priorities
	enum{
		PRIO_MIN = 0,
		PRIO_MAX = 255,
	};

private:
	struct ResourceHolder
	{
		int32 m_iUser;		//Object index of holder
		uint8 m_Priority;	//Prio of user
		int m_ReservedTick;	//The gametick when the resource was last reserved
		int m_LastPollTick; //The last gametick when the user polled resource handler. Slot must be polled every frame, or it will be released
		ResourceHolder()
		{
			m_iUser = 0;
			m_Priority = PRIO_MIN;
			m_ReservedTick = -1;
			m_LastPollTick = -1;
		}
	};
	TArray<ResourceHolder> m_lUsers; 

	//A resource holder cannot be ousted for this number of frames directly after reserving a slot
	int16 m_MinReservedTime;

	//A resource holder must poll at least once every this number of frames or his reservation will 
	//expire automatically. A value higher than 1 means, of course, that resource may be temporarily 
	//double-used
	int16 m_PollInterval;

public:
	CAI_Resource_Activity();

	// Free all resources used by the object
	void Destroy();

	//Poll resource handler if this activity level is allowed, given a priority. 
	//This may cause other resource holders to lose their current activity level.
	//If a reservation time is used, this is the time the slot will be considered 
	//automatically polled. If the NewUse flag pointer is given this will be set 
	//to true if we didn't previoulsy hold a slot or false otherwise.
	bool Poll(int _iObj, uint8 _iPriority, int _GameTick, int _ReservationTime = 0, bool* _NewUse = NULL);

	//Check if we can poll successfully, without actually polling
	bool Peek(int _iObj, uint8 _iPriority, int _GameTick);

	//Explicitly release resource slot held by this user
	void Release(int _iObj, int _GameTick);
	//Release all resource slots held
	void ReleaseAll();

	//Set number of resource slots and the minimum number of frames a slot is reserved
	void Init(int _nSlots, int _MinReservedTime, int _PollInterval = 1);

	//Is this resource handler initialized?
	bool IsInitialized();

	// Returns a random user from m_lUsers if any or 0 if there are none
	int GetRandomUser();
};


//Keeps track of the general accumulated AI activity level, for optimization purposes
class CAI_ActivityCounter
{
public:
	//Activity score for a bunch of activities. These values are all just heuristics.
	enum {
		INACTIVE		= 0, //Not spawned or deactivated
		IDLE			= 1, //Standing around, just perceiving
		MOVING			= 2, //Simple movement
		PATHFINDING		= 5, //Pathfinding and possibly moving
		MELEE			= 10,//Fighting in melee (generally with close proximity to other characters, which causes expansive physics)
		RANGEDCOMBAT	= 5, //Fighting in ranged combat (i.e. evading and shooting frequently)
	};
private:
	//Total current and previous activity level 
	int m_CurrentActivity;
	int m_PreviousActivity;

	//Current tick numbe
	int m_CurrentTick;

public:
	CAI_ActivityCounter();	

	//Lets a bot report current activity level. Make sure a player doesn't report or it 
	//might fuck things up. Also, make sure noone reports twice in the same frame, or value will be incorrect.
	void Report(int _ActivityLevel, int _GameTick);
	
	//Get previous activity score (i.e. total score for all reporting bots the previous frame)
	int GetScore();
};

//Useful simple list-type.
template<class T>
class TSimpleDynamicList
{
	//Class T must implement method
	//bool operator==(const T& _Compare);

protected:
	//The elements and the current first empty position
	TArray<T> m_lList;
	int m_iFirstEmpty;

	//The null value
	T m_Null;

public:
	TSimpleDynamicList();

	//Create list with given null value
	void Create(T _Null, int _Grow = -1);

	void Destroy();

	//Clear all data
	void Clear();

	//Set first empty position to _x and return this position
	int Add(T _x);

	//Set position _i to empty
	void Remove(int _i);

	//Find first position with value _x. Returns -1 on failure.
	int Find(T _x);

	//Returns the number of non-empty values in list
	int GetNum();

	//Returns the full length of the list, including empty spaces. Use for iteration.
	int Length();

	//Returns reference to element at position _i
	T& Get(int _i);

	//Returns reference to element nbr _i
	T& GetNth(int _i);

	//Returns the null value
	T Null();

	//Check if element at given position is valid (i.e. non-null)
	bool IsValid(int _i);
};

typedef TSimpleDynamicList<int> CSimpleIntList;



//Simple sorted list template. Optimized for fast iteration, adding elements is slow.
template <class T>
class TSimpleSortedList : public TSimpleDynamicList<T*> 
{
	// The class T must implement: int GetPriority() const
protected:
	//Ascendingly sorted elements?
	bool m_bSortAscending;

public:
	typedef TSimpleDynamicList<T*> CSuper;
	//Create list
	void Create(bool _bAscending, int _Grow = -1);

	//Sort element into list
	int Add(T* _pX);
};


//Useful hashed map of int->T
template<class T>
class TIntHashMap
{
protected:

	//Hash slot class
	class CHashSlot
	{
	private:
		//Index-value pair container class
		class CMapElt
		{
		public:
			int m_iIndex;
			T m_Val;
			CMapElt(){};
			CMapElt(int _i, T _Val)
			{
				m_iIndex = _i;
				m_Val = _Val;
			};
		};
		TArray<CMapElt> m_lElts;
		
		//NULL value. Any elements with this as value is assumed to be empty
		T m_Null;

	public:
		CHashSlot(){};

		//Clear slot
		void Clear()
		{
			m_lElts.Clear();
		};

		//Create hash slot with given null value
		void Create(T _NullVal)
		{
			m_Null = _NullVal;
			Clear();
		};

		//Add element
		void Add(int _i, T _Val)
		{
			if (_Val == m_Null)
				//Cannot add null element
				return;

			int iFirstEmpty = -1;
			for (int i = 0; i < m_lElts.Len(); i++)
			{
				if (m_lElts[i].m_Val == m_Null)
				{
					//Empty slot 
					if (iFirstEmpty == -1)
						iFirstEmpty = i;
				}
				//Non-empty slot, check if we've already got this index
				else if (m_lElts[i].m_iIndex == _i)
					return;
			}

			//Insert element
			if (iFirstEmpty == -1)
				//Insert last
				m_lElts.Add(CMapElt(_i, _Val));
			else
				m_lElts[iFirstEmpty] = CMapElt(_i, _Val);
		};

		//Remove element
		void Remove(int _i)
		{
			for (int i = 0; i < m_lElts.Len(); i++)
			{
				if ((m_lElts[i].m_iIndex == _i) && !(m_lElts[i].m_Val == m_Null))
				{
					m_lElts[i].m_Val = m_Null;
					return;
				}
			}
		};

		//Get value of element, or NULL if there is no corresponding element
		T& Get(int _i)
		{
			for (int i = 0; i < m_lElts.Len(); i++)
			{
				if ((m_lElts[i].m_iIndex == _i) && !(m_lElts[i].m_Val == m_Null))
				{
					return m_lElts[i].m_Val;
				}
			}

			//No element found
			return m_Null;
		};
	};

	//The map element hash
	TArray<CHashSlot> m_lHash;

	//The null element
	T m_Null;

public:
	TIntHashMap();

	//Create hash with given number of hash-slots.
	//One must also specify the value to be returned when failing to retrieve an item (the null value)
	void Create(T _NullVal, int _nSlots);

	//Clear hash
	void Clear();

	//Retrieve element that _i maps to or null element if there is no map for _i
	T& Get(int _i);

	//Add new map 
	void Add(int _i, T _Val);

	//Remove map
	void Remove(int _i);

	//Get null value
	T Null();
};


//Hash which sorts elements into box-partitioned 3D-space (world units in fp32). 
template<class T>
class TProximityHash
{
private:
	class CHashElt
	{
	public:
		//The element 
		T m_Elt;
		//Index to next element in hash box
		int16 m_iNext;

		CHashElt()
		{
			m_iNext = -1;
		};
	};

	//All elements in the hash
	TThinArray<CHashElt> m_lHashElts;

	//Index of first "empty" element in element list
	int16 m_iFirstEmpty;

	//Iteration hashelt index, when getting nodes of a box
	int16 m_iNextElt;

	//The dimensions of a hash box
	int m_iXDim;
	int m_iYDim;
	int m_iZDim;

	//The hash list (each slot holds the index to the first belonging element)
	TThinArray<int16> m_liHash;

	//Dimension factors: position (x,y,z) has hash value x * m_iYZ + y * m_iZ + z
	int m_iYZ;
	int m_iZ;

	//Null value
	T m_Null;

	//Get hash list index given position
	int16 HashValue(const CVec3Dint32& _Pos);

public:
	TProximityHash(){};

	//Set max size of hash, dimensions of boxes and dimensions of world. Initialize all elements
	TProximityHash(int _iSize, const CVec3Dfp32& _Dims, const CVec3Dfp32& _WorldDims, T _Null);

	//Add element at given position. Fails if position is invalid or hash is full
	bool Add(T _iElt, const CVec3Dfp32& _Pos);

	//Get first element from box which encapsules given position. Returns null if there is no nodes in box.
	T GetFirst(const CVec3Dfp32& _Pos);

	//Get next element (assuming GetFirst have been called). Returns null if there is no more nodes in box.
	T GetNext();

	//Add all elements in hashboxes which encapsulate the sphere defined by position and radius, except those 
	//that encapsulate the sphere defined by position and minradius. Return number of elements.
	int GetBatch(const TArray<T> * _plResult, const CVec3Dfp32& _Pos, fp32 _Radius = 0.0f, fp32 _MinRadius = -1.0f);

	//Get null value
	T Null();
};




//Template definitions///////////////////////////////////////////////////////////////////

//TSimpleDynamicList///////////////////////////////////////////////////////////////////////
template<class T>
TSimpleDynamicList<T>::TSimpleDynamicList()
{
	m_lList.Clear();
	m_iFirstEmpty = 0;
};

//Create list with given null value
template<class T>
void TSimpleDynamicList<T>::Create(T _Null, int _Grow)
{
	m_Null = _Null;
	m_iFirstEmpty = 0;
	m_lList.Clear();
if (_Grow > 0)
	m_lList.SetGrow(_Grow);
};

//Clear all data
template<class T>
void TSimpleDynamicList<T>::Clear()
{
	for (int i = 0; i < m_lList.Len(); i++)
	{
		m_lList[i] = m_Null;
	};
	m_iFirstEmpty = 0;
};

//Clear all data
template<class T>
void TSimpleDynamicList<T>::Destroy()
{
	for (int i = 0; i < m_lList.Len(); i++)
	{
		m_lList[i] = m_Null;
	};
	m_lList.Destroy();
	m_iFirstEmpty = 0;
};

//Set first empty position to _x and return this position
template<class T>
int TSimpleDynamicList<T>::Add(T _x)
{
	if (_x == m_Null)
		//Cannot add null element
		return -1;

	//Set element
	int Pos;
	if (m_iFirstEmpty < m_lList.Len())
	{
		Pos = m_iFirstEmpty;
		m_lList[m_iFirstEmpty] = _x;
	}
	else
	{
		Pos = m_lList.Len();
		m_lList.Add(_x);
	}

	//Update first empty slot
	for (int i = m_iFirstEmpty + 1; i < m_lList.Len(); i++)
	{
		if (m_lList[i] == m_Null)
		{
			m_iFirstEmpty = i;		
			return Pos;
		};
	};
	//No empty positions, first empty is after current list end.
	m_iFirstEmpty = m_lList.Len();
	return Pos;
};

//Set position _i to empty
template<class T>
void TSimpleDynamicList<T>::Remove(int _i)
{
	if (m_lList.ValidPos(_i))
	{
		m_lList[_i] = m_Null;
		if (_i < m_iFirstEmpty)
			m_iFirstEmpty = _i;
	};
};

//Find first position with value _x. Returns -1 on failure.
template<class T>
int TSimpleDynamicList<T>::Find(T _x)
{
	for (int i = 0; i < m_lList.Len(); i++)
	{
		if (m_lList[i] == _x)
			return i;
	};	
	//Couldn't find _x
	return -1;
};

//Returns the number of non-empty values in list
template<class T>
int TSimpleDynamicList<T>::GetNum()
{
	int Res = m_iFirstEmpty;
	for (int i = m_iFirstEmpty + 1; i < m_lList.Len(); i++)
	{
		if (m_lList[i] != m_Null)
			Res++;
	};	
	return Res;
};

//Returns the full length of the list, including empty slots
template<class T>
int TSimpleDynamicList<T>::Length()
{
	return m_lList.Len();
};

//Returns element at position _i
template<class T>
T& TSimpleDynamicList<T>::Get(int _i)
{
	if (m_lList.ValidPos(_i))
		return m_lList[_i];
	else
		return m_Null;
};

//Returns _i:th element ignoring NULLS
//NOTE Slow if there are many holes
template<class T>
T& TSimpleDynamicList<T>::GetNth(int _i)
{
	if (_i >= GetNum())
	{
		return(m_Null);
	}

	if (_i < m_iFirstEmpty)
	{
		return(Get(_i));
	}
	else
	{	int Nulls = 0;
		for (int k = m_iFirstEmpty; k < m_lList.Len(); k++)
		{
			if (m_lList[k] != Null())
			{
				if (k-Nulls >= _i)
				{
					return(Get(k));
				}
			}
			else
			{
				Nulls++;
			}
		}
	}
	return(m_Null);
};

//Returns the null value
template<class T>
T TSimpleDynamicList<T>::Null()
{
	return m_Null;
};

//Check if element at given position is valid (i.e. non-null)
template<class T>
bool TSimpleDynamicList<T>::IsValid(int _i)
{
	return m_lList.ValidPos(_i) && !(m_lList[_i] == m_Null);
};
	

//TSimpleSortedList//////////////////////////////////////////////////////////////////////////////

//Create list
template <class T>
void TSimpleSortedList<T>::Create(bool _bAscending, int _Grow)
{
	m_bSortAscending = _bAscending;		
	TSimpleDynamicList<T*>::Create(NULL, _Grow);
};

//Sort element into list
template <class T>
int TSimpleSortedList<T>::Add(T* _pX)
{
	if (!_pX)
		//Cannot add null element
		return -1;

	//Find position to insert in
	int Pos = 0;
	int Prio = _pX->GetPriority();
	if (m_bSortAscending)
	{
		while ((Pos < CSuper::m_lList.Len()) &&
			   ((CSuper::m_lList[Pos] == NULL) || (CSuper::m_lList[Pos]->GetPriority() < Prio)))
		   Pos++;
	}
	else
	{
		while ((Pos < CSuper::m_lList.Len()) &&
			   ((CSuper::m_lList[Pos] == NULL) || (CSuper::m_lList[Pos]->GetPriority() > Prio)))
		   Pos++;
	}

	//Insert in, or before found position, and shuffle other elements
	if ((Pos > 0) && (CSuper::m_lList[Pos - 1] == NULL))
	{
		//Empty position in just the right spot, no shuffle needed
		CSuper::m_lList[Pos - 1] = _pX;
		Pos--;
	}
	else if (Pos == CSuper::m_lList.Len())
	{
		//Add to end of list
		CSuper::m_lList.Add(_pX);
	}
	else if (CSuper::m_iFirstEmpty < Pos)
	{
		//Empty position before selected position; shuffle elements towards start of list to 
		//compress list and leave room for new element
		for (int i = CSuper::m_iFirstEmpty; i < Pos; i++)
		{
			CSuper::m_lList[i] = CSuper::m_lList[i + 1];
		}
		CSuper::m_lList[Pos - 1] = _pX;
		Pos--;
	}
	else
	{
		//Empty position after selected position, shuffle elements towards end of list
		if (CSuper::m_iFirstEmpty == CSuper::m_lList.Len())
			CSuper::m_lList.Add(CSuper::m_lList[CSuper::m_lList.Len() - 1]);
		for (int i = CSuper::m_iFirstEmpty - 1; i > Pos; i--)
		{
			CSuper::m_lList[i] = CSuper::m_lList[i - 1];
		}
		CSuper::m_lList[Pos] = _pX;
	}

	//Update first empty slot
	for (int i = CSuper::m_iFirstEmpty; i < CSuper::m_lList.Len(); i++)
	{
		if (CSuper::m_lList[i] == NULL)
		{
			CSuper::m_iFirstEmpty = i;		
			return Pos;
		};
	};
	//No empty positions, first empty is after current list end.
	CSuper::m_iFirstEmpty = CSuper::m_lList.Len();
	return Pos;
};


//TIntHashMap/////////////////////////////////////////////////////////////////////////////////
template<class T>
TIntHashMap<T>::TIntHashMap()
{
	m_lHash.Clear();
};

//Create hash with given number of hash-slots.
//One must also specify the value to be returned when failing to retrieve an item
template<class T>
void TIntHashMap<T>::Create(T _NullVal, int _nSlots)
{
	m_Null = _NullVal;
	m_lHash.SetLen(_nSlots);
	for (int i = 0; i < m_lHash.Len(); i++)
	{
		m_lHash[i].Create(m_Null);	
	}
};


//Clear hash
template<class T>
void TIntHashMap<T>::Clear()
{
	for (int i = 0; i < m_lHash.Len(); i++)
	{
		m_lHash[i].Clear();	
	}
};

//Retrieve element that _i maps to
template<class T>
T& TIntHashMap<T>::Get(int _i)
{
	int iSlot = Abs(_i) % m_lHash.Len();
	return m_lHash[iSlot].Get(_i);
};

//Add new map 
template<class T>
void TIntHashMap<T>::Add(int _i, T _Val)
{
	int iSlot = Abs(_i) % m_lHash.Len();
	m_lHash[iSlot].Add(_i, _Val);	
};

//Remove map
template<class T>
void TIntHashMap<T>::Remove(int _i)
{
	int iSlot = Abs(_i) % m_lHash.Len();
	m_lHash[iSlot].Remove(_i);	
};

//Get null value
template<class T>
T TIntHashMap<T>::Null()
{
	return m_Null;
};
	

//Set max size of hash and dimensions of boxes and initialize all elements
template<class T>
TProximityHash<T>::TProximityHash(int _Size, const CVec3Dfp32& _Dims, const CVec3Dfp32& _WorldDims, T _Null)
{
	MAUTOSTRIP(TProximityHash_ctor, MAUTOSTRIP_VOID);

	//All fp32 values are trunced to int32
	CVec3Dint32	Dims = CVec3Dint32((int32)_Dims[0], (int32)_Dims[1], (int32)_Dims[2]);
	CVec3Dint32	WorldDims = CVec3Dint32((int32)_WorldDims[0], (int32)_WorldDims[1], (int32)_WorldDims[2]);

	//Set null value
	m_Null = _Null;

	//Set element size and initialize elements
	{
		m_lHashElts.SetLen(_Size);
		for (int i = 0; i < m_lHashElts.Len(); i++)
			m_lHashElts[i].m_Elt = m_Null;
	}
	

	//Set dimensions
	m_iXDim = Dims[0];
	m_iYDim = Dims[1];
	m_iZDim = Dims[2];

	//Calculate number of boxes in each dimension
	int nXBoxes = (WorldDims[0] / Dims[0]) + ((WorldDims[0] % Dims[0]) ? 1 : 0);
	int nYBoxes = (WorldDims[1] / Dims[1]) + ((WorldDims[1] % Dims[1]) ? 1 : 0);
	int nZBoxes = (WorldDims[2] / Dims[2]) + ((WorldDims[2] % Dims[2]) ? 1 : 0);

	//Set hash size
	m_liHash.SetLen(nXBoxes * nYBoxes * nZBoxes);
	for (int i = 0; i < m_liHash.Len(); i++)
		m_liHash[i] = -1;

	//Set dimension factors
	m_iYZ = nYBoxes * nZBoxes;
	m_iZ = nZBoxes;

	//Reset iterator and first empty index
	m_iNextElt = -1;
	m_iFirstEmpty = (_Size > 0) ? 0 : -1;
};


//Get hash list index given position
template<class T>
int16 TProximityHash<T>::HashValue(const CVec3Dint32& _Pos)
{
	MAUTOSTRIP(TProximityHash_HashValue, 0);
	return (_Pos[0] / m_iXDim) * m_iYZ + (_Pos[1] / m_iYDim) * m_iZ + (_Pos[2] / m_iZDim);
};


//Add node with given index at given position. Fails if position is invalid or hash is full
template<class T>
bool TProximityHash<T>::Add(T _Elt, const CVec3Dfp32& _Pos)
{
	CVec3Dint32 Pos = CVec3Dint32((int32)_Pos[0], (int32)_Pos[1], (int32)_Pos[2]);

	MAUTOSTRIP(TProximityHash_Add, false);
	//Check if element array is full
	if (m_iFirstEmpty >= m_lHashElts.Len())
		return false;

	//Check if position is valid
	int iHash = HashValue(Pos);	
	if (!m_liHash.ValidPos(iHash))
		return false;

	//Add element to element list
	m_lHashElts[m_iFirstEmpty].m_Elt = _Elt;		

	//Insert node into hash list 
	//Point new elements next to previous hash box first element
	m_lHashElts[m_iFirstEmpty].m_iNext = m_liHash[iHash];		
	//Set new element as first element in hash box
	m_liHash[iHash] = m_iFirstEmpty;

	m_iFirstEmpty++;
	return true;
};


//Get first node index from box which encapsules given position. Returns -1 if there is no nodes im box.
template<class T>
T TProximityHash<T>::GetFirst(const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(TProximityHash_GetFirst, 0);
	CVec3Dint32 Pos = CVec3Dint32((int32)_Pos[0], (int32)_Pos[1], (int32)_Pos[2]);
	int iHash = HashValue(_Pos);

	//Valid hash value?
	if (m_liHash.ValidPos(iHash))
	{
		m_iNextElt = m_liHash[iHash];
		return GetNext();
	}
	else
	{
		m_iNextElt = -1;
		return m_Null;
	}
};

//Get next node index (assuming GetFirst have been called). Returns -1 if there is no more nodes in box.
template<class T>
T TProximityHash<T>::GetNext()
{
	MAUTOSTRIP(TProximityHash_GetNext, 0);
	if (m_iNextElt == -1)
	{
		//No more elements
		return m_Null;
	}
	else
	{
		T iRes = m_lHashElts[m_iNextElt].m_Elt;
		m_iNextElt = m_lHashElts[m_iNextElt].m_iNext;
		return iRes;
	}
};


//Add all elements in hashboxes which encapsulate the sphere defined by position and range delimiter, except those 
//that encapsulate the sphere defined by position and lower range delimiter. Return number of elements.
template<class T>
int TProximityHash<T>::GetBatch(const TArray<T> * _plResult, const CVec3Dfp32& _Pos, fp32 _RangeDelimiter, fp32 _LowerRangeDelimiter)
{
	if (!_plResult)
		return 0;

	//No elements unless radius is greater than minradius
//	if (_Radius <= _MinRadius)
//		return 0;

	//Trunc position and radii
	CVec3Dint32 Pos = CVec3Dint32((int32)_Pos[0], (int32)_Pos[1], (int32)_Pos[2]);
	int32 RangeDelimiter = (int32)_RangeDelimiter;
	int32 LowerRangeDelimiter = (int32)_LowerRangeDelimiter;

	//Calculate boxradius/minradius in each dimension, erring on the side of caution 
	//(i.e. propably getting mor boxes than those strictly needed
/*	//TODO....
	int XRadius[2] = {RangeDelimiter + m_iXDim - 1
	int YRadius[2] = RangeDelimiter + m_iYDim - 1;
	int ZRadius[2] = RangeDelimiter + m_iZDim - 1;
	int XMinRadius[2] = (_LowerRangeDelimiter >= 0) ? (LowerRangeDelimiter + m_iXDim - 1) : -1;
	int YMinRadius[2] = (_LowerRangeDelimiter >= 0) ? (LowerRangeDelimiter + m_iYDim - 1) : -1;
	int ZMinRadius[2] = (_LowerRangeDelimiter >= 0) ? (LowerRangeDelimiter + m_iZDim - 1) : -1;

	//Iterate over the encapsulating boxes, skip those encapsulating the lower range
	for (int x = )*/
	return 0;
};


//Get null value
template<class T>
T TProximityHash<T>::Null()
{
	return m_Null;
};



#endif

