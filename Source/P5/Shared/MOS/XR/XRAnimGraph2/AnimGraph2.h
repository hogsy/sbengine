#ifndef AnimGraph2_h
#define AnimGraph2_h

//--------------------------------------------------------------------------------

#include "AnimGraph2Defs.h"
#include "AnimGraph2_Action.h"
#include "AnimGraph2_Reaction.h"
#include "AnimGraph2_AnimLayer.h"
#include "AnimGraph2_State.h"
#include "AnimGraph2_GraphBlock.h"
#include "../../Classes/GameWorld/WMapData.h"

//--------------------------------------------------------------------------------

#define Mask(nBits)								((1 << (nBits)) - 1)
#define ShiftedMask(Shift, nBits)				(Mask(nBits) << (Shift))
#define ShiftedValue(Shift, nBits, Value)		(((Value) & Mask(nBits)) << (Shift))
#define SetBits(Storage, Shift, nBits, Value)	((Storage) = ((Storage) & ~ShiftedMask(Shift, nBits)) | ShiftedValue(Shift, nBits, Value))
#define GetBits(Storage, Shift, nBits)			(((Storage) >> (Shift)) & Mask(nBits))

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG2_Animation
{
public:
	int16		m_iAnimContainerResource;
	int16		m_iAnimSeq; // Withing container.
	CXR_Anim_SequenceData* m_pSeq;
	uint8		m_Flags;
	// Flag field for tracking (debug)
	//	#ifdef WAGI_RESOURCEMANAGEMENT_LOG
	enum
	{
		XRAG2_TAGLOADED					= 1 << 0,
		XRAG2_TAGUSEDWHENNOTLOADED		= 1 << 1,
		XRAG2_OVERLAYREMOVEWHENFINISHED	= 1 << 2,
	};
	//	#endif


	CXRAG2_Animation() { Clear(); }

	CXRAG2_Animation(int16 _iAnimContainerResource, int16 _iAnimSeq)
	{
		m_iAnimContainerResource = _iAnimContainerResource;
		m_iAnimSeq = _iAnimSeq;
		m_pSeq = NULL;
		m_Flags = 0;
	}

	void Clear()
	{
		m_iAnimContainerResource = 0;
		m_iAnimSeq = -1;
		m_pSeq = NULL;
		//#ifdef WAGI_RESOURCEMANAGEMENT_LOG
		m_Flags = 0;
		//#endif
	}

	void ClearCache()
	{
		m_pSeq = NULL;
	}

	bool IsValid() const
	{
		return ((m_iAnimContainerResource != 0) && (m_iAnimSeq >= 0));
	}

	spCXR_Anim_SequenceData GetAnimSequenceData(CWorldData* _pWData) const
	{
		if (m_pSeq)
			return m_pSeq;

		if (!IsValid())
			return NULL;

		spCWResource spAnimContainerResource = _pWData->GetResourceRef(m_iAnimContainerResource);
		if (spAnimContainerResource == NULL || spAnimContainerResource->GetClass() != WRESOURCE_CLASS_XSA)
			return NULL;

		CWRes_Anim* pAnimRes = (CWRes_Anim*)(const CWResource*)spAnimContainerResource;
		pAnimRes->m_TouchTime = _pWData->m_TouchTime; // Added by Anton

		CXR_Anim_Base* pAnimBase = pAnimRes->GetAnim();
		if (pAnimBase == NULL)
			return NULL;

		const_cast<CXRAG2_Animation*>(this)->m_pSeq = (CXR_Anim_SequenceData*)pAnimBase->GetSequence(m_iAnimSeq);

		return m_pSeq;
	}

	M_INLINE spCXR_Anim_SequenceData GetAnimSequenceData_MapData(CMapData* _pWData) const { return GetAnimSequenceData(_pWData->m_spWData);	}
};

class CXRAG2_ICallbackParams
{
private:

	const int16* m_pParams;
	uint8		m_nParams;
	CMTime		*m_pRetTime;

public:

