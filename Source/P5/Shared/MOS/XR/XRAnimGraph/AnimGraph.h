#ifndef AnimGraph_h
#define AnimGraph_h

//--------------------------------------------------------------------------------

#include "AnimGraphDefs.h"

//--------------------------------------------------------------------------------

#define Mask(nBits)								((1 << (nBits)) - 1)
#define ShiftedMask(Shift, nBits)				(Mask(nBits) << (Shift))
#define ShiftedValue(Shift, nBits, Value)		(((Value) & Mask(nBits)) << (Shift))
#define SetBits(Storage, Shift, nBits, Value)	((Storage) = ((Storage) & ~ShiftedMask(Shift, nBits)) | ShiftedValue(Shift, nBits, Value))
#define GetBits(Storage, Shift, nBits)			(((Storage) >> (Shift)) & Mask(nBits))

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

class CXRAG_ICallbackParams
{
	private:

		const fp32*	m_pParams;
		uint8		m_nParams;
		CMTime		*m_pRetTime;

	public:

		CXRAG_ICallbackParams()
		{
			m_pParams = NULL;
			m_nParams = 0;
		}

		CXRAG_ICallbackParams(const CXRAG_ICallbackParams& _IParams)
		{
			m_pParams = _IParams.m_pParams;
			m_nParams = _IParams.m_nParams;
		}

		CXRAG_ICallbackParams(const fp32* _pParams, uint8 _nParams)
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

		fp32 GetParam(uint8 _iParam) const
		{
			if ((_iParam < 0) || (_iParam >= m_nParams))
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

			CStr Result = CStrF("%.3f", m_pParams[0]);
			for (int iParam = 1; iParam < m_nParams; iParam++)
				Result += " " + CStrF("%.3f", m_pParams[iParam]);
			return Result;
		}

};

//--------------------------------------------------------------------------------

class CXRAG_EffectInstance
{
	public:
	enum
	{
		EIOWholeStruct = 1
	};

		CAGCallbackParamIndex	m_iParams; // 2
		CAGEffectID				m_ID; // 1
		uint8					m_nParams;


/*
// FIXME:

		Use State->iParamBase and compress into 16bit (making a total of 32bits instead of 64bits).
		Mayby State iBase is needed. Restrict n total effect params to 16k would suffice.
		Make nice interface for iParams and nParams.

		0 - 16383	(14bit) iPropertyParam
		0 - 3		(2bit) nPropertyParams
*/
		CXRAG_EffectInstance()
		{
			m_ID = AG_EFFECTID_NULL;
			m_iParams = AG_CALLBACKPARAMINDEX_NULL;
			m_nParams = 0;
		}

		CXRAG_EffectInstance(CAGEffectID _ID, CAGCallbackParamIndex _iParams, uint8 _nParams)
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

class CXRAG_StateConstant
{
	public:
	enum
	{
		EIOWholeStruct = 1
	};

		fp32					m_Value;
		CAGStateConstantID	m_ID;

		CXRAG_StateConstant()
		{
			m_ID = 0;
			m_Value = 0;
		}

		CXRAG_StateConstant(CAGStateConstantID _ID, fp32 _Value)
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

class CXRAG_ConditionNode
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
#define AG_CNODE_PROPERTY_BITS						8
#define AG_CNODE_PROPERTYPARAMSINDEX_BITS			5
#define AG_CNODE_NUMPROPERTYPARAMS_BITS				2
#define AG_CNODE_OPERATOR_BITS						3
#define AG_CNODE_TRUEACTION_BITS					7
#define AG_CNODE_FALSEACTION_BITS					7

#define AG_CNODE_PROPERTY_SHIFT						0
#define AG_CNODE_PROPERTYPARAMSINDEX_SHIFT			(AG_CNODE_PROPERTY_SHIFT + AG_CNODE_PROPERTY_BITS)
#define AG_CNODE_NUMPROPERTYPARAMS_SHIFT			(AG_CNODE_PROPERTYPARAMSINDEX_SHIFT + AG_CNODE_PROPERTYPARAMSINDEX_BITS)
#define AG_CNODE_OPERATOR_SHIFT						(AG_CNODE_NUMPROPERTYPARAMS_SHIFT + AG_CNODE_NUMPROPERTYPARAMS_BITS)
#define AG_CNODE_TRUEACTION_SHIFT					(AG_CNODE_OPERATOR_SHIFT + AG_CNODE_OPERATOR_BITS)
#define AG_CNODE_FALSEACTION_SHIFT					(AG_CNODE_TRUEACTION_SHIFT + AG_CNODE_TRUEACTION_BITS)

#ifdef PLATFORM_XENON
#pragma bitfield_order(push)
#pragma bitfield_order(lsb_to_msb)
#endif
		union
		{
			struct
			{
				uint32	m_Property:AG_CNODE_PROPERTY_BITS;
				uint32	m_PropertyParamsIndex:AG_CNODE_PROPERTYPARAMSINDEX_BITS;
				uint32	m_NumPropertyParams:AG_CNODE_NUMPROPERTYPARAMS_BITS;
				uint32	m_Operator:AG_CNODE_OPERATOR_BITS;
				uint32	m_TrueAction:AG_CNODE_TRUEACTION_BITS;
				uint32	m_FalseAction:AG_CNODE_FALSEACTION_BITS;
			};
			uint32 m_Shared32;
		};
#ifdef PLATFORM_XENON
#pragma bitfield_order(pop)
#endif
		fp32		m_Constant; // [ 32bit Constant ]

