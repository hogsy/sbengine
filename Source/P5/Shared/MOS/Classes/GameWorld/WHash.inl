
#ifndef PLATFORM_VPU
	#include "WClass.h"
	#include "WObjCore.h"
	//#include "WPhysState.h"
	#include "MFloat.h"
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)

//#include "WPhysState_Hash.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Hash, new version
|__________________________________________________________________________________________________
\*************************************************************************************************/

// -------------------------------------------------------------------
//  TWO_Hash_RO
// -------------------------------------------------------------------
template<typename TSPECIALIZATION>
TWO_Hash_RO<TSPECIALIZATION>::TWO_Hash_RO()
{
	m_BoxSize = 0;
	m_BoxShiftSize = 0;
	m_BoxAndSize = 0;
	m_nBoxAndX = 0;
	m_nBoxAndY = 0;
	m_nBoxesX = 0;
	m_nBoxesY = 0;
	m_nBoxes = 0;
	m_HashAndSizeX = 0;
	m_HashAndSizeY = 0;

	m_FirstLarge = 0;
}

template<typename TSPECIALIZATION>
TWO_Hash_RO<TSPECIALIZATION>::~TWO_Hash_RO()
{
}


template<typename TSPECIALIZATION>
void TWO_Hash_RO<TSPECIALIZATION>::Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject)
{
	// setup the settings
	CWO_Hash_Settings::Init(_nBoxes, _BoxShiftSize, _MaxIDs, _bUseLarge, _bIsServerCWObject);

	// setup data storage
	m_Data.Init(_nBoxes, _MaxIDs);
}

/*
template<typename TSPECIALIZATION>
void TWO_Hash_RO<TSPECIALIZATION>::Init(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject)
{

}*/

template<typename TSPECIALIZATION>
bool TWO_Hash_RO<TSPECIALIZATION>::BoxHashEnum(const CWO_EnumParams_Box& _Params, int iHash, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetValue, int _MaxEnumRets, int &_nIDs)
{
	
	int iLink = m_Data.GetHash(iHash);
	while(iLink)
	{
		int ID = iLink >> 2;
		const CWO_HashLink& L = m_Data.GetLink(iLink);
		
		iLink = L.m_iNext;
		const CWO_HashElement& E = m_Data.GetElement(ID);

		if(m_Data.IsElementTagged(ID))
			continue;

		//if(TSPECIALIZATION::TagPolicy::IsTagged(E))
			//continue;
		/*
#ifndef PLATFORM_VPU // TODO: these defines should be removed in some pretty way -kma
		if (E.m_iHash[0] & 0x8000)
			continue;
#endif*/
		//if(IsTagged(ID))
		//	continue;

		if(!E.BoxEnumCheck(_Params, *this))
			continue;
	
		const CBox3Dfp32* pBox = E.GetBox();
		fp32 MaxSep = 
			Max(Max(Max(pBox->m_Min[0] - _Params.m_Box.m_Max.k[0], _Params.m_Box.m_Min.k[0] - pBox->m_Max.k[0]),
					Max(pBox->m_Min[1] - _Params.m_Box.m_Max.k[1], _Params.m_Box.m_Min.k[1] - pBox->m_Max.k[1])),
				Max(pBox->m_Min[2] - _Params.m_Box.m_Max.k[2], _Params.m_Box.m_Min.k[2] - pBox->m_Max.k[2]));

		if (MaxSep > 0.0f)
			continue;

/*		if (pBox->m_Min.k[0] > _Params.m_Box.m_Max.k[0]) continue;
		if (pBox->m_Min.k[1] > _Params.m_Box.m_Max.k[1]) continue;
		if (pBox->m_Min.k[2] > _Params.m_Box.m_Max.k[2]) continue;
		if (pBox->m_Max.k[0] < _Params.m_Box.m_Min.k[0]) continue;
		if (pBox->m_Max.k[1] < _Params.m_Box.m_Min.k[1]) continue;
		if (pBox->m_Max.k[2] < _Params.m_Box.m_Min.k[2]) continue;*/
		if (_nIDs >= _MaxEnumRets)
			return false;

		m_Data.TagElement(ID);
		
		//Tag(ID);
		//TSPECIALIZATION::TagPolicy::Tag(E);
		/*
#ifndef PLATFORM_VPU
		E.m_iHash[0] |= 0x8000;
#endif*/

		_pEnumRetValue[_nIDs++] = E.GetReturnValue(ID);
	}
	return true;
}


