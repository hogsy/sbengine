#include "PCH.h"
#include "MRegistry.h"

MRTC_CRuntimeClass CRegistry::m_RuntimeClass = {"CRegistry", NULL, NULL};
MRTC_CClassInit g_ClassRegCRegistry(&CRegistry::m_RuntimeClass);
MRTC_CRuntimeClass* CRegistry::MRTC_GetRuntimeClass() const { return &m_RuntimeClass; };

// -------------------------------------------------------------------
CStr CRegistry::GetName(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetName, CStr());
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisName();
}

const char* CRegistry::GetNameUnsafe(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetName2, NULL);
	const CRegistry* pR = GetChildUnsafe(_iKey);
	return pR->GetThisNameUnsafe();
}

CStr CRegistry::GetValue(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetValue, CStr());
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisValue();
}

int32 CRegistry::GetValuei(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetValuei, 0);
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisValuei();
}

fp32 CRegistry::GetValuef(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetValuef, 0.0f);
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisValuef();
}

const TArray<uint8> CRegistry::GetValued(int _iKey) const
{
	MAUTOSTRIP(CRegistry_GetValued, TArray<uint8>());
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisValued();
}

TArray<uint8> CRegistry::GetValued(int _iKey)
{
	MAUTOSTRIP(CRegistry_GetValued_2, TArray<uint8>());
	const CRegistry* pR = GetChild(_iKey);
	return pR->GetThisValued();
}


// -------------------------------------------------------------------
CStr CRegistry::GetValue(const char* _pName, CStr _DefVal) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		return _DefVal;
	}
	else
		return pR->GetThisValue();
}

int32 CRegistry::GetValuei(const char* _pName, int32 _DefVal) const
{
	MAUTOSTRIP(CRegistry_GetValuei_3, 0);
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		return _DefVal;
	}
	else
		return pR->GetThisValuei();
}

fp32 CRegistry::GetValuef(const char* _pName, fp32 _DefVal) const
{
	MAUTOSTRIP(CRegistry_GetValuef_3, 0.0f);
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		return _DefVal;
	}
	else
		return pR->GetThisValuef();
}

// -------------------------------------------------------------------
CStr CRegistry::GetValue(const char* _pName, CStr _DefVal, bint _bAddValue)
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	CRegistry* pR = Find(_pName);
	if (!pR)
	{
		if (_bAddValue)
			AddKey(_pName, _DefVal);
		return _DefVal;
	}
	else
		return pR->GetThisValue();
}

int32 CRegistry::GetValuei(const char* _pName, int32 _DefVal, bint _bAddValue)
{
	MAUTOSTRIP(CRegistry_GetValuei_3, 0);
	CRegistry* pR = Find(_pName);
	if (!pR)
	{
		if (_bAddValue)
			AddKeyi(_pName, _DefVal);
		return _DefVal;
	}
	else
		return pR->GetThisValuei();
}

fp32 CRegistry::GetValuef(const char* _pName, fp32 _DefVal, bint _bAddValue)
{
	MAUTOSTRIP(CRegistry_GetValuef_3, 0.0f);
	CRegistry* pR = Find(_pName);
	if (!pR)
	{
		if (_bAddValue)
			AddKeyf(_pName, _DefVal);
		return _DefVal;
	}
	else
		return pR->GetThisValuef();
}

// 
CStr CRegistry::GetValue(const char* _pName) const
{
	MAUTOSTRIP(CRegistry_GetValue_4, CStr());
	return GetValue(_pName, CStr());
}

int32 CRegistry::GetValuei(const char* _pName) const
{
	MAUTOSTRIP(CRegistry_GetValuei_4, 0);
	return GetValuei(_pName, 0);
}

fp32 CRegistry::GetValuef(const char* _pName) const
{
	MAUTOSTRIP(CRegistry_GetValuef_4, 0.0f);
	return GetValuef(_pName, 0.0f);
}

const TArray<uint8> CRegistry::GetValued(const char* _pName) const
{
	MAUTOSTRIP(CRegistry_GetValued_3, TArray<uint8>());
	const CRegistry* pR = Find(_pName);
	if (pR && pR->GetType() == REGISTRY_TYPE_DATA)
	{
		return pR->GetThisValued();
	}
	else
		return TArray<uint8>();
}

TArray<uint8> CRegistry::GetValued(const char* _pName)
{
	MAUTOSTRIP(CRegistry_GetValued_4, TArray<uint8>());
	const CRegistry* pR = Find(_pName);
	if (pR && pR->GetType() == REGISTRY_TYPE_DATA)
	{
		return pR->GetThisValued();
	}
	else
		return TArray<uint8>();
}

void CRegistry::XRG_LogDump(int _Level) const
{
	MAUTOSTRIP(CRegistry_XRG_LogDump, MAUTOSTRIP_VOID);
	if (_Level == 0)
	{
		LogFile("-------------------------------------------------------------------");
		LogFile(" X-REG LOGDUMP");
		LogFile("-------------------------------------------------------------------");
	}

	CStr s = CStr(' ', _Level*4);
	s += CStrF("%-24s, %s", (const char*) GetThisName(), (const char*)GetThisValue());
	LogFile(s);

	{ for(int i = 0; i < GetNumChildren(); i++)
		GetChild(i)->XRG_LogDump(_Level + 1); }
}

void CRegistry::XRG_WriteChildren(CCFile* _pFile) const
{
	MAUTOSTRIP(CRegistry_XRG_WriteChildren, MAUTOSTRIP_VOID);
	MSCOPE(CRegistry::XRG_WriteChildren, REGISTRY);
	if(GetNumChildren())
	{
		for(int i = 0; i < GetNumChildren(); i++)
			GetChild(i)->XRG_Write_r(_pFile, 0);
	}
}

CStr CRegistry::GetThisValuesAsStr(int _ReqLevel) const
{
	CStr Ret;
	int nDim = GetDimensions();
	int nSeq = Anim_ThisGetNumSeq();
	bint bAnim = true;
	bint bTimed = false;
	if (nSeq == 0)
	{
		bAnim = false;
		nSeq = 1;
	}
	else
	{
		bTimed = Anim_ThisGetEnableTimed();
	}

	CStr SeqTabs;
	CStr KFTabs;
	if (nSeq > 1)
	{
		FillChar(SeqTabs.GetBuffer(_ReqLevel+4), _ReqLevel+3, 9);
		SeqTabs.SetChar(0, '\r');
		SeqTabs.SetChar(1, '\n');
		SeqTabs.SetChar(_ReqLevel+3, 0);
		FillChar(KFTabs.GetBuffer(_ReqLevel+5), _ReqLevel+4, 9);
		KFTabs.SetChar(0, '\r');
		KFTabs.SetChar(1, '\n');
		KFTabs.SetChar(_ReqLevel+4, 0);
	}

	for (int s = 0; s < nSeq; ++s)
	{
		CStr Sequence;
		int nKF = 1;
		if (bAnim)
		{
			nKF = Anim_ThisGetNumKF(s);
			if (nSeq == 1 && nKF <= 1)
				bAnim = false;
			else
			{
				CStr SeqOpt;
				fp32 LoopStart = Anim_ThisGetSeqLoopStart(s);
				fp32 LoopEnd = Anim_ThisGetSeqLoopEnd(s);
				if (LoopStart >= 0)
					SeqOpt += CStrF("LS=%f", LoopStart);
				if (LoopEnd >= 0)
				{
					if (SeqOpt != "")
						SeqOpt += CStrF(", LE=%f", LoopEnd);
					else
						SeqOpt += CStrF("LE=%f", LoopEnd);
				}

				if (SeqOpt != "" || nSeq > 1 || s > 0)
					Sequence = SeqTabs + CStrF("<%s>", SeqOpt.Str());
			}
		}

		for (int k = 0; k < nKF; ++k)
		{
			CStr KeyFrame;
			CStr Temp[REGISTRY_MAX_DIMENSIONS];
			if (bAnim)
				Anim_ThisGetKFValuea(s, k, nDim, Temp);
			else
				GetThisValuea(nDim, Temp);
			for (int i = 0; i < nDim; ++i)
			{
				CStr Val = Temp[i];
				if (Val.IsAnsi())
				{
					if (XRG_NeedEscSeq(Val.Str(), Val.Len()))
						Val = "\"" + CreateEscSeq(Val.Str(), Val.Len()) + "\"";
				}
				else
				{
					if (XRG_NeedEscSeq(Val.StrW(), Val.Len()))
						Val = "\"" + CreateEscSeq(Val.StrW(), Val.Len()) + "\"";
				}
				if (i == 0)
					KeyFrame += Val;
				else
					KeyFrame += "," + Val;

			}
			if (bTimed && bAnim)
				KeyFrame += CStrF("\t@%f", Anim_ThisGetKFTime(s, k));

			if (nSeq > 1)
			{
				if (k == 0)
					Sequence += KFTabs + KeyFrame;
				else
					Sequence += "; " + KFTabs + KeyFrame;
			}
			else
			{
				if (k == 0)
					Sequence += KeyFrame;
				else
					Sequence += "; " + KeyFrame;
			}
		}

//		if (s == 0)
			Ret += Sequence;
//		else
//			Ret += " # " + Sequence;
	}

	return Ret;
}

