//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph2_Action.h"

//--------------------------------------------------------------------------------
//- MoveToken ------------------------------------------------------------------
//--------------------------------------------------------------------------------
void CXRAG2_MoveToken::Clear()
{
	m_TokenID = GetDefaultTokenID();
	m_iTargetState = GetDefaultTargetStateIndex();
	m_iTargetGraphBlock = GetDefaultTargetGraphBlockIndex();
	m_AnimBlendDuration = GetDefaultAnimBlendDuration();
	m_AnimBlendDelay = GetDefaultAnimBlendDelay();
	m_AnimTimeOffset = GetDefaultAnimTimeOffset();
	m_TargetStateType = AG2_STATETYPE_NORMAL;
}

void CXRAG2_MoveToken::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION3:
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_AnimTimeOffset);
			_pFile->ReadLE(m_AnimBlendDuration);
			_pFile->ReadLE(m_AnimBlendDelay);
			_pFile->ReadLE(m_iTargetState);
			_pFile->ReadLE(m_iTargetGraphBlock);
			_pFile->ReadLE(m_TokenID);
			_pFile->ReadLE(m_TargetStateType);
		}
		break;

	default:
		Error_static("CXRAG2_MoveToken::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_MoveToken::Write(CCFile* _pFile)
{
	_pFile->WriteLE(m_AnimTimeOffset);
	_pFile->WriteLE(m_AnimBlendDuration);
	_pFile->WriteLE(m_AnimBlendDelay);
	_pFile->WriteLE(m_iTargetState);
	_pFile->WriteLE(m_iTargetGraphBlock);
	_pFile->WriteLE(m_TokenID);
	_pFile->WriteLE(m_TargetStateType);
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_MoveToken::SwapLE()
{
	::SwapLE(m_AnimTimeOffset);
	::SwapLE(m_AnimBlendDuration);
	::SwapLE(m_AnimBlendDelay);
	::SwapLE(m_iTargetState);
	::SwapLE(m_iTargetGraphBlock);
	::SwapLE(m_TokenID);
	::SwapLE(m_TargetStateType);
}
#endif
//--------------------------------------------------------------------------------
//- MoveAnimgraph ------------------------------------------------------------------
//--------------------------------------------------------------------------------
void CXRAG2_MoveAnimGraph::Clear()
{
	m_AnimBlendDuration = GetDefaultAnimBlendDuration();
	m_AnimBlendDelay = GetDefaultAnimBlendDelay();
	m_AnimGraphName = GetDefaultAnimGraphName();
	m_TokenID = GetDefaultTokenID();
}

void CXRAG2_MoveAnimGraph::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
	case XR_ANIMGRAPH2_VERSION3:
	case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_AnimBlendDuration);
			_pFile->ReadLE(m_AnimBlendDelay);
			_pFile->ReadLE(m_AnimGraphName);
			_pFile->ReadLE(m_TokenID);
		}
		break;

	default:
		Error_static("CXRAG2_MoveToken::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

void CXRAG2_MoveAnimGraph::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_AnimBlendDuration);
	_pFile->WriteLE(m_AnimBlendDelay);
	_pFile->WriteLE(m_AnimGraphName);
	_pFile->WriteLE(m_TokenID);
#endif
}

#ifndef CPU_LITTLEENDIAN
void CXRAG2_MoveAnimGraph::SwapLE()
{
	::SwapLE(m_AnimBlendDuration);
	::SwapLE(m_AnimBlendDelay);
	::SwapLE(m_AnimGraphName);
	::SwapLE(m_TokenID);
}
#endif
//--------------------------------------------------------------------------------
//- Action -----------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_Action::Clear()
{
	
	m_nEffectInstances = 0;
	m_iBaseEffectInstance = GetDefaultBaseEffectInstanceIndex();
	m_iBaseMoveToken = GetDefaultMoveTokenIndex();
	m_nMoveTokens = 0;
	
}

//--------------------------------------------------------------------------------

void CXRAG2_Action::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION3:
		case XR_ANIMGRAPH2_VERSION:
		{
			_pFile->ReadLE(m_iBaseEffectInstance);
			_pFile->ReadLE(m_iBaseMoveToken);
			_pFile->ReadLE(m_nMoveTokens);
			_pFile->ReadLE(m_nEffectInstances);
		}
		break;

		default:
			Error_static("CXRAG2_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG2_Action::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iBaseEffectInstance);
	_pFile->WriteLE(m_iBaseMoveToken);
	_pFile->WriteLE(m_nMoveTokens);
	_pFile->WriteLE(m_nEffectInstances);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG2_Action::SwapLE()
{
	::SwapLE(m_iBaseEffectInstance);
	::SwapLE(m_iBaseMoveToken);
	::SwapLE(m_nMoveTokens);
	::SwapLE(m_nEffectInstances);
}
#endif

//--------------------------------------------------------------------------------