	CXRAG2_ICallbackParams()
	{
		m_pParams = NULL;
		m_nParams = 0;
	}

	CXRAG2_ICallbackParams(const CXRAG2_ICallbackParams& _IParams)
	{
		m_pParams = _IParams.m_pParams;
		m_nParams = _IParams.m_nParams;
	}

	CXRAG2_ICallbackParams(const int16* _pParams, uint8 _nParams)
	{
		m_pParams = _pParams;
		m_nParams = _nParams;
	}

	uint8 GetNumParams() const
	{
		if (m_pParams == NULL)
			return 0;

		return m_nParams;
	}

	int16 GetParam(uint8 _iParam) const
	{
		// Unsigned variable cannot be < 0
		if ((_iParam >= m_nParams))
			return 0;

		if (m_pParams == NULL)
			return 0;

		return m_pParams[_iParam];
	}

	CStr GetStr() const
	{
		if (m_pParams == NULL)
			return "";

		if (m_nParams == 0)
			return "";

		CStr Result = CStrF("%d", m_pParams[0]);
		for (int iParam = 1; iParam < m_nParams; iParam++)
			Result += " " + CStrF("%d", m_pParams[iParam]);
		return Result;
	}

};

//--------------------------------------------------------------------------------

class CXRAG2_EffectInstance
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	CAG2CallbackParamIndex	m_iParams; // 2
	CAG2EffectID				m_ID; // 1
	uint8					m_nParams;


	/*
	// FIXME:

	Use State->iParamBase and compress into 16bit (making a total of 32bits instead of 64bits).
	Mayby State iBase is needed. Restrict n total effect params to 16k would suffice.
	Make nice interface for iParams and nParams.

	0 - 16383	(14bit) iPropertyParam
	0 - 3		(2bit) nPropertyParams
	*/
	CXRAG2_EffectInstance()
	{
		m_ID = AG2_EFFECTID_NULL;
		m_iParams = AG2_CALLBACKPARAMINDEX_NULL;
		m_nParams = 0;
	}

	CXRAG2_EffectInstance(CAG2EffectID _ID, CAG2CallbackParamIndex _iParams, uint8 _nParams)
	{
		m_ID = _ID;
		m_iParams = _iParams;
		m_nParams = _nParams;
	}

public:

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

//--------------------------------------------------------------------------------

class CXRAG2_StateConstant
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	fp32					m_Value;
	CAG2StateConstantID	m_ID;

	CXRAG2_StateConstant()
	{
		m_ID = 0;
		m_Value = 0;
	}

	CXRAG2_StateConstant(CAG2StateConstantID _ID, fp32 _Value)
	{
		m_ID = _ID;
		m_Value = _Value;
	}

public:

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

//--------------------------------------------------------------------------------

class CXRAG2_ConditionNode
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	/*
	Ranges:
	0 - 255 (8bit) iProperty
	0 - 31	(5bit) iPropertyParam
	0 - 3	(2bit) nPropertyParams
	0 - 7	(3bit) iOperator
	0 - 63	(7bit) TrueAction (+EndParse)
	0 - 63	(7bit) FalseAction (+EndParse)

	NodeAction = EndParse bit + iNode | iTargetState
	iNode = 0 - 63 (6 bit)
	iAction = 0 - 63 (6 bit)

	Features/Limitations:
	32 unique property parameter touples per state (similar touples are shared, void touples are free)
	4 property parameters per touple
	8 operators per character class
	64 nodes per state
	64 actions per state

	fp32 GetPropertyParam(uint8 _iPropertyParam)
	{
	int16 jPropertyParam = pState->GetPropertyParamsBaseIndex() + iPropertyParam;
	fp32 Param = pAG->GetPropertyParam(jPropertyParam);
	return Param;
	}

	*/

	// [ 8bit Property + 5bit iPropertyParams + 2bit nPropertyParams + 3bit Operator + 7bit TrueAction + 7bit FalseAction ]