// TODO: make a function of these macros -kma
/*
#ifdef PLATFORM_VPU
	#define CWO_HASHENUM(iHash)																		\
	{																								\
		int iLink = m_lHash[iHash];																	\
		while(iLink)																				\
		{																							\
			int ID = iLink >> 2;																	\
			const CWO_HashLink L = m_lLinks[iLink];													\
			iLink = L.m_iNext;																		\
			CWO_HashElement E = m_lElements[ID];													\
			const CBox3Dfp32* pBox = &E.m_Box;														\
			if (pBox->m_Min.k[0] > _Params.m_Box.m_Max.k[0]) continue;								\
			if (pBox->m_Min.k[1] > _Params.m_Box.m_Max.k[1]) continue;								\
			if (pBox->m_Min.k[2] > _Params.m_Box.m_Max.k[2]) continue;								\
			if (pBox->m_Max.k[0] < _Params.m_Box.m_Min.k[0]) continue;								\
			if (pBox->m_Max.k[1] < _Params.m_Box.m_Min.k[1]) continue;								\
			if (pBox->m_Max.k[2] < _Params.m_Box.m_Min.k[2]) continue;								\
			if (nIDs >= _MaxEnumRets)																\
				goto QuitEnum;																		\
			_pEnumRetValue[nIDs++] = *pBox;															\
		}																							\
	}
#else

	#define CWO_HASHENUM(iHash)																		\
	{																								\
		int iLink = m_lHash[iHash];																	\
		while(iLink)																				\
		{																							\
			int ID = iLink >> 2;																	\
			const CWO_HashLink& L = m_pLinks[iLink];												\
			iLink = L.m_iNext;																		\
			CWO_HashElement& E = m_pElements[ID];													\
			if (E.m_iHash[0] & 0x8000)																\
				continue;																			\
			if(!E.BoxEnumCheck(_Params, *this))														\
				continue;																			\
			const CBox3Dfp32* pBox = E.GetBox();													\
			if (pBox->m_Min.k[0] > _Params.m_Box.m_Max.k[0]) continue;								\
			if (pBox->m_Min.k[1] > _Params.m_Box.m_Max.k[1]) continue;								\
			if (pBox->m_Min.k[2] > _Params.m_Box.m_Max.k[2]) continue;								\
			if (pBox->m_Max.k[0] < _Params.m_Box.m_Min.k[0]) continue;								\
			if (pBox->m_Max.k[1] < _Params.m_Box.m_Min.k[1]) continue;								\
			if (pBox->m_Max.k[2] < _Params.m_Box.m_Min.k[2]) continue;								\
			if (nIDs >= _MaxEnumRets)																\
				goto QuitEnum;																		\
			E.m_iHash[0] |= 0x8000;																	\
			_pEnumRetValue[nIDs++] = ID;															\
		}																							\
	}
#endif
*/
template<typename TSPECIALIZATION>
int TWO_Hash_RO<TSPECIALIZATION>::EnumerateBox(const CWO_EnumParams_Box& _Params, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetValue, int _MaxEnumRets)
{
#ifndef PLATFORM_SPU
	static void *g_EnumeratingLock = 0x0;
	if(!g_EnumeratingLock)
	{
		g_EnumeratingLock = MRTC_SystemInfo::OS_MutexOpen("EnumBoxLock");
	}

	MRTC_SystemInfo::OS_MutexLock(g_EnumeratingLock);
#endif
	
	int nIDs = 0;

	// Add all large IDs
	if (m_bUseLarge)
	{
		if(!BoxHashEnum(_Params, GetLargeHashIndex(), _pEnumRetValue, _MaxEnumRets, nIDs))
			goto QuitEnum;
	}

	{
		// Calculate extents of the box in the hash grid
		int x1, x2, y1, y2;
		if (_Params.m_Box.m_Max.k[0] - _Params.m_Box.m_Min.k[0] > m_MaxXEnum)
		{
			x1 = 0;
			x2 = m_nBoxesX-1;
		}
		else
		{
			x1 = ((RoundToInt(_Params.m_Box.m_Min.k[0]) >> m_BoxShiftSize)) & m_nBoxAndX;
			x2 = ((RoundToInt(_Params.m_Box.m_Max.k[0]) >> m_BoxShiftSize)) & m_nBoxAndX;
		}
		
		if (_Params.m_Box.m_Max.k[1] - _Params.m_Box.m_Min.k[1] > m_MaxYEnum)
		{
			y1 = 0;
			y2 = m_nBoxesY-1;
		}
		else
		{
			y1 = ((RoundToInt(_Params.m_Box.m_Min.k[1]) >> m_BoxShiftSize)) & m_nBoxAndY;
			y2 = ((RoundToInt(_Params.m_Box.m_Max.k[1]) >> m_BoxShiftSize)) & m_nBoxAndY;
		}

		int y = y1;
		while(1)
		{
			int x = x1;
			while(1)
			{
				if(!BoxHashEnum(_Params, GetHashIndex(x, y), _pEnumRetValue, _MaxEnumRets, nIDs))
					goto QuitEnum;
				//CWO_HASHENUM(GetHashIndex(x, y));
				if (x == x2) break;
				x = (x + 1) & m_nBoxAndX;
			}
			
			if (y == y2) break;
			y = (y + 1) & m_nBoxAndY;
		}
	}
QuitEnum:

	//FinalizeEnum(_pEnumRetValue, nIDs);

	m_Data.FinalizeEnum(_pEnumRetValue, nIDs);

	/*
#ifndef PLATFORM_VPU // TODO: remove this ugly define -kma
	// Untag found objects
	for(int i = 0; i < nIDs; i++)
	{
		m_lElements[_pEnumRetValue[i]].m_iHash[0] &= ~0x8000;
	}
#endif
	*/
#ifndef PLATFORM_SPU
	MRTC_SystemInfo::OS_MutexUnlock(g_EnumeratingLock);
#endif
	return nIDs;
}

