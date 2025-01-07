#ifndef WAGI_h
#define WAGI_h

//--------------------------------------------------------------------------------

#include "WAGI_Context.h"
#include "WAGI_Token.h"

#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"
#include "../../../XR/XRAnimGraph/AnimGraph.h"
#include "../../../XR/XRAnimGraph/AnimList.h"
#include "../WDataRes_AnimGraph.h"
//#include "../WAnimGraphInstance/WAG_ClientData.h"
#include "../WAnimGraphInstance/WAGI_Defs.h"
#include "../WAnimGraphInstance/WAGI_StateInstPacked.h"
#include "../WAnimGraphInstance/WAGI_SIID.h"

#include "../WPhysState.h"
#include "../WObjCore.h"

#ifdef PLATFORM_PS2
# define WAGI_CLIENTUPDATE_DIRECTACCESS
#endif

// Direct access activated for now
//#if !defined(PLATFORM_XBOX) || (defined(PLATFORM_XBOX) && !defined(M_Profile))
//#if defined(PLATFORM_XBOX)
#define WAGI_CLIENTUPDATE_NORMAL 0
#define WAGI_CLIENTUPDATE_DIRECTACCESS 1
//#endif

//--------------------------------------------------------------------------------
template <class T, int S>
class TAGStaticArray
{
public:
	T m_aData[S];
	int32 m_Used;

	TAGStaticArray()
	{
		m_Used = 0;
	}
	
	int Len() const { return m_Used; }
	void SetLen(int _Len)
	{
		M_ASSERT(_Len<S, "TAGStaticArray: Damn it!!!!!!");
		m_Used = _Len;
	}

	int Add(const T &_Element)
	{
		M_ASSERT(m_Used<S, "TAGStaticArray: Damn it");
		m_aData[m_Used++] = _Element;
		return m_Used-1;
	}

	void Del(int _Index)
	{
		//m_aData[_Index] = m_aData[--m_Used];
		M_ASSERT(_Index >= 0 && _Index < m_Used, "TAGStaticArray: Del");

		for(int32 i = _Index; i < m_Used-1; i++)
			m_aData[i] = m_aData[i+1];
		m_Used--;
	}

	const T *GetBasePtr() const { return m_aData; }
	T *GetBasePtr() { return m_aData; }

	const T &operator[](const int _Index) const { return m_aData[_Index]; }
	T &operator[](const int _Index) { return m_aData[_Index]; }
};


class CWAGI_Token_Packed : public CReferenceCount
{
	MRTC_DECLARE;
	public:

		CWAGI_SIID				m_PlayingSIID;
		CMTime					m_RefreshGameTime;
		TArray<CWAGI_SIP>		m_lSIPs;
		TArray<CWAGI_SIID>		m_lRemSIIDs;
		//TAGStaticArray<CWAGI_SIP,64>		m_lSIPs;
		//TAGStaticArray<CWAGI_SIID,64>		m_lRemSIIDs;
		int8					m_ID;
		uint8					m_Flags;
	public:

		CWAGI_Token_Packed() {}

		CWAGI_Token_Packed(uint8 _ID)
		{
			m_ID = _ID;
			m_Flags = 0;
		}

		const CWAGI_SIP* GetMatchingSIP(const CWAGI_SIID* _pSIID) const;
		CWAGI_SIP* GetMatchingSIP(const CWAGI_SIID* _pSIID, bool _bCreateNonExistent);
		void AddRemSIID(const CWAGI_SIID& _RemSIID)
		{
			m_Flags |= AGI_PACKEDTOKEN_REMSIIDS;
			m_lRemSIIDs.Add(_RemSIID);
		}

		bool RemoveMatchingSIP(const CWAGI_SIID* _pRemSIID)
		{
			for (int iSIP = 0; iSIP < m_lSIPs.Len(); iSIP++)
			{
				CWAGI_SIP* pSIP = &(m_lSIPs[iSIP]);
				if (_pRemSIID->GetEnqueueTime().AlmostEqual(pSIP->GetEnqueueTime()) &&
					(_pRemSIID->GetEnterActionIndex() == pSIP->GetEnterActionIndex()))
				{
					m_lSIPs.Del(iSIP);
					return true;
				}
			}
			return false;
		}

		void operator=(const CWAGI_Token_Packed &_CopyFrom)
		{
			m_PlayingSIID = _CopyFrom.m_PlayingSIID;
			m_RefreshGameTime = _CopyFrom.m_RefreshGameTime;
			m_lSIPs = _CopyFrom.m_lSIPs;
			m_lRemSIIDs = _CopyFrom.m_lRemSIIDs;
			m_ID = _CopyFrom.m_ID;
			m_Flags = _CopyFrom.m_Flags;
		}

