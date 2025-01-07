//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph_AnimLayer.h"

//--------------------------------------------------------------------------------
//- AnimLayer_Small --------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Small::Clear()
{
	m_iAnim = CXRAG_AnimLayer::GetDefaultAnimIndex();
	m_iBaseJoint = CXRAG_AnimLayer::GetDefaultBaseJointIndex();
}

//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Small::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_iAnim);
			_pFile->ReadLE(m_iBaseJoint);
			uint8 Pad;
			_pFile->ReadLE(Pad);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_iAnim);
			_pFile->ReadLE(m_iBaseJoint);
		}
		break;

		default:
			Error_static("CXRAG_StateAnim::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Small::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_iAnim);
	_pFile->WriteLE(m_iBaseJoint);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------
#ifndef CPU_LITTLEENDIAN
void CXRAG_AnimLayer_Small::SwapLE()
{
	::SwapLE(m_iAnim);
	::SwapLE(m_iBaseJoint);
}
#endif
//--------------------------------------------------------------------------------
//- AnimLayer_Full ---------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Full::Clear()
{
	m_iAnim = CXRAG_AnimLayer::GetDefaultAnimIndex();
	m_iBaseJoint = CXRAG_AnimLayer::GetDefaultBaseJointIndex();
	m_iMergeOperator = CXRAG_AnimLayer::GetDefaultMergeOperator();
	m_iTimeControlProperty = CXRAG_AnimLayer::GetDefaultTimeControlProperty();
	m_TimeOffset = CXRAG_AnimLayer::GetDefaultTimeOffset();
	m_TimeScale = CXRAG_AnimLayer::GetDefaultTimeScale();
	m_Opacity = CXRAG_AnimLayer::GetDefaultOpacity();
}

//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Full::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case 0x0100:
		{
			_pFile->ReadLE(m_iAnim);
			_pFile->ReadLE(m_iBaseJoint);
			_pFile->ReadLE(m_iMergeOperator);
			_pFile->ReadLE(m_iTimeControlProperty);
			_pFile->ReadLE(m_TimeOffset);
			_pFile->ReadLE(m_TimeScale);
			_pFile->ReadLE(m_Opacity);
		}
		break;
		case 0x0101:
		{
			_pFile->ReadLE(m_TimeOffset);
			_pFile->ReadLE(m_TimeScale);
			_pFile->ReadLE(m_Opacity);

			_pFile->ReadLE(m_iAnim);
			_pFile->ReadLE(m_iBaseJoint);
			_pFile->ReadLE(m_iMergeOperator);
			_pFile->ReadLE(m_iTimeControlProperty);
		}
		break;

		default:
			Error_static("CXRAG_StateAnim::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG_AnimLayer_Full::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_TimeOffset);
	_pFile->WriteLE(m_TimeScale);
	_pFile->WriteLE(m_Opacity);

	_pFile->WriteLE(m_iAnim);
	_pFile->WriteLE(m_iBaseJoint);
	_pFile->WriteLE(m_iMergeOperator);
	_pFile->WriteLE(m_iTimeControlProperty);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------
#ifndef CPU_LITTLEENDIAN
void CXRAG_AnimLayer_Full::SwapLE()
{
	::SwapLE(m_iAnim);
	::SwapLE(m_iBaseJoint);
	::SwapLE(m_iMergeOperator);
	::SwapLE(m_iTimeControlProperty);
	::SwapLE(m_TimeOffset);
	::SwapLE(m_TimeScale);
	::SwapLE(m_Opacity);
}
#endif
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
