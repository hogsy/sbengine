//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph_Action.h"

//--------------------------------------------------------------------------------
//- Action_Small ------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_Action_Small::Clear()
{
	m_TokenID = GetDefaultTokenID();
	m_iTargetState = GetDefaultTargetStateIndex();
	m_AnimBlendDuration = GetDefaultAnimBlendDuration();
}

//--------------------------------------------------------------------------------

void CXRAG_Action_Small::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			uint8 Pad;
			_pFile->ReadLE(Pad);
			_pFile->ReadLE(m_TokenID);
			_pFile->ReadLE(m_iTargetState);
			_pFile->ReadLE(m_AnimBlendDuration);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_AnimBlendDuration);
			_pFile->ReadLE(m_iTargetState);
			_pFile->ReadLE(m_TokenID);
		}
		break;

		default:
			Error_static("CXRAG_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_Action_Small::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_AnimBlendDuration);
	_pFile->WriteLE(m_iTargetState);
	_pFile->WriteLE(m_TokenID);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG_Action_Small::SwapLE()
{
	::SwapLE(m_TokenID);
	::SwapLE(m_iTargetState);
	::SwapLE(m_AnimBlendDuration);
}
#endif

//--------------------------------------------------------------------------------
//- Action_Full ------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_Action_Full::Clear()
{
	m_TokenID = GetDefaultTokenID();
	m_iTargetState = GetDefaultTargetStateIndex();
	m_nEffectInstances = GetDefaultNumEffectInstances();
	m_iBaseEffectInstance = GetDefaultBaseEffectInstanceIndex();
	m_AnimBlendDuration = GetDefaultAnimBlendDuration();
	m_AnimBlendDelay = GetDefaultAnimBlendDelay();
	m_AnimTimeOffset = GetDefaultAnimTimeOffset();
	m_iAnimTimeOffsetType = GetDefaultAnimTimeOffsetType();
}

//--------------------------------------------------------------------------------

void CXRAG_Action_Full::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_iTargetState);
			_pFile->ReadLE(m_TokenID);
			_pFile->ReadLE(m_iAnimTimeOffsetType);
			_pFile->ReadLE(m_AnimTimeOffset);
			_pFile->ReadLE(m_AnimBlendDuration);
			_pFile->ReadLE(m_AnimBlendDelay);
			_pFile->ReadLE(m_iBaseEffectInstance);
			_pFile->ReadLE(m_nEffectInstances);
			uint8 Padd;
			_pFile->ReadLE(Padd);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_AnimTimeOffset);
			_pFile->ReadLE(m_AnimBlendDuration);
			_pFile->ReadLE(m_AnimBlendDelay);

			_pFile->ReadLE(m_iBaseEffectInstance);
			_pFile->ReadLE(m_iTargetState);
			_pFile->ReadLE(m_TokenID);
			_pFile->ReadLE(m_nEffectInstances);
			_pFile->ReadLE(m_iAnimTimeOffsetType);
		}
		break;

		default:
			Error_static("CXRAG_Action::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_Action_Full::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_AnimTimeOffset);
	_pFile->WriteLE(m_AnimBlendDuration);
	_pFile->WriteLE(m_AnimBlendDelay);
	_pFile->WriteLE(m_iBaseEffectInstance);
	_pFile->WriteLE(m_iTargetState);
	_pFile->WriteLE(m_TokenID);
	_pFile->WriteLE(m_nEffectInstances);
	_pFile->WriteLE(m_iAnimTimeOffsetType);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------

#ifndef CPU_LITTLEENDIAN
void CXRAG_Action_Full::SwapLE()
{
	::SwapLE(m_iTargetState);
	::SwapLE(m_TokenID);
	::SwapLE(m_iAnimTimeOffsetType);
	::SwapLE(m_AnimTimeOffset);
	::SwapLE(m_AnimBlendDuration);
	::SwapLE(m_AnimBlendDelay);
	::SwapLE(m_iBaseEffectInstance);
	::SwapLE(m_nEffectInstances);
}
#endif

//--------------------------------------------------------------------------------
