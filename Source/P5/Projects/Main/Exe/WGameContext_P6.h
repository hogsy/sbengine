#ifndef __GAMECONTEXTP6_H
#define __GAMECONTEXTP6_H

#include "WGameContextMain.h"

class CExtraContent_P6  : public CExtraContent_Offline
{
	virtual void UpdateContentList();	// initates a content update
};

class CGameContext_P6 : public CGameContextMod
{
public:
	virtual void Create(CStr _WorldPathes, CStr _GameName, spCXR_Engine _spEngine);
	CExtraContent_P6 m_ExtraContent_Offline_P6;
};

#endif

