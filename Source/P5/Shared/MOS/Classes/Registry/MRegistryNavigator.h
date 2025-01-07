#ifndef _INC_MRegistryNavigator_h
#define _INC_MRegistryNavigator_h

#include "../../MSystem/Misc/MConsole.h"

class CRegistry;
// -------------------------------------------------------------------
//  CRegistryNavigator
// -------------------------------------------------------------------

/*
	Enables registry browsing and editing from console.
*/
class CRegistryNavigator : public CConsoleClient
{
	CStr m_CurrentDir;
	bool m_bAdded;
public:

	CRegistryNavigator();
	~CRegistryNavigator();

	void Log(CRegistry* _pR, int _Level);
	void Dump_r(CRegistry* _pR, int _Level);
	CStr ConcatPath(CStr _Path, CStr _PathAdd);
	void Con_rcd(CStr _s);
	void Con_rdir();
	void Con_rdirspec(const CStr _s);
	virtual void Con_rdump();
	virtual void Con_rdumpspec(CStr _s);
	void Con_rset(CStr _s);
	void Con_regset(CStr _key, CStr _value);
	void Register(CScriptRegisterContext &_RegContext);
};

typedef TPtr<CRegistryNavigator> spCRegistryNavigator;

#endif // _INC_MRegistryNavigator_h
