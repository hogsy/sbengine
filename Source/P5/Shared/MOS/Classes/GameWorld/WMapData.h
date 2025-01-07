#ifndef __WMAPDATA_H
#define __WMAPDATA_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Resource index mapper

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMapData

	History:		
		97xxxx:		Created File
\*____________________________________________________________________________________________*/


#include "WData.h"
#include "WDataRes_Core.h"
#include "WDataRes_Sound.h"
#include "WDataRes_Anim.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CMapData
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	WMAPDATA_STATE_NOCREATE = 1,			// CMapData will not try to create resources that do not exist.
};

class CRegistryDirectory
{
public:
	virtual ~CRegistryDirectory(){}
	virtual int NumReg() pure;
	virtual CRegistry *GetReg(int _iReg) pure;
};

class CMapData : public CReferenceCount
{
	MRTC_DECLARE;

	class CIndexMap : public CMap32
	{
		uint m_nUsed;
	public:
		void Create(uint _nMaxIDs);
		uint GetNewID();
	};

protected:
//	TArray<int> m_lResourceIDs;
	TArray<spCWResource> m_lspResources;
	CStr m_WorldName;
	int m_State;

#if 0
	CStringHash m_Hash;
	void Hash_Insert(int _iRc);
#else
	TPtr<CStringHash> m_spHash;

	void Hash_Insert(int _iRc);
#endif
	CIndexMap m_IndexMap;

public:
	spCWorldData m_spWData;
	class CWorld_Server *m_pWServer;

	CMapData();
	~CMapData();
	dllvirtual void Create(spCWorldData _spWData);
	dllvirtual void SetState(int _State);
	dllvirtual int GetState();
	dllvirtual void SetWorld(CStr _WorldName);
	dllvirtual CStr GetWorld();

	dllvirtual void SetIndexMap(const TThinArray<uint32>& _lResourceNameHashes);
	dllvirtual void GetIndexMap(TThinArray<uint32>& _lResourceNameHashes) const;

	// -------------------------------------------------------------------
	//  Path resolving.. these just forward the calls to CWorldData
	M_INLINE CStr ResolveFileName(CStr _FileName) { return m_spWData->ResolveFileName(_FileName); };
	M_INLINE CStr ResolvePath(CStr _FileName) { return m_spWData->ResolvePath(_FileName); };
	M_INLINE TArray_SimpleSortable<CStr> ResolveDirectory(CStr _Path, bool _bDirectories) { return m_spWData->ResolveDirectory(_Path, _bDirectories); };
	M_INLINE CRegistry* GetGameReg() { return m_spWData->GetGameReg(); };

	dllvirtual int Resource_GetCreateFlags();
	dllvirtual void Resource_PushCreateFlags(int _Flags);
	dllvirtual void Resource_PopCreateFlags();

	// -------------------------------------------------------------------
	dllvirtual int GetWorldResourceIndex(int _iRc);
protected:
	dllvirtual void PostCreateResource(int _iRc);

public:
	dllvirtual int GetNumResources();
	dllvirtual void SetNumResources(int _nRc);
	dllvirtual CStr GetResourceName(int _iRc);
	dllvirtual int GetResourceClass(int _iRc);
	dllvirtual int SetResource(int _iRc, const char* _pName, int _RcClass);

	dllvirtual int GetResourceIndex(const char* _pName, int _RcClass);
	dllvirtual int GetResourceIndex(const char* _pRcName);

	M_FORCEINLINE CWResource* GetResource(int _iRc)
	{
		if (_iRc < 0 || _iRc >= m_lspResources.Len()) return 0;
		spCWResource* lpRc = m_lspResources.GetBasePtr();
		CWResource* pRc = lpRc[_iRc];
		if (pRc && !pRc->IsLoaded())
			m_spWData->Resource_LoadSync(pRc->m_iRc);
		if (pRc && pRc->m_Flags & WRESOURCE_CORRUPT)
			return NULL;
		return pRc;
	}

	M_FORCEINLINE CWResource* GetResourceAsync(int _iRc)
	{
		if (_iRc < 0 || _iRc >= m_lspResources.Len()) return 0;
		spCWResource* lpRc = m_lspResources.GetBasePtr();
		CWResource* pRc = lpRc[_iRc];
		if (pRc && pRc->m_Flags & WRESOURCE_CORRUPT)
			return NULL;
		return pRc;
	}

protected:
#ifndef MDISABLE_CLIENT_SERVER_RES
	// CWorld_Server and CWorldData use only:
	dllvirtual class CWObject* CreateObject(const char* _pClassName);
	dllvirtual const TThinArray<spCRegistry> *GetObjectTemplates(const char *_pClassName, const TThinArray<CMat4Dfp32> **_pplMat);
#endif

	int GetResourceIndex_BSPModel1(const char* _pName);
	int GetResourceIndex_BSPModel2(const char* _pName);
	int GetResourceIndex_BSPModel3(const char* _pName);
	int GetResourceIndex_BSPModel4(const char* _pName);
public:
	// Native-resource retrival
	dllvirtual int GetResourceIndex_Model(const char* _ModelName);
	dllvirtual int GetResourceIndex_BSPModel(const char* _pName);
	dllvirtual int GetResourceIndex_CustomModel(const char* _ModelName);
	dllvirtual int GetResourceIndex_Wave(const char* _WaveName);
	dllvirtual int GetResourceIndex_Sound(const char* _SoundName);
	dllvirtual int GetResourceIndex_Anim(const char* _AnimName);
#ifndef M_DISABLE_TODELETE
	dllvirtual int GetResourceIndex_AnimList(const char* _AnimListName);
#endif
	dllvirtual int GetResourceIndex_AnimGraph2(const char* _pName);
	dllvirtual int GetResourceIndex_Dialogue(const char* _DialogueName);
	dllvirtual int GetResourceIndex_Registry(const char* _RegName);
	dllvirtual int GetResourceIndex_Surface(const char* _pName);
	dllvirtual int GetResourceIndex_Font(const char* _pName);
	dllvirtual int GetResourceIndex_XWData(const char* _pName);
	dllvirtual int GetResourceIndex_XWNavGrid(const char* _pName);
	dllvirtual int GetResourceIndex_XWNavGraph(const char* _pName);
	dllvirtual int GetResourceIndex_ObjectAttribs(const char* _pName);
	dllvirtual int GetResourceIndex_FacialSetup(const char* _pName);