		int Write(uint8* _pData) const;
		int Read(const uint8* _pData);
		void Write(CCFile* _pFile) const;
		void Read(CCFile* _pFile);

		void Log(CStr _Prefix) const;

};

class CWAGI_EnterStateEntry
{
public:
	CAGStateIndex	m_iState;
	CAGActionIndex	m_iEnterStateAction;
	int8			m_TokenID;
	CWAGI_EnterStateEntry(CAGStateIndex _iState, CAGActionIndex	_iAction, int8 _TokenID)
	{
		m_iState = _iState;
		m_iEnterStateAction = _iAction;
		m_TokenID = _TokenID;
	}
	CWAGI_EnterStateEntry()
	{
		m_iState = -1;
		m_iEnterStateAction = -1;
		m_TokenID = -1;
	}
	void CopyFrom(const CWAGI_EnterStateEntry& _Entry)
	{
		m_iState = _Entry.m_iState;
		m_iEnterStateAction = _Entry.m_iEnterStateAction;
		m_TokenID = _Entry.m_TokenID;
	}
	int Write(uint8* _pData) const
	{
		uint8* pBase = _pData;
		PTR_PUTINT16(_pData, m_iState);
		PTR_PUTINT16(_pData, m_iEnterStateAction);
		PTR_PUTINT8(_pData, m_TokenID);

		return (_pData - pBase);
	}

	int Read(const uint8* _pData)
	{
		const uint8* pBase = _pData;
		PTR_GETINT16(_pData, m_iState);
		PTR_GETINT16(_pData, m_iEnterStateAction);
		PTR_GETINT8(_pData, m_TokenID);
		return (_pData - pBase);
	}

	void Write(CCFile* _pFile) const
	{
		_pFile->WriteLE(m_iState);
		_pFile->WriteLE(m_iEnterStateAction);
		_pFile->WriteLE(m_TokenID);
	}
	void Read(CCFile* _pFile)
	{
		_pFile->ReadLE(m_iState);
		_pFile->ReadLE(m_iEnterStateAction);
		_pFile->ReadLE(m_TokenID);
	}
	
};

//--------------------------------------------------------------------------------

class CWAGI_Packed : public CReferenceCount
{
	public:

		uint32						m_iRandseed;
		int32						m_iAnimGraphRes;
		int32						m_iAnimListRes;
		TArray<CWAGI_Token_Packed>	m_lTokens;
#ifndef WAGI_CLIENTUPDATE_DIRECTACCESS
		TArray<CWAGI_EnterStateEntry>	m_lEnterStateEntries;
#endif
		TArray<int8>				m_lRemTokenIDs;
		//TAGStaticArray<CWAGI_Token_Packed,6>	m_lTokens;
		//TAGStaticArray<int8,6>				m_lRemTokenIDs;

		CMTime						m_OverlayAnim_StartTime;
		int16						m_OverlayAnim_iAnimContainerResource;
		int16						m_OverlayAnim_iAnimSeq;
		int16						m_OverlayAnim_iAnimContainerResourceLipSync;
		int16						m_OverlayAnim_iAnimSeqLipSync;
		int8						m_OverlayAnim_LipSyncBaseJoint;
		uint8						m_Flags;
		

	public:

		CWAGI_Packed()
		{
			m_Flags = 0;
			m_iRandseed = 0;
			m_iAnimGraphRes = 0;
			m_iAnimListRes = 0;

			m_OverlayAnim_iAnimContainerResource = 0;
			m_OverlayAnim_iAnimSeq = -1;
			m_OverlayAnim_iAnimContainerResourceLipSync = 0;
			m_OverlayAnim_iAnimSeqLipSync = -1;
			m_OverlayAnim_LipSyncBaseJoint = 0;
			m_OverlayAnim_StartTime = AGI_UNDEFINEDTIME;
		}

		void CreateDiff(const CWAGI_Packed* _pPAGIRef, CWAGI_Packed* _pPAGIDiff) const;
		void UpdateWithDiff(const CWAGI_Packed* _pPAGIDiff);

		const CWAGI_Token_Packed* GetTokenFromID(int8 _TokenID) const;
		CWAGI_Token_Packed* GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent);

		uint32 GetCurStateFlags(int8 _TokenID, uint8 _iFlags) const;

		int Read(const uint8* _pData);
		int Write(uint8* _pData) const;
		void Read(CCFile* _pFile);
		void Write(CCFile* _pFile) const;

		void Log(CStr _Prefix, CStr _Name) const;
};
//--------------------------------------------------------------------------------