bint g_RegistryTypesSave[] =		{0		, 0		, 1		, 1			, 1			, 0			, 0		, 1		, 0			};
bint g_RegistryTypesSaveAnim[] =	{0		, 0		, 1		, 1			, 1			, 1			, 1		, 1		, 1			};
const char *g_RegistryTypes[] =	{"void"		, "str"	, "bin"	, "uint8"	, "int16"	, "int32"	, "fp32"	, "fp2"	,  "uint32", NULL};
const char *g_RegistryAnimIP[] =	{"none", "linear", "qubic", "quadric", "cardinal", "cardinalconstshape","linear_v3q","cubic_v3q", NULL};

void CRegistry::XRG_Write_r(CCFile* _pFile, int _ReqLevel) const
{
	MAUTOSTRIP(CRegistry_XRG_Write_r, MAUTOSTRIP_VOID);
	MSCOPE(CRegistry::XRG_Write_r, REGISTRY);
	CStr Key = GetThisName();
	CStr Value = GetThisValuesAsStr(_ReqLevel);

	bint bWrite = Key != "" || Value != "" || GetParent() != 0;

	if (bWrite && Key == "")
		Key = "unnamed";

	if (bWrite)
	{
		bint bKeyEsc = XRG_NeedEscSeq((char*)Key, Key.Len());
		
	/*	CStr s("*%s \"%s\"", 
			(char*)GetThisName() ? (char*)GetThisName() : "", 
			(char*)GetThisValue() ? (char*)GetThisValue() : "");*/
		char Tabs[256];
		FillChar(&Tabs, Min(_ReqLevel, 255), 9);

		_pFile->Write(&Tabs, _ReqLevel);
	//	_pFile->Writeln(s);

		_pFile->WriteLE(uint8('*'));

		bint bAnimated = Anim_ThisGetAnimated();
		if (bAnimated && Anim_ThisGetNumSeq() == 1 && Anim_ThisGetNumKF() <= 1)
			bAnimated = false;
		int nDim = GetDimensions();
		CStr PreOptions;
		int Type = GetType();
		if (nDim > 1)
		{
			if (PreOptions != "")
				PreOptions += CStrF(", T=%s[%d]", g_RegistryTypes[Type], nDim);
			else
				PreOptions += CStrF("T=%s[%d]", g_RegistryTypes[Type], nDim);
		}
		else if (bAnimated)
		{
			if (g_RegistryTypesSaveAnim[Type])
			{
				if (PreOptions != "")
					PreOptions += CStrF(", T=%s", g_RegistryTypes[Type]);
				else
					PreOptions += CStrF("T=%s", g_RegistryTypes[Type]);
			}
		}
		else if (g_RegistryTypesSave[Type])
		{
			if (PreOptions != "")
				PreOptions += CStrF(", T=%s", g_RegistryTypes[Type]);
			else
				PreOptions += CStrF("T=%s", g_RegistryTypes[Type]);
		}

		uint32 Flags = GetThisUserFlags();
		if (Flags != 0)
		{
			if (PreOptions != "")
				PreOptions += CStrF(", UF=0x%x", Flags);
			else
				PreOptions += CStrF("UF=0x%x", Flags);
		}

		if (bAnimated)
		{
			int nData = 0;
			int IPType = Anim_ThisGetInterpolate(NULL, nData);
			if (IPType != 0)
			{
				if (PreOptions != "")
					PreOptions += CStrF(", IP=%s", g_RegistryAnimIP[IPType]);
				else
					PreOptions += CStrF("IP=%s", g_RegistryAnimIP[IPType]);
			}
			if (nData != 0)
			{
				fp32 TempData[16];
				fp32 *pData = TempData;
				if (nData > 16)
					pData = new fp32[nData];

				Anim_ThisGetInterpolate(pData, nData);
				CStr Stri;
				for (int i = 0; i < nData; ++i)
				{
					if (i == 0)
						Stri = CStrF("%f", pData[i]);
					else
						Stri += CStrF(" %f", pData[i]);
				}

				if (PreOptions != "")
					PreOptions += CStrF(", ID=%s", Stri.Str());
				else
					PreOptions += CStrF("ID=%s", Stri.Str());

				if (nData > 16)
					delete [] pData;
			}

			uint32 AnimFlags = Anim_ThisGetFlags();
			if (AnimFlags != 0)
			{
				CStr Flags;
				if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP)
					Flags += "L";
				if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
					Flags += "P";
				if (AnimFlags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
					Flags += "C";

				if (PreOptions != "")
					PreOptions += CStrF(", AF=%s", Flags.Str());
				else
					PreOptions += CStrF("AF=%s", Flags.Str());
			}
		}

		if (PreOptions != "")
		{
			CStr ToWrite = CStrF("<%s>", PreOptions.Str());
			_pFile->Write(ToWrite.Str(), ToWrite.Len());
		}

		if (bKeyEsc)
		{
			CStr s = CreateEscSeq((char*)Key, Key.Len());
			_pFile->WriteLE(uint8('"'));
			_pFile->Write((char*)s, s.Len());
			_pFile->WriteLE(uint8('"'));
		}
		else
			_pFile->Write((char*)Key, Key.Len());

		_pFile->WriteLE(uint8(' '));

		_pFile->Write(Value.Ansi().Str(), Value.Len());

	//_pFile->Writeln("\a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\y\z");
		_pFile->WriteLE(uint8('\r'));
		_pFile->WriteLE(uint8('\n'));

		if(GetNumChildren())
		{
			_pFile->Write(&Tabs, _ReqLevel);
			_pFile->Writeln("{");
			for(int i = 0; i < GetNumChildren(); i++)
				GetChild(i)->XRG_Write_r(_pFile, _ReqLevel+1);
			_pFile->Write(&Tabs, _ReqLevel);
			_pFile->Writeln("}");
		}
	}
	else
	{
		if(GetNumChildren())
		{
			for(int i = 0; i < GetNumChildren(); i++)
				GetChild(i)->XRG_Write_r(_pFile, _ReqLevel);
		}
	}
}

