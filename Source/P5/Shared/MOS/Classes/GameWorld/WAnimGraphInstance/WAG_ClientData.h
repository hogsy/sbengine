#ifndef WAG_ClientData_h
#define WAG_ClientData_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph/AnimGraph.h"
#include "WAGI.h"
//#include "../../../XR/Classes/GameWorld/WAnimGraphInstance/WAGI_Context.h"

//--------------------------------------------------------------------------------

class CWAGI_Token;
class CWAGI_Context;
class CWAGI_StateInstance;
class CWO_ClientData_AnimGraphInterface;

class CAGVal
{
	uint8 m_Data[sizeof(fp32) > sizeof(CMTime) ? sizeof(fp32) : sizeof(CMTime)];
public:
	int m_bIsTime;

	M_INLINE CAGVal()
	{
		m_bIsTime = false;
		*((fp32 *)m_Data) = 0.0f;
	}

	M_INLINE static CAGVal From(fp32 _Fp)
	{
		CAGVal Ret;
		Ret.SetFp(_Fp);
		return Ret;
	}

	M_INLINE static CAGVal From(CMTime &_Time)
	{
		CAGVal Ret;
		Ret.SetTime(_Time);
		return Ret;
	}

	M_INLINE CAGVal Scale(fp32 _Scale) const
	{
		CAGVal Ret;
		if (m_bIsTime)
			Ret.SetTime(GetTime().Scale(_Scale));
		else
			Ret.SetFp(GetFp() * _Scale);
		return Ret;
	}

	M_INLINE int Compare(const CAGVal &_Other) const 
	{
		if (m_bIsTime || _Other.m_bIsTime)
			return CompareTime(_Other);
		else
			return CompareFp(_Other);
	}

	M_INLINE int CompareFp(const CAGVal &_Other) const
	{
		if (GetFp() > _Other.GetFp())
			return 1;
		else if (GetFp() < _Other.GetFp())
			return -1;
		else
			return 0;
	}

	M_INLINE int CompareTime(const CAGVal &_Other) const 
	{
		return GetTime().Compare(_Other.GetTime());
	}

	M_INLINE void SetFp(const fp32 &_Fp)
	{
		m_bIsTime = false;
		*((fp32 *)m_Data) = _Fp;
	}

	M_INLINE void SetTime(const CMTime &_Time)
	{
		m_bIsTime = true;
		*((CMTime *)m_Data) = _Time;
	}

	M_INLINE const fp32 &GetFp() const
	{
		M_ASSERT(!m_bIsTime, "You should not convert a time to a fp32 value, precision will be lost");
		return *((fp32 *)m_Data);
	}

	M_INLINE const CMTime GetTime() const
	{
		if (m_bIsTime)
			return *((CMTime *)m_Data);
		else
			return CMTime::CreateFromSeconds(GetFp());
	}
};

