#ifndef WAGI_Token_h
#define WAGI_Token_h

//--------------------------------------------------------------------------------

//#include "WAGI.h"
#include "WAGI_Context.h"
#include "WAGI_StateInstQueue.h"
//#include "WAGI_StateInstPacked.h"

#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"

class CWAGI;
class CWAGI_StateInstance_Packed;

//--------------------------------------------------------------------------------

class CWAGI_Token : public CWAGI_SIQ // <m_ID>
{
	private:

		CWAGI*						m_pAGI;
//		const CWAGI_StateInstance*	m_pCachedStateInstance; // Lazy cache only...
//		const CXRAG_State*			m_pCachedState; // Cache
		CAGNodeIndex				m_CachedBaseNodeIndex;

		int8						m_ID;
//		bool						m_bFirstRefresh;
		CMTime						m_RefreshGameTime;


	private:

		void EvalNodeInit();
		uint16 EvalNode(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, const CXRAG_ConditionNode* _pNode, fp32& _TimeFraction) const;
		uint16 EvalNodeAction(const CWAGI_Context* _pContext, int16 _iStatePropertyParamBase, uint16 _NodeAction, fp32& _TimeFraction);

	public:

		CWAGI_Token() { Clear(); }
		CWAGI_Token(int8 _ID, CWAGI* _pAGI) { Clear(); m_ID = _ID; SetAGI(_pAGI); }

		void Clear();
		void CopyFrom(const CWAGI_Token* _pToken);

		void SetAGI(CWAGI* _pAGI) { m_pAGI = _pAGI; SetQueueAGI(_pAGI); }
		CWAGI* GetAGI() { return m_pAGI; }

		int8 GetID() const { return m_ID; }
		int16 GetStateIndex() const;
		CMTime GetEnterStateTime() const;
//		bool IsFirstRefresh() const { return m_bFirstRefresh; }

		CMTime GetRefreshGameTime() const { return m_RefreshGameTime; }
		void SetRefreshGameTime(CMTime _RefreshGameTime) { m_RefreshGameTime = _RefreshGameTime; }

		void EnterState(const CWAGI_Context* _pContext, int16 _iAction, fp32 _ForceOffset = 0.0f);
		void LeaveState(const CWAGI_Context* _pContext, int16 _iAction);

		uint32 Refresh(CWAGI_Context* _pContext);
//		void UpdateStateInstance(CWAGI_StateInstance_Packed* _pSIP);
//		void GetAnimLayers(const CWAGI_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers) const;

};

//--------------------------------------------------------------------------------

#endif /* WAGI_Token_h */