#ifndef PLATFORM_VPU

/*




*/

 // TODO: remove these ugly defines
#define HashTrunc(x) RoundToInt(M_Floor(x))

#define CWO_HASHENUM_LINECHECK(iHash,bVisBox)													\
{																								\
	int iLink = m_Data.GetHash(iHash);															\
	CBox3Dfp32 Box;																				\
	while(iLink)																				\
	{																							\
		const int ID = iLink >> 2;																\
		const CWO_HashLink& L = m_Data.GetLink(iLink);											\
		iLink = L.m_iNext;																		\
		CWO_HashElement& E = m_Data.GetElement(ID);												\
		/*if (E.m_iHash[0] & 0x8000)*/															\
		if (m_Data.IsElementTagged(ID))															\
			continue;																			\
		const CWObject_CoreData* pObj = E.m_pObj;												\
		const CWO_PhysicsState &State = pObj->GetPhysState();									\
		int Mask = (State.m_ObjectFlags & _Params.m_ObjectFlags) | 								\
				   (State.m_ObjectIntersectFlags & _Params.m_ObjectIntersectFlags);				\
		if (m_bIsServerCWObject)																\
			Mask |= ((CWObject*)pObj)->m_IntersectNotifyFlags & _Params.m_ObjectNotifyFlags;    \
		if(Mask == 0)																			\
			continue;																			\
		const CBox3Dfp32* pBox = pObj->GetAbsBoundBox();										\
		if (bVisBox && (State.m_ObjectFlags & OBJECT_FLAGS_CHARACTER))				\
		{																						\
			pBox = &Box;																		\
			pObj->GetAbsVisBoundBox(Box);														\
		}																						\
		CVec3Dfp32 HitPos;																		\
		if (!pBox->IntersectLine(_Params.m_V0, _Params.m_V1, HitPos)) continue; 				\
		if (nIDs >= _MaxEnumIDs)																\
			goto QuitEnum;																		\
		/*E.m_iHash[0] |= 0x8000;*/																\
		m_Data.TagElement(ID);																	\
		_pEnumRetIDs[nIDs++] = ID;																\
	}																							\
}