class CWAGI : public CReferenceCount
{
	private:

		uint32						m_iRandseed;

		int32						m_iAnimGraphRes;
		int32						m_iAnimListRes;
		spCXRAG						m_spAnimGraph;
		spCXRAG_AnimList			m_spAnimList;
		int							m_nAnimGraphRefs;
		int							m_nAnimListRefs;

		TArray<CWAGI_Token>			m_lTokens;

		TArray<CWAGI_EnterStateEntry>	m_lEnterStateEntries;

		CXRAG_Animation				m_OverlayAnim;
		// Lipsync overlay animation, assume it starts at the same time as the other one
		CXRAG_Animation				m_OverlayAnimLipSync;
		CMTime						m_OverlayAnim_StartTime;
		int8						m_OverLayAnimLipSyncBaseJoint;
		bool						m_bOverlayAnimFirstVelocityRequest;
		bool						m_bDisableAll;
		bool						m_bForceRefresh;

		class CWO_ClientData_AnimGraphInterface* m_pEvaluator;

		friend class CWAGI_Token;
		friend class CWAGI_StateInstance;
		friend class CWAGI_SIQ;

	public:

		CWAGI() { Clear(); }
		CWAGI(const CWAGI& _Copy) { CopyFrom(&_Copy); }

		void SetEvaluator(CWO_ClientData_AnimGraphInterface* _pEvaluator) { m_pEvaluator = _pEvaluator; }

		void Log(CStr _Prefix, CStr _Name) const;

		uint32 GetRandseed() const { return m_iRandseed; }

		uint8 GetNumTokens() { return m_lTokens.Len(); }
		const CWAGI_Token* GetToken(uint8 _iToken) { return &(m_lTokens[_iToken]); }

		const CWAGI_Token* GetTokenFromID(int8 _TokenID) const;
		CWAGI_Token* GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent = false);
		
		// Overloadable, but superclass functions must be called from within overloading functions.
		void Clear();
		void CopyFrom(const CWAGI* _pAnimGraphInstance);

		bool AcquireAllResources(const CWAGI_Context* _pContext);
		void UnacquireAllResources();

		void TagAnimSetHelper(const CXRAG_Action* _pAction, int32 _iEffect);
		void TagAnimSetFromName(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const CStr& _SetName);
		void TagAnimSetFromDialog(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _Type, const TArray<int32>& _lDialog);
		void TagAnimSetFromBehavior(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TThinArray<int32>& _lBehavior);
		void TagAnimSetFromVigilance(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _WeaponClass, const TThinArray<int32>& _lVigilance);
		void TagAnimSetFromActionCutscene(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<int32>& _liACS);
		void TagAnimSetFromItems(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _ItemClass1, int32 _ItemClass2);
		void TagAnimSetFromTargetState(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, CStr _StateAction);
		void TagAnimSetFromLadderStepOffType(const CWAGI_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, int32 _StepOffType);


		// Tag entire animation set from containername (ie MovBas)
		//void TagAnimSet(const CWAGI_Context* _pContext, CStr _ContainerName);
		// Tag single animation from containername and index (ie MovBas:index)
		//void TagAnim(const CWAGI_Context* _pContext, CStr _ContainerName, int32 _iResource);
		// Tag animation in animlist (direct indexed)
		//void TagAnim(const CWAGI_Context* _pContext, int32 _iAnim);

		// Resources