#define AG2_CNODE_PROPERTY_BITS						8
#define AG2_CNODE_PROPERTYPARAMSINDEX_BITS			5
#define AG2_CNODE_NUMPROPERTYPARAMS_BITS				2
#define AG2_CNODE_OPERATOR_BITS						3
#define AG2_CNODE_TRUEACTION_BITS					7
#define AG2_CNODE_FALSEACTION_BITS					7

#define AG2_CNODE_PROPERTY_SHIFT						0
#define AG2_CNODE_PROPERTYPARAMSINDEX_SHIFT			(AG2_CNODE_PROPERTY_SHIFT + AG2_CNODE_PROPERTY_BITS)
#define AG2_CNODE_NUMPROPERTYPARAMS_SHIFT			(AG2_CNODE_PROPERTYPARAMSINDEX_SHIFT + AG2_CNODE_PROPERTYPARAMSINDEX_BITS)
#define AG2_CNODE_OPERATOR_SHIFT						(AG2_CNODE_NUMPROPERTYPARAMS_SHIFT + AG2_CNODE_NUMPROPERTYPARAMS_BITS)
#define AG2_CNODE_TRUEACTION_SHIFT					(AG2_CNODE_OPERATOR_SHIFT + AG2_CNODE_OPERATOR_BITS)
#define AG2_CNODE_FALSEACTION_SHIFT					(AG2_CNODE_TRUEACTION_SHIFT + AG2_CNODE_TRUEACTION_BITS)

	M_BITFIELD6(uint32,
		m_Property, AG2_CNODE_PROPERTY_BITS,
		m_PropertyParamsIndex, AG2_CNODE_PROPERTYPARAMSINDEX_BITS,
		m_NumPropertyParams, AG2_CNODE_NUMPROPERTYPARAMS_BITS,
		m_Operator, AG2_CNODE_OPERATOR_BITS,
		m_TrueAction, AG2_CNODE_TRUEACTION_BITS,
		m_FalseAction, AG2_CNODE_FALSEACTION_BITS) m_Data;
	fp32		m_Constant; // [ 32bit Constant ]

	CAG2PropertyID	GetProperty() const { return m_Data.m_Property; }
	CAG2OperatorID	GetOperator() const { return m_Data.m_Operator; }
	uint8	GetPropertyParamsIndex() const { return m_Data.m_PropertyParamsIndex; }
	uint8	GetNumPropertyParams() const { return m_Data.m_NumPropertyParams; }
	uint8	GetTrueAction() const { return m_Data.m_TrueAction; }
	uint8	GetFalseAction() const { return m_Data.m_FalseAction; }
	fp32		GetConstant() const { return m_Constant; }

	void	SetProperty(CAG2PropertyID _PropertyID) { m_Data.m_Property = _PropertyID; }
	void	SetPropertyParamsIndex(uint8 _iPropertyParams) { m_Data.m_PropertyParamsIndex = _iPropertyParams; }
	void	SetNumPropertyParams(uint8 _nPropertyParams) { m_Data.m_NumPropertyParams = _nPropertyParams; }
	void	SetOperator(CAG2OperatorID _OperatorID) { m_Data.m_Operator = _OperatorID; }
	void	SetTrueAction(uint8 _TrueAction) { m_Data.m_TrueAction = _TrueAction; }
	void	SetFalseAction(uint8 _FalseAction) { m_Data.m_FalseAction = _FalseAction; }
	void	SetConstant(fp32 _Constant) { m_Constant = _Constant; };

	bool operator==(CXRAG2_ConditionNode& _Node)
	{
		return (m_Data.m_BitUnion == _Node.m_Data.m_BitUnion);
	}

public:

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