void CRegistry::XRG_Write(const CStr& _Filename) const
{
	MAUTOSTRIP(CRegistry_XRG_Write, MAUTOSTRIP_VOID);
	CCFile File;
	File.Open(_Filename, CFILE_WRITE);

	XRG_Write_r(&File, 0);

	File.Close();
}

void CRegistry::WriteSimple(CCFile* _pFile) const
{
	MAUTOSTRIP(CRegistry_WriteSimple, MAUTOSTRIP_VOID);
	int nCh = GetNumChildren();
	for (int i = 0; i < nCh; i++)
	{
		const CRegistry* pR = GetChild(i);
		_pFile->Writeln(pR->GetThisName() + "=" + pR->GetThisValue());
	}
}

void CRegistry::WriteSimple(const CStr& _Filename) const
{
	MAUTOSTRIP(CRegistry_WriteSimple_2, MAUTOSTRIP_VOID);
	TPtr<CCFile> spF = MNew(CCFile);
	if (spF == NULL) Error_static("Save", "Out of memory");
	
	spF->Open(_Filename, CFILE_WRITE | CFILE_UNICODE);
	WriteSimple(spF);
	spF->Close();
}

void CRegistry::WriteRegistryDir(CStr _Dir, CCFile* _pFile) const
{
	MAUTOSTRIP(WriteRegistryDir, MAUTOSTRIP_VOID);
	const CRegistry *pReg = GetDir(_Dir);
	if (pReg!=NULL)
	{
		pReg->WriteSimple(_pFile);
	}
}

void CRegistry::WriteRegistryDir(CStr _Dir, CStr _Filename) const
{
	MAUTOSTRIP(CRegistry_WriteRegistryDir, MAUTOSTRIP_VOID);
	const CRegistry *pReg = GetDir(_Dir);
	if (pReg!=NULL)
	{
		pReg->WriteSimple(_Filename); 
	}
}


CStr ParseEscSeq_Char(const char* p, int& _Pos, int _Len, bint _bUseSlash)
{
	MAUTOSTRIP(ParseEscSeq_Char, CStr());
	return ParseEscSeq(p, _Pos, _Len, _bUseSlash);
}



CRegistry* CRegistry::GetDir(const char* _pDir)
{
	CFStr Dir = _pDir;
	CRegistry* pReg = this;
	while(Dir != "")
	{
		CRegistry* pChild = pReg->FindChild(Dir.GetStrSep("\\"));
		if (!pChild) return NULL;
		pReg = pChild;
	}
	return pReg;	
}

const CRegistry* CRegistry::GetDir(const char* _pDir) const
{
	CFStr Dir = _pDir;
	const CRegistry* pReg = this;
	while(Dir != "")
	{
		const CRegistry* pChild = pReg->FindChild(Dir.GetStrSep("\\"));
		if (!pChild) return NULL;
		pReg = pChild;
	}
	return pReg;	
}

int CRegistry::GetNumNodes_r() const
{
	int nNodes = 1;
	int nChildren = GetNumChildren();
	for(int i = 0; i < nChildren; i++)
		nNodes += GetChild(i)->GetNumNodes_r();
	return nNodes;
}


CRegistry* CRegistry::FindChild(const char* _pKey)
{
	int iCh = FindIndex(_pKey);
	CRegistry* Ret = NULL;
	if (iCh >= 0) 
		Ret = GetChild(iCh);
	return Ret;
}

const CRegistry* CRegistry::FindChild(const char* _pKey) const
{
	int iCh = FindIndex(_pKey);
	const CRegistry* Ret = NULL;
	if (iCh >= 0) 
		Ret = GetChild(iCh);
	return Ret;
}

CRegistry* CRegistry::FindChild(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue)
{
	int iCh = FindIndex(_pKey, _pValue, _bCaseSensitiveValue);
	CRegistry* Ret = NULL;
	if (iCh >= 0) 
		Ret = GetChild(iCh);
	return Ret;
}

const CRegistry* CRegistry::FindChild(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue) const
{
	int iCh = FindIndex(_pKey, _pValue, _bCaseSensitiveValue);
	const CRegistry* Ret = NULL;
	if (iCh >= 0) 
		Ret = GetChild(iCh);
	return Ret;
}

CRegistry* CRegistry::Find(const char* _pKey)
{
	if(!_pKey)
		return NULL;

	if (strchr(_pKey, '\\'))
	{
		CRegistry* pR = GetDir(_pKey);
		return pR;
	}
	else
	{
		int iKey = FindIndex(_pKey);
		if (iKey >= 0)
			return GetChild(iKey);
		else
			return NULL;
	}
}
 
const CRegistry* CRegistry::Find(const char* _pKey) const
{
	if (strchr(_pKey, '\\'))
	{
		const CRegistry* pR = GetDir(_pKey);
		return pR;
	}
	else
	{
		int iKey = FindIndex(_pKey);
		if (iKey >= 0)
			return GetChild(iKey);
		else
			return NULL;
	}
}

spCRegistry CRegistry::EvalTemplate_r(const CRegistry* _pReg, bool _bRecursive) const
{
	// this == Template DB.
	// _spReg == Incoming tree
	// ret == Evaluated tree.
	spCRegistry spReg = _pReg->Duplicate();

	const CRegistry* pReg = spReg->Find("CLASSNAME");
	const CRegistry* pRegSuper = NULL;
	if (pReg) pRegSuper = FindChild(pReg->GetThisValue());

	if (pReg && pRegSuper)
	{
		if(_pReg == pRegSuper)
			Error_static("CRegistry::EvalTemplate_r", "Infinite inheritance: " + _pReg->GetThisName());

		spCRegistry spSuper = EvalTemplate_r(pRegSuper);
		CStr SuperClass = spSuper->GetValue("CLASSNAME", "", 0);
		spSuper->CopyDir(spReg);
		spReg = spSuper;
		if (SuperClass != "") spReg->SetValue("CLASSNAME", (char*)SuperClass);
	}
	else
	{
		if(_bRecursive)
		{
			for(int i = 0; i < spReg->GetNumChildren(); i++)
			{
				if (spReg->GetChild(i)->GetNumChildren())
					spReg->SetValue(i, EvalTemplate_r(spReg->GetChild(i), true));
			}
		}
	}

	return spReg;
}

// -------------------------------------------------------------------
void CRegistry::SetThisKey(const char* _pName, const char* _pValue)
{
	SetThisName(_pName);
	SetThisValue(_pValue);
}

void CRegistry::SetThisKey(const char* _pName, const wchar* _pValue)
{
	SetThisName(_pName);
	SetThisValue(_pValue);
}

void CRegistry::SetThisKey(const char* _pName, CStr _Value)
{
	SetThisName(_pName);
	SetThisValue(_Value);
}

void CRegistry::SetThisKeyi(const char* _pName, int32 _Value, int _StoreType)
{
	SetThisName(_pName);
	SetThisValuei(_Value, _StoreType);
}

void CRegistry::SetThisKeyf(const char* _pName, fp32 _Value, int _StoreType)
{
	SetThisName(_pName);
	SetThisValuef(_Value, _StoreType);
}

void CRegistry::SetThisKeyd(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick)
{
	SetThisName(_pName);
	SetThisValued(_pValue, _Size, _bQuick);
}

void CRegistry::SetThisKeyd(const char* _pName, TArray<uint8> _lValue, bint _bReference)
{
	SetThisName(_pName);
	SetThisValued(_lValue, _bReference);
}


void CRegistry::AddKey(const char* _pName, const char* _pValue)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValue(_pValue);
}

void CRegistry::AddKey(const char* _pName, const wchar* _pValue)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValue(_pValue);
}

void CRegistry::AddKey(const char* _pName, CStr _Value)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValue(_Value);
}

