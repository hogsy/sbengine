#ifndef __WDATARES_ANIMLIST_H
#define __WDATARES_ANIMLIST_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Animation resources class

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_Anim
					CWRes_AnimList
\*____________________________________________________________________________________________*/

#include "WData.h"

// -------------------------------------------------------------------
//  ANIMATION
// -------------------------------------------------------------------
class CWRes_Anim : public CWResource
{
	MRTC_DECLARE;

	spCXR_Anim_Base m_spAnim;
	bool m_bExists;

	// FIXME:
	CWorldData* m_pWData;

public:
	CWRes_Anim();
	virtual CXR_Anim_Base* GetAnim();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual int IsLoaded();
	virtual void OnLoad();
	virtual void OnUnload();
	virtual void OnRegisterMapData(class CWorldData* _pWData, class CMapData* _pMapData);
};

// -------------------------------------------------------------------
//  ANIMATIONLIST
// -------------------------------------------------------------------
class CWRes_AnimList : public CWResource
{
	MRTC_DECLARE;

	spCWResource m_lspAnims[256];
	int8 m_Sequences[256];
	CWorldData* m_pWData;

public:

	CWRes_AnimList();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);

	virtual CXR_Anim_Base *GetAnim(int _Index, bool _bReportFail = true);
	virtual int GetSequence(int _Index);
};


#endif