		CAGPropertyID	GetProperty() const { return m_Property; }
		CAGOperatorID	GetOperator() const { return m_Operator; }
		uint8	GetPropertyParamsIndex() const { return m_PropertyParamsIndex; }
		uint8	GetNumPropertyParams() const { return m_NumPropertyParams; }
		uint8	GetTrueAction() const { return m_TrueAction; }
		uint8	GetFalseAction() const { return m_FalseAction; }
		fp32		GetConstant() const { return m_Constant; }

		void	SetProperty(CAGPropertyID _PropertyID) { m_Property = _PropertyID; }
		void	SetPropertyParamsIndex(uint8 _iPropertyParams) { m_PropertyParamsIndex = _iPropertyParams; }
		void	SetNumPropertyParams(uint8 _nPropertyParams) { m_NumPropertyParams = _nPropertyParams; }
		void	SetOperator(CAGOperatorID _OperatorID) { m_Operator = _OperatorID; }
		void	SetTrueAction(uint8 _TrueAction) { m_TrueAction = _TrueAction; }
		void	SetFalseAction(uint8 _FalseAction) { m_FalseAction = _FalseAction; }
		void	SetConstant(fp32 _Constant) { m_Constant = _Constant; };

		bool operator==(CXRAG_ConditionNode& _Node)
		{
			return (m_Shared32 == _Node.m_Shared32);
/*
			return ((GetProperty() == _Node.GetProperty()) &&
					(GetPropertyParamsIndex() == _Node.GetPropertyParamsIndex()) &&
					(GetNumPropertyParams() == _Node.GetNumPropertyParams()) &&
					(GetOperator() == _Node.GetOperator()) &&
					(GetTrueAction() == _Node.GetTrueAction()) &&
					(GetFalseAction() == _Node.GetFalseAction()) &&
					(GetConstant() == _Node.GetConstant()));
*/
		}

	public:

		void Read(CCFile* _pFile, int _Ver);
		void Write(CCFile* _pFile);
#ifndef CPU_LITTLEENDIAN
		void SwapLE();
#endif

};

//--------------------------------------------------------------------------------

#include "AnimGraph_Action.h"
#include "AnimGraph_AnimLayer.h"
#include "AnimGraph_State.h"

//--------------------------------------------------------------------------------

class CXRAG_ActionHashEntry
{
public:
	enum
	{
		EIOWholeStruct = 1
	};

	uint32			m_HashKey;
	CAGActionIndex	m_iAction; // 2

public:

	CXRAG_ActionHashEntry() { Clear(); }

	void Clear()
	{
		m_HashKey = 0;
		m_iAction = AG_ACTIONINDEX_NULL;
	}

	CXRAG_ActionHashEntry(uint32 _HashKey, CAGActionIndex _iAction)
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

class CXRAG_NameAndID
{
public:
	enum
	{
		EIOWholeStruct = 0
	};

	CFStr	m_Name;
	uint8	m_ID;

public:

	CXRAG_NameAndID() { Clear(); }

	void Clear()
	{
		m_Name = "";
		m_ID = -1;
	}

	CXRAG_NameAndID(CStr _Name, uint8 _ID)
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

//--------------------------------------------------------------------------------

typedef class CXRAG CXR_AnimGraph;
typedef TPtr<class CXRAG> spCXR_AnimGraph;
typedef TPtr<class CXRAG> spCXRAG;
class CXRAG : public CReferenceCount
{
	private:
	public:
		
		CFStr									m_Name;

		TThinArray<CXRAG_State_Full>			m_lFullStates;
		TThinArray<CXRAG_State_Small>			m_lSmallStates;

		TThinArray<CXRAG_AnimLayer_Full>		m_lFullAnimLayers;
		TThinArray<CXRAG_AnimLayer_Small>		m_lSmallAnimLayers;
		TThinArray<int16>						m_lAnimLayersMap; // Maps (chunked?) AnimLayers into mized full/small lists.

		TThinArray<CXRAG_Action_Full>			m_lFullActions;
		TThinArray<CXRAG_Action_Small>			m_lSmallActions;
		TThinArray<int16>						m_lActionsMap; // Maps (chunked?) Actions into mized full/small lists.

		TThinArray<CXRAG_ConditionNode>			m_lNodes;
		TThinArray<CXRAG_EffectInstance>		m_lEffectInstances;
		TThinArray<CXRAG_StateConstant>			m_lStateConstants;
		TThinArray<fp32>							m_lCallbackParams;

