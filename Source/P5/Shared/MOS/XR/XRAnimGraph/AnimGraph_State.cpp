//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph_State.h"

//--------------------------------------------------------------------------------
//- State_Small ------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_State_Small::Clear()
{
	m_lFlags[0] = CXRAG_State::GetDefaultFlags(0);
	m_lFlags[1] = CXRAG_State::GetDefaultFlags(1);

	m_iBaseNode = CXRAG_State::GetDefaultBaseNodeIndex();
	m_iBaseAction = CXRAG_State::GetDefaultBaseActionIndex();
	m_iBasePropertyParam = CXRAG_State::GetDefaultBasePropertyParamIndex();
	m_iBaseConstant = CXRAG_State::GetDefaultBaseConstantIndex();
	m_nConstants = CXRAG_State::GetDefaultNumConstants();

	m_iBaseAnimLayer = CXRAG_State::GetDefaultBaseAnimLayerIndex();
	m_nAnimLayers = CXRAG_State::GetDefaultNumAnimLayers();
	m_Priority = CXRAG_State::GetDefaultPriority();
}

//--------------------------------------------------------------------------------
		
void CXRAG_State_Small::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_lFlags[0]);
			_pFile->ReadLE(m_lFlags[1]);
			_pFile->ReadLE(m_iBaseNode);
			_pFile->ReadLE(m_iBaseAction);
			_pFile->ReadLE(m_iBasePropertyParam);
			_pFile->ReadLE(m_iBaseConstant);
			_pFile->ReadLE(m_nConstants);

			uint8 Pad0;
			_pFile->ReadLE(Pad0);

			_pFile->ReadLE(m_iBaseAnimLayer);
			_pFile->ReadLE(m_nAnimLayers);
			_pFile->ReadLE(m_Priority);
		}
		break;

		case 0x0101:
		{
			_pFile->ReadLE(m_lFlags[0]);
			_pFile->ReadLE(m_lFlags[1]);

			_pFile->ReadLE(m_iBaseNode);
			_pFile->ReadLE(m_iBaseAction);
			_pFile->ReadLE(m_iBasePropertyParam);
			_pFile->ReadLE(m_iBaseConstant);
			_pFile->ReadLE(m_iBaseAnimLayer);

			_pFile->ReadLE(m_nConstants);
			_pFile->ReadLE(m_nAnimLayers);
			_pFile->ReadLE(m_Priority);
		}
		break;

		default:
			Error_static("CXRAG_State::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_State_Small::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_lFlags[0]);
	_pFile->WriteLE(m_lFlags[1]);

	_pFile->WriteLE(m_iBaseNode);
	_pFile->WriteLE(m_iBaseAction);
	_pFile->WriteLE(m_iBasePropertyParam);
	_pFile->WriteLE(m_iBaseConstant);
	_pFile->WriteLE(m_iBaseAnimLayer);

	_pFile->WriteLE(m_nConstants);
	_pFile->WriteLE(m_nAnimLayers);
	_pFile->WriteLE(m_Priority);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------


#ifndef CPU_LITTLEENDIAN
void CXRAG_State_Small::SwapLE()
{
	::SwapLE(m_lFlags[0]);
	::SwapLE(m_lFlags[1]);
	::SwapLE(m_iBaseNode);
	::SwapLE(m_iBaseAction);
	::SwapLE(m_iBasePropertyParam);
	::SwapLE(m_iBaseConstant);
	::SwapLE(m_nConstants);

	::SwapLE(m_iBaseAnimLayer);
	::SwapLE(m_nAnimLayers);
	::SwapLE(m_Priority);
}
#endif

//--------------------------------------------------------------------------------
//- State_Full -------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_State_Full::Clear()
{
	m_lFlags[0] = CXRAG_State::GetDefaultFlags(0);
	m_lFlags[1] = CXRAG_State::GetDefaultFlags(1);

	m_iBaseNode = CXRAG_State::GetDefaultBaseNodeIndex();
	m_iBaseAction = CXRAG_State::GetDefaultBaseActionIndex();
	m_iBasePropertyParam = CXRAG_State::GetDefaultBasePropertyParamIndex();
	m_iBaseConstant = CXRAG_State::GetDefaultBaseConstantIndex();
	m_nConstants = CXRAG_State::GetDefaultNumConstants();

	m_AnimQueue_ProceedCondition_iProperty = CXRAG_State::GetDefaultAQPCProperty();
	m_AnimQueue_ProceedCondition_iOperator = CXRAG_State::GetDefaultAQPCOperator();
	m_AnimQueue_ProceedCondition_Constant = CXRAG_State::GetDefaultAQPCConstant();

	m_iBaseAnimLayer = CXRAG_State::GetDefaultBaseAnimLayerIndex();
	m_nAnimLayers = CXRAG_State::GetDefaultNumAnimLayers();
	m_Priority = CXRAG_State::GetDefaultPriority();
}

//--------------------------------------------------------------------------------

void CXRAG_State_Full::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_lFlags[0]);
			_pFile->ReadLE(m_lFlags[1]);
			_pFile->ReadLE(m_iBaseNode);
			_pFile->ReadLE(m_iBaseAction);
			_pFile->ReadLE(m_iBasePropertyParam);
			_pFile->ReadLE(m_iBaseConstant);
			_pFile->ReadLE(m_nConstants);

			uint8 Pad;
			_pFile->ReadLE(Pad);

			_pFile->ReadLE(m_AnimQueue_ProceedCondition_iProperty);
			_pFile->ReadLE(m_AnimQueue_ProceedCondition_iOperator);
			_pFile->ReadLE(m_AnimQueue_ProceedCondition_Constant);

			_pFile->ReadLE(m_iBaseAnimLayer);
			_pFile->ReadLE(m_nAnimLayers);
			_pFile->ReadLE(m_Priority);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_lFlags[0]);
			_pFile->ReadLE(m_lFlags[1]);

			_pFile->ReadLE(m_AnimQueue_ProceedCondition_Constant);

			_pFile->ReadLE(m_iBaseNode);
			_pFile->ReadLE(m_iBaseAction);
			_pFile->ReadLE(m_iBasePropertyParam);
			_pFile->ReadLE(m_iBaseConstant);
			_pFile->ReadLE(m_iBaseAnimLayer);

			_pFile->ReadLE(m_AnimQueue_ProceedCondition_iProperty);
			_pFile->ReadLE(m_AnimQueue_ProceedCondition_iOperator);

			_pFile->ReadLE(m_nConstants);
			_pFile->ReadLE(m_nAnimLayers);
			_pFile->ReadLE(m_Priority);
		}
		break;

		default:
			Error_static("CXRAG_State::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_State_Full::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_lFlags[0]);
	_pFile->WriteLE(m_lFlags[1]);

	_pFile->WriteLE(m_AnimQueue_ProceedCondition_Constant);

	_pFile->WriteLE(m_iBaseNode);
	_pFile->WriteLE(m_iBaseAction);
	_pFile->WriteLE(m_iBasePropertyParam);
	_pFile->WriteLE(m_iBaseConstant);
	_pFile->WriteLE(m_iBaseAnimLayer);

	_pFile->WriteLE(m_AnimQueue_ProceedCondition_iProperty);
	_pFile->WriteLE(m_AnimQueue_ProceedCondition_iOperator);

	_pFile->WriteLE(m_nConstants);
	_pFile->WriteLE(m_nAnimLayers);
	_pFile->WriteLE(m_Priority);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG_State_Full::SwapLE()
{
	::SwapLE(m_lFlags[0]);
	::SwapLE(m_lFlags[1]);
	::SwapLE(m_iBaseNode);
	::SwapLE(m_iBaseAction);
	::SwapLE(m_iBasePropertyParam);
	::SwapLE(m_iBaseConstant);
	::SwapLE(m_nConstants);

	::SwapLE(m_AnimQueue_ProceedCondition_iProperty);
	::SwapLE(m_AnimQueue_ProceedCondition_iOperator);
	::SwapLE(m_AnimQueue_ProceedCondition_Constant);

	::SwapLE(m_iBaseAnimLayer);
	::SwapLE(m_nAnimLayers);
	::SwapLE(m_Priority);
}
#endif

//--------------------------------------------------------------------------------
