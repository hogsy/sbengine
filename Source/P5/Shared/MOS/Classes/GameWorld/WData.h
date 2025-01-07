#ifndef _INC_WDATA
#define _INC_WDATA

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Resource management classes
					
	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios, 2003
					
	Contents:		class CWorldData
					class CWResource
					
	Comments:	

		Resource philosophy:

		- Resources are static data, they do not contain a visible state to it's user. 
			(with exception to interfaces that requires context-allocation)

		- A resource can be constructed fully from it's name. For example:
			"BSP:ELM3.XW:23"

		- When created, resources are assigned a unique ID. The resource IDs are 
			identical on the server and it's clients. This way any kind of resource
			can be identified with only a 16-bit word.
			However, when a game is saved the resource mapping may change. Therefore, 
			savegame code must store the proper resource name instead of the ID.

		- Any resource can be used by any number of entities, servers, clients, 
			games, etc.. , simultaneously.

		- Resources are not, and need not be, stored in savegames or transfered to 
			clients upon connection. (files may be downloaded once if the resources
			are not present on the client. Once downloaded, they shall remain static.
			If a resource modifaction is required it should be assigned a new name)

		- Since resources are virtualy read-only they can be loaded and unloaded
			by the resource-manager at any time it sees fit. The game doesn't
			need to know about this.


		/mh
					
	History:		
		97????:		Created File

		010208:		New comments

\*____________________________________________________________________________________________*/

#include "../../MOS.h"
#include "../../MSystem/Raster/MTextureContainers.h"
//#include "WClient.h"
#include "WClass.h"
#include "../../XR/XR.h"

#define RESOURCE_MEMSCOPE int __Resource_Scope_Mem0 = MRTC_MemUsed()
#define RESOURCE_MEMDELTA (__Resource_Scope_Mem0 - MRTC_MemUsed())

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWResource, base class for all resources
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	WRESOURCE_LOADED				= 1,
	WRESOURCE_PRECACHE				= 2,
	WRESOURCE_ASYNCLOADING			= 4,
	WRESOURCE_ASYNCLOADHIGHPRIOR	= 8,
	WRESOURCE_CORRUPT				= 16,
	WRESOURCE_RESIDENT				= 32,

};

class CWResource : public CReferenceCount
{
public:
	CStr m_Name;

	volatile unsigned int m_Flags : 8;
	unsigned int m_iRcClass : 8;
	unsigned int m_iRc : 16;
	
public:
//	fp64 m_TouchTime;
	CMTime m_TouchTime;	// JK-NOTE: Do we _really_ need the precision of fp64 here?
	int m_MemUsed;

	CWResource();
	virtual bool Create(class CWorldData* _pWData, const char* _pName, class CMapData* _pMapData, int _iRcClass);
	virtual CStr GetFilesRequired() const { return ""; };
	virtual CStr GetName() const { return m_Name; };
	virtual int GetClass() const { return m_iRcClass; };

	virtual int IsLoaded() { return m_Flags & WRESOURCE_LOADED; };
	virtual int IsPrecache() { return m_Flags & WRESOURCE_PRECACHE; };
	virtual int IsAsyncLoading() { return m_Flags & WRESOURCE_ASYNCLOADING; };
	virtual int IsResident() { return m_Flags & WRESOURCE_RESIDENT; };

	virtual void OnRefresh();
	virtual void OnHibernate();
	virtual void OnLoad();
	virtual void OnUnload();
	virtual void OnPrecache(class CXR_Engine* _pEngine);	// Precache resources used by this resource, such as textures.
	virtual void OnPostPrecache(class CXR_Engine* _pEngine);	// Do PostPrecache work on this resource (such as freeing compressed clusters)
	virtual void OnRegisterMapData(class CWorldData* _pWData, class CMapData* _pMapData);
};