typedef bool (CWO_ClientData_AnimGraphInterface::*PFN_ANIMGRAPH_CONDITION)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
typedef CAGVal (CWO_ClientData_AnimGraphInterface::*PFN_ANIMGRAPH_PROPERTY)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
typedef bool (CWO_ClientData_AnimGraphInterface::*PFN_ANIMGRAPH_OPERATOR)(const CWAGI_Context* _pContext, const CAGVal&, const CAGVal&);
typedef void (CWO_ClientData_AnimGraphInterface::*PFN_ANIMGRAPH_EFFECT)(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#define MAX_ANIMGRAPH_DEFCONDITIONS	10
#define MAX_ANIMGRAPH_DEFPROPERTIES	76
#define MAX_ANIMGRAPH_DEFOPERATORS	7
#define MAX_ANIMGRAPH_DEFEFFECTS	25
#define NUM_DIALOGUE_PROPERTIES 6

class CWO_ClientData_AnimGraphInterface
{
	protected:
		class CAnimEvent
		{
		public:

			CMTime			m_GameTime;
			uint16			m_Value;
		};

		PFN_ANIMGRAPH_CONDITION*	m_lpfnConditions;
		PFN_ANIMGRAPH_PROPERTY*		m_lpfnProperties;
		PFN_ANIMGRAPH_OPERATOR*		m_lpfnOperators;
		PFN_ANIMGRAPH_EFFECT*		m_lpfnEffects;

		static PFN_ANIMGRAPH_CONDITION ms_lpfnDefConditions[MAX_ANIMGRAPH_DEFCONDITIONS];
		static PFN_ANIMGRAPH_PROPERTY ms_lpfnDefProperties[MAX_ANIMGRAPH_DEFPROPERTIES];
		static PFN_ANIMGRAPH_OPERATOR ms_lpfnDefOperators[MAX_ANIMGRAPH_DEFOPERATORS];
		static PFN_ANIMGRAPH_EFFECT ms_lpfnDefEffects[MAX_ANIMGRAPH_DEFEFFECTS];

		int							m_nConditions;
		int							m_nProperties;
		int							m_nOperators;
		int							m_nEffects;

		CMTime						m_EnterStateTime;
		int16						m_iEnterAction;
		int16						m_iRandseed;
		fp32							m_AnimLoopDuration;
		fp32							m_EnterAnimTimeOffset;

		fp32							m_WallCollision; // Remove this (should be be handled as post anim effect)

		// Important idle ticker
		uint8					m_ImportantAGEvent;
		// Register handled responses here...
		uint8					m_HandledResponse;
		spCWAGI					m_spDummyAGI;

public:
		fp32							m_Dialogue_Cur[NUM_DIALOGUE_PROPERTIES];
		fp32							m_Dialogue_Once[NUM_DIALOGUE_PROPERTIES];

		CWO_ClientData_AnimGraphInterface()
		{
			m_lpfnConditions = NULL;
			m_lpfnProperties = NULL;
			m_lpfnOperators = NULL;
			m_lpfnEffects = NULL;

			m_ImportantAGEvent = 0;
		}

		virtual void AG_RegisterCallbacks(CWorld_PhysState* _pWPhysState)
		{
			AG_RegisterCallbacks2(ms_lpfnDefConditions, MAX_ANIMGRAPH_DEFCONDITIONS,
								  ms_lpfnDefProperties, MAX_ANIMGRAPH_DEFPROPERTIES,
								  ms_lpfnDefOperators, MAX_ANIMGRAPH_DEFOPERATORS,
								  ms_lpfnDefEffects, MAX_ANIMGRAPH_DEFEFFECTS);
		}

		void AG_RegisterCallbacks2(PFN_ANIMGRAPH_CONDITION* _lpfnConditions, int _nConditions,
			PFN_ANIMGRAPH_PROPERTY* _lpfnProperties, int _nProperties,
			PFN_ANIMGRAPH_OPERATOR* _lpfnOperators, int _nOperators, 
			PFN_ANIMGRAPH_EFFECT* _lpfnEffects, int _nEffects)
		{
			m_lpfnConditions = _lpfnConditions;
			m_lpfnProperties = _lpfnProperties;
			m_lpfnOperators = _lpfnOperators;
			m_lpfnEffects = _lpfnEffects;
			m_nConditions = _nConditions;
			m_nProperties = _nProperties;
			m_nOperators = _nOperators;
			m_nEffects = _nEffects;
		}

		virtual void SetDummyAG(spCWAGI _spAGI) { m_spDummyAGI = _spAGI; }
		virtual void AG_RefreshGlobalProperties(const CWAGI_Context* _pContext);
		virtual void AG_RefreshStateInstanceProperties(const CWAGI_Context* _pContext, const CWAGI_StateInstance* _pStateInstance);
		virtual void AG_OnEnterState(const CWAGI_Context* _pContext, CAGTokenID _TokenID, CAGStateIndex _iState, CAGActionIndex _iEnterAction) {}

		M_INLINE void RegisterImportantAGEvent() { m_ImportantAGEvent = 0; }
		M_INLINE void IncrementLastAGEvent() { m_ImportantAGEvent++; }
		M_INLINE uint8 GetImportantAGEvent() { return m_ImportantAGEvent; }
		M_INLINE void SetWallCollision(fp32 _Collision) { m_WallCollision = _Collision; }
		M_INLINE fp32 GetWallCollision() { return m_WallCollision; }

		CAGVal AG_EvaluateProperty(const CWAGI_Context* _pContext, int _iProperty, const CXRAG_ICallbackParams* _pParams)
		{
#ifndef	M_RTM
			if (m_lpfnProperties == NULL)
				return CAGVal();

			if ((_iProperty < 0) || (_iProperty >= m_nProperties))
				return CAGVal();
#endif	// M_RTM

			if(m_lpfnProperties[_iProperty] != NULL)
				return (this->*(m_lpfnProperties[_iProperty]))(_pContext,  _pParams);

			return CAGVal();
		}
		
		bool AG_EvaluateOperator(const CWAGI_Context* _pContext, int _iOperator, const CAGVal &_OperandA, const CAGVal &_OperandB)
		{
#ifndef	M_RTM
			if (m_lpfnOperators == NULL)
				return false;

			if ((_iOperator < 0) || (_iOperator >= m_nOperators))
				return false;
#endif	// M_RTM

			return (this->*(m_lpfnOperators[_iOperator]))(_pContext, _OperandA, _OperandB);
		}
		
	
		bool AG_EvaluateCondition(const CWAGI_Context* _pContext, int _iProperty, const CXRAG_ICallbackParams* _pParams, int _iOperator, const CAGVal& _Constant, fp32& _TimeFraction)
		{
#ifndef	M_RTM
			if ((m_lpfnConditions != NULL) && 
				((_iProperty >= 0) && (_iProperty < m_nConditions)) &&
				(m_lpfnConditions[_iProperty] != NULL))
#else	// M_RTM
			if(m_lpfnConditions[_iProperty] != NULL)
#endif	// M_RTM
			{
				return (this->*(m_lpfnConditions[_iProperty]))(_pContext, _pParams, _iOperator, _Constant, _TimeFraction);
			}
			else
			{
#ifndef	M_RTM
				if (m_lpfnOperators == NULL)
					return false;

				if ((_iOperator < 0) || (_iOperator >= m_nOperators))
					return false;
#endif	// M_RTM

				_TimeFraction = 0;
				return (this->*(m_lpfnOperators[_iOperator]))(_pContext, AG_EvaluateProperty(_pContext, _iProperty, _pParams), _Constant);
			}
		}
		
		void AG_InvokeEffect(const CWAGI_Context* _pContext, uint8 _iEffect, const CXRAG_ICallbackParams* _pParams)
		{
#ifndef	M_RTM
			if (m_lpfnEffects == NULL)
				return;

			if ((_iEffect < 0) || (_iEffect >= m_nEffects))
				return;
#endif	// M_RTM

			if (m_lpfnEffects[_iEffect] == NULL)
				return;

			(this->*(m_lpfnEffects[_iEffect]))(_pContext, _pParams);
		}

		M_INLINE CMTime GetEnterStateTime() { return m_EnterStateTime; }

		bool Condition_StateTime(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		bool Condition_AnimExitPoint(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		bool Condition_AnimLoopCount(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		bool Condition_AnimLoopCountOffset(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		bool Condition_StateTimeOffset(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);
		// 0<->1
		bool Condition_LoopedAnimCount(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams, int iOperator, const CAGVal &_Constant, fp32& _TimeFraction);

		CAGVal Property_StateTime(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { CMTime TmpTime = _pContext->m_GameTime - m_EnterStateTime; return CAGVal::From(TmpTime); };
		CAGVal Property_Dialogue0(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[0]); }
		CAGVal Property_Dialogue1(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[1]); }
		CAGVal Property_Dialogue2(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[2]); }
		CAGVal Property_Dialogue3(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[3]); }
		CAGVal Property_Dialogue4(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[4]); }
		CAGVal Property_Dialogue5(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From(m_Dialogue_Cur[5]); }
		CAGVal Property_StateTimeOffset(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_Rand1(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_Rand255(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams) { return CAGVal::From((Property_Rand1(_pContext,_pParams).GetFp() * 255.0f)); }
		CAGVal Property_LoopedAnimTime(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);
		CAGVal Property_HandledResponse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		bool OperatorEQ(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) == 0); }
		bool OperatorGT(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) > 0); }
		bool OperatorLT(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) < 0); }
		bool OperatorGE(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) >= 0); }
		bool OperatorLE(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) <= 0); }
		bool OperatorNE(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB) { return (_OperandA.Compare(_OperandB) != 0); }
		bool OperatorMOD(const CWAGI_Context* _pContext, const CAGVal &_OperandA, const CAGVal &_OperandB);

		void Effect_RegisterHandledResponse(const CWAGI_Context* _pContext, const CXRAG_ICallbackParams* _pParams);

		void Clear();
		void Copy(const CWO_ClientData_AnimGraphInterface& _CD);
		bool ConditionHelper(const CWAGI_Context* _pContext, fp32 TimeOffset, fp32& _TimeFraction, bool _bOpEqual, bool _bOpGreater, bool _bOpLess);
};

//--------------------------------------------------------------------------------

#endif /* WAG_ClientData_h */