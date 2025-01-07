
#include "PCH.h"

#include "MRegistryNavigator.h"
#include "../../MSystem/Misc/MRegistry.h"
#include "../../MSystem/MSystem.h"

/*
Enables registry browsing and editing from console.
*/
CRegistryNavigator::CRegistryNavigator()
{
	m_bAdded = false;
	AddToConsole();
	m_bAdded = true;
}

CRegistryNavigator::~CRegistryNavigator()
{
	//M_TRY
	//{
		if (m_bAdded) RemoveFromConsole();
	/*}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

void CRegistryNavigator::Log(CRegistry* _pR, int _Level)
{
	int nCh = _pR->GetNumChildren();
	for(int i = 0; i < nCh; i++)
	{
		CRegistry* pElem = _pR->GetChild(i);
		ConOut(CStr(' ', _Level*4) + pElem->GetThisName() + " = " + pElem->GetThisValue());
	}
}

void CRegistryNavigator::Dump_r(CRegistry* _pR, int _Level)
{
	int nCh = _pR->GetNumChildren();
	for(int i = 0; i < nCh; i++)
	{
		CRegistry* pElem = _pR->GetChild(i);
		ConOutL(CStr(' ', _Level*4) + pElem->GetThisName() + " = " + pElem->GetThisValue());
		if (pElem->GetNumChildren())
			Dump_r(pElem, _Level+1);
	}
}

CStr CRegistryNavigator::ConcatPath(CStr _Path, CStr _PathAdd)
{
	CStr Dir = _PathAdd;
	while(Dir != "")
	{
		CStr Name = Dir.GetStrSep("\\");
		if (Name == "")
			_Path = "";
		else
		{
			if (Name == "..")
			{
				int pos = _Path.Len()-2;
				if (pos)
				{
					const char* pS = _Path;
					while(pos > 0 && pS[pos] != '\\') pos--;
					_Path = _Path.Copy(0, pos);
				}
				else
					_Path = "";
			}
			else
				_Path += Name + "\\";
		}
	}
	return _Path;
}

void CRegistryNavigator::Con_rcd(CStr _s)
{
	m_CurrentDir = ConcatPath(m_CurrentDir, _s);
	ConOut("Current directory " + m_CurrentDir);
}

void CRegistryNavigator::Con_rdir()
{
	Con_rdirspec(CStr(""));
}

void CRegistryNavigator::Con_rdirspec(CStr _s)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;
	CStr Path = ConcatPath(m_CurrentDir, _s);
	CRegistry* pR;
	if(Path == "")
		pR = pSys->GetRegistry();
	else
		pR = pSys->GetRegistry()->Find(Path);
	if (pR)
	{
		ConOut("Registry directory " + Path);
		Log(pR, 0);
	}
	else
		ConOut("Invalid directory " + Path);
}

void CRegistryNavigator::Con_rdump()
{
	Con_rdumpspec(CStr(""));
}

void CRegistryNavigator::Con_rdumpspec(CStr _s)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;
	CStr Path = ConcatPath(m_CurrentDir, _s);
	CRegistry* pR;
	if(Path == "")
		pR = pSys->GetRegistry();
	else
		pR = pSys->GetRegistry()->Find(Path);
	if (pR)
	{
		ConOutL("Registry directory " + Path);
		Dump_r(pR, 0);
	}
	else
		ConOut("Invalid directory " + Path);
}

void CRegistryNavigator::Con_rset(CStr _s)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;
	
	CStr Key = _s.GetStrSep("=");
	CStr Value = _s;
	CStr Path = ConcatPath(m_CurrentDir, Key);
	CRegistry* pR = pSys->GetRegistry()->CreateDir(Path);
	//		CRegistry* pR = pSys->GetRegistry()->Find(Path);
	if (pR)
	{
		pR->SetThisValue(Value);
	}
	else
		ConOutL("Invalid key " + Path);
}

void CRegistryNavigator::Con_regset(CStr _key, CStr _value)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys) return;
	
	CRegistry* pReg = pSys->GetRegistry()->CreateDir(_key);
	if (pReg) pReg->SetThisValue(_value);
}

void CRegistryNavigator::Register(CScriptRegisterContext & _RegContext)
{
	_RegContext.RegFunction("rcd", this, &CRegistryNavigator::Con_rcd);
	_RegContext.RegFunction("rdir", this, &CRegistryNavigator::Con_rdir);
	_RegContext.RegFunction("rdump", this, &CRegistryNavigator::Con_rdump);
	_RegContext.RegFunction("rdump2", this, &CRegistryNavigator::Con_rdump);
	_RegContext.RegFunction("rset", this, &CRegistryNavigator::Con_rset);
	_RegContext.RegFunction("regset", this, &CRegistryNavigator::Con_regset);
}
