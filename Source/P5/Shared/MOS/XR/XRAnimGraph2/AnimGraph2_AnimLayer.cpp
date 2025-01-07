//--------------------------------------------------------------------------------

#include "PCH.h"
#include "MDA.h"
#include "MFile.h"
#include "AnimGraph2_AnimLayer.h"

//--------------------------------------------------------------------------------
//- AnimLayer_Full ---------------------------------------------------------------
//--------------------------------------------------------------------------------

void CXRAG2_AnimLayer::Clear()
{
	m_iAnim = CXRAG2_AnimLayer::GetDefaultAnimIndex();
	m_AnimFlags = AG2_ANIMFLAGS_NULL;
	m_iBaseJoint = CXRAG2_AnimLayer::GetDefaultBaseJointIndex();
	m_iMergeOperator = CXRAG2_AnimLayer::GetDefaultMergeOperator();
	m_iTimeControlProperty = CXRAG2_AnimLayer::GetDefaultTimeControlProperty();
	m_TimeOffset = CXRAG2_AnimLayer::GetDefaultTimeOffset();
	m_TimeScale = CXRAG2_AnimLayer::GetDefaultTimeScale();
	m_Opacity = CXRAG2_AnimLayer::GetDefaultOpacity();
}

//--------------------------------------------------------------------------------

void CXRAG2_AnimLayer::Read(CCFile* _pFile, int _Ver)
{
	switch(_Ver)
	{
		case XR_ANIMGRAPH2_VERSION3:
		{
			_pFile->ReadLE(m_TimeOffset);
			_pFile->ReadLE(m_TimeScale);
			_pFile->ReadLE(m_Opacity);

			_pFile->ReadLE(m_iAnim);
			int16 Flags;
			_pFile->ReadLE(Flags);
			m_AnimFlags = Flags;
			_pFile->ReadLE(m_iBaseJoint);
			_pFile->ReadLE(m_iMergeOperator);
			_pFile->ReadLE(m_iTimeControlProperty);
		}
		case XR_ANIMGRAPH2_VERSION:
			{
				_pFile->ReadLE(m_TimeOffset);
				_pFile->ReadLE(m_TimeScale);
				_pFile->ReadLE(m_Opacity);

				_pFile->ReadLE(m_iAnim);
				_pFile->ReadLE(m_AnimFlags);
				_pFile->ReadLE(m_iBaseJoint);
				_pFile->ReadLE(m_iMergeOperator);
				_pFile->ReadLE(m_iTimeControlProperty);
			}
		break;

		default:
			Error_static("CXRAG2_StateAnim::Read", CStrF("Unsupported version %.4x", _Ver));
	}
}

//--------------------------------------------------------------------------------

void CXRAG2_AnimLayer::Write(CCFile* _pFile)
{
#ifndef	PLATFORM_CONSOLE
	_pFile->WriteLE(m_TimeOffset);
	_pFile->WriteLE(m_TimeScale);
	_pFile->WriteLE(m_Opacity);

	_pFile->WriteLE(m_iAnim);
	_pFile->WriteLE(m_AnimFlags);
	_pFile->WriteLE(m_iBaseJoint);
	_pFile->WriteLE(m_iMergeOperator);
	_pFile->WriteLE(m_iTimeControlProperty);
#endif	// PLATFORM_CONSOLE
}

//--------------------------------------------------------------------------------
#ifndef CPU_LITTLEENDIAN
void CXRAG2_AnimLayer::SwapLE()
{
	::SwapLE(m_iAnim);
	::SwapLE(m_AnimFlags);
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