//		void SetWorldData(spCWorldData _spWData; ) { m_spWData = _spWData; }
//		CWorldData* GetWorldData() { return m_spWData; }
		void SetResourceIndex_AnimGraph(int32 _iAnimGraphRes);
		void SetResourceIndex_AnimList(int32 _iAnimListRes);
		int32 GetResourceIndex_AnimGraph() { return m_iAnimGraphRes; };
		int32 GetResourceIndex_AnimList() { return m_iAnimListRes; };
		void SetOverlayAnim(int32 _iAnimContainerResource, int16 _iAnimSeq, CMTime _StartTime);
		void SetOverlayAnimLipSync(int32 _iAnimContainerResource, int16 _iAnimSeq, int8 _BaseJoint);
		void ClearOverlayAnim();
		bool HasOverlayAnim() { return m_OverlayAnim.IsValid(); }
		void DisableAll() { ClearTokens(); m_bDisableAll = true; }
		bool GetDisableAll() { return m_bDisableAll; }
		void ClearTokens() { m_lTokens.Clear(); }

		void Refresh(const CWAGI_Context* _pContext);
		void RefreshPredictionMisses(const CWAGI_Context* _pContext);
		void GetAnimLayers(const CWAGI_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg);
		bool GetAnimVelocity(const CWAGI_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg);
		bool GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _pLayer, int32 _iToken, int32 _iAnim, int32 _StartTick) const;
		// If we only need the rotational velocity
		bool GetAnimOffset(const CWAGI_Context* _pContext, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset);
		bool GetAnimRelOffset(const CWAGI_Context* _pContext, CVec3Dfp32 _MoveOffset, CQuatfp32 _RotOffset, CVec3Dfp32& _MoveRelOffset, CQuatfp32& _RotRelOffset);
		bool GetAnimRotVelocity(const CWAGI_Context* _pContext, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg);
		bool HasAnimVelocity(const CWAGI_Context* _pContext, int _iDisableStateInstanceAnimsCallbackMsg);
		void CheckAnimEvents(const CWAGI_Context* _pContext, int _iCallbackMessage);
		// 
		void FindBreakoutPoints(const CWAGI_Context* _pContext, int16 _iState, CXR_Anim_BreakoutPoints& _Points, fp32 _Offset);
		void FindEntryPoints(const CWAGI_Context* _pContext, int16 _iState, CXR_Anim_EntryPoints& _Points);
		//
		void FindBreakoutSequence(const CWAGI_Context* _pContext, int16 _iState, fp32 _Offset, int16& _iSequence, CMTime& _AnimTime);
		bool FindEntryPoint(const CWAGI_Context* _pContext, int16 _iState, int16 _iPrevSequence, CMTime _PrevTime, fp32& _Offset);

		// Find absolute distance for a certain offset in an animation, animation is found through given moveaction
		// No the most beautiful or useful functions, but they work
		bool GetAbsoluteAnimOffset(const CWAGI_Context* _pContext, int32 _iAnim, fp32 _TimeOffset, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset);
		CAGAnimIndex GetAnimFromAction(const CWAGI_Context* _pContext, CStr& _MoveAction, fp32& _TimeOffset) const;
		bool GetAnimFromFirstActionInState(const CWAGI_Context* _pContext, CAGAnimIndex _iAnim, int32 _iToken, CAGAnimIndex& _iTargetAnim, CAGActionIndex& _iTargetAction) const;

virtual int OnCreateClientUpdate(uint8* _pData, const CWAGI_Packed* _pPAGI, int8 _Type) const;
static	int OnClientUpdate(CWAGI_Context* _pContext, const uint8* _pData, CWAGI_Packed* _pPAGI, CWAGI* _pAGI);
		void SyncWithPAGI(CWAGI_Context* _pContext, const CWAGI_Packed* _pClientPAGI);
		void PredictionMiss_AddToken(CWAGI_Context* _pContext, int8 _TokenID);
		void PredictionMiss_RemoveToken(CWAGI_Context* _pContext, int8 _TokenID);

	public:

		uint32 GetTokenStateFlags(const CWAGI_Context *_pContext, int8 _TokenID, uint8 _iFlags) const;
		uint32 GetStateFlags(const CXRAG_State* _pState, uint8 _iFlags) const;
		bool GetTokenStateConstantValue(const CWAGI_Context *_pContext, int8 _TokenID, uint16 _StateConstantID, fp32& _Value) const;
		fp32 GetTokenStateConstantValueDef(const CWAGI_Context *_pContext, int8 _TokenID, uint16 _StateConstantID, fp32 _DefaultValue) const;
		bool GetStateConstantValue(CAGStateIndex _iState, uint16 _StateConstantID, fp32& _Value) const;
		fp32 GetStateConstantValueDef(CAGStateIndex _iState, uint16 _StateConstantID, fp32 _DefaultValue) const;
		bool GetStateConstantValue(const CXRAG_State* _pState, uint16 _StateConstantID, fp32& _Value) const;
		fp32 GetStateConstantValueDef(const CXRAG_State* _pState, uint16 _StateConstantID, fp32 _DefaultValue) const;

		void ForceRefresh() { m_bForceRefresh = true; }


		// Diff from Templar, apparently needed in "OnDeltaSave"
		void CreatePackedState(CWAGI_Packed* _pPackedAGIState) const;
	private:

