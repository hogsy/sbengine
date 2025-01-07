
#include "PCH.h"
#include "WDataRes_Anim.h"
#include "WDataRes_Core.h"
#include "WObjects/WObj_AnimUtils.h"

MRTC_IMPLEMENT_DYNAMIC(CWRes_Anim, CWResource);
#ifndef M_DISABLE_TODELETE
MRTC_IMPLEMENT_DYNAMIC(CWRes_AnimList, CWResource);
#endif

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)

// -------------------------------------------------------------------
//  ANIMATION
// -------------------------------------------------------------------
CWRes_Anim::CWRes_Anim()
{
	MAUTOSTRIP(CWRes_Anim_ctor, MAUTOSTRIP_VOID);
	m_bExists = true;
}

CXR_Anim_Base* CWRes_Anim::GetAnim()
{
	MAUTOSTRIP(CWRes_Anim_GetAnim, NULL);
//	MSCOPE(CWRes_Anim::GetAnim, RES_ANIM);
	
	if (!m_spAnim && m_bExists)
	{
		CStr Name = m_Name.Copy(4, 100000);
		CStr ClassName = Name.GetStrSep(":");
		CStr Params = Name;

		CStr FileName = m_pWData->ResolveFileName("ANIM\\" + ClassName + ".XSA");

		spCReferenceCount spObjLoader = (CReferenceCount*) MRTC_GOM()->CreateObject("CXR_Anim_Base");
		spCXR_Anim_Base spAnim = safe_cast<CXR_Anim_Base>((CReferenceCount*)spObjLoader);
		if (!spAnim) Error("Create", "Unable to instance CXR_Anim_Base-object.");

		m_spAnim = spAnim->ReadMultiFormat(FileName, ANIM_READ_NOCOMMENTS);
		if(Params != "")
		{
			uint8 Strip[128];
//			int Len = Params.Len();
//			Params = "ffffffffffffffffffff";
			Params.MakeUpperCase();
			int c = 0;
			for(int i = Params.Len() - 1; i >= 0; i--)
			{
				int Val = Params[i];
				if(Val > '9')
					Val -= 'A' - 10;
				else
					Val -= '0';

				if(c & 1)
					Strip[c >> 1] |= Val << 4;
				else
					Strip[c >> 1] = Val;
				c++;
			}

			spCXR_Anim_Base spAnim = m_spAnim->StripSequences(Strip, (c + 1) >> 1);
			if(spAnim != NULL)
				m_spAnim = spAnim;
		}

		if (!m_spAnim)
			m_bExists = false;
	}

	return m_spAnim;
}

bool CWRes_Anim::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Anim_Create, false);
//	MSCOPE(CWRes_Anim::Create, RES_ANIM);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	m_pWData = _pWData;
	CStr ClassName = CStr(_pName).Copy(4, 100000).GetStrSep(":");
	CStr FileName = _pWData->ResolveFileName("ANIM\\" + ClassName + ".XSA");
	if (!CDiskUtil::FileExists(FileName))
	{
		//ConOutL(CStrF("§cf80WARNING: (CWRes_Anim::Create) Could not find %s", FileName.Str()));
		return false;
	}

	return true;
}

int CWRes_Anim::IsLoaded()
{
	MAUTOSTRIP(CWRes_Anim_IsLoaded, 0);
	return m_spAnim != NULL;
}

void CWRes_Anim::OnLoad()
{
	MAUTOSTRIP(CWRes_Anim_OnLoad, MAUTOSTRIP_VOID);
//	MSCOPE(CWRes_Anim::OnLoad, RES_ANIM);
	
	if (IsLoaded()) return;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys->GetEnvironment()->GetValue("rs_preload_anim", "1").Val_int() == 0) return;
	GetAnim();
}

void CWRes_Anim::OnUnload()
{
	MAUTOSTRIP(CWRes_Anim_OnUnload, MAUTOSTRIP_VOID);
//	MSCOPE(CWRes_Anim::OnUnload, RES_ANIM);
	
	m_spAnim = NULL;
}

void CWRes_Anim::OnRegisterMapData(class CWorldData* _pWData, class CMapData* _pMapData)
{
	CXR_Anim_Base *pAnim = GetAnim();
	for(int i = 0; i < pAnim->GetNumSequences(); i++)
	{
		CXR_Anim_SequenceData *pSeq = pAnim->GetSequence(i);
		CMTime Time,EndTime;
		Time.Reset();
		EndTime = CMTime::CreateFromSeconds(10000.0f);
		int16 iKey = 0;
		const CXR_Anim_DataKey *pKey;
		int32 ScanMask = ANIM_EVENT_MASK_SOUND | ANIM_EVENT_MASK_EFFECT;
		while((pKey = pSeq->GetEvents(Time, EndTime, ScanMask,iKey)))
		{
			if ((pKey->m_Type == ANIM_EVENT_TYPE_SOUND) || 
				(pKey->m_Type == ANIM_EVENT_TYPE_SETSOUND))
			{
				_pMapData->GetResourceIndex_Sound(pKey->Data());
			}
			else if (pKey->m_Type == ANIM_EVENT_TYPE_EFFECT)
			{
				if (pKey->m_Param == ANIM_EVENT_EFFECTTYPE_WEAPON)
				{
					CStr KeyData(pKey->Data());
					KeyData.GetStrSep("#");
					_pMapData->GetResourceIndex_Model(KeyData);
				}
			}
		}
	}
}