typedef TPtr<CWResource> spCWResource;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldData
|__________________________________________________________________________________________________
\*************************************************************************************************/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				Resource manager
						
	Comments:
		CWorldData contain all resources for a gamecontext. It can be 
		used by several instances of a game at once and share the 
		resources between the instances.

		CMapData is a game-instance specific mapping of the resources.
		It maps game resource IDs to CWorldData resources. The resource 
		mapping is identical on the server and client (Their CMapData 
		map identical IDs to identical resources) but their respective
		CWorldData may differ.
\*____________________________________________________________________*/

#include "../../Classes/Miscellaneous/MFileContainer.h"

class CWorldData : public CConsoleClient
{

	MRTC_DECLARE;

	
public:
	enum
	{
		FLAGS_SCANTEXTURES = 1,
		FLAGS_SCANWAVES = 2,
		FLAGS_SCANSURFACES = 4,
		FLAGS_SCANVIDEOS = 8,
	};
	
	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		Class:				CWObject execution statistics

		Comments:

			Public information data structure used and constructed 
			by CWRes_Class (Resource for CWObject classes)
			
			Used to track CPU utilization and network bandwidth on a per 
			CWObject-class basis. (Do not confuse this with resource-classes)

	\*____________________________________________________________________*/
#ifndef MDISABLE_CLIENT_SERVER_RES

	class CWD_ClassStatistics
	{
	public:
		CMTime m_ExecuteTime;
		int m_nExecute;
		int m_NetUpdate;
		int m_nNetUpdates;

		CMTime m_ClientExecuteTime;
		CMTime m_ClientRenderTime;
		int m_nClientExecute;
		int m_nClientRender;

		void Clear()
		{
			m_ExecuteTime.Reset();
			m_nExecute = 0;
			m_NetUpdate = 0;
			m_nNetUpdates = 0;

			m_ClientExecuteTime.Reset();
			m_nClientExecute = 0;
			m_ClientRenderTime.Reset();
			m_nClientRender = 0;
		}

		CWD_ClassStatistics()
		{
			Clear();
		}
	};
#endif

public:
//	fp64 m_TouchTime;
//	fp32	m_TouchTime;	//JK-NOTE: Do we _really_ need the precision of fp64 for this?

	CMFileContainer m_DialogContainer;
	CMTime m_TouchTime;
	CFileAsyncWrite m_AsyncWriter;

	virtual void Create(spCRegistry _spGameReg, int _Flags = FLAGS_SCANTEXTURES | FLAGS_SCANWAVES | FLAGS_SCANSURFACES | FLAGS_SCANVIDEOS) pure;
	virtual void OnRefresh() pure;
	//virtual int GetStatus() pure;
	//virtual void SetStatus(int) pure;
	//virtual void ClearStatus(int) pure;
	virtual void Hash_Clear() {}
	virtual void Hash_Update() pure;

	// -------------------------------------------------------------------
	static CStr ResolveFileName(CStr *_pPaths, int _nPaths, CStr _FileName);	// Returns a complete filename
	static CStr ResolvePath(CStr *_pPaths, int _nPaths, CStr _FileName);		// Returns the path to where from _FileName should be read.

	virtual CStr ResolveFileName(CStr _FileName, bool _bAllowCache = true) pure;	// Returns a complete filename
	virtual CStr ResolvePath(CStr _FileName) pure;									// Returns the path to where from _FileName should be read.
	virtual TArray_SimpleSortable<CStr> ResolveDirectory(CStr _Path, bool _bDirectories) pure;	// Reads a combined directory from the worldpathes. _Path must contain a wildcard. eg. Sbz1\Worlds\*.XW
	virtual bool AddWorldPath(CStr _Path) pure;
	virtual bool RemoveWorldPath(CStr _Path) pure;
protected:
	virtual CStr GetBasePath() pure;												// Equal to m_lWorldPathes[0]

public:
	virtual CRegistry* GetGameReg() pure;
	virtual TArray<spCTextureContainer> &GetTextureContainers() pure;