// Almost the same as above but without property params, and type of property instead + 
// increased number of operators
class CXRAG2_ConditionNodeV2
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	/*
	Ranges:
	0 - 255 (8bit) iProperty
	0 - 7   (3bit) PropertyType   (float/int/bool/unknown)
	0 - 15	(4bit) iOperator
	0 - 63	(7bit) TrueAction (+EndParse)
	0 - 63	(7bit) FalseAction (+EndParse)

	// Ok we got 4 bits to spare if something is needed

	NodeAction = EndParse bit + iNode | iTargetState
	iNode = 0 - 63 (6 bit)
	iAction = 0 - 63 (6 bit)

	Features/Limitations:
	32 unique property parameter touples per state (similar touples are shared, void touples are free)
	4 property types (what type of property, ie which property array to look in)
	16 operators per character class
	64 nodes per state	  (should add 1 extra vit to this
	64 actions per state  (should add 1 extra bit to this)
	*/

	// [ 8bit Property + 5bit iPropertyParams + 2bit nPropertyParams + 3bit Operator + 7bit TrueAction + 7bit FalseAction ]
#define AG2_CNODEV2_PROPERTY_BITS						8
#define AG2_CNODEV2_PROPERTYTYPE_BITS					3
#define AG2_CNODEV2_OPERATOR_BITS						4
#define AG2_CNODEV2_TRUEACTION_BITS						7
#define AG2_CNODEV2_FALSEACTION_BITS					7

/*#define AG2_CNODE_PROPERTY_SHIFT						0
#define AG2_CNODE_OPERATOR_SHIFT						(AG2_CNODE_NUMPROPERTYPARAMS_SHIFT + AG2_CNODE_NUMPROPERTYPARAMS_BITS)
#define AG2_CNODE_TRUEACTION_SHIFT					(AG2_CNODE_OPERATOR_SHIFT + AG2_CNODE_OPERATOR_BITS)
#define AG2_CNODE_FALSEACTION_SHIFT					(AG2_CNODE_TRUEACTION_SHIFT + AG2_CNODE_TRUEACTION_BITS)*/

	M_BITFIELD5(uint32, m_Property, AG2_CNODEV2_PROPERTY_BITS, m_PropertyType, AG2_CNODEV2_PROPERTYTYPE_BITS, m_Operator, AG2_CNODEV2_OPERATOR_BITS, m_TrueAction, AG2_CNODEV2_TRUEACTION_BITS, m_FalseAction, AG2_CNODEV2_FALSEACTION_BITS) m_Data;

	// Depending on what type of property we have, the constants will be interpreted differently
	union
	{
		fp32		m_ConstantFloat;	// [ 32bit Float Constant ]
		int32	m_ConstantInt;		// [ 32bit Int Constant ]
		int32	m_ConstantBool;		// [ 32bit Bool Constant ] (bool won't work well on ps3/360 it seems...)
	};

	M_INLINE CAG2PropertyID	GetProperty() const { return m_Data.m_Property; }
	M_INLINE CAG2OperatorID	GetOperator() const { return m_Data.m_Operator; }
	M_INLINE uint8	GetPropertyType() const { return m_Data.m_PropertyType; }
	M_INLINE uint8	GetTrueAction() const { return m_Data.m_TrueAction; }
	M_INLINE uint8	GetFalseAction() const { return m_Data.m_FalseAction; }
	M_INLINE fp32		GetConstantFloat() const { return m_ConstantFloat; }
	M_INLINE int32	GetConstantInt() const { return m_ConstantInt; }
	M_INLINE int32	GetConstantBool() const { return m_ConstantBool; }

	M_INLINE void	SetProperty(CAG2PropertyID _PropertyID) { m_Data.m_Property = _PropertyID; }
	M_INLINE void	SetPropertyType(uint8 _PropertyType) { m_Data.m_PropertyType = _PropertyType; }
	M_INLINE void	SetOperator(CAG2OperatorID _OperatorID) { m_Data.m_Operator = _OperatorID; }
	M_INLINE void	SetTrueAction(uint8 _TrueAction) { m_Data.m_TrueAction = _TrueAction; }
	M_INLINE void	SetFalseAction(uint8 _FalseAction) { m_Data.m_FalseAction = _FalseAction; }
	M_INLINE void	SetConstantFloat(fp32 _Constant) { m_ConstantFloat = _Constant; };
	M_INLINE void	SetConstantInt(int32 _Constant) { m_ConstantInt = _Constant; };
	M_INLINE void	SetConstantBool(bool _Constant) { m_ConstantBool = _Constant; };

	bool operator==(CXRAG2_ConditionNodeV2& _Node)
	{
		return (m_Data.m_BitUnion == _Node.m_Data.m_BitUnion);
	}