// -------------------------------------------------------------------
//  ANIMATIONLIST
// -------------------------------------------------------------------
#ifndef M_DISABLE_TODELETE
CWRes_AnimList::CWRes_AnimList()
{
	MAUTOSTRIP(CWRes_AnimList_ctor, MAUTOSTRIP_VOID);
	MemSet(m_Sequences, 0, sizeof(m_Sequences));
	m_pWData = NULL;
}

bool CWRes_AnimList::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_AnimList_Create, false);
//	MSCOPE(CWRes_AnimList::Create, RES_ANIM);
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;

	m_pWData = _pWData;

	int Len = CStrBase::StrLen(_pName);
	if(Len < 5)
		return false;

	spCRegistry spReg;

	CStr Name = &_pName[4];
	
	while(Name != "")
	{		
		CStr FileName = _pWData->ResolveFileName("ANIM\\" + Name + ".XRG");
		if(!CDiskUtil::FileExists(FileName))
			break;

		spCRegistry spReg2 = REGISTRY_CREATE;
		spReg2->XRG_Read(FileName);
		if(spReg2->GetNumChildren() < 1 || spReg2->GetName(0) != "ANIMLIST")
			Error("Create", CStrF("File %s is an invalid AnimList", (char *)FileName));

		Name = spReg2->GetValue(0);

		if(spReg)
			spReg2->CopyDir(spReg);
		spReg = spReg2;
	}

	if(!spReg || spReg->GetNumChildren() < 1)
		return false;

	CRegistry *pList = spReg->GetChild(0);
	if(pList->GetNumChildren() > 256)
		Error("Create", CStrF("AnimList %s contains more than 255 animations (%s=%s)", &_pName[4], (char *)pList->GetName(256), (char *)pList->GetValue(256)));

	for(int i = 0; i < pList->GetNumChildren(); i++)
	{
		CFStr Sequence = pList->GetValue(i);
		CFStr Anim = Sequence.GetStrSep(":");
		if(Anim != "")
		{
			int iSeq = Sequence.Val_int();
			if(iSeq > 255 || iSeq < 0)
				Error("Create", CStrF("AnimList %s contains a sequence index greater than 255 (anim %i)", &_pName[4], i));
			m_Sequences[i] = iSeq;

			int iAnim = 0;
			{
//				MSCOPE(GetResourceIndex, RES_ANIM);
				RESOURCE_MEMSCOPE;
				iAnim = _pWData->GetResourceIndex("ANM:" + Anim, WRESOURCE_CLASS_XSA, _pMapData);
				m_MemUsed -= RESOURCE_MEMDELTA;
			}

			if(iAnim == 0)
				ConOutL(CStrF("§cf80WARNING: (CWRes_AnimList::Create) Animlist %s had an invalid sequence (%s=%s) at ID %i", &_pName[4], pList->GetName(i).Str(), pList->GetValue(i).Str(), i));
			else
				m_lspAnims[i] = _pWData->GetResourceRef(iAnim);
		}
		else
		{
			m_Sequences[i] = 0;
			m_lspAnims[i] = NULL;
		}
	}

	return true;
}

CXR_Anim_Base *CWRes_AnimList::GetAnim(int _Index, bool _bReportFail)
{
	MAUTOSTRIP(CWRes_AnimList_GetAnim, NULL);
	if(!m_lspAnims[_Index])
	{
		if(_bReportFail)
			ConOutL(CStrF("Animation %i in AnimList %s does not exist (Get)", _Index, (char *)m_Name));
		return NULL;
	}

	if(m_lspAnims[_Index]->GetClass() != WRESOURCE_CLASS_XSA)
		Error("GetAnim", "Internal error");

	CWRes_Anim *pAnimRes = (CWRes_Anim *)(CWResource *)m_lspAnims[_Index];

	return pAnimRes->GetAnim();
}

int CWRes_AnimList::GetSequence(int _Index)
{
	MAUTOSTRIP(CWRes_AnimList_GetSequence, 0);
	return (uint8)m_Sequences[_Index];
}

#endif