template<typename TSPECIALIZATION>
int TWO_Hash_RO<TSPECIALIZATION>::EnumerateLine(const CWO_EnumParams_Line& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs, bool _bVisBox)
{
#ifndef PLATFORM_SPU
	static void *g_EnumeratingLock = 0x0;
	if(!g_EnumeratingLock)
	{
		g_EnumeratingLock = MRTC_SystemInfo::OS_MutexOpen("EnumLineLock");
	}

	MRTC_SystemInfo::OS_MutexLock(g_EnumeratingLock);
#endif

	MAUTOSTRIP(CWO_Hash_EnumerateLine_2, 0);
/*	CVec3Dfp32 VMin, VMax;
	for(int k = 0; k < 3; k++)
	{
		VMin.k[k] = Min(_p0.k[k], _p1.k[k]);
		VMax.k[k] = Max(_p0.k[k], _p1.k[k]);
	}

	return EnumerateBox(VMin, VMax, _ObjectFlags, _pEnumRetIDs, _MaxEnumIDs);
*/
//#ifndef M_RTM
		int nIter = 0;
//#endif
	int nIDs = 0;
	if (m_bUseLarge)
	{
		CWO_HASHENUM_LINECHECK(GetLargeHashIndex(), _bVisBox);
	}


/*	
	const CVec3Dfp32 _p0 = _Params.m_V0;
	const CVec3Dfp32 _p1 = _Params.m_V1;
	CPnt p0(RoundToInt(_p0.k[0]) >> m_BoxShiftSize, RoundToInt(_p0.k[1]) >> m_BoxShiftSize);
	CPnt p1(RoundToInt(_p1.k[0]) >> m_BoxShiftSize, RoundToInt(_p1.k[1]) >> m_BoxShiftSize);

	int dx = Abs(p1.x - p0.x);
	int dy = Abs(p1.y - p0.y);
	int dec;

	if (dx > dy) 
	{
		if (p1.x < p0.x) 
		{
			Swap(p0.x, p1.x);
			Swap(p0.y, p1.y);
		};
		int ydir = (p0.y < p1.y) ? 1: -1;
		int y = p0.y;
		dec = 0;
		for (int i = p0.x; i <= p1.x; i++) 
		{
			CWO_HASHENUM_LINECHECK(GetHashIndex(i & m_nBoxAndX, y & m_nBoxAndY));
			CWO_HASHENUM_LINECHECK(GetHashIndex(i & m_nBoxAndX, (y+ydir) & m_nBoxAndY));

			dec += dy;
			if (dec >= dx) { dec -= dx; y += ydir; }
		};
	}
	else 
	{
		if (p1.y < p0.y) 
		{
			Swap(p0.x, p1.x);
			Swap(p0.y, p1.y);
		};
		int xdir = (p0.x < p1.x) ? 1: -1;
		int x = p0.x;
		dec = 0;
		for (int i = p0.y; i <= p1.y; i++)
		{
			CWO_HASHENUM_LINECHECK(GetHashIndex(x & m_nBoxAndX, i & m_nBoxAndY));
			CWO_HASHENUM_LINECHECK(GetHashIndex((x + xdir) & m_nBoxAndX, i & m_nBoxAndY));
			dec += dx;
			if (dec >= dy) { dec -= dy; x += xdir; }
		};
	};*/


	{
		CVec2Dfp32 _p0, _p1;
		_p0[0] = _Params.m_V0[0] * m_BoxesPerUnit;
		_p0[1] = _Params.m_V0[1] * m_BoxesPerUnit;
		_p1[0] = _Params.m_V1[0] * m_BoxesPerUnit;
		_p1[1] = _Params.m_V1[1] * m_BoxesPerUnit;
		// ConOut(CStrF("%s, %s   =>  %s to %s, %f", _Params.m_V0.GetString().Str(), _Params.m_V1.GetString().Str(), _p0.GetString().Str(), _p1.GetString().Str(), m_BoxesPerUnit));

		CVec2Dfp32 dv;
		_p1.Sub(_p0, dv);
		
		if (M_Fabs(dv[1]) > M_Fabs(dv[0]))
		{
			// y-loop
			const fp32 dxdy = dv[0] / M_Fabs(dv[1]);
			int y0 = HashTrunc(_p0[1]);
			const int y1 = HashTrunc(_p1[1]);
			const int ystep = int(Sign(dv[1]));
			fp32 x = _p0[0];
			if (y0 < y1)
				x += dxdy*(fp32(y0)-_p0[1]);
			else
				x -= dxdy*(1.0f + fp32(y0)-_p0[1]);
			while(1)
			{
				int xint = HashTrunc(x);
				CWO_HASHENUM_LINECHECK(GetHashIndex(xint & m_nBoxAndX, y0 & m_nBoxAndY), _bVisBox);
				x += dxdy;
				const int xint2 = HashTrunc(x);
				if (xint != xint2)
					CWO_HASHENUM_LINECHECK(GetHashIndex(xint2 & m_nBoxAndX, y0 & m_nBoxAndY), _bVisBox);

				if (y0 == y1)
					break;
				y0 += ystep;
//		#ifndef M_RTM
				nIter++;
				if (nIter > 1024)
					break;
//		#endif
			}
		}
		else
		{
			// x-loop
			fp32 dydx;
			if (dv[0] == 0.0f)
				dydx = 0.0f;
			else
				dydx = dv[1] / M_Fabs(dv[0]);
			int x0 = HashTrunc(_p0[0]);
			const int x1 = HashTrunc(_p1[0]);
			const int xstep = int(Sign(dv[0]));
			fp32 y = _p0[1];
			if (x0 < x1)
				y += dydx*(fp32(x0)-_p0[0]);
			else
				y -= dydx*(1.0f + fp32(x0)-_p0[0]);

			while(1)
			{
				const int yint = HashTrunc(y);
				CWO_HASHENUM_LINECHECK(GetHashIndex(x0 & m_nBoxAndX, yint & m_nBoxAndY), _bVisBox);
				y += dydx;
				const int yint2 = HashTrunc(y);
				if (yint != yint2)
					CWO_HASHENUM_LINECHECK(GetHashIndex(x0 & m_nBoxAndX, yint2 & m_nBoxAndY), _bVisBox);

				if (x0 == x1)
					break;
				x0 += xstep;
//		#ifndef M_RTM
				nIter++;
				if (nIter > 1024)
					break;
//		#endif
			}
		}
	}


QuitEnum:
#ifndef M_RTM
	if (nIter > 1024)
	{
		ConOutL(CStrF("(CWO_Hash::EnumerateLine) Invalid trace %s - %s", _Params.m_V0.GetString().Str(), _Params.m_V1.GetString().Str()));
		M_ASSERT(0, "!");
	}
#endif

	m_Data.FinalizeEnum(_pEnumRetIDs, nIDs);

	/*
	for(int i = 0; i < nIDs; i++)
	{
		m_Data.GetElement(_pEnumRetIDs[i]).m_iHash[0] &= ~0x8000;
		//m_lElements[]
	}*/
#ifndef PLATFORM_SPU
	MRTC_SystemInfo::OS_MutexUnlock(g_EnumeratingLock);
#endif
	return nIDs;
}