	// Client / Server
#ifndef MDISABLE_CLIENT_SERVER_RES
	dllvirtual int GetResourceIndex_Class(const char* _pClassName);
	dllvirtual int GetResourceIndex_Template(const char* _pName);
#endif

protected:
	dllvirtual void GetResourceIndex_Model_Directory_r(const CStr _BasePath, const CStr _Path);
	dllvirtual void GetResourceIndex_Anim_Directory_r(const CStr _BasePath, const CStr _Path);
public:
	dllvirtual void GetResourceIndex_Model_Directory(const CStr _Path);
	dllvirtual void GetResourceIndex_Anim_Directory(const CStr _Path);

	// Pre-typecasted convenience methods for native resources.
#ifndef MDISABLE_CLIENT_SERVER_RES
	dllvirtual MRTC_CRuntimeClass_WObject* GetResource_Class(int _iClass);
	dllvirtual CWorldData::CWD_ClassStatistics* GetResource_ClassStatistics(int _iClass);
#endif
	
	class CXR_Model* GetResource_Model(int _iModel)
	{
		CWResource* pRc = GetResource(_iModel);
		if (!pRc) return NULL;

		CWRes_Model* pRCModel = TDynamicCast<CWRes_Model>(pRc);
		if (!pRCModel) return NULL;

		pRc->m_TouchTime = m_spWData->m_TouchTime;

		CXR_Model* pModel = pRCModel->GetModel();

		return pModel;
	}

	CSC_SFXDesc* GetResource_SoundDesc(CWResource *_pRes)
	{
		// Get SFX Desc from
		if (_pRes->GetClass() == WRESOURCE_CLASS_SOUND)
		{
			CWRes_Sound *pResSound = (CWRes_Sound*) _pRes;
			return pResSound->GetSound();
		}
		else if (_pRes->GetClass() == WRESOURCE_CLASS_WAVE)
		{
			CWRes_Wave *pResWave = (CWRes_Wave*) _pRes;
			return pResWave->GetSound();
		}
		else
			return NULL;
	}

	class CSC_SFXDesc* GetResource_SoundDesc(int _iSound)
	{
		// Get resource
		CWResource* pRC = GetResource(_iSound);
		if (!pRC)
			return NULL;

		return GetResource_SoundDesc(pRC);
	}

	class CXR_Anim_Base* GetResource_Anim(int _iAnim)
	{
		MSCOPE(GetResource_Anim, RES_ANIM);
		CWResource* pRC = GetResource(_iAnim);
		if (!pRC) return NULL;

		if (pRC->GetClass() == WRESOURCE_CLASS_XSA)
		{
			CWRes_Anim* pRCSound = (CWRes_Anim*) pRC;
			return pRCSound->GetAnim();
		}
		else
			return NULL;
	}
#ifndef M_DISABLE_TODELETE
	class CWRes_AnimList *GetResource_AnimList(int _iAnimList)
	{
		CWResource* pRC = GetResource(_iAnimList);
		if(!pRC)
			return NULL;

		if(pRC->GetClass() == WRESOURCE_CLASS_ANIMLIST)
			return (class CWRes_AnimList *)pRC;
		else
			return NULL;
	}
#endif

	// AGMERGE
	class CWRes_AnimGraph2 *GetResource_AnimGraph2(int _iAnimGraph2)
	{
		MSCOPE(GetResource_AnimGraph2, RES_ANIMGRAPH2);

		CWResource* pRC = GetResource(_iAnimGraph2);
		if(!pRC)
			return NULL;

		if(pRC->GetClass() == WRESOURCE_CLASS_XAH)
			return (class CWRes_AnimGraph2*)pRC;
		else
			return NULL;
	}

	dllvirtual class CWRes_Dialogue *GetResource_Dialogue(int _iDialogue);
	dllvirtual class CRegistry* GetResource_Registry(int _iReg);
	dllvirtual class CXW_Surface* GetResource_Surface(int _iRc);
	dllvirtual int GetResource_SurfaceID(int _iRc);
	dllvirtual class CRC_Font* GetResource_Font(int _iReg);
	dllvirtual TAP<const uint8> GetResource_XWData(int _iRc, uint _iXWResource);
	dllvirtual class CXR_BlockNav_Grid_GameWorld* GetResource_XWNavGrid(int _iRc);
	dllvirtual class CXR_NavGraph* GetResource_XWNavGraph(int _iRc);
	dllvirtual CRegistryDirectory *GetResource_ObjectAttribs(int _iRc);
	dllvirtual class CFacialSetup* GetResource_FacialSetup(int _iRc);

	// IO, system use only.
	dllvirtual void Write(CDataFile* _pDFile);
	dllvirtual void Read(CDataFile* _pDFile);

	//
	dllvirtual void LogUsed(CStr _Filename);

	// Debug dump
	dllvirtual void Dump(int _DumpFlags);

	friend class CWorld_Server;
	friend class CWorld_ServerCore;
};

typedef TPtr<CMapData> spCMapData;

#endif	// _INC_WMAPDATA