static	int PackDiffSIPs(TArray<CWAGI_SIP>& _lDiffSIPs, uint8* _pData);
static	int UnpackDiffSIPs(TArray<CWAGI_SIP>& _lDiffSIPs, const uint8* _pData);
		void UpdateStateInstance(CWAGI_StateInstance_Packed* _pSIP);

	private:

		bool AcquireResource_AnimGraph(const CWAGI_Context* _pContext);
		bool AcquireResource_AnimList(const CWAGI_Context* _pContext);

		bool EvaluateCondition(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, const CXRAG_ConditionNode* _pNode, fp32& _TimeFraction);
		void InvokeEffects(const CWAGI_Context* _pContext, int16 _iBaseEffectInstance, uint8 _nEffects);

		void CheckAnimEvents(const CWAGI_Context* _pContext, CXR_Anim_SequenceData* _pAnimSeqData, CMTime _StartTime, CMTime _EndTime, int _iCallbackMessage) const;

	public:

		// FIXME: Provide functionality of a 'PerformAction' intead.
		void DoActionEffects(const CWAGI_Context* _pContext, int16 _iAction);
		void MoveAction(const CWAGI_Context* _pContext, int8 _ActionTokenID, int16 _iAction, fp32 _ForceOffset = 0.0f, int32 _MaxQueued = 0);

		// FIXME: Abstract action indices completely. Force usage of HashKeys or Names instead.
		int16 GetActionIndexFromHashKey(uint32 _ActionHashKey) const { return m_spAnimGraph->GetActionIndexFromHashKey(_ActionHashKey); }

		void ClearAnimListCache();

	public:

		const CXRAG_ConditionNode* GetNode(int16 _iNode) const { return m_spAnimGraph->GetNode(_iNode); }
		const CXRAG_StateConstant* GetStateConstant(int16 _iStateConstant) const { return m_spAnimGraph->GetStateConstant(_iStateConstant); }
		const CXRAG_Action* GetAction(int16 _iAction) const { return m_spAnimGraph->GetAction(_iAction); }
		const CXRAG_State* GetState(int16 _iState) const { return m_spAnimGraph->GetState(_iState); }
		const CXRAG_AnimLayer* GetAnimLayer(int16 _iAnimLayer) const { return m_spAnimGraph->GetAnimLayer(_iAnimLayer); }
		CXRAG_AnimList* GetAnimList()
		{
			if (m_spAnimList == NULL)
				return NULL;

			return ((CXRAG_AnimList*)m_spAnimList);
		}
#ifndef M_RTM
		const CStr GetExportedActionName(int _iExportedActionName) const { return m_spAnimGraph->GetExportedActionName(_iExportedActionName); }
		const CStr GetStateName(int _iState) const { return m_spAnimGraph->GetExportedStateName(_iState); }
#endif
#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		void StartLoggingAnimMisses() { if (m_spAnimList) m_spAnimList->StartLogging(); }
#endif

		void AddEnterStateEntry(const CWAGI_EnterStateEntry& _OnEnterStateEntry);
		void ClientConsumeEnterStateEntries(CWAGI_Context* _pContext);
		void ServerClearEnterStateEntries();

	public:

#ifdef M_RTM
		M_INLINE static bool DebugEnabled(const CWAGI_Context* _pContext) {return false;}
#else
		static bool DebugEnabled(const CWAGI_Context* _pContext)

		{
			if(_pContext->m_pWPhysState && _pContext->m_pObj)
			{
				uint32 AGIDebugFlags = 0;

				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
				if (pReg != NULL)
					AGIDebugFlags = pReg->GetValuei("AGI_DEBUG_FLAGS");

				bool bDebugServer = (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
				bool bDebugClient = (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
				bool bDebugPlayer = (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
				bool bDebugNonPlayer = (AGIDebugFlags & AGI_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0;

				bool bIsPlayer = ((_pContext->m_pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
				if (((_pContext->m_pWPhysState->IsServer() && bDebugServer) ||
					(_pContext->m_pWPhysState->IsClient() && bDebugClient)) &&
					((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
					return true;
			}

			return false;
		}

		//Get debug info about current state for all tokens
		CStr DebugStateInfo(const CWAGI_Context* _pContext);
#endif
};
typedef TPtr<CWAGI> spCWAGI;
//--------------------------------------------------------------------------------

#define AGIResScope(pAGI, pWPhysState, FailAction)				\
	CWAGI_ResourceScope __AGIResScope(pAGI);					\
	if (!__AGIResScope.Acquire(pWPhysState)) {FailAction;}		\

class CWAGI_ResourceScope
{
private:

	CWAGI* m_pAGI;

public:

	CWAGI_ResourceScope(const CWAGI* _pAGI)
	{
		m_pAGI = const_cast<CWAGI*>(_pAGI);
	}

	bool Acquire(const CWAGI_Context* _pContext)
	{
		return (m_pAGI->AcquireAllResources(_pContext));
	}

	~CWAGI_ResourceScope()
	{
		m_pAGI->UnacquireAllResources();
	}
};

//--------------------------------------------------------------------------------

#endif /* WAGI_h */
