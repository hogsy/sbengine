#ifndef __WSERVERMOD_H
#define __WSERVERMOD_H

#include "../../Shared/MOS/Classes/GameWorld/Server/WServer_Core.h"
#include "../../Shared/MOS/Classes/GameWorld/WDeltaGameState.h"

class CAnimContainerCollector
{
public:
	class CSequenceHandler
	{
	protected:
		TArray<uint8> m_BitArray;
	public:
		void AddEntry(aint _Pos);
		char GetHex(uint8 _Val);
		CStr GetHexString();
	};
	class CContainerEntry
	{
	public:
		CSequenceHandler m_SeqHandler;
		TArray<int32> m_liAnims;
		CStr m_ContainerName;
		int32 m_iContainerResource;

		CContainerEntry()
		{
			m_iContainerResource = -1;
		}
	};

	TArray<CContainerEntry> m_Containers;

	void AddEntry(CStr _Name, int32 _iAnimSeq, int32 _iAnim);

	void LoadData(CMapData* _pMapData, CWorldData* _pWData);
	void Destroy()
	{
		m_Containers.Destroy();
	}
};
//class spCXRAG_AnimList;

class CWServer_Mod : public CWorld_ServerCore
{
protected:
	MRTC_DECLARE;


	virtual void World_Save(CStr _SaveName, bool _bDotest = false);
	virtual void World_Load(CStr _SaveName);
	virtual bool World_SaveExist(CStr _SaveName);
	virtual bool World_DeleteSave(CStr _SaveName);

	virtual void World_Close();
	virtual void World_Change(const CFStr &_WorldName, int _Flags);
	virtual uint32 World_TranslateSpawnFlags(const char* _pFlagsStr) const;
	virtual const char* World_GetCurrSpawnFlagsStr(uint32 _IgnoreMask = 0) const;
	void DoOnFinishDeltaLoad();

	TArray<spCRegistry> m_lspEvalRPGTemplates;
	CAnimContainerCollector m_AnimCollector;
	TArray<int32> m_liAnimGraph2;
	TArray<int32> m_lItemAnimTypesNeeded;
public:
	CWServer_Mod();

	virtual CRegistry *GetEvaledRPGTemplate(const char *_pName);
	virtual void AddEvaledRPGTemplate(spCRegistry _spReg);

	// Name of container, sequence in container, index in animation list
	virtual void AddAnimContainerEntry(CStr _Name, int32 _iAnimSeq, int32 _iAnim = -1);
	// Register loaded animations with list
	virtual void RegisterAnimGraph2(int32 _iAnimGraph2Res);
	virtual void LoadAnimations();
	virtual void AddAnimTypeItem(int32 _Type);
	//virtual void AddItemAnimType(int32 _Type);
	//virtual void AddItem(CStr _ItemAnimsets, int32 _ItemAnimType);
	//virtual void AddExtraAnimSet(CStr _ItemAnimset, int32 _ExtraSetType);
	virtual const TArray<int32>& GetItemAnimTypesNeeded() const;
	//virtual int32 GetItemAnimTypes() const;

	void mpGetGameMode();
	void Con_DialogueInfo(CStr _Char);
	void Con_ExecuteTestRun();

#ifndef M_RTM
	void Con_GlassWire(int _v);
#endif

	CWorld_DeltaGameState m_GameState;
	
	// for auto-testruns
	bool m_AutoTestRunning : 1;

	void Register(CScriptRegisterContext &_RegContext);

	enum
	{
		SERVERMOD_EXTRANIMSET_TYPE_TRANQ = 1,
	};
};

#endif