// -------------------------------------------------------------------
//  TWO_Hash_RW
// -------------------------------------------------------------------
template<typename TSPECIALIZATION>
TWO_Hash_RW<TSPECIALIZATION>::TWO_Hash_RW()
{}

template<typename TSPECIALIZATION>
TWO_Hash_RW<TSPECIALIZATION>::~TWO_Hash_RW()
{}

template<typename TSPECIALIZATION>
void TWO_Hash_RW<TSPECIALIZATION>::Create(int _nBoxes, int _BoxShiftSize, int _MaxIDs, bool _bUseLarge, bool _bIsServerCWObject)
{
	TWO_Hash_RO<TSPECIALIZATION>::Create(_nBoxes, _BoxShiftSize, _MaxIDs, _bUseLarge, _bIsServerCWObject);
}


// remove these ugly ugly macros
#define CWO_LINK_INSERT(E, iHash, ID, iElemHash)											\
{																							\
	E.m_iHash[iElemHash] = iHash;															\
	int iLink = CWO_Hash_RO::GetLinkIndex(ID, iElemHash);									\
	CWO_HashLink& L = CWO_Hash_RO::m_Data.GetLink(iLink);									\
	if (CWO_Hash_RO::m_Data.GetHash(iHash))													\
		CWO_Hash_RO::m_Data.GetLink(CWO_Hash_RO::m_Data.GetHash(iHash)).m_iPrev = iLink;	\
	L.m_iNext = CWO_Hash_RO::m_Data.GetHash(iHash);											\
	L.m_iPrev = 0;																			\
	CWO_Hash_RO::m_Data.GetHash(iHash) = iLink;												\
}