void CRegistry::AddKeyi(const char* _pName, int32 _Value, int _StoreType)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValuei(_Value, _StoreType);
}

void CRegistry::AddKeyf(const char* _pName, fp32 _Value, int _StoreType)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValuef(_Value, _StoreType);
}

void CRegistry::AddKeyd(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValued(_pValue, _Size, _bQuick);
}

void CRegistry::AddKeyd(const char* _pName, TArray<uint8> _lValue, bint _bReference)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValued(_lValue, _bReference);
}

void CRegistry::AddKey(const CRegistry* _pReg)
{
	AddReg(_pReg->Duplicate());
}

// -------------------------------------------------------------------
void CRegistry::SetValue(const char* _pName, const char* _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKey(_pName, _Value);
	else
		pR->SetThisValue(_Value);
}

void CRegistry::SetValue(const char* _pName, const wchar* _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKey(_pName, _Value);
	else
		pR->SetThisValue(_Value);
}

void CRegistry::SetValue(const char* _pName, CStr _Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKey(_pName, _Value);
	else
		pR->SetThisValue(_Value);
}

void CRegistry::SetValuei(const char* _pName, int32 _Value, int _StoreType)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyi(_pName, _Value, _StoreType);
	else
		pR->SetThisValuei(_Value, _StoreType);
}

void CRegistry::SetValuef(const char* _pName, fp32 _Value, int _StoreType)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyf(_pName, _Value, _StoreType);
	else
		pR->SetThisValuef(_Value, _StoreType);
}

void CRegistry::SetValued(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyd(_pName, _pValue, _Size, _bQuick);
	else
		pR->SetThisValued(_pValue, _Size, _bQuick);
}

void CRegistry::SetValued(const char* _pName, TArray<uint8> _lValue, bint _bReference)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyd(_pName, _lValue, _bReference);
	else
		pR->SetThisValued(_lValue, _bReference);
}

// -------------------------------------------------------------------
void CRegistry::SetValue(const CRegistry* _pReg)
{
	int iKey = FindIndex(_pReg->GetThisName());
	if (iKey < 0) 
	{
		AddReg(_pReg->Duplicate());
	}
	else
	{
		*GetChild(iKey) = *_pReg;
	}
}

// -------------------------------------------------------------------
void CRegistry::SetValue(int _iKey, const char* _pValue)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValue(_pValue);
}

void CRegistry::SetValue(int _iKey, const wchar* _pValue)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValue(_pValue);
}

void CRegistry::SetValue(int _iKey, CStr _Value)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValue(_Value);
}

void CRegistry::SetValuei(int _iKey, int32 _Value, int _StoreType)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValuei(_Value, _StoreType);
}

void CRegistry::SetValuef(int _iKey, fp32 _Value, int _StoreType)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValuef(_Value, _StoreType);
}

void CRegistry::SetValue(int _iKey, const CRegistry* _pReg)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	*pR = *_pReg;
}

void CRegistry::SetValued(int _iKey, const uint8* _pValue, int _Size, bint _bQuick)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValued(_pValue, _Size, _bQuick);
}

void CRegistry::SetValued(int _iKey, TArray<uint8> _lValue, bint _bReference)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisValued(_lValue, _bReference);
}

// -------------------------------------------------------------------
void CRegistry::RenameKey(const char* _pName, const char* _pNewName)
{
	RenameKey(FindIndex(_pName), _pNewName);
}

void CRegistry::RenameKey(int _iKey, const char* _pNewName)
{
	CRegistry* pR = GetChild(_iKey);
	if (pR)
	{
		pR->SetThisName(_pNewName);
	}
}

// -------------------------------------------------------------------

void CRegistry::SetUserFlags(int _iKey, uint32 _Value)
{
	CRegistry* pR = GetChild(_iKey);
	if (!pR) return;
	pR->SetThisUserFlags(_Value);
}

uint32 CRegistry::GetUserFlags(int _iKey) const
{
	const CRegistry* pR = GetChild(_iKey);
	if (!pR) return 0;
	return pR->GetThisUserFlags();
}

void CRegistry::SetUserFlags(const char* _pName, uint32 _Value)
{
	SetUserFlags(FindIndex(_pName), _Value);
}

uint32 CRegistry::GetUserFlags(const char* _pName) const
{
	return GetUserFlags(FindIndex(_pName));
}

// -------------------------------------------------------------------
void CRegistry::DeleteKey(const char* _pName)
{
	int iKey = FindIndex(_pName);
	if (iKey >= 0)
		DeleteKey(iKey);
}

spCRegistry CRegistry::Duplicate() const
{
	MAUTOSTRIP(CRegistry_Dynamic_Duplicate, NULL);
	spCRegistry spR = REGISTRY_CREATE;
	if (!spR) MemError_static("Duplicate");
	*spR = *this;
	return spR;
}

///////////////////////////////////////////////////////////////////
// Animation
///////////////////////////////////////////////////////////////////

///////////////////////
// This
///////////////////////


// Get Value
CStr CRegistry::Anim_ThisGetKFValue(int _iSeq, int _iKF, CStr _Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValue();
		else
			return _Default;
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
		return _Default;
	return Anim_ThisGetKFValue(_iSeq, _iKF);
}

int32 CRegistry::Anim_ThisGetKFValuei(int _iSeq, int _iKF, int32 _Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValuei();
		else
			return _Default;
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
		return _Default;
	return Anim_ThisGetKFValuei(_iSeq, _iKF);
}

fp32 CRegistry::Anim_ThisGetKFValuef(int _iSeq, int _iKF, fp32 _Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValuef();
		else
			return _Default;
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
		return _Default;
	return Anim_ThisGetKFValuef(_iSeq, _iKF);
}

TArray<uint8> CRegistry::Anim_ThisGetKFValued(int _iSeq, int _iKF, const TArray<uint8> &_Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValued();
		else
			return _Default;
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
		return _Default;
	return Anim_ThisGetKFValued(_iSeq, _iKF);
}


void CRegistry::Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest, const CStr *_Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValuea(_nDim, _pDest);
		else
		{
			for (int i = 0; i< _nDim; ++i)
			{
				_pDest[i] = _Default[i];
			}
			return;
		}
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i< _nDim; ++i)
		{
			_pDest[i] = _Default[i];
		}
		return;
	}
	
	Anim_ThisGetKFValuea(_iSeq, _iKF, _nDim, _pDest);
}

void CRegistry::Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest, const int32 *_Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValueai(_nDim, _pDest);
		else
		{
			for (int i = 0; i< _nDim; ++i)
			{
				_pDest[i] = _Default[i];
			}
			return;
		}
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i< _nDim; ++i)
		{
			_pDest[i] = _Default[i];
		}
		return;
	}
	
	Anim_ThisGetKFValueai(_iSeq, _iKF, _nDim, _pDest);
}

void CRegistry::Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp32 *_pDest, const fp32 *_Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValueaf(_nDim, _pDest);
		else
		{
			for (int i = 0; i< _nDim; ++i)
			{
				_pDest[i] = _Default[i];
			}
			return;
		}
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i< _nDim; ++i)
		{
			_pDest[i] = _Default[i];
		}
		return;
	}
	
	Anim_ThisGetKFValueaf(_iSeq, _iKF, _nDim, _pDest);
}

void CRegistry::Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const
{
	if (!Anim_ThisGetAnimated())
	{
		if (GetType() != REGISTRY_TYPE_VOID)
			return GetThisValuead(_nDim, _pDest);
		else
		{
			for (int i = 0; i< _nDim; ++i)
			{
				_pDest[i] = _Default[i];
			}
			return;
		}
	}
	if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, _iKF))
	{
		for (int i = 0; i< _nDim; ++i)
		{
			_pDest[i] = _Default[i];
		}
		return;
	}
	
	Anim_ThisGetKFValuead(_iSeq, _iKF, _nDim, _pDest);
}

// IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void CRegistry::Anim_ThisGetIPValuea(int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const CStr *_Default) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValuea(_nDim, _pDest);
			else
			{
				break;
			}
		}

		fp32 Frac = 0;
		int Keys[2];
		fp32 Delta;
		Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
		if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
			break;

		if (Frac > 0.5)
			Anim_ThisGetKFValuea(_iSeq, Keys[1], _nDim, _pDest);
		else
			Anim_ThisGetKFValuea(_iSeq, Keys[0], _nDim, _pDest);

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i] = _Default[i];
}

void CRegistry::Anim_ThisGetIPValueai(int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest, const int32 *_Default) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValueai(_nDim, _pDest);
			else
			{
				break;
			}
		}

		int nParams = 0;
		if (Anim_ThisGetInterpolate(NULL, nParams) == REGISTRY_ANIMIP_NONE)
		{
			fp32 Frac = 0;
			int Keys[2];
			fp32 Delta;
			Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
			if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
				break;

			if (Frac > 0.5)
				Anim_ThisGetKFValueai(_iSeq, Keys[1], _nDim, _pDest);
			else
				Anim_ThisGetKFValueai(_iSeq, Keys[0], _nDim, _pDest);
		}
		else
		{
			fp32 Temp[REGISTRY_MAX_DIMENSIONS];
			fp32 Default[REGISTRY_MAX_DIMENSIONS];
			for (int i = 0; i < _nDim; ++i)
				Default[i] = _Default[i];
			Anim_ThisGetIPValueaf(_iSeq, _Time, _nDim, Temp, Default);
			for (int i = 0; i < _nDim; ++i)
				_pDest[i] = (int32)Temp[i];
		}

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i] = _Default[i];
}

