#ifndef _INC_WOBJ_PACKER
#define _INC_WOBJ_PACKER

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Data Packer

	Author:			Magnus Auvinen

	Copyright:		Starbreeze Studios 2003

	Contents:		CDataPacker

	History:		
\*____________________________________________________________________________________________*/

//
// Used to pack data into m_Data
//
template<int32 TINDEX, int32 TMASK, int32 TSHIFT>
class CDataPacker
{
public:
	M_INLINE int32 Get(CWObject_CoreData *_pObj)
	{
		return (_pObj->m_Data[TINDEX]>>TSHIFT)&TMASK;
	}

	M_INLINE void Set(CWObject_CoreData *_pObj, uint32 _Param)
	{
		M_ASSERT((_Param & ~TMASK) == 0, "CDataPacker, _Param out of range!");
		_pObj->m_Data[TINDEX] &= ~(TMASK<<TSHIFT);
		_pObj->m_Data[TINDEX] |= (_Param&TMASK)<<TSHIFT;
	}
};

#endif // _INC_WOBJ_PACKER