#if 0
#define CWO_VALIDATE_HASH(E, iHash, ID, iElemHash)							\
{																			\
	uint16 iLink = GetLinkIndex(ID, iElemHash);								\
	uint16 iLinkNext = m_lLinks[iLink].m_iNext;								\
	while(iLink)															\
	{																		\
		if(iLinkNext == iLink)												\
			M_BREAKPOINT;													\
		if(iLinkNext) iLinkNext = CWO_Hash_RO::m_lLinks[iLinkNext].m_iNext;	\
		if(iLinkNext == iLink)												\
			M_BREAKPOINT;													\
		if(iLinkNext) iLinkNext = CWO_Hash_RO::m_lLinks[iLinkNext].m_iNext;	\
		iLink = CWO_Hash_RO::m_lLinks[iLink].m_iNext;						\
	}																		\
}
#endif

template<typename TSPECIALIZATION>
void TWO_Hash_RW<TSPECIALIZATION>::Insert(CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWO_Hash_Insert_2, MAUTOSTRIP_VOID);
	int ID = _pObj->m_iObject;
	//M_ASSERT(ID && ID < CWO_Hash_RO::m_lElements.Len(), "!");

	CWO_Hash_Settings &Settings = *this; // shortcut -kma

	//	Remove(ID);
	CWO_HashElement& E = CWO_Hash_RO::m_Data.GetElement(ID);

	E.Assign(_pObj);
	//E.m_pObj = _pObj;
	//E.m_Box = *pPhysBoundBox;
	const CBox3Dfp32* pPhysBoundBox = E.GetBox(); //_pObj->GetAbsBoundBox();

	int MaxSize = 0;
	if(Settings.m_bUseLarge)
	{
		int SizeX = RoundToInt(pPhysBoundBox->m_Max.k[0] - pPhysBoundBox->m_Min.k[0] + 0.5f);
		int SizeY = RoundToInt(pPhysBoundBox->m_Max.k[1] - pPhysBoundBox->m_Min.k[1] + 0.5f);
		if (SizeX > SizeY)
			MaxSize = SizeX;
		else
			MaxSize = SizeY;

		if (MaxSize > Settings.m_BoxSize)
		{
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetLargeHashIndex(), ID, 0);
//			CWO_VALIDATE_HASH(E, GetLargeHashIndex(), ID, 0);
			return;
		}
	}

	int x0 = (TruncToInt(pPhysBoundBox->m_Min.k[0] + 0.001f) >> Settings.m_BoxShiftSize) & Settings.m_nBoxAndX;
	int y0 = (TruncToInt(pPhysBoundBox->m_Min.k[1] + 0.001f) >> Settings.m_BoxShiftSize) & Settings.m_nBoxAndY;
	int x1 = (TruncToInt(pPhysBoundBox->m_Max.k[0] - 0.001f) >> Settings.m_BoxShiftSize) & Settings.m_nBoxAndX;
	int y1 = (TruncToInt(pPhysBoundBox->m_Max.k[1] - 0.001f) >> Settings.m_BoxShiftSize) & Settings.m_nBoxAndY;

	if (x0 != x1)
	{
		if (y0 != y1)
		{
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y0), ID, 0);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y0), ID, 0);
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y1), ID, 1);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y1), ID, 1);
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x1, y0), ID, 2);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x1, y0), ID, 2);
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x1, y1), ID, 3);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x1, y1), ID, 3);
		}
		else
		{
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y0), ID, 0);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y0), ID, 0);
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x1, y0), ID, 1);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x1, y0), ID, 1);
		}
	}
	else
	{
		if (y0 != y1)
		{
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y0), ID, 0);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y0), ID, 0);
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y1), ID, 1);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y1), ID, 1);
		}
		else
		{
			CWO_LINK_INSERT(E, CWO_Hash_RO::GetHashIndex(x0, y0), ID, 0);
//			CWO_VALIDATE_HASH(E, GetHashIndex(x0, y0), ID, 0);
		}
	}
	
}