void CRegistry::Anim_ThisGetIPValueaf(int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValueaf(_nDim, _pDest);
			else
			{
				break;
			}
		}

		fp32 Params[16];
		int nParams = 16;
		int iInt = Anim_ThisGetInterpolate(Params, nParams);
		switch (iInt)
		{
		case REGISTRY_ANIMIP_NONE:
			{
				fp32 Frac = 0;
				int Keys[2];
				fp32 Delta;
				Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
				if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
					break;

				if (Frac > 0.5)
					Anim_ThisGetKFValueaf(_iSeq, Keys[1], _nDim, _pDest);
				else
					Anim_ThisGetKFValueaf(_iSeq, Keys[0], _nDim, _pDest);
				return;
			}
			break;
		case REGISTRY_ANIMIP_LINEAR:
			{
				fp32 Frac = 0;
				int Keys[2];
				fp32 Delta;
				Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
				if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
					break;

				fp32 Temp0[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp1[REGISTRY_MAX_DIMENSIONS];
				Anim_ThisGetKFValueaf(_iSeq, Keys[0], _nDim, Temp0);
				Anim_ThisGetKFValueaf(_iSeq, Keys[1], _nDim, Temp1);

				for (int i = 0; i < _nDim; ++i)
				{
					_pDest[i] = Temp0[i] * (1.0 - Frac) + Temp1[i] * (Frac);
				}
				return;
			}
			break;
		case REGISTRY_ANIMIP_CUBIC:
			{
				fp32 Frac = 0;
				int Keys[4];
				fp32 Deltas[3];
				Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, Deltas, 1, 2);
				if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
					break;

				bint bPerPointTension = nParams > 0 && Params[0] > 0.1;

				fp32 Temp0[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp1[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp2[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp3[REGISTRY_MAX_DIMENSIONS];
				int nUsedDim = Max(_nDim + bPerPointTension, GetDimensions());
				if (nUsedDim > REGISTRY_MAX_DIMENSIONS)
					Error_static(M_FUNCTION, "Invalid number of dimensions");
				Anim_ThisGetKFValueaf(_iSeq, Keys[0], nUsedDim, Temp0);
				Anim_ThisGetKFValueaf(_iSeq, Keys[1], nUsedDim, Temp1);
				Anim_ThisGetKFValueaf(_iSeq, Keys[2], nUsedDim, Temp2);
				Anim_ThisGetKFValueaf(_iSeq, Keys[3], nUsedDim, Temp3);

				if (bPerPointTension)
				{
					int iTension = GetDimensions() - 1;

					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;

					fp32 tsA0 = Temp0[iTension] * Deltas[1] / Deltas[0];
					fp32 tsA2 = Temp2[iTension] * Deltas[1] / Deltas[2];

					// dQuatA

					// Spline it
					for(int i = 0; i < _nDim; i++)
					{
						fp32 v0 = (Temp1[i] - Temp0[i]) * tsA0;
						fp32 v1 = (Temp2[i] - Temp1[i]) * Temp1[iTension];
						v0 += v1;
						v1 += (Temp3[i] - Temp2[i]) * tsA2;

						fp32 p0 = Temp1[i];
						fp32 p1 = Temp2[i];
						fp32 D = p0;
						fp32 C = v0;
						fp32 B = fp32(3.0)*(p1 - D) - (fp32(2.0)*v0) - v1;
						fp32 A = -(fp32(2.0) * B + v0 - v1) / fp32(3.0);
						_pDest[i] = A*tCube + B*tSqr + C*Frac + D;
					}
				}
				else
				{
					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;

					fp32 tTension = nParams > 1 ? Params[1] : 0.5;

					fp32 tsA0 = tTension * Deltas[1] / Deltas[0];
					fp32 tsA2 = tTension * Deltas[1] / Deltas[2];

					// dQuatA

					// Spline it
					for(int i = 0; i < _nDim; i++)
					{
						fp32 v0 = (Temp1[i] - Temp0[i]) * tsA0;
						fp32 v1 = (Temp2[i] - Temp1[i]) * tTension;
						v0 += v1;
						v1 += (Temp3[i] - Temp2[i]) * tsA2;

						fp32 p0 = Temp1[i];
						fp32 p1 = Temp2[i];
						fp32 D = p0;
						fp32 C = v0;
						fp32 B = fp32(3.0)*(p1 - D) - (fp32(2.0)*v0) - v1;
						fp32 A = -(fp32(2.0) * B + v0 - v1) / fp32(3.0);
						_pDest[i] = A*tCube + B*tSqr + C*Frac + D;
					}
				}
				return;
			}
			break;
		case REGISTRY_ANIMIP_QUADRIC:
			{
			}
			break;
		case REGISTRY_ANIMIP_CARDINAL:
			{
				fp32 Frac = 0;
				int Keys[4];
				fp32 Deltas[3];
				Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, Deltas, 1, 2);
				if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
					break;

				bint bPerPointTension = nParams > 0 && Params[0] > 0.1;

				fp32 Temp0[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp1[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp2[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp3[REGISTRY_MAX_DIMENSIONS];
				int nUsedDim = Max(_nDim + bPerPointTension, GetDimensions());
				if (nUsedDim > REGISTRY_MAX_DIMENSIONS)
					Error_static(M_FUNCTION, "Invalid number of dimensions");
				Anim_ThisGetKFValueaf(_iSeq, Keys[0], nUsedDim, Temp0);
				Anim_ThisGetKFValueaf(_iSeq, Keys[1], nUsedDim, Temp1);
				Anim_ThisGetKFValueaf(_iSeq, Keys[2], nUsedDim, Temp2);
				Anim_ThisGetKFValueaf(_iSeq, Keys[3], nUsedDim, Temp3);

				if (bPerPointTension)
				{
					int iTension = GetDimensions() - 1;
					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;
//					fp32 tTension = 0.5f;

					fp32 h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
					fp32 h2 = -2.0f * tCube + 3.0f * tSqr;
					fp32 h3 = tCube - 2.0f * tSqr + Frac;
					fp32 h4 = tCube - tSqr;

					// Account for various timedeltas
					fp32 ts2 = 2.0f * Deltas[1] /(Deltas[1] + Deltas[2]);
					fp32 ts1 = 2.0f * Deltas[1] /(Deltas[0] + Deltas[1]);

					// Cardinal splines uses variable tension (a)
					// Catmull-Clarke splines uses a fixed 0.5 (a)
					for (int i = 0; i < _nDim; ++i)
					{
						fp32 T1 = (Temp2[i] - Temp0[i]) * Temp1[iTension] * ts1;
						fp32 T2 = (Temp3[i] - Temp1[i]) * Temp2[iTension] * ts2;
						_pDest[i] = Temp1[i] * h1;
						_pDest[i] += Temp2[i] * h2;
						_pDest[i] += T1 * h3;
						_pDest[i] += T2 * h4;
					}
				}
				else
				{
					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;
					fp32 tTension = nParams > 1 ? Params[1] : 0.5;

					fp32 h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
					fp32 h2 = -2.0f * tCube + 3.0f * tSqr;
					fp32 h3 = tCube - 2.0f * tSqr + Frac;
					fp32 h4 = tCube - tSqr;

					// Account for various timedeltas
					fp32 ts2 = 2.0f * Deltas[1] /(Deltas[1] + Deltas[2]);
					fp32 ts1 = 2.0f * Deltas[1] /(Deltas[0] + Deltas[1]);

					// Cardinal splines uses variable tension (a)
					// Catmull-Clarke splines uses a fixed 0.5 (a)
					for (int i = 0; i < _nDim; ++i)
					{
						fp32 T1 = (Temp2[i] - Temp0[i]) * tTension * ts1;
						fp32 T2 = (Temp3[i] - Temp1[i]) * tTension * ts2;
						_pDest[i] = Temp1[i] * h1;
						_pDest[i] += Temp2[i] * h2;
						_pDest[i] += T1 * h3;
						_pDest[i] += T2 * h4;
					}
				}
				return;
			}
			break;
		case REGISTRY_ANIMIP_CARDINAL_TIMEDISCONNECTED:
			{
//#if 0
				fp32 Frac = 0;
				int Keys[4];
//				fp32 DeltasIn[3];
				uint32 DeltaCalc[6];
				Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, DeltaCalc, 1, 2);
				if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
					break;

				bint bPerPointTension = nParams > 0 && Params[0] > 0.1;

				fp32 Temp0[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp1[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp2[REGISTRY_MAX_DIMENSIONS];
				fp32 Temp3[REGISTRY_MAX_DIMENSIONS];
				fp32 TempDC0[REGISTRY_MAX_DIMENSIONS];
				fp32 TempDC1[REGISTRY_MAX_DIMENSIONS];
				int nUsedDim = _nDim + bPerPointTension + 1;
				if (nUsedDim > REGISTRY_MAX_DIMENSIONS)
					Error_static(M_FUNCTION, "Invalid number of dimensions");
				Anim_ThisGetKFValueaf(_iSeq, Keys[0], nUsedDim, Temp0);
				Anim_ThisGetKFValueaf(_iSeq, Keys[1], nUsedDim, Temp1);
				Anim_ThisGetKFValueaf(_iSeq, Keys[2], nUsedDim, Temp2);
				Anim_ThisGetKFValueaf(_iSeq, Keys[3], nUsedDim, Temp3);

				int nDim = GetDimensions();
				int iTime = nDim - 1;

				int iLastKF = Anim_ThisGetNumKF(_iSeq)-1;
				Anim_ThisGetKFValueaf(_iSeq, iLastKF, nUsedDim, TempDC0);
				
				fp32 SeqLen = TempDC0[iTime];
				fp32 LoopStart = Anim_ThisGetSeqLoopStart(_iSeq);
				fp32 LoopEnd = Anim_ThisGetSeqLoopEnd(_iSeq);
				if (LoopStart < 0)
					LoopStart = 0;
				if (LoopEnd < 0)
					LoopEnd = SeqLen;
				fp32 Deltas[3];

				class CDummy
				{
				public:
					static fp32 GetKFDelta(uint32 _Flags, fp32 _Time0, fp32 _Time1, fp32 _SecLen, fp32 _LoopEnd, fp32 _LoopStart)
					{
						fp32 Duration = 0;
						if (_Flags & EGetKFFlags_Type)
						{
							uint32 Type0 = _Flags >> EGetKFFlags_TypeShift;
							switch(Type0)
							{
							case EGetKFFlags_VSeqEnd:
								Duration = (_SecLen - _Time0) + (_Time1 - _LoopStart);
								break;
							case EGetKFFlags_VLoopEnd:
								Duration = (_LoopEnd - _Time0) + (_Time1 - _LoopStart);
								break;
							case EGetKFFlags_VAssign:
								Duration = _Time0;
								break;
							}				
						}
						else
						{
							Duration = _Time0 - _Time1;
						}
						return Duration;
					}
				};

				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[0] & EGetKFFlags_Value, nUsedDim, TempDC0);
				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[1] & EGetKFFlags_Value, nUsedDim, TempDC1);
				Deltas[0] = CDummy::GetKFDelta(DeltaCalc[0], TempDC0[iTime], TempDC1[iTime], SeqLen, LoopEnd, LoopStart);
				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[2] & EGetKFFlags_Value, nUsedDim, TempDC0);
				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[3] & EGetKFFlags_Value, nUsedDim, TempDC1);
				Deltas[1] = CDummy::GetKFDelta(DeltaCalc[2], TempDC0[iTime], TempDC1[iTime], SeqLen, LoopEnd, LoopStart);
				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[4] & EGetKFFlags_Value, nUsedDim, TempDC0);
				Anim_ThisGetKFValueaf(_iSeq, DeltaCalc[5] & EGetKFFlags_Value, nUsedDim, TempDC1);
				Deltas[2] = CDummy::GetKFDelta(DeltaCalc[4], TempDC0[iTime], TempDC1[iTime], SeqLen, LoopEnd, LoopStart);

				if (bPerPointTension)
				{
					int iTension = nDim - 2;
					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;
//					fp32 tTension = 0.5f;

					fp32 h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
					fp32 h2 = -2.0f * tCube + 3.0f * tSqr;
					fp32 h3 = tCube - 2.0f * tSqr + Frac;
					fp32 h4 = tCube - tSqr;

					// Account for various timedeltas
					fp32 ts2 = 2.0f * Deltas[1] /(Deltas[1] + Deltas[2]);
					fp32 ts1 = 2.0f * Deltas[1] /(Deltas[0] + Deltas[1]);

					// Cardinal splines uses variable tension (a)
					// Catmull-Clarke splines uses a fixed 0.5 (a)
					for (int i = 0; i < _nDim; ++i)
					{
						fp32 T1 = (Temp2[i] - Temp0[i]) * Temp1[iTension] * ts1;
						fp32 T2 = (Temp3[i] - Temp1[i]) * Temp2[iTension] * ts2;
						_pDest[i] = Temp1[i] * h1;
						_pDest[i] += Temp2[i] * h2;
						_pDest[i] += T1 * h3;
						_pDest[i] += T2 * h4;
					}
				}
				else
				{
					fp32 tSqr = Sqr(Frac);
					fp32 tCube = tSqr * Frac;
					fp32 tTension = nParams > 1 ? Params[1] : 0.5;

					fp32 h1 = 2.0f * tCube - 3.0f * tSqr + 1.0f;
					fp32 h2 = -2.0f * tCube + 3.0f * tSqr;
					fp32 h3 = tCube - 2.0f * tSqr + Frac;
					fp32 h4 = tCube - tSqr;

					// Account for various timedeltas
					fp32 ts2 = 2.0f * Deltas[1] /(Deltas[1] + Deltas[2]);
					fp32 ts1 = 2.0f * Deltas[1] /(Deltas[0] + Deltas[1]);

					// Cardinal splines uses variable tension (a)
					// Catmull-Clarke splines uses a fixed 0.5 (a)
					for (int i = 0; i < _nDim; ++i)
					{
						fp32 T1 = (Temp2[i] - Temp0[i]) * tTension * ts1;
						fp32 T2 = (Temp3[i] - Temp1[i]) * tTension * ts2;
						_pDest[i] = Temp1[i] * h1;
						_pDest[i] += Temp2[i] * h2;
						_pDest[i] += T1 * h3;
						_pDest[i] += T2 * h4;
					}
				}
				return;
//#endif
			}
			break;
		}
	}
	while (0);

	// Default;
	if (_Default)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = _Default[i];
	}
	else
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = 0;
	}
		
}