public:

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------

class CXRAG2_ActionHashEntry
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	uint32			m_HashKey;
	CAG2ActionIndex	m_iAction; // 2

public:

	CXRAG2_ActionHashEntry() { Clear(); }

	void Clear()
	{
		m_HashKey = 0;
		m_iAction = AG2_ACTIONINDEX_NULL;
	}

	CXRAG2_ActionHashEntry(uint32 _HashKey, CAG2ActionIndex _iAction)
	{
		m_HashKey = _HashKey;
		m_iAction = _iAction;
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

//--------------------------------------------------------------------------------

class CXRAG2_NameAndID
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	CFStr	m_Name;
	uint8	m_ID;

public:

	CXRAG2_NameAndID() { Clear(); }

	void Clear()
	{
		m_Name = "";
		m_ID = ~0;
	}

	CXRAG2_NameAndID(CStr _Name, uint8 _ID)
	{
		m_Name = _Name;
		m_ID = _ID;
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

class CXRAG2_NameAndValue
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	CFStr	m_Name;
	int32	m_Value;

public:

	CXRAG2_NameAndValue() { Clear(); }

	void Clear()
	{
		m_Name = "";
		m_Value = -1;
	}

	CXRAG2_NameAndValue(CStr _Name, int32 _Value)
	{
		m_Name = _Name;
		m_Value = _Value;
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

};

class CXRAG2_AnimNames
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
	
	
	// Sequence and resource are not saved, created when loading/needed
	mutable CXR_Anim_SequenceData* m_pSeq;
	mutable int16		m_iAnimContainerResource;

	int16	m_iAnimSeq;
	int16	m_iContainerName;

	CXRAG2_AnimNames()
	{
		m_pSeq = NULL;
		m_iAnimContainerResource = 0;
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

class CXRAG2_AnimContainerNames
{
public:
	enum
	{
		EIOWholeStruct = 0
	};
	TThinArray<CStr> m_lNames;
	CStr GetContainerName(int32 _iContainerName) const
	{
		if (m_lNames.ValidPos(_iContainerName))
			return m_lNames[_iContainerName];
		return CStr("");
	}
	void Clear()
	{
		m_lNames.Clear();
	}

	void Read(CCFile* _pFile, int _Ver);
	void Write(CCFile* _pFile);
};
//--------------------------------------------------------------------------------

typedef class CXRAG2 CXR_AnimGraph2;
typedef TPtr<class CXRAG2> spCXR_AnimGraph2;
typedef TPtr<class CXRAG2> spCXRAG2;
class CXRAG2 : public CReferenceCount
{
private:
public:

	CFStr									m_Name;

	TThinArray<CXRAG2_GraphBlock>			m_lGraphBlocks;

	TThinArray<CXRAG2_State>				m_lFullStates;
	TThinArray<CXRAG2_SwitchState>			m_lSwitchStates;

	TThinArray<CXRAG2_AnimLayer>			m_lFullAnimLayers;
	// Should perhaps be renamed to animations (FIXME)
	TThinArray<CXRAG2_AnimNames>			m_lAnimNames;
	CXRAG2_AnimContainerNames				m_AnimContainerNames;

	TThinArray<CXRAG2_Action>				m_lFullActions;
	TThinArray<CXRAG2_Reaction>				m_lFullReactions;
	TThinArray<CXRAG2_SwitchStateActionVal> m_lSwitchStateActionVals;

	TThinArray<CXRAG2_MoveToken>			m_lMoveTokens;
	TThinArray<CXRAG2_MoveAnimGraph>		m_lMoveAnimGraphs;
	//TThinArray<CXRAG2_ConditionNode>		m_lNodes;
	TThinArray<CXRAG2_ConditionNodeV2>		m_lNodesV2;
	TThinArray<CXRAG2_EffectInstance>		m_lEffectInstances;
	TThinArray<CXRAG2_StateConstant>		m_lStateConstants;
	TThinArray<int16>						m_lCallbackParams;

	TThinArray<CXRAG2_ActionHashEntry>		m_lActionHashEntries;

#ifndef M_RTM
	TThinArray<CFStr>						m_lExportedGraphBlockNames;
	TThinArray<CFStr>						m_lExportedActionNames;
	TThinArray<CFStr>						m_lExportedReactionNames;
	TThinArray<CFStr>						m_lExportedStateNames;
	TThinArray<CFStr>						m_lExportedSwitchStateNames;
	TThinArray<CFStr>						m_lExportedAnimGraphNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedPropertyConditionNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedPropertyFunctionNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedPropertyFloatNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedPropertyIntNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedPropertyBoolNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedOperatorNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedEffectNames;
	TThinArray<CXRAG2_NameAndID>				m_lExportedStateConstantNames;
	TThinArray<CXRAG2_NameAndValue>				m_lExportedImpulseTypeNames;
	TThinArray<CXRAG2_NameAndValue>				m_lExportedImpulseValueNames;
#endif
	//TThinArray<CXRAG2_StateFlag>			m_lStateFlags; <CFStr, uint32>

public:

	CStr GetName() { return m_Name; }

	void Clear();

	// Set anim container resources
	void SetAnimContainerResource(const CStr& _ContainerName, int32 _iContainerRes);
	const CXR_Anim_SequenceData* GetAnimSequenceData(class CWorldData* _pWData, int32 _iAnim) const;
	void ClearAnimSequenceCache();
	void LoadAllContainers(class CMapData *_pMapData);


	// Always supported
	int16 GetNumGraphBlocks() const { return m_lGraphBlocks.Len(); }
	int16 GetNumStates() const { return m_lFullStates.Len(); }
	int16 GetNumAnimLayers() const { return m_lFullAnimLayers.Len(); }
	int16 GetNumAnimNames() const { return m_lAnimNames.Len(); }
	int16 GetNumNodes() const { return m_lNodesV2.Len(); }
	int16 GetNumActions() const { return m_lFullActions.Len(); }
	int16 GetNumSwitchStateActionVals() const { return m_lSwitchStateActionVals.Len(); }
	int16 GetNumSwitchStates() const { return m_lSwitchStates.Len(); }
	int16 GetNumMoveTokens() const { return m_lMoveTokens.Len(); }
	int16 GetNumMoveAnimGraphs() const { return m_lMoveAnimGraphs.Len(); }
	int16 GetNumReactions() const { return m_lFullReactions.Len(); }
	int16 GetNumEffectInstances() const { return m_lEffectInstances.Len(); }
	int16 GetNumStateConstants() const { return m_lStateConstants.Len(); }
	int16 GetNumCallbackParams() const { return m_lCallbackParams.Len(); }
	int16 GetNumActionHashEntries() const { return m_lActionHashEntries.Len(); }

	CStr GetAnimContainerName(int32 _iContainer) const;
	const CXRAG2_AnimNames* GetAnimName(CAG2AnimLayerIndex _iAnimLayer) const;
	const CXRAG2_GraphBlock* GetGraphBlock(CAG2StateIndex _iState) const;
	const CXRAG2_State* GetState(CAG2StateIndex _iState) const;
	const CXRAG2_SwitchState* GetSwitchState(CAG2StateIndex _iState) const;
	const CXRAG2_AnimLayer* GetAnimLayer(CAG2AnimLayerIndex _iAnimLayer) const;
	const CXRAG2_ConditionNodeV2* GetNode(CAG2NodeIndex _iNode) const;
	const CXRAG2_Action* GetAction(CAG2ActionIndex _iAction) const;
	const CXRAG2_SwitchStateActionVal* GetSwitchStateActionVal(CAG2ActionIndex _iAction) const;
	const CXRAG2_MoveToken* GetMoveToken(CAG2MoveTokenIndex _iMoveToken) const;
	const CXRAG2_MoveAnimGraph* GetMoveAnimGraph(CAG2MoveAnimgraphIndex _iMoveAG) const;
	const CXRAG2_Reaction* GetReaction(CAG2ReactionIndex _iReaction) const;
	const CXRAG2_EffectInstance* GetEffectInstance(CAG2EffectInstanceIndex _iEffectInstance) const;
	const CXRAG2_StateConstant* GetStateConstant(CAG2StateConstantIndex _iStateConstant) const;
	CXRAG2_ICallbackParams GetICallbackParams(CAG2CallbackParamIndex _iParams, uint8 _nParams) const;
	CAG2ActionIndex GetActionIndexFromHashKey(uint32 _ActionHashKey) const;

	// Find matching reaction within the graphblock
	CAG2ReactionIndex GetMatchingReaction(int16 _iBlock, CXRAG2_Impulse _Impulse) const;
	CAG2MoveTokenIndex GetMatchingMoveTokenInt(int16 _iSwitchState, int _Val) const;
	// Find a graphblock that matches the impulse
	CAG2GraphBlockIndex GetMatchingGraphBlock(CXRAG2_Impulse _Impulse) const;

	void GetGraphBlockConstants(CXRAG2_Impulse _Impulse, TArray<CXRAG2_StateConstant>& _lConstants) const;
	void GetGraphBlockConstants(const CXRAG2_GraphBlock* _pBlock, TArray<CXRAG2_StateConstant>& _lConstants) const;

#ifndef M_RTM
	// Exported (Debug)
	int16 GetNumExportedActionNames() const { return m_lExportedActionNames.Len(); }
	int16 GetNumExportedReactionNames() const { return m_lExportedReactionNames.Len(); }
	int32 GetNumExportedAnimGraphNames() const { return m_lExportedAnimGraphNames.Len(); }
	CStr GetExportedActionName(int16 _iExportedAction) const { if (GetNumExportedActionNames() > _iExportedAction) return m_lExportedActionNames[_iExportedAction]; return "";}
	CStr GetExportedReactionName(int16 _iExportedReaction) const { if (GetNumExportedReactionNames() > _iExportedReaction) return m_lExportedReactionNames[_iExportedReaction]; return "";}
	CStr GetExportedAnimGraphName(int16 _iExportedAGName) const { if (GetNumExportedAnimGraphNames() > _iExportedAGName) return m_lExportedAnimGraphNames[_iExportedAGName]; return "";}
	CAG2AnimIndex GetExportedActionAnimIndex(int16 _iExportedAction) const;
	int16 GetExportedActionIndexFromActionIndex(CAG2ActionIndex _iAction) const;

	int GetExportedImpulseTypeValueFromIndex(uint8 _iImpulseType) const;
	int GetExportedImpulseValueFromIndex(uint8 _iImpulseValue) const;
	CStr GetExportedStateName(int16 _iState) const;
	CStr GetExportedSwitchStateName(int16 _iState) const;
	CStr GetExportedGraphBlockName(int16 _iState) const;

	uint8 GetNumExportedPropertiesFunctions() { return m_lExportedPropertyFunctionNames.Len(); }
	uint8 GetNumExportedPropertiesFloat() { return m_lExportedPropertyFloatNames.Len(); }
	uint8 GetNumExportedPropertiesInt() { return m_lExportedPropertyIntNames.Len(); }
	uint8 GetNumExportedPropertiesBool() { return m_lExportedPropertyBoolNames.Len(); }
	uint8 GetNumExportedOperators() { return m_lExportedOperatorNames.Len(); }
	uint8 GetNumExportedEffects() { return m_lExportedEffectNames.Len(); }
	uint8 GetNumExportedStateConstant() { return m_lExportedStateConstantNames.Len(); }
	

	CStr GetExportedPropertyConditionNameFromIndex(uint8 _iProperty) const;
	CStr GetExportedPropertyFunctionNameFromIndex(uint8 _iProperty) const;
	CStr GetExportedPropertyFloatNameFromIndex(uint8 _iProperty) const;
	CStr GetExportedPropertyIntNameFromIndex(uint8 _iProperty) const;
	CStr GetExportedPropertyBoolNameFromIndex(uint8 _iProperty) const;
	CStr GetExportedOperatorNameFromIndex(uint8 _iOperator) const;
	CStr GetExportedEffectNameFromIndex(uint8 _iEffect) const;
	CStr GetExportedStateConstantNameFromIndex(uint8 _iStateConstant) const;
	CStr GetExportedImpulseTypeNameFromIndex(uint8 _iStateConstant) const;
	CStr GetExportedImpulseValueNameFromIndex(uint8 _iStateConstant) const;
	CStr GetExportedPropertyConditionNameFromID(CAG2PropertyID _PropertyID) const;
	CStr GetExportedPropertyFunctionNameFromID(CAG2PropertyID _PropertyID) const;
	CStr GetExportedPropertyFloatNameFromID(CAG2PropertyID _PropertyID) const;
	CStr GetExportedPropertyIntNameFromID(CAG2PropertyID _PropertyID) const;
	CStr GetExportedPropertyBoolNameFromID(CAG2PropertyID _PropertyID) const;
	CStr GetExportedOperatorNameFromID(CAG2OperatorID _OperatorID) const;
	CStr GetExportedEffectNameFromID(CAG2EffectID _EffectID) const;
	CStr GetExportedStateConstantNameFromID(CAG2StateConstantID _StateConstantID) const;
	void LogDump(CStr _LogFileName, uint32 _DumpFlags = -1) const;
#endif
	// IO
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile);
	void Read(const char* _pFileName, bool bCreateLog = false);
	void Write(const char* _pFileName, bool bCreateLog = false);
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif

	//--------------------------------------------------------------------------------

	CStr CheckConsistency(bool _bThrowException);

private:

	bool ReportInconsistency(CStr _Msg, CStr& _Result);
	bool CheckGraphConsistency(CStr& _Result);
	bool CheckStateConsistency(const CXRAG2_State* _pState, CStr& _Result);
	bool CheckNodeConsistency(const CXRAG2_State* _pState, const CXRAG2_ConditionNodeV2* _pNode, CStr& _Result);
	bool CheckNodeActionConsistency(const CXRAG2_State* _pState, int16 _NodeAction, CStr& _Result);
	bool CheckActionEffectsConsistency(int _iBaseEffect, int _nEffects, CStr& _Result);
	bool CheckStateConstantsConsistency(int _iBaseConstant, int _nConstants, CStr& _Result);

	//--------------------------------------------------------------------------------

#ifndef M_RTM
	void LogDump_GraphBlock(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_Structure(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, const CXRAG2_GraphBlock* _pBlock) const;
	void LogDump_Structure_Reactions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, const CXRAG2_GraphBlock* _pBlock) const;
	void LogDump_Structure_State(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG2_State* _pState) const;
	void LogDump_Structure_SwitchState(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG2_SwitchState* _pState) const;
	void LogDump_Structure_Node(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG2_State* _pState, int16 _iNode) const;
	void LogDump_Structure_NodeAction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG2_State* _pState, int16 _NodeAction) const;
	void LogDump_Actions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_SwitchStateActionVals(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_MoveTokens(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_MoveAnimGraphs(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_Reactions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ConditionNodes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_AnimLayers(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_AnimNames(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_EffectInstances(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_StateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_CallbackParams(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedActions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedSwitchStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedPropertiesCondition(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedPropertiesFunction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedPropertiesFloat(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedPropertiesInt(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedPropertiesBool(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedOperators(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedEffects(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedStateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedImpulseTypes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
	void LogDump_ExportedImpulseValues(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;

#endif
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#endif /* AnimGraph_h */