template<typename TSPECIALIZATION>
void TWO_Hash_RW<TSPECIALIZATION>::Remove(int _ID)
{
	MAUTOSTRIP(CWO_Hash_Remove, MAUTOSTRIP_VOID);
	CWO_HashElement& E = CWO_Hash_RO::m_Data.GetElement(_ID);

	for(int iElemHash = 0; iElemHash < 4; iElemHash++)
	{
		if (E.m_iHash[iElemHash])
		{
			CWO_HashLink& L = CWO_Hash_RO::m_Data.GetLink(CWO_Hash_RO::GetLinkIndex(_ID, iElemHash));
			if (L.m_iPrev)
				CWO_Hash_RO::m_Data.GetLink(L.m_iPrev).m_iNext = L.m_iNext;
			else
				CWO_Hash_RO::m_Data.GetHash(E.m_iHash[iElemHash]) = L.m_iNext;
			if (L.m_iNext)
				CWO_Hash_RO::m_Data.GetLink(L.m_iNext).m_iPrev = L.m_iPrev;
			E.m_iHash[iElemHash] = 0;
		}
	}

	E.Release();
}

// -------------------------------------------------------------------
//  CWO_SpaceEnumSpecialization_Common
// -------------------------------------------------------------------
void CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CElementMixin::Assign(CWObject_CoreData *_pObj)
{
	/*
	#ifndef M_RTM
		if (_pObj->m_bIsInWOHash)
			Error_static("(CWO_Hash::Insert)", "Object already in hash");
		++_pObj->m_bIsInWOHash;
	#endif
	*/
	m_pObj = _pObj;
	const CWO_PhysicsState &State = _pObj->GetPhysState();  

	m_ObjectFlags = State.m_ObjectFlags;
	m_ObjectIntersectFlags = State.m_ObjectIntersectFlags;

	bool bClientObj = (_pObj->MRTC_GetRuntimeClass()->m_ClassNameHash == MHASH4('CWOb','ject','_Cli','ent'));
	m_IntersectNotifyFlags = bClientObj ? 0 : safe_cast<CWObject>(_pObj)->m_IntersectNotifyFlags;
	m_RigidBodyID = State.m_RigidBodyID;
}

void CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CElementMixin::Release()
{
	/*
	#ifndef M_RTM
		if (m_pObj)
			--m_pObj->m_bIsInWOHash;
	#endif
	*/
	m_pObj = NULL;
}

const CBox3Dfp32 *CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CElementMixin::GetBox() const
{
	return m_pObj->GetAbsBoundBox();
}

bool CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CElementMixin::BoxEnumCheck(const CWO_EnumParams_Box& _Params, const CWO_Hash_Settings &_Settings) const
{
	// If Rigid Body ID is smaller or equal to current object, skip redundant overlap tests
	//if ((m_RigidBodyID ^ 0xFFFF) && (_Params.m_RigidBodyID ^ 0xFFFF)  && (m_RigidBodyID <= _Params.m_RigidBodyID) )
	//	return false;

	int ObjectFlags = m_ObjectFlags; 
	int ObjectIntersectFlags = m_ObjectIntersectFlags;
	int NotifyFlags = _Settings.m_bIsServerCWObject ? m_IntersectNotifyFlags : 0;

	// using the values above will not work as long as game objects manually update their physstate without re-inserting into the hash.
	// -> until all game code has been fixed and verified, let's read the actual values instead...
	if (m_pObj)
	{
		ObjectFlags = m_pObj->GetPhysState().m_ObjectFlags;
		ObjectIntersectFlags = m_pObj->GetPhysState().m_ObjectIntersectFlags;
		NotifyFlags = _Settings.m_bIsServerCWObject ? safe_cast<CWObject>(m_pObj)->m_IntersectNotifyFlags : 0;
	}

	int Mask = (ObjectFlags & _Params.m_ObjectFlags) | (ObjectIntersectFlags & _Params.m_ObjectIntersectFlags);
	if (_Settings.m_bIsServerCWObject)
		Mask |= (NotifyFlags & _Params.m_ObjectNotifyFlags);

	return (Mask != 0);
}

void CWO_SpaceEnumSpecialization_Common::CHashSpecialization::CStorage::Init(int _nBoxes, int _MaxIDs)
{
	m_lLinks.SetLen(_MaxIDs*4);
	m_lElements.SetLen(_MaxIDs);
	m_lHash.SetLen(_nBoxes*_nBoxes+2);
	m_lTags.SetLen((_MaxIDs+31)/32);

	FillChar(m_lLinks.GetBasePtr(), m_lLinks.ListSize(), 0);
	FillChar(m_lElements.GetBasePtr(), m_lElements.ListSize(), 0);
	FillChar(m_lHash.GetBasePtr(), m_lHash.ListSize(), 0);
	FillChar(m_lTags.GetBasePtr(), m_lTags.ListSize(), 0);
}