void CRegistry::Anim_ThisGetIPValuead(int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValuead(_nDim, _pDest);
			else
			{
				break;
			}
		}

		fp32 Frac = 0;
		int Keys[2];
		fp32 Delta;
		Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
		if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
			break;

		if (Frac > 0.5)
			Anim_ThisGetKFValuead(_iSeq, Keys[1], _nDim, _pDest);
		else
			Anim_ThisGetKFValuead(_iSeq, Keys[0], _nDim, _pDest);

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i] = _Default[i];
}

void CRegistry::Anim_ThisGetIPValuea(int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValuea(_nDim, _pDest);
			else
			{
				break;
			}
		}

		fp32 Frac = 0;
		int Keys[2];
		fp32 Delta;
		Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
		if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
			break;

		if (Frac > 0.5)
			Anim_ThisGetKFValuea(_iSeq, Keys[1], _nDim, _pDest);
		else
			Anim_ThisGetKFValuea(_iSeq, Keys[0], _nDim, _pDest);

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i].Clear();
}

void CRegistry::Anim_ThisGetIPValuead(int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValuead(_nDim, _pDest);
			else
			{
				break;
			}
		}

		fp32 Frac = 0;
		int Keys[2];
		fp32 Delta;
		Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
		if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
			break;

		if (Frac > 0.5)
			Anim_ThisGetKFValuead(_iSeq, Keys[1], _nDim, _pDest);
		else
			Anim_ThisGetKFValuead(_iSeq, Keys[0], _nDim, _pDest);

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i].Clear();
}

void CRegistry::Anim_ThisGetIPValueai(int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest) const
{
	do
	{
		if (!Anim_ThisGetAnimated())
		{
			if (GetType() != REGISTRY_TYPE_VOID)
				return GetThisValueai(_nDim, _pDest);
			else
			{
				break;
			}
		}

		int nParams = 0;
		if (Anim_ThisGetInterpolate(NULL, nParams) == REGISTRY_ANIMIP_NONE)
		{
			fp32 Frac = 0;
			int Keys[2];
			fp32 Delta;
			Anim_ThisGetKF(_iSeq, _Time, Frac, Keys, &Delta, 0, 1);
			if (!Anim_ThisIsValidSequenceKeyframe(_iSeq, Keys[0]))
				break;

			if (Frac > 0.5)
				Anim_ThisGetKFValueai(_iSeq, Keys[1], _nDim, _pDest);
			else
				Anim_ThisGetKFValueai(_iSeq, Keys[0], _nDim, _pDest);
		}
		else
		{
			fp32 Temp[REGISTRY_MAX_DIMENSIONS];
			Anim_ThisGetIPValueaf(_iSeq, _Time, _nDim, Temp);
			for (int i = 0; i < _nDim; ++i)
				_pDest[i] = (int32)Temp[i];
		}

		return;
	}
	while (0);

	for (int i = 0; i < _nDim; ++i)
		_pDest[i] = 0;
}

void CRegistry::Anim_ThisGetIPValueaf(int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest) const
{
	return Anim_ThisGetIPValueaf(_iSeq, _Time, _nDim, _pDest, NULL);
}

// Get Interpolated Value

CStr CRegistry::Anim_ThisGetIPValue(int _iSeq, const CMTime &_Time, CStr _Default) const
{
	CStr Dest;
	Anim_ThisGetIPValuea(_iSeq, _Time, 1, &Dest, &_Default);
	return Dest;
}
int32 CRegistry::Anim_ThisGetIPValuei(int _iSeq, const CMTime &_Time, int32 _Default) const
{
	int32 Dest;
	Anim_ThisGetIPValueai(_iSeq, _Time, 1, &Dest, &_Default);
	return Dest;
}
fp32 CRegistry::Anim_ThisGetIPValuef(int _iSeq, const CMTime &_Time, fp32 _Default) const
{
	fp32 Dest;
	Anim_ThisGetIPValueaf(_iSeq, _Time, 1, &Dest, &_Default);
	return Dest;
}
TArray<uint8> CRegistry::Anim_ThisGetIPValued(int _iSeq, const CMTime &_Time, const TArray<uint8> &_Default) const
{
	TArray<uint8> Dest;
	Anim_ThisGetIPValuead(_iSeq, _Time, 1, &Dest, &_Default);
	return Dest;
}


CStr CRegistry::Anim_ThisGetIPValue(int _iSeq, fp32 _Time, CStr _Default) const
{
	CStr Dest;
	Anim_ThisGetIPValuea(_iSeq, CMTime::CreateFromSeconds(_Time), 1, &Dest, &_Default);
	return Dest;
}

int32 CRegistry::Anim_ThisGetIPValuei(int _iSeq, fp32 _Time, int32 _Default) const
{
	int32 Dest;
	Anim_ThisGetIPValueai(_iSeq, CMTime::CreateFromSeconds(_Time), 1, &Dest, &_Default);
	return Dest;
}

fp32 CRegistry::Anim_ThisGetIPValuef(int _iSeq, fp32 _Time, fp32 _Default) const
{
	fp32 Dest;
	Anim_ThisGetIPValueaf(_iSeq, CMTime::CreateFromSeconds(_Time), 1, &Dest, &_Default);
	return Dest;
}

TArray<uint8> CRegistry::Anim_ThisGetIPValued(int _iSeq, fp32 _Time, const TArray<uint8> &_Default) const
{
	TArray<uint8> Dest;
	Anim_ThisGetIPValuead(_iSeq, CMTime::CreateFromSeconds(_Time), 1, &Dest, &_Default);
	return Dest;
}

CStr CRegistry::Anim_ThisGetIPValue(int _iSeq, const CMTime &_Time) const
{
	CStr Dest;
	Anim_ThisGetIPValuea(_iSeq, _Time, 1, &Dest);
	return Dest;
}

int32 CRegistry::Anim_ThisGetIPValuei(int _iSeq, const CMTime &_Time) const
{
	int32 Dest;
	Anim_ThisGetIPValueai(_iSeq, _Time, 1, &Dest);
	return Dest;
}

fp32 CRegistry::Anim_ThisGetIPValuef(int _iSeq, const CMTime &_Time) const
{
	fp32 Dest;
	Anim_ThisGetIPValueaf(_iSeq, _Time, 1, &Dest);
	return Dest;
}

TArray<uint8> CRegistry::Anim_ThisGetIPValued(int _iSeq, const CMTime &_Time) const
{
	TArray<uint8> Dest;
	Anim_ThisGetIPValuead(_iSeq, _Time, 1, &Dest);
	return Dest;
}


/// Forwards

