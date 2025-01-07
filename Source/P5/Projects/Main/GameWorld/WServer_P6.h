#ifndef __WSERVER_P6_H__
#define __WSERVER_P6_H__

#include "WServerMod.h"

class CWServer_P6 : public CWServer_Mod
{
	MRTC_DECLARE;
private:
public:
	void Register(CScriptRegisterContext &_RegContext);
};

#endif