#endif

// -------------------------------------------------------------------
//  TWO_SpaceEnum_RO
// -------------------------------------------------------------------
template<typename TSPECIALIZATION>
void TWO_SpaceEnum_RO<TSPECIALIZATION>::Init(int _nBoxesSmall, int _BoxShiftSizeSmall, int _nBoxesLarge, int _BoxShiftSizeLarge, int _nObjects, bool _bIsServerCWObject)
{
	m_Hash1.Init(_nBoxesSmall, _BoxShiftSizeSmall, _nObjects, false, _bIsServerCWObject);
	m_Hash2.Init(_nBoxesLarge, _BoxShiftSizeLarge, _nObjects, true, _bIsServerCWObject);
}

template<typename TSPECIALIZATION>
int TWO_SpaceEnum_RO<TSPECIALIZATION>::EnumerateBox(const CWO_EnumParams_Box& _Params, typename TSPECIALIZATION::EnumerateReturnType *_pEnumRetIDs, int _MaxEnumIDs)
{
	int n = m_Hash1.EnumerateBox(_Params, _pEnumRetIDs, _MaxEnumIDs);
	n += m_Hash2.EnumerateBox(_Params, &_pEnumRetIDs[n], _MaxEnumIDs-n);
	return n;
}

template<typename TSPECIALIZATION>
int TWO_SpaceEnum_RO<TSPECIALIZATION>::EnumerateLine(const CWO_EnumParams_Line& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs, bool _bVisBox)
{
	int n = m_Hash1.EnumerateLine(_Params, _pEnumRetIDs, _MaxEnumIDs, _bVisBox);
	n += m_Hash2.EnumerateLine(_Params, &_pEnumRetIDs[n], _MaxEnumIDs-n, _bVisBox);
	return n;
}


// -------------------------------------------------------------------
//  TWO_SpaceEnum_RW
// -------------------------------------------------------------------
#ifndef PLATFORM_VPU

template<typename TSPECIALIZATION>
void TWO_SpaceEnum_RW<TSPECIALIZATION>::Create(int _nBoxesSmall, int _BoxShiftSizeSmall, int _nBoxesLarge, int _BoxShiftSizeLarge, int _nObjects, bool _bIsServerCWObject)
{
	// The hash-table with the largest grid has a "large-list" for all unhashable objects.
	TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash1.Create(_nBoxesSmall, _BoxShiftSizeSmall, _nObjects, false, _bIsServerCWObject);
	TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash2.Create(_nBoxesLarge, _BoxShiftSizeLarge, _nObjects, true, _bIsServerCWObject);
}

template<typename TSPECIALIZATION>
void TWO_SpaceEnum_RW<TSPECIALIZATION>::Insert(CWObject_CoreData* _pObj)
{
	Remove(_pObj->m_iObject);

	// Figure out which hash-table should be used.
	// TODO: This could cause some bizarr problems because this isn't the box that we will be using at all times
	const CBox3Dfp32* pPhysBoundBox = _pObj->GetAbsBoundBox();

	fp32 MaxSize = pPhysBoundBox->m_Max.k[0] - pPhysBoundBox->m_Min.k[0];
	if (pPhysBoundBox->m_Max.k[1] - pPhysBoundBox->m_Min.k[1] > MaxSize) MaxSize = pPhysBoundBox->m_Max.k[1] - pPhysBoundBox->m_Min.k[1];

	if (RoundToInt(MaxSize) < (1 << TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash1.m_BoxShiftSize) + 0.001f)
		TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash1.Insert(_pObj);
	else
		TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash2.Insert(_pObj);
}

template<typename TSPECIALIZATION>
void TWO_SpaceEnum_RW<TSPECIALIZATION>::Remove(int _ID)
{
	TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash1.Remove(_ID);
	TWO_SpaceEnum_RO<TSPECIALIZATION>::m_Hash2.Remove(_ID);
}


template<typename TSPECIALIZATION>
TWO_SpaceEnum_RW<TSPECIALIZATION>::TWO_SpaceEnum_RW()
{}

template<typename TSPECIALIZATION>
TWO_SpaceEnum_RW<TSPECIALIZATION>::~TWO_SpaceEnum_RW()
{}

#endif