void CRegistry::Anim_ThisGetIPValuea(int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const
{
	return Anim_ThisGetIPValuea(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest);
}

void CRegistry::Anim_ThisGetIPValueai(int _iSeq, fp32 _Time, int _nDim, int32 *_pDest) const
{
	return Anim_ThisGetIPValueai(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest);
}
void CRegistry::Anim_ThisGetIPValueaf(int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest) const
{
	return Anim_ThisGetIPValueaf(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest);
}
void CRegistry::Anim_ThisGetIPValuead(int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest) const
{
	return Anim_ThisGetIPValuead(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest);
}

CStr CRegistry::Anim_ThisGetIPValue(int _iSeq, fp32 _Time) const
{
	return Anim_ThisGetIPValue(_iSeq, CMTime::CreateFromSeconds(_Time));
}

int32 CRegistry::Anim_ThisGetIPValuei(int _iSeq, fp32 _Time) const
{
	return Anim_ThisGetIPValuei(_iSeq, CMTime::CreateFromSeconds(_Time));
}

fp32 CRegistry::Anim_ThisGetIPValuef(int _iSeq, fp32 _Time) const
{
	return Anim_ThisGetIPValuef(_iSeq, CMTime::CreateFromSeconds(_Time));
}

TArray<uint8> CRegistry::Anim_ThisGetIPValued(int _iSeq, fp32 _Time) const
{
	return Anim_ThisGetIPValued(_iSeq, CMTime::CreateFromSeconds(_Time));
}

void CRegistry::Anim_ThisGetIPValuea(int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const CStr *_Default) const
{
	return Anim_ThisGetIPValuea(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest, _Default);
}

void CRegistry::Anim_ThisGetIPValueai(int _iSeq, fp32 _Time, int _nDim, int32 *_pDest, const int32 *_Default) const
{
	return Anim_ThisGetIPValueai(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest, _Default);
}

void CRegistry::Anim_ThisGetIPValueaf(int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const
{
	return Anim_ThisGetIPValueaf(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest, _Default);
}
void CRegistry::Anim_ThisGetIPValuead(int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const
{
	return Anim_ThisGetIPValuead(_iSeq, CMTime::CreateFromSeconds(_Time), _nDim, _pDest, _Default);
}


void CRegistry::GetValuea(int _iKey, int _nDim, CStr *_pDest) const
{
	const CRegistry* pR = GetChild(_iKey);
	pR->GetThisValuea(_nDim, _pDest);
}

void CRegistry::GetValueai(int _iKey, int _nDim, int32 *_pDest) const
{
	const CRegistry* pR = GetChild(_iKey);
	pR->GetThisValueai(_nDim, _pDest);
}

void CRegistry::GetValueaf(int _iKey, int _nDim, fp32 *_pDest) const
{
	const CRegistry* pR = GetChild(_iKey);
	pR->GetThisValueaf(_nDim, _pDest);
}

void CRegistry::GetValuead(int _iKey, int _nDim, TArray<uint8> *_pDest) const
{
	const CRegistry* pR = GetChild(_iKey);
	pR->GetThisValuead(_nDim, _pDest);
}

void CRegistry::GetValuea(const char* _pName, int _nDim, CStr *_pDest) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = CStr();
	}
	else
		return pR->GetThisValuea(_nDim, _pDest);
}

void CRegistry::GetValueai(const char* _pName, int _nDim, int32 *_pDest) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = 0;
	}
	else
		return pR->GetThisValueai(_nDim, _pDest);
}

void CRegistry::GetValueaf(const char* _pName, int _nDim, fp32 *_pDest) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = 0.0f;
	}
	else
		return pR->GetThisValueaf(_nDim, _pDest);
}

void CRegistry::GetValuead(const char* _pName, int _nDim, TArray<uint8> *_pDest) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = TArray<uint8>();
	}
	else
		return pR->GetThisValuead(_nDim, _pDest);
}

void CRegistry::GetValuea(const char* _pName, int _nDim, CStr *_pDest, const CStr *_Default) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = _Default[i];
	}
	else
		return pR->GetThisValuea(_nDim, _pDest);
}

void CRegistry::GetValueai(const char* _pName, int _nDim, int32 *_pDest, const int32 *_Default) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = _Default[i];
	}
	else
		return pR->GetThisValueai(_nDim, _pDest);
}

void CRegistry::GetValueaf(const char* _pName, int _nDim, fp32 *_pDest, const fp32 *_Default) const
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = _Default[i];
	}
	else
		return pR->GetThisValueaf(_nDim, _pDest);
}

void CRegistry::GetValuead(const char* _pName, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default)
{
	MAUTOSTRIP(CRegistry_GetValue_3, CStr());
	const CRegistry* pR = Find(_pName);
	if (!pR)
	{
		for (int i = 0; i < _nDim; ++i)
			_pDest[i] = _Default[i];
	}
	else
		return pR->GetThisValuead(_nDim, _pDest);
}


void CRegistry::SetValuea(const char* _pName, int _nDim, const CStr *_Value)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeya(_pName, _nDim, _Value);
	else
		pR->SetThisValuea(_nDim, _Value);
}

void CRegistry::SetValueai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyai(_pName, _nDim, _Value, _StoreType);
	else
		pR->SetThisValueai(_nDim, _Value, _StoreType);
}

void CRegistry::SetValueaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyaf(_pName, _nDim, _Value, _StoreType);
	else
		pR->SetThisValueaf(_nDim, _Value, _StoreType);
}

void CRegistry::SetValuead(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	CRegistry* pR = Find(_pName);
	if (!pR) 
		AddKeyad(_pName, _nDim, _lValue, _bReference);
	else
		pR->SetThisValuead(_nDim, _lValue, _bReference);
}

void CRegistry::SetValuea(int _iKey, int _nDim, const CStr *_Value)
{
	CRegistry* pR = GetChild(_iKey);
	pR->SetThisValuea(_nDim, _Value);
}

void CRegistry::SetValueai(int _iKey, int _nDim, const int32 *_Value, int _StoreType)
{
	CRegistry* pR = GetChild(_iKey);
	pR->SetThisValueai(_nDim, _Value, _StoreType);
}

void CRegistry::SetValueaf(int _iKey, int _nDim, const fp32 *_Value, int _StoreType)
{
	CRegistry* pR = GetChild(_iKey);
	pR->SetThisValueaf(_nDim, _Value, _StoreType);
}

void CRegistry::SetValuead(int _iKey, int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	CRegistry* pR = GetChild(_iKey);
	pR->SetThisValuead(_nDim, _lValue, _bReference);
}



void CRegistry::SetThisKeya(const char* _pName, int _nDim, const CStr *_Value)
{
	SetThisName(_pName);
	SetThisValuea(_nDim, _Value);
}

void CRegistry::SetThisKeyai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType)
{
	SetThisName(_pName);
	SetThisValueai(_nDim, _Value, _StoreType);
}

void CRegistry::SetThisKeyaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType)
{
	SetThisName(_pName);
	SetThisValueaf(_nDim, _Value, _StoreType);
}

void CRegistry::SetThisKeyad(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	SetThisName(_pName);
	SetThisValuead(_nDim, _lValue, _bReference);
}

void CRegistry::AddKeya(const char* _pName, int _nDim, const CStr *_Value)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValuea(_nDim, _Value);
}

void CRegistry::AddKeyai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValueai(_nDim, _Value, _StoreType);
}

void CRegistry::AddKeyaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValueaf(_nDim, _Value, _StoreType);
}

void CRegistry::AddKeyad(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference)
{
	spCRegistry spReg = CreateDir(_pName);
	spReg->SetThisValuead(_nDim, _lValue, _bReference);
}

