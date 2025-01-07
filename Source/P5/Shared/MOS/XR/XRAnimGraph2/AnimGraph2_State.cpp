//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph2_State.h"

//--------------------------------------------------------------------------------
//- State_Full -------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_State::Clear()
{
	m_lFlags[0] = CXRAG2_State::GetDefaultFlags(0);
	m_lFlags[1] = CXRAG2_State::GetDefaultFlags(1);

	m_iBaseNode = CXRAG2_State::GetDefaultBaseNodeIndex();
	m_iBaseAction = CXRAG2_State::GetDefaultBaseActionIndex();
	m_iBasePropertyParam = CXRAG2_State::GetDefaultBasePropertyParamIndex();
	m_iBaseConstant = CXRAG2_State::GetDefaultBaseConstantIndex();
	m_nConstants = CXRAG2_State::GetDefaultNumConstants();

	m_iBaseAnimLayer = CXRAG2_State::GetDefaultBaseAnimLayerIndex();
	m_nAnimLayers = CXRAG2_State::GetDefaultNumAnimLayers();
	m_Priority = CXRAG2_State::GetDefaultPriority();
}

//--------------------------------------------------------------------------------

void CXRAG2_State::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION3:
		case XR_ANIMGRAPH2_VERSION:
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
			Error_static("CXRAG2_State::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG2_State::Write(CCFile* _pFile)
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
void CXRAG2_State::SwapLE()
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

void CXRAG2_SwitchState::Clear()
{
	m_iProperty = ~0;
	m_PropertyType = 0;
	m_iBaseActionValIndex = -1;
	m_NumActionVal = 0;
	m_Priority = 0;
}

void CXRAG2_SwitchState::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION3:
	case XR_ANIMGRAPH2_VERSION:
		{
			//_pFile->ReadLE(m_iBaseAction);
			_pFile->ReadLE(m_iProperty);
			_pFile->ReadLE(m_PropertyType);

			_pFile->ReadLE(m_iBaseActionValIndex);
			_pFile->ReadLE(m_NumActionVal);
			_pFile->ReadLE(m_Priority);
		}
		break;

	default:
		Error_static("CXRAG2_State::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_SwitchState::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	//_pFile->WriteLE(m_iBaseAction);
	_pFile->WriteLE(m_iProperty);
	_pFile->WriteLE(m_PropertyType);

	_pFile->WriteLE(m_iBaseActionValIndex);
	_pFile->WriteLE(m_NumActionVal);
	_pFile->WriteLE(m_Priority);
#endif
}


#ifndef CPU_LITTLEENDIAN
void CXRAG2_SwitchState::SwapLE()
{
	::SwapLE(m_iProperty);
	::SwapLE(m_PropertyType);
	::SwapLE(m_iBaseActionValIndex);
	::SwapLE(m_NumActionVal);
//	::SwapLE(m_iBasePropertyParam);
	::SwapLE(m_Priority);
}
#endif

void CXRAG2_SwitchStateActionVal::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION3:
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_iMoveToken);
			_pFile->ReadLE(m_ConstantInt);
		}
		break;

	default:
		Error_static("CXRAG2_SwitchStateActionVal::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_SwitchStateActionVal::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iMoveToken);
	_pFile->WriteLE(m_ConstantInt);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_SwitchStateActionVal::SwapLE()
{
	::SwapLE(m_iMoveToken);
	::SwapLE(m_ConstantInt);
}
#endif
