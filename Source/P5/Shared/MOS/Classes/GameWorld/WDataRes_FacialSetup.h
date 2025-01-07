/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WDataRes_FacialSetup.h

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWRes_FacialSetup, CFacialSetup

	Comments:

	History:		
		050916:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WDataRes_FacialSetup_h__
#define __WDataRes_FacialSetup_h__

#include "WDataRes_Core.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFacialSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CFacialSetup
{
public:
	struct Group
	{
		TThinArray<uint8>     m_liMoveTracks;
		TThinArray<uint8>     m_liRotTracks;
		TThinArray<CVec3Dfp32> m_lMoveKeys;	// array of keys
		TThinArray<CQuatfp32> m_lRotKeys;	// array of keys

		void Eval(fp32 _t, CVec3Dfp32* _pMove, CVec3Dfp32* _pRot) const;
	};
	Group m_IdlePose;
	TThinArray<Group> m_lGroups;

	uint m_iMaxMove;
	uint m_iMaxRot;

	enum { MaxTracks = 96 };
	TBitArray<MaxTracks> m_MoveMask;
	TBitArray<MaxTracks> m_RotMask;

public:
	dllvirtual bool Init(const CXR_Anim_Base& _Anim);
	dllvirtual void Eval(const fp32* _pInput, CXR_SkeletonInstance* _pOutput) const;
	dllvirtual void GetFaceData(CXR_SkeletonInstance* _pInput, int _BaseTrack, int _nTracks, fp32* _pDest) const;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWRes_FacialSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWRes_FacialSetup : public CWResource
{
	MRTC_DECLARE;

	CFacialSetup m_FacialSetup;

public:
	CWRes_FacialSetup();
	virtual CFacialSetup* GetData();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnPrecache(class CXR_Engine* _pEngine);
};


#endif // __WDataRes_FacialSetup_h__