	// -------------------------------------------------------------------
	virtual const char* GetResourceClassName(int _iRcClass) pure;
	virtual const char* GetResourceClassPrefix(int _iRcClass) pure;

	virtual int ResourceExists(const char* _pRcName) pure;
	virtual int AddResource(spCWResource _spRes) pure;

	// Find resource from partial name
	virtual int ResourceExistsPartial(const char* _pRcName) pure;

	virtual int GetResourceIndex(const char* _pRcName, int _RcClass, CMapData* _pMapData) pure;
	virtual int GetResourceIndex(const char* _pRcName, CMapData* _pMapData) pure;
	virtual CWResource* GetResource(const char* _RcName, int _RcClass, CMapData* _pMapData) pure;
	virtual CWResource* GetResource(int _iRc) pure;
	virtual spCWResource GetResourceRef(int _iRc) pure;
	virtual TArray<spCWResource> &GetResourceList() pure;

	// -------------------------------------------------------------------
	virtual int XDF_GetSize() pure;
	virtual int XDF_GetPosition() pure;

	virtual int Resource_WorldNeedXDF() pure;
	virtual int Resource_WorldOpenClient(CStr _WorldName) pure;
	virtual int Resource_WorldCloseClient() pure;

	virtual int Resource_WorldOpenServer(CStr _WorldName) pure;
	virtual int Resource_WorldCloseServer() pure;

	virtual int Resource_GetCreateFlags() pure;
	virtual void Resource_PushCreateFlags(int _Flags) pure;
	virtual void Resource_PopCreateFlags() pure;

	virtual int Resource_GetMaxIndex() pure;

	virtual bool Resource_AsyncCacheRefresh(CStr _PrioCopy = CStr()) pure;
	virtual void Resource_AsyncCacheEnable(uint32 _Mask) pure;
	virtual void Resource_AsyncCacheDisable(uint32 _Mask) pure;
	virtual uint32 Resource_AsyncCacheGetEnabled() pure;
	virtual void Resource_AsyncCacheBlockOnCategory(int _Category) pure;
	virtual void Resource_AsyncCacheBlockOnFile(int _Category, CStr _FileName) pure;
	virtual bool Resource_AsyncCacheCategoryDone(int _Category) pure;
	virtual bool Resource_AsyncCacheFileDone(int _Category, CStr _FileName) pure;

	virtual int Resource_GetFlags(int _iRc) pure;
	virtual bool Resource_IsValid(int _iRc) pure;
	virtual int Resource_GetClass(int _iRc) pure;
	virtual const char* Resource_GetName(int _iRc) pure;
	virtual const char* Resource_GetClassPrefix(int _iRc) pure;
	virtual void Resource_Delete(int _iRc) pure;
	virtual void Resource_Unload(int _iRc) pure;
	virtual void Resource_Load(int _iRc) pure;
	virtual void Resource_LoadSync(int _iRc) pure;
	virtual void Resource_Precache(int _iRc, class CXR_Engine* _pEngine) pure;
	virtual void Resource_PostPrecache(int _iRc, class CXR_Engine* _pEngine) pure;
	virtual void Resource_Touch(int _iRc) pure;

	virtual void Resource_AutoHibernate() pure;
	virtual void Resource_DeleteUnreferenced(class CXR_VBManager* _pVBM, int _Mask = -1) pure;

	// -------------------------------------------------------------------
	virtual TArray<spCWaveContainer_Plain> GetWCList() pure;
	virtual void AddWC(spCWaveContainer_Plain _spWC) pure;
	virtual void RemoveWC(spCWaveContainer_Plain _spWC) pure;

#ifndef MDISABLE_CLIENT_SERVER_RES
	virtual void ClearClassStatistics() pure;
#endif

	friend class CMapData;
};

typedef TPtr<CWorldData> spCWorldData;

#endif	// _INC_WDATA