		TThinArray<CXRAG_ActionHashEntry>		m_lActionHashEntries;

#ifndef M_RTM
		TThinArray<CFStr>						m_lExportedActionNames;
		TThinArray<CFStr>						m_lExportedStateNames;
		TThinArray<CXRAG_NameAndID>				m_lExportedPropertyNames;
		TThinArray<CXRAG_NameAndID>				m_lExportedOperatorNames;
		TThinArray<CXRAG_NameAndID>				m_lExportedEffectNames;
		TThinArray<CXRAG_NameAndID>				m_lExportedStateConstantNames;
#endif
		//TThinArray<CXRAG_StateFlag>			m_lStateFlags; <CFStr, uint32>

	public:

		CStr GetName() { return m_Name; }

		void Clear();

		// Always supported
		int16 GetNumStates() const { return (m_lFullStates.Len() + m_lSmallStates.Len()); }
		int16 GetNumAnimLayers() const { return (m_lFullAnimLayers.Len() + m_lSmallAnimLayers.Len()); }
		int16 GetNumNodes() const { return m_lNodes.Len(); }
		int16 GetNumActions() const { return (m_lFullActions.Len() + m_lSmallActions.Len()); }
		int16 GetNumEffectInstances() const { return m_lEffectInstances.Len(); }
		int16 GetNumStateConstants() const { return m_lStateConstants.Len(); }
		int16 GetNumCallbackParams() const { return m_lCallbackParams.Len(); }
		int16 GetNumActionHashEntries() const { return m_lActionHashEntries.Len(); }

		const CXRAG_State* GetState(CAGStateIndex _iState) const;
		const CXRAG_AnimLayer* GetAnimLayer(CAGAnimLayerIndex _iAnimLayer) const;
		const CXRAG_ConditionNode* GetNode(CAGNodeIndex _iNode) const;
		const CXRAG_Action* GetAction(CAGActionIndex _iAction) const;
		const CXRAG_EffectInstance* GetEffectInstance(CAGEffectInstanceIndex _iEffectInstance) const;
		const CXRAG_StateConstant* GetStateConstant(CAGStateConstantIndex _iStateConstant) const;
		CXRAG_ICallbackParams GetICallbackParams(CAGCallbackParamIndex _iParams, uint8 _nParams) const;
		CAGActionIndex GetActionIndexFromHashKey(uint32 _ActionHashKey) const;

#ifndef M_RTM
		// Exported (Debug)
		int16 GetNumExportedActionNames() const { return m_lExportedActionNames.Len(); }
		CStr GetExportedActionName(int16 _iExportedAction) const { if (GetNumExportedActionNames() > _iExportedAction) return m_lExportedActionNames[_iExportedAction]; return CStr("");}
		CAGAnimIndex GetExportedActionAnimIndex(int16 _iExportedAction) const;
		int16 GetExportedActionIndexFromActionIndex(CAGActionIndex _iAction) const;

		CStr GetExportedStateName(int16 _iState) const;

		uint8 GetNumExportedProperties() { return m_lExportedPropertyNames.Len(); }
		uint8 GetNumExportedOperators() { return m_lExportedOperatorNames.Len(); }
		uint8 GetNumExportedEffects() { return m_lExportedEffectNames.Len(); }
		uint8 GetNumExportedStateConstant() { return m_lExportedStateConstantNames.Len(); }

		CStr GetExportedPropertyNameFromIndex(uint8 _iProperty) const;
		CStr GetExportedOperatorNameFromIndex(uint8 _iOperator) const;
		CStr GetExportedEffectNameFromIndex(uint8 _iEffect) const;
		CStr GetExportedStateConstantNameFromIndex(uint8 _iStateConstant) const;
		CStr GetExportedPropertyNameFromID(CAGPropertyID _PropertyID) const;
		CStr GetExportedOperatorNameFromID(CAGOperatorID _OperatorID) const;
		CStr GetExportedEffectNameFromID(CAGEffectID _EffectID) const;
		CStr GetExportedStateConstantNameFromID(CAGStateConstantID _StateConstantID) const;
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
		bool CheckStateConsistency(const CXRAG_State* _pState, CStr& _Result);
		bool CheckNodeConsistency(const CXRAG_State* _pState, const CXRAG_ConditionNode* _pNode, CStr& _Result);
		bool CheckNodeActionConsistency(const CXRAG_State* _pState, int16 _NodeAction, CStr& _Result);
		bool CheckActionEffectsConsistency(int _iBaseEffect, int _nEffects, CStr& _Result);
		bool CheckStateConstantsConsistency(int _iBaseConstant, int _nConstants, CStr& _Result);

		//--------------------------------------------------------------------------------

#ifndef M_RTM
		void LogDump_Structure(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_Structure_State(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, int16 _iState, const CXRAG_State* _pState) const;
		void LogDump_Structure_Node(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG_State* _pState, int16 _iNode) const;
		void LogDump_Structure_NodeAction(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent, CStr _BranchStr, const CXRAG_State* _pState, int16 _NodeAction) const;
		void LogDump_Actions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ConditionNodes(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_AnimLayers(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_EffectInstances(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_StateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_CallbackParams(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedActions(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedStates(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedProperties(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedOperators(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedEffects(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
		void LogDump_ExportedStateConstants(CCFile* _pFile, uint32 _DumpFlags, CStr _Indent) const;
#endif
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#endif /* AnimGraph_h */