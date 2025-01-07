#ifndef __WDATACORE_H
#define __WDATACORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Resource management classes, implementation
					
	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson
					
	Copyright:		Starbreeze Studios 2003
					
	Contents:		CWorldDataCore
					
	History:		
		010923:		Created File. Moved implementation of CWorldData to CWorldDataCore to
					make interface cleaner and to reduce file recompiles due to header changes.
\*____________________________________________________________________________________________*/

#include "WData.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldDataLoader
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CWorldDataLoader : public CReferenceCount, public MRTC_Thread
{
protected:
	TArray<CWResource*> m_lpLoadQueue;

	const char* Thread_GetName() const;

public:
	int m_Status;

	CWorldDataLoader();
	virtual void Create();
	virtual int Thread_Main();
	virtual void AddToQueue(CWResource* _pRc, bool _bUrgent);
	virtual void RemoveFromQueue(CWResource* _pRc);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldDataCore
|__________________________________________________________________________________________________
\*************************************************************************************************/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource manager implementation						
\*____________________________________________________________________*/

class CWorldDataCore : public CWorldData
{
	MRTC_DECLARE;

protected:

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class:				Resource class descriptor

		Comments:

			A CWResourceClassDesc describes a resource class. To add game-
			specific resource classes, inherit CWorldData and add your custom 
			resource classes to the m_lClasses list in the Create method. 
			Also, you must change the *RESOURCECLASS "CWorldData" key in the 
			game-registry.

			Resource classes must be inherited from CWResource and behave 
			in accordance with the "Resource philosophy" ramble in the top 
			of this file.  :)
	\*____________________________________________________________________*/

	class CWD_ResourceClassDesc : public CObj
	{
	public:
		CStr m_ClassName;
		char m_Prefix[4];
//		fp64 m_AutoHibernateTimeout;
//		fp64 m_AutoUnloadTimeout;
		fp32 m_AutoHibernateTimeout;	//JK-NOTE: Do we _really_ need the precision of fp64 for this?
		fp32 m_AutoUnloadTimeout;	//JK-NOTE: Do we _really_ need the precision of fp64 for this?
		int m_MemUsed;
		int m_nResources;

		void Create(CStr _ClassName, CStr _Prefix, fp64 _AutoHibernateTimeout, fp64 _AutoUnloadTimeout)
		{
			if (_Prefix.Len() != 3) Error("CWResourceDesc", "Resource class prefix must be 3 chars.");
			m_ClassName = _ClassName;
			memcpy(m_Prefix, (char*) _Prefix, 3);
			m_Prefix[3] = 0;
			m_AutoHibernateTimeout = _AutoHibernateTimeout;
			m_AutoUnloadTimeout = _AutoUnloadTimeout;
			m_nResources = 0;
		}

		CWD_ResourceClassDesc()
		{
			m_Prefix[0] = 0;
			m_MemUsed = 0;
			m_AutoHibernateTimeout = 0.0f;
			m_AutoUnloadTimeout = 0.0f;
		};

		CWD_ResourceClassDesc(CStr _ClassName, CStr _Prefix, fp64 _AutoHibernateTimeout = 0.0f, fp64 _AutoUnloadTimeout = 0.0f)
		{
			Create(_ClassName, _Prefix, _AutoHibernateTimeout, _AutoUnloadTimeout); 
		}
	};

protected:	
	// -------------------------------------------------------------------
	CWorldDataLoader m_Loader;

	int m_bCreated;
//	int m_Status;

	TArray<spCTextureContainer> m_lspTC;				// Installed texture-containers.
	TArray<spCWaveContainer_Plain> m_lspWC;				// Wavecontainers, samples & SFX-desc.
	TPtr<CTextureContainer_Video> m_spTCVideo;			// Texture container for all videos under GamePath/Videos/
	TPtr<CTextureContainer_Video> m_spTCSecVideo;		// Secondary video container

	TArray<CWD_ResourceClassDesc> m_lClasses;

	TArray<spCWResource> m_lspResources;
	CMTime m_TimeLastRefresh;
	CMTime m_TimeLastHibernate;
	int m_iLastHibernate;

	TArray<spCWResource> m_lspResourceForceIncludeRef;

#if 0
	void Hash_Insert(int _iRc);
	CStringHash m_Hash;
#else
	TPtr<CStringHash> m_spHash;
	TPtr<CStringHash> m_spPartialNameHash;

	void Hash_Update();
	void Hash_Insert(int _iRc);
	virtual void Hash_Clear();
#endif

	// Resource-scanning
	spCWaveContainer_Plain ReadWaveContainer(CStr _Name);
	void ReadTextureContainer(CStr _Name);

	TPtr<CReferenceCount> m_spAsyncCopyContext;
	uint32 m_AsyncDisable;

		
	CStr m_CachePath;
	TArray<CStr> m_lWorldPathes;					// Used by ResolveFileName, last path in the list has the highest priority.
//	CStr m_WorldPath;								// If you think you need this you should probably use ResolveFileName instead.

	spCRegistry m_spGameReg;

private:
	uint64 FindTextureContainers_r(uint _iWorldPath, CStr _Path, TArray<CStr>& _lFileNames) const;
	void ReadTextureContainers(TAP<const CStr> _lFileNames);

public:
	CWorldDataCore();
	~CWorldDataCore();
	virtual void Create(spCRegistry _spGameReg, int _Flags = FLAGS_SCANTEXTURES | FLAGS_SCANWAVES | FLAGS_SCANSURFACES | FLAGS_SCANVIDEOS);
	virtual void OnRefresh();
/*	virtual int GetStatus();
	virtual void SetStatus(int);
	virtual void ClearStatus(int);*/

	// -------------------------------------------------------------------
	struct CContainerPath
	{
		CStr m_Path;
		int m_iContentDirectory;
	};
	void ScanWaveContainers_r(int _iContentDirectory, CStr _Path, TArray<CContainerPath> &_lContainers);
	void ScanWaveContainers(CStr _Path);
	void ScanTextureContainers(CStr _Path);
	void ScanVideos(CStr _Path, CTextureContainer_Video* _pTCVideo, CStr _Ext);

	// -------------------------------------------------------------------
	virtual CStr ResolveFileName(CStr _FileName, bool _bAllowCache = true);	// Returns a complete filename
	virtual CStr ResolvePath(CStr _FileName);		// Returns the path to where from _FileName should be read.
	virtual TArray_SimpleSortable<CStr> ResolveDirectory(CStr _Path, bool _bDirectories);	// Reads a combined directory from the worldpathes. _Path must contain a wildcard. eg. Sbz1\Worlds\*.XW

	virtual bool AddWorldPath(CStr _Path);
	virtual bool RemoveWorldPath(CStr _Path);

protected:
	virtual CStr GetBasePath();						// Equal to m_lWorldPathes[0]

public:
	MRTC_CriticalSection m_WDataLock;
	virtual CRegistry* GetGameReg() { return m_spGameReg; };

	virtual TArray<spCTextureContainer> &GetTextureContainers() { return m_lspTC; }

	// -------------------------------------------------------------------
	virtual const char* GetResourceClassName(int _iRcClass);
	virtual const char* GetResourceClassPrefix(int _iRcClass);

	virtual int ResourceExists(const char* _pRcName);
	virtual int AddResource(spCWResource _spRes);

	virtual int ResourceExistsPartial(const char* _pRcName);

	virtual int GetResourceIndex(const char* _pRcName, int _RcClass, CMapData* _pMapData);
	virtual int GetResourceIndex(const char* _pRcName, CMapData* _pMapData);
	virtual CWResource* GetResource(const char* _RcName, int _RcClass, CMapData* _pMapData);
	virtual CWResource* GetResource(int _iRc);
	virtual spCWResource GetResourceRef(int _iRc);
	virtual TArray<spCWResource> &GetResourceList() { return m_lspResources; }

	enum
	{
		WRESOURCE_CREATEFLAGSSTACKDEPTH = 16
	};

	uint8 m_lCreateFlagsStack[WRESOURCE_CREATEFLAGSSTACKDEPTH];
	int m_iCreateFlagsStack;

	spCTextureContainer m_spTextureContainer;
	spCWaveContainer m_spWaveContainer;
	spCWaveContainer m_spWaveContainerStreamed;
/*
	CStr m_WorldNameClient;
	CStr m_WorldNameServer;

	CStr m_WorldName;
*/
	CFStr m_WorldName;
	CFStr m_WorldNameClient;
	CFStr m_WorldNameServer;

	int m_XDFSize_Server;
	int m_XDFSize_Precache;
	int m_XDFSize_MaxPos;

	virtual int XDF_GetSize();
	virtual int XDF_GetPosition();

	void Resource_WorldOpen(CStr _WorldName);
	void Resource_WorldClose();

	virtual int Resource_WorldNeedXDF();
	virtual int Resource_WorldOpenClient(CStr _WorldName);
	virtual int Resource_WorldCloseClient();

	virtual int Resource_WorldOpenServer(CStr _WorldName);
	virtual int Resource_WorldCloseServer();


	virtual int Resource_GetCreateFlags();
	virtual void Resource_PushCreateFlags(int _Flags);
	virtual void Resource_PopCreateFlags();

	virtual int Resource_GetMaxIndex();

	virtual int Resource_GetFlags(int _iRc);
	virtual bool Resource_IsValid(int _iRc);
	virtual int Resource_GetClass(int _iRc);
	virtual const char* Resource_GetName(int _iRc);
	virtual const char* Resource_GetClassPrefix(int _iRc);
	virtual void Resource_Delete(int _iRc);
	virtual void Resource_Unload(int _iRc);
	virtual void Resource_Load(int _iRc);
	virtual void Resource_LoadSync(int _iRc);
	virtual void Resource_Precache(int _iRc, class CXR_Engine* _pEngine);
	virtual void Resource_PostPrecache(int _iRc, class CXR_Engine* _pEngine);
	virtual void Resource_Touch(int _iRc);

	virtual void Resource_AutoHibernate();
	virtual void Resource_DeleteUnreferenced(class CXR_VBManager* _pVBM, int _Mask);
	
	virtual bool Resource_AsyncCacheRefresh(CStr _PrioCopy = CStr());
	virtual void Resource_AsyncCacheBlockOnCategory(int _Category);
	virtual void Resource_AsyncCacheBlockOnFile(int _Category, CStr _FileName);
	virtual void Resource_AsyncCacheEnable(uint32 _Mask);
	virtual void Resource_AsyncCacheDisable(uint32 _Mask);
	virtual uint32 Resource_AsyncCacheGetEnabled();
	virtual bool Resource_AsyncCacheCategoryDone(int _Category);
	virtual bool Resource_AsyncCacheFileDone(int _Category, CStr _FileName);

	// -------------------------------------------------------------------
	virtual TArray<spCWaveContainer_Plain> GetWCList() { return m_lspWC; }
	virtual void AddWC(spCWaveContainer_Plain _spWC)
	{
		m_lspWC.Add(_spWC);
	}
	virtual void RemoveWC(spCWaveContainer_Plain _spWC)
	{
		for (int i = 0; i < m_lspWC.Len(); ++i)
		{
			if (m_lspWC[i] == _spWC)
			{
				m_lspWC.Del(i);
				break;
			}
		}
		
	}

#ifndef MDISABLE_CLIENT_SERVER_RES
	virtual void ClearClassStatistics();
#endif

protected:
	virtual void UpdateResourceClass(const char* _pPrefix);

public:
	virtual void Con_RS_UpdateSurfaces();
	virtual void Con_RS_UpdateAnims();
	virtual void Con_RS_UpdateXMD();
	virtual void Con_RS_UpdateRegistry();
	virtual void Con_RS_Classes();
	virtual void Con_RS_Resources();
	virtual void Con_RS_ResourcesByClass(int _iClass);
	virtual void Con_RS_ResourcesSorted();
	void Register(CScriptRegisterContext &_RegContext);

	friend class CMapData;
};

typedef TPtr<CWorldDataCore> spCWorldDataCore;

#endif // _INC_WDATACORE
