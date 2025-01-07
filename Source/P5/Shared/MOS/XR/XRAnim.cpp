#include "PCH.h"

#include "MFloat.h"
#include "XRAnim.h"
#include "../MOS.h"
//#include "../Classes/Gameworld/WPhysState.h"

#if defined(PLATFORM_DOLPHIN) && !defined(USE_VIRTUAL_MEMORY) //needs ARAM-manager
# include "../../MSystem/Sound/dolphin/MSnd_Dolphin.h"
#endif

MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_Keyframe, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_Sequence, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_SequenceTracks, CReferenceCount);
MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_Base, CReferenceCount); 

// MRTC_IMPLEMENT_DYNAMIC(CXR_Anim, CReferenceCount);
// MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_Compressed, CReferenceCount);

// #define ANIM_DATAKEY_VERSION	0x0100
//#define ANIM_DATAKEY_VERSION	0x0101	// (New from 0x0100) Added absolute time flag, removed m_Volume
//#define ANIM_DATAKEY_VERSION	0x0200	// (New from 0x0101) Big rewrite of how data keys are stored
#define ANIM_DATAKEY_VERSION	0x0201	// (New from 0x0200) Fixed alignment problems

// #define ANIM_KEYFRAME_VERSION	0x0103
//#define ANIM_KEYFRAME_VERSION	0x0104	// (New from 0x0103) Added absolute time flag
#define ANIM_KEYFRAME_VERSION	0x0200	// (New from 0x0104) Removed m_Data, added m_AbsTime
// AGMERGE
//#define ANIM_SEQUENCE_VERSION	0x0101
//#define ANIM_SEQUENCE_VERSION	0x0102	// (New from 0x0101) Removed subloops and loopseams (i.e. all Rep shitto is gone).
//#define ANIM_SEQUENCE_VERSION	0x0103	// (New from 0x0102) Added m_AbsTimeFlag reading/writing
//#define ANIM_SEQUENCE_VERSION	0x0200  // (New from 0x0103) DataKey I/O changed
#define ANIM_SEQUENCE_VERSION	0x0201  // (New from 0x0200) TimeCode


#define ANIM_ROTTRACKVERSION_100		0x0100
#define ANIM_MOVETRACKVERSION_100		0x0100
#define ANIM_SEQUENCETRACKVERSION_100	0x0100
#define ANIM_SEQUENCETRACK_VERSION		0x0101	// (New from 0x0100) DataKey I/O changed

// *** Added by Backman
/*void CQuaternion16::Unit()
{	
	CQuatfp32 temp;
	temp.Unit();
	Set(temp);
}

void CQuaternion16::Create(const CMat4Dfp32& _Mat)
{
	CQuatfp32 temp;
	temp.Create(_Mat);
	Set(temp);
}

void CQuaternion16::CreateMatrix(CMat4Dfp32& _Mat) const
{
	CQuatfp32 temp;
	Get(temp);
	temp.CreateMatrix(_Mat);
}

void CQuaternion16::Set(const CQuatfp32& _q)
{
	k[0] = FloatToInt16(_q.k[0]);
	k[1] = FloatToInt16(_q.k[1]);
	k[2] = FloatToInt16(_q.k[2]);
	k[3] = FloatToInt16(_q.k[3]);
}

const void CQuaternion16::Get(CQuatfp32& _q) const
{
	_q.k[0] = Int16ToFloat(k[0]);
	_q.k[1] = Int16ToFloat(k[1]);
	_q.k[2] = Int16ToFloat(k[2]);
	_q.k[3] = Int16ToFloat(k[3]);
}
*/
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Keyframe
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CXR_Anim_RotKey::GetRot(CQuatfp32& _q)
{
	MAUTOSTRIP(CXR_Anim_RotKey_GetRot, MAUTOSTRIP_VOID);

#ifdef USE_QUATERNION16
	m_Rot.GetQuatfp32(_q);
#else
	_q = m_Rot;
#endif
}

void CXR_Anim_RotKey::GetRot(CQuatfp32& _q) const
{
	MAUTOSTRIP(CXR_Anim_RotKey_GetRot, MAUTOSTRIP_VOID);

#ifdef USE_QUATERNION16
	m_Rot.GetQuatfp32(_q);
#else
	_q = m_Rot;
#endif
}

void CXR_Anim_RotKey::SetRot(const CQuatfp32& _q)
{
	MAUTOSTRIP(CXR_Anim_RotKey_SetRot, MAUTOSTRIP_VOID);

#ifdef USE_QUATERNION16
	m_Rot.Create(_q);
#else
	m_Rot = _q;
#endif
}

void CXR_Anim_RotKey::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CXR_Anim_RotKey_Read, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_RotKey::Read);

#ifdef USE_QUATERNION16 
	CQuatfp32 tempRot;
#endif

	switch(_Version)
	{
	case 0x00:
	case 0x01:
		{
#ifdef USE_QUATERNION16
			for(int i = 0; i < 4; i++)
				_pF->ReadLE(tempRot.k[i]);
			m_Rot.Create(tempRot);
#else
			for(int i = 0; i < 4; i++)
				_pF->ReadLE(m_Rot.k[i]);
#endif
		}
		break;

	case 0x02 :
		{
			// The rotation is stored in euler angles with 10,10 & 11 bit 
			// precision respectively for the X,Y & Z angles.

			uint32 PackedAngles;
			_pF->ReadLE(PackedAngles);
			CVec3Dfp32 Angles;
			Angles[0] = fp32(PackedAngles & 1023) * (1.0f/1024.0f);
			Angles[1] = fp32((PackedAngles >> 10) & 1023) * (1.0f/1024.0f);
			Angles[2] = fp32((PackedAngles >> 20) & 2047) * (1.0f/2048.0f);
			CMat4Dfp32 Mat;
			Angles.CreateMatrixFromAngles(0, Mat);
#ifdef USE_QUATERNION16
			tempRot.Create(Mat);
			m_Rot.Create(tempRot);
#else
			m_Rot.Create(Mat);
#endif	
		}
		break;

	case 0x03 :
		{
			// The rotation is stored in euler angles with 10,11 & 11 bit 
			// precision respectively for the X,Y & Z angles.

			uint32 PackedAngles;
			_pF->ReadLE(PackedAngles);
			CVec3Dfp32 Angles;
			Angles[0] = fp32(PackedAngles & 1023) * (1.0f/1024.0f);
			Angles[1] = fp32((PackedAngles >> 10) & 2047) * (1.0f/2048.0f);
			Angles[2] = fp32((PackedAngles >> 21) & 2047) * (1.0f/2048.0f);
			CMat4Dfp32 Mat;
			Angles.CreateMatrixFromAngles(0, Mat);
#ifdef USE_QUATERNION16
			tempRot.Create(Mat);
			m_Rot.Create(tempRot);
#else
			m_Rot.Create(Mat);
#endif
		}
		break;

	case 0x04 :
		{
			// The rotation is stored in euler angles with 16,16 & 16 bit 
			// precision respectively for the X,Y & Z angles.
			uint16 Angles16[3];
			_pF->ReadLE(Angles16[0]);
			_pF->ReadLE(Angles16[1]);
			_pF->ReadLE(Angles16[2]);
			CVec3Dfp32 Angles;
			Angles[0] = fp32(Angles16[0]) * (1.0f/65536.0f);
			Angles[1] = fp32(Angles16[1]) * (1.0f/65536.0f);
			Angles[2] = fp32(Angles16[2]) * (1.0f/65536.0f);
			CMat4Dfp32 Mat;
			Angles.CreateMatrixFromAngles(0, Mat);
#ifdef USE_QUATERNION16
			tempRot.Create(Mat);
			m_Rot.Create(tempRot);
#else
			m_Rot.Create(Mat);
#endif
		}
		break;

	case 0x05 :
		{	// The rotation is stored as 4 16-bit quaternion components

#ifdef USE_QUATERNION16
			m_Rot.Read(_pF, _Version);
//			m_Rot.ReadLE(_pF);
#else
			CXR_Anim_Quatint16 TempQuat;
			TempQuat.Read(_pF, _Version);
			TempQuat.GetQuatfp32(m_Rot);
#endif
		}

	default :
		Error_static("CXR_Anim_RotKey::Read", CStrF("Unsupported version %.4x", _Version));
	}
	
	if(m_Rot.k[3] < 0.0f)
	{
		m_Rot.k[0] = -m_Rot.k[0];
		m_Rot.k[1] = -m_Rot.k[1];
		m_Rot.k[2] = -m_Rot.k[2];
		m_Rot.k[3] = -m_Rot.k[3];
	}
}

void CXR_Anim_RotKey::Write(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CXR_Anim_RotKey_Write, MAUTOSTRIP_VOID);

#ifdef USE_QUATERNION16
	CQuatfp32 tempRot;
	m_Rot.GetQuatfp32(tempRot);
#endif

	switch(_Version)
	{
	case 0x00:
	case 0x01:
		{
		#ifdef USE_QUATERNION16
			for(int i = 0; i < 4; i++)
			{
				_pF->WriteLE(tempRot.k[i]);
			}
		#else
			for(int i = 0; i < 4; i++)
			{
				_pF->WriteLE(m_Rot.k[i]);
			}
		#endif
		}
		break;

	case 0x02:
		{
			CVec3Dfp32 Angles;
			CMat4Dfp32 Mat;
		#ifdef USE_QUATERNION16
			tempRot.CreateMatrix3x3(Mat);
		#else
			m_Rot.CreateMatrix3x3(Mat);
		#endif
			Angles.CreateAnglesFromMatrix(0, Mat);

			uint32 PackedAngles = 
				((RoundToInt(Angles[0] * 1024.0f) & 1023) << 0) +
				((RoundToInt(Angles[1] * 1024.0f) & 1023) << 10) +
				((RoundToInt(Angles[2] * 2048.0f) & 2047) << 20);

			_pF->WriteLE(PackedAngles);
		}
		break;

	case 0x03:
		{
			CVec3Dfp32 Angles;
			CMat4Dfp32 Mat;
		#ifdef USE_QUATERNION16
			tempRot.CreateMatrix3x3(Mat);
		#else
			m_Rot.CreateMatrix3x3(Mat);
		#endif
			Angles.CreateAnglesFromMatrix(0, Mat);

			uint32 PackedAngles = 
				((RoundToInt(Angles[0] * 1024.0f) & 1023) << 0) +
				((RoundToInt(Angles[1] * 2048.0f) & 2047) << 10) +
				((RoundToInt(Angles[2] * 2048.0f) & 2047) << 21);

			_pF->WriteLE(PackedAngles);
		}
		break;

	case 0x04:
		{
			CVec3Dfp32 Angles;
			CMat4Dfp32 Mat;
		#ifdef USE_QUATERNION16
			tempRot.CreateMatrix3x3(Mat);
		#else
			m_Rot.CreateMatrix3x3(Mat);
		#endif
			Angles.CreateAnglesFromMatrix(0, Mat);

			uint16 Angles16[3];
			Angles16[0]= RoundToInt(Angles[0] * 65536.0f);
			Angles16[1]= RoundToInt(Angles[1] * 65536.0f);
			Angles16[2]= RoundToInt(Angles[2] * 65536.0f);
			_pF->WriteLE(Angles16[0]);
			_pF->WriteLE(Angles16[1]);
			_pF->WriteLE(Angles16[2]);
		}
	break;

	case 0x05:
		{
		#ifdef USE_QUATERNION16
			m_Rot.Write(_pF);
		#else
			CXR_Anim_Quatint16 Temp(m_Rot);
			Temp.Write(_pF);
		#endif
		}

	default:
		Error_static("CXR_Anim_RotKey::Write", CStrF("Unsupported version %.4x", _Version));
		break;
	}
}

// -------------------------------------------------------------------

void CXR_Anim_MoveKey::GetMove(CVec3Dfp32& _m) const
{
	_m = m_Move;
}

void CXR_Anim_MoveKey::SetMove(const CVec3Dfp32& _m)
{
	m_Move = _m;
}

void CXR_Anim_MoveKey::Read(CCFile* _pF, int _Version)
{
	MAUTOSTRIP(CXR_Anim_MoveKey_Read, MAUTOSTRIP_VOID);
	switch(_Version)
	{
	case 0x0100:
	case 0x0101:
		{
			m_Move.Read(_pF);	
			fp32 dummyVelocity;
			_pF->ReadLE(dummyVelocity);
		}
		break;

	case 0x0102:
	case 0x0103:
	case 0x0104:
	case 0x0200:
		{
			m_Move.Read(_pF);
		}
		break;

	default :
		Error_static("CXR_Anim_MoveKey::Read", CStrF("Unsupported version %.4x", _Version));
	}
}

void CXR_Anim_MoveKey::Write(CCFile* _pF) const
{
	MAUTOSTRIP(CXR_Anim_MoveKey_Write, MAUTOSTRIP_VOID);
	m_Move.Write(_pF);
}



// -------------------------------------------------------------------
void CXR_Anim_Keyframe::Read(CCFile* _pF, uint _nVersion)
{
	MAUTOSTRIP(CXR_Anim_Keyframe_Read, MAUTOSTRIP_VOID);
	uint32 Ver = _nVersion;

	switch(Ver & 0xffff)
	{
	case 0x0100 :
	case 0x0101 :
	case 0x0102 :
		{
			{
				uint32 nRot; _pF->ReadLE(nRot);
				m_lRotKeys.SetLen(nRot);
				for(int i = 0; i < nRot; i++)
					m_lRotKeys[i].Read(_pF, Ver & 0xff);
			}
			{
				uint32 nMove; _pF->ReadLE(nMove);
				m_lMoveKeys.SetLen(nMove);
				for(int i = 0; i < nMove; i++)
					m_lMoveKeys[i].Read(_pF, Ver);
			}
		}
		break;

	case 0x0200:
		_pF->ReadLE(m_AbsTime);
		// continue to read like 0x0104

	case 0x0103:
	case 0x0104:
		{
			int RotVer = (Ver >> 16) & 0xff;

			{
				uint32 nRot; _pF->ReadLE(nRot);
				m_lRotKeys.SetLen(nRot);
				for(int i = 0; i < nRot; i++)
					m_lRotKeys[i].Read(_pF, RotVer);
			}
			{
				uint32 nMove; _pF->ReadLE(nMove);
				m_lMoveKeys.SetLen(nMove);
				for(int i = 0; i < nMove; i++)
					m_lMoveKeys[i].Read(_pF, Ver & 0xffff);
			}
		}
		break;

	default :
		Error("Read", CStrF("Unsupported file-format version %.4x", Ver));
	}


	if (Ver == 0x100)
	{
#ifdef PLATFORM_CONSOLE
		Error("Read", CStrF("Unsupported on console file-format version %.4x", Ver));
#else
/*		if (m_lRotKeys.Len() >= 2)
		{
			CQuatfp32 Q0, QDest;
			Q0 = m_lRotKeys[0].m_Rot;
			Q0.Inverse();
			m_lRotKeys[1].m_Rot.Multiply(Q0, QDest);
			m_lRotKeys[1].m_Rot = QDest;
		}

		if (m_lMoveKeys.Len() >= 2)
		{
			m_lMoveKeys[1].m_Move -= m_lMoveKeys[0].m_Move;
		}*/

		if ((m_lMoveKeys.Len() >= 2) && (m_lRotKeys.Len() >= 2))
		{
			CMat4Dfp32 MKey0, MKey1, MKey, MTmp;

			m_lRotKeys[0].m_Rot.CreateMatrix(MKey0);
			m_lRotKeys[1].m_Rot.CreateMatrix(MKey1);
			m_lMoveKeys[0].m_Move.SetRow(MKey0, 3);
			m_lMoveKeys[1].m_Move.SetRow(MKey1, 3);

			CMat4Dfp32 MKey0Inv;
			MKey0.InverseOrthogonal(MKey0Inv);
			MKey1.Multiply(MKey0Inv, MTmp);

			m_lRotKeys[1].m_Rot.Create(MTmp);
			m_lMoveKeys[1].m_Move = CVec3Dfp32::GetRow(MTmp, 3);

			/*
			CQuatfp32 tempRot0,tempRot1;
			m_lRotKeys[0].m_Rot16.Get(tempRot0);
			m_lRotKeys[1].m_Rot16.Get(tempRot1);
			
			tempRot0.CreateMatrix(MKey0);
			tempRot1.CreateMatrix(MKey1);
			m_lMoveKeys[0].m_Move.SetRow(MKey0, 3);
			m_lMoveKeys[1].m_Move.SetRow(MKey1, 3);
			
			CMat4Dfp32 MKey0Inv;
			MKey0.InverseOrthogonal(MKey0Inv);
			MKey1.Multiply(MKey0Inv, MTmp);
			*/
		}
#endif
	}
}

void CXR_Anim_Keyframe::Write(CCFile* _pF, const CXR_AnimWriteInfo& _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_Keyframe_Write, MAUTOSTRIP_VOID);
	uint32 Ver = ANIM_KEYFRAME_VERSION + (_WriteInfo.m_RotKeyVersion << 16);
	_pF->WriteLE(Ver);

	_pF->WriteLE(m_AbsTime);
	{
		uint32 nRot = m_lRotKeys.Len();  _pF->WriteLE(nRot);
		for(int i = 0; i < nRot; i++)
			m_lRotKeys[i].Write(_pF, _WriteInfo.m_RotKeyVersion);
	}
	{
		uint32 nMove = m_lMoveKeys.Len();  _pF->WriteLE(nMove);
		for(int i = 0; i < nMove; i++)
			m_lMoveKeys[i].Write(_pF);
	}
}

void CXR_Anim_Keyframe::operator= (const CXR_Anim_Keyframe& _Keyframe)
{
	MAUTOSTRIP(CXR_Anim_Keyframe_operator_assign, MAUTOSTRIP_VOID);
	m_AbsTime = _Keyframe.m_AbsTime;
	m_lRotKeys.SetLen(_Keyframe.m_lRotKeys.Len());
	m_lMoveKeys.SetLen(_Keyframe.m_lMoveKeys.Len());
	memcpy(m_lRotKeys.GetBasePtr(), _Keyframe.m_lRotKeys.GetBasePtr(), m_lRotKeys.ListSize());
	memcpy(m_lMoveKeys.GetBasePtr(), _Keyframe.m_lMoveKeys.GetBasePtr(), m_lMoveKeys.ListSize());
/*	m_lRotKeys.QuickSetLen(0);
	m_lRotKeys.Add(&_Keyframe.m_lRotKeys);
	m_lMoveKeys.QuickSetLen(0);
	m_lMoveKeys.Add(&_Keyframe.m_lMoveKeys);*/
}

spCXR_Anim_Keyframe CXR_Anim_Keyframe::Duplicate() const
{
	MAUTOSTRIP(CXR_Anim_Keyframe_Duplicate, NULL);
	spCXR_Anim_Keyframe spAnim_Keyframe = MNew(CXR_Anim_Keyframe);

	*spAnim_Keyframe = *this;

	return spAnim_Keyframe;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Sequence
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CXR_Anim_SequenceData::Clear()
{
	m_AbsTimeFlag = false;
	m_Flags = 0;
	m_Duration = 0.0f;
	m_pNextAnimSequenceLayer = NULL;
	m_iNextAnimSequenceLayer = -1;
	m_TrackMask.Fill();
}

// Returns the time between key _iFrame and the next
fp32 CXR_Anim_SequenceData::GetFrameDuration(int _iFrame) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_GetFrameDelta, 0);
	int nFrames = GetNumKeys();

	if ((_iFrame >= 0) && (_iFrame < nFrames-1))
	{
		return(GetFrame(_iFrame+1)->m_AbsTime - GetFrame(_iFrame)->m_AbsTime);
	}
	else
	{	// Frames before the first or beyond the next to last have zero duration
		return(0.0f);
	}
}


void CXR_Anim_SequenceData::CheckSequenceForBreakouts()
{
	if (HasEvents(ANIM_EVENT_MASK_BREAKOUT | ANIM_EVENT_MASK_ENTRY))
		m_Flags |= ANIM_SEQFLAGS_HASBREAKOUTPOINTS;
}


void QuaternionSpline(const CXR_Anim_RotKey* _pRot0,
						const CXR_Anim_RotKey* _pRot1,
						const CXR_Anim_RotKey* _pRot2,
						const CXR_Anim_RotKey* _pRot3,
						CQuatfp32* _pDest, fp32 _tFrac,
						fp32 _t01,
						fp32 _t12,
						fp32 _t23,
						int _nQ,
						const CXR_Anim_TrackMask* _pTrackMask)
{
	MAUTOSTRIP(QuaternionSpline, MAUTOSTRIP_VOID);

	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	fp32 k = 0.5f;
	fp32 tsA0 = (_t01 != 0.0f) ? (k * _t12 / _t01) : 0.0f;
	fp32 tsA1 = k;
	fp32 tsB0 = k;
	fp32 tsB1 = (_t23 != 0.0f) ? (k * _t12 / _t23) : 0.0f;

	const CQuatfp32* pRot[4];
	CQuatfp32 First, Last;

	for(int iQ = 0; iQ < _nQ; iQ++)
	{
		if(_pTrackMask && !_pTrackMask->IsEnabledRot(iQ))
			continue;

		// Handle sequence endpoint splining
		CQuatfp32 First, Last;

#ifdef USE_QUATERNION16
		CQuatfp32 rot[4];

		_pRot1[iQ].GetRot(rot[1]);
		_pRot2[iQ].GetRot(rot[2]);

		pRot[1] = &(rot[1]);
		pRot[2] = &(rot[2]);
#else
		pRot[1] = &(_pRot1[iQ].m_Rot);
		pRot[2] = &(_pRot2[iQ].m_Rot);
#endif

		if (_pRot0 == NULL)
		{
			pRot[0] = &First;
			(*pRot[2]).Lerp(*pRot[1], 2.0f, First);
			if (pRot[1]->DotProd(First) < 0.0f)
			{
				First.k[0] = -First.k[0];
				First.k[1] = -First.k[1];
				First.k[2] = -First.k[2];
				First.k[3] = -First.k[3];
			}
		}
		else
		{
#ifdef USE_QUATERNION16
			_pRot0[iQ].GetRot(rot[0]);
			pRot[0] = &(rot[0]);
#else
			pRot[0] = &(_pRot0[iQ].m_Rot);
#endif
		}
			
		if (_pRot3 == NULL)
		{
			pRot[3] = &Last;
			(*pRot[1]).Lerp(*pRot[2], 2.0f, Last);
			if (pRot[2]->DotProd(Last) < 0.0f)
			{
				Last.k[0] = -Last.k[0];
				Last.k[1] = -Last.k[1];
				Last.k[2] = -Last.k[2];
				Last.k[3] = -Last.k[3];
			}
		}
		else
		{
#ifdef USE_QUATERNION16
			_pRot3[iQ].GetRot(rot[3]);
			pRot[3] = &(rot[3]);
#else
			pRot[3] = &(_pRot3[iQ].m_Rot);
#endif
		}

		/*
		int i;
		// dQuat1
		fp32 dQA[4];
		for(i = 0; i < 4; i++)
		{
			// dQA[i] = (pRot[2]->k[i] - pRot[0]->k[i]) * (tsA0 + tsA1);
			dQA[i] = (pRot[1]->k[i] - pRot[0]->k[i]) * tsA0;
			dQA[i] += (pRot[2]->k[i] - pRot[1]->k[i]) * tsA1;
		}
				

		// dQuat2
		fp32 dQB[4];
		for(i = 0; i < 4; i++)
		{
			// dQB[i] = (pRot[3]->k[i] - pRot[1]->k[i]) * (tsB0 + tsB1);
			dQB[i] = (pRot[2]->k[i] - pRot[1]->k[i]) * tsB0;
			dQB[i] += (pRot[3]->k[i] - pRot[2]->k[i]) * tsB1;
		}
				

		// Spline it
		for(i = 0; i < 4; i++)
		{
			fp32 v0 = dQA[i];
			fp32 v1 = dQB[i];
			fp32 p0 = pRot[1]->k[i];
			fp32 p1 = pRot[2]->k[i];
			fp32 D = p0;
			fp32 C = v0;
			fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			fp32 A = -(2.0f * B + v0 - v1) / 3.0f;
			_pDest[iQ].k[i] = A*tCube + B*tSqr + C*_tFrac + D;
		}
		*/

		// Spline it
		for(int i = 0; i < 4; i++)
		{
			fp32 p0 = pRot[1]->k[i];
			fp32 p1 = pRot[2]->k[i];
			fp32 p1mp0 = p1 - p0;
			fp32 v0 = (p0 - pRot[0]->k[i]) * tsA0 + p1mp0 * tsA1;
			fp32 v1 = p1mp0 * tsB0 + (pRot[3]->k[i] - p1) * tsB1; 
			fp32 B = 3.0f*(p1mp0) - (2.0f*v0) - v1;
			_pDest[iQ].k[i] = (-(2.0f * B + v0 - v1) *(1.0f/ 3.0f))*tCube + B*tSqr + v0*_tFrac + p0;
		}

		// FIXME: These sqrt's can be combined for SIMD execution. Not a huge win, just a possible improvement.
		_pDest[iQ].Normalize();
	}
}

//--------------------------------------------------------------------------------

void VectorSpline(const CXR_Anim_MoveKey* _pMove0,
				const CXR_Anim_MoveKey* _pMove1,
				const CXR_Anim_MoveKey* _pMove2,
				const CXR_Anim_MoveKey* _pMove3, 
				CVec3Dfp32* _pDest, fp32 _tFrac,
				fp32 _t01,
				fp32 _t12,
				fp32 _t23,
				int _nV,
				const CXR_Anim_TrackMask* _pTrackMask)
{
	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	fp32 k = 0.5f;
	fp32 tsA0 = (_t01 != 0.0f) ? (k * _t12 / _t01) : 0.0f;
	fp32 tsA1 = k;
	fp32 tsB0 = k;
	fp32 tsB1 = (_t23 != 0.0f) ? (k * _t12 / _t23) : 0.0f;

	const CVec3Dfp32* pMove[4];

	for(int iV = 0; iV < _nV; iV++)
	{
		if(_pTrackMask && !_pTrackMask->IsEnabledMove(iV))
			continue;
		CVec3Dfp32 First, Last;
		if (_pMove0 == NULL)
		{	// Extrapolate before first frame
			pMove[0] = &First;
			First = LERP(_pMove2[iV].m_Move, _pMove1[iV].m_Move,2.0f);
		}
		else
		{
			pMove[0] = &(_pMove0[iV].m_Move);
		}

		pMove[1] = &(_pMove1[iV].m_Move);
		pMove[2] = &(_pMove2[iV].m_Move);
			
		if (_pMove3 == NULL)
		{	// Extrapolate beyond last frame
			pMove[3] = &Last;
			Last = LERP(_pMove1[iV].m_Move,_pMove2[iV].m_Move,2.0f);
		}
		else
		{
			pMove[3] = &(_pMove3[iV].m_Move);
		}

		// CVec3Dfp32 dMA = (*pMove[2] - *pMove[0]) * (tsA0 + tsA1);
		CVec3Dfp32 dMA = (*pMove[1] - *pMove[0]) * tsA0;
		dMA += (*pMove[2] - *pMove[1]) * tsA1;

		// CVec3Dfp32 dMB = (*pMove[3] - *pMove[1]) * (tsB0 + tsB1);
		CVec3Dfp32 dMB = (*pMove[2] - *pMove[1]) * tsB0;
		dMB += (*pMove[3] - *pMove[2]) * tsB1;

		// Spline it
		for(int i = 0; i < 3; i++)
		{
			fp32 v0 = dMA.k[i];
			fp32 v1 = dMB.k[i];
			fp32 p0 = pMove[1]->k[i];
			fp32 p1 = pMove[2]->k[i];
			fp32 D = p0;
			fp32 C = v0;
			fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			fp32 A = -(2.0f * B + v0 - v1) * (1.0f/3.0f);
			_pDest[iV].k[i] = A*tCube + B*tSqr + C*_tFrac + D;

		}
	}
}

void VectorSpline(const CXR_Anim_MoveKey* _pMove0,
				  const CXR_Anim_MoveKey* _pMove1,
				  const CXR_Anim_MoveKey* _pMove2,
				  const CXR_Anim_MoveKey* _pMove3, 
				  vec128* _pDest, fp32 _tFrac,
				  fp32 _t01,
				  fp32 _t12,
				  fp32 _t23,
				  int _nV,
				  const CXR_Anim_TrackMask* _pTrackMask)
{
	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	fp32 k = 0.5f;
	fp32 tsA0 = (_t01 != 0.0f) ? (k * _t12 / _t01) : 0.0f;
	fp32 tsA1 = k;
	fp32 tsB0 = k;
	fp32 tsB1 = (_t23 != 0.0f) ? (k * _t12 / _t23) : 0.0f;

	const CVec3Dfp32* pMove[4];

	for(int iV = 0; iV < _nV; iV++)
	{
		if(_pTrackMask && !_pTrackMask->IsEnabledMove(iV))
			continue;
		CVec3Dfp32 First, Last;
		if (_pMove0 == NULL)
		{	// Extrapolate before first frame
			pMove[0] = &First;
			First = LERP(_pMove2[iV].m_Move, _pMove1[iV].m_Move,2.0f);
		}
		else
		{
			pMove[0] = &(_pMove0[iV].m_Move);
		}

		pMove[1] = &(_pMove1[iV].m_Move);
		pMove[2] = &(_pMove2[iV].m_Move);

		if (_pMove3 == NULL)
		{	// Extrapolate beyond last frame
			pMove[3] = &Last;
			Last = LERP(_pMove1[iV].m_Move,_pMove2[iV].m_Move,2.0f);
		}
		else
		{
			pMove[3] = &(_pMove3[iV].m_Move);
		}

		// CVec3Dfp32 dMA = (*pMove[2] - *pMove[0]) * (tsA0 + tsA1);
		CVec3Dfp32 dMA = (*pMove[1] - *pMove[0]) * tsA0;
		dMA += (*pMove[2] - *pMove[1]) * tsA1;

		// CVec3Dfp32 dMB = (*pMove[3] - *pMove[1]) * (tsB0 + tsB1);
		CVec3Dfp32 dMB = (*pMove[2] - *pMove[1]) * tsB0;
		dMB += (*pMove[3] - *pMove[2]) * tsB1;

		// Spline it
		CVec4Dfp32 dest;
		for(int i = 0; i < 3; i++)
		{
			fp32 v0 = dMA.k[i];
			fp32 v1 = dMB.k[i];
			fp32 p0 = pMove[1]->k[i];
			fp32 p1 = pMove[2]->k[i];
			fp32 D = p0;
			fp32 C = v0;
			fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			fp32 A = -(2.0f * B + v0 - v1) * (1.0f/3.0f);
			dest.k[i] = A*tCube + B*tSqr + C*_tFrac + D;

		}
		dest.k[3] = 1.0f;
		_pDest[iV]=dest;
	}
}


//--------------------------------------------------------------------------------
// Reverses signs on rotations so that any two consecutive quats will interpolate correctly
// Call whenever keys are added, deleted or modified
void CXR_Anim_SequenceData::AlignRotations()
{
	MAUTOSTRIP(CXR_Anim_SequenceData_AlignRotations, MAUTOSTRIP_VOID);
	CQuatfp32 prev,cur;
	CXR_Anim_Keyframe* pCurKey = GetFrame(0);
	CXR_Anim_Keyframe* pPrevKey = GetFrame(0);
	int nkeys = GetNumKeys();
	for(int iFrm = 0; iFrm < nkeys; iFrm++)
	{
		// Arrange frame quaternions so that to avoid checking sign of DotProd()
		// except possibly at frame N-1 to 0 (which we should never evaluate IMHO)
		// Do comparisons for each track
		pCurKey = GetFrame(iFrm);
		int nRotCur = pCurKey->m_lRotKeys.Len();
		int nRotPrev = pPrevKey->m_lRotKeys.Len();
		int nRot = Min(nRotPrev,nRotCur);
		for (int iRot = 0; iRot < nRot; iRot++)
		{
			pPrevKey->m_lRotKeys[iRot].GetRot(prev);
			pCurKey->m_lRotKeys[iRot].GetRot(cur);
			if (prev.DotProd(cur) < 0.0f)
			{	
				cur.k[0] = -cur.k[0];
				cur.k[1] = -cur.k[1];
				cur.k[2] = -cur.k[2];
				cur.k[3] = -cur.k[3];
				pCurKey->m_lRotKeys[iRot].SetRot(cur);
			}
		}
		pPrevKey = pCurKey;
	}
}

void CXR_Anim_SequenceData::Initialize()
{
	MAUTOSTRIP(CXR_Anim_SequenceData_Initialize, MAUTOSTRIP_VOID);
	int nKeys = GetNumKeys();

	if((m_Flags & ANIM_SEQFLAGS_LOOPTYPE_MASK) >> ANIM_SEQFLAGS_LOOPTYPE_SHIFT == ANIM_SEQ_LOOPTYPE_CONTINUOUS)
		m_Flags |= ANIM_SEQFLAGS_LOOP;
	
	if (!nKeys) return;

	CQuatfp32 prev,cur;
	CXR_Anim_Keyframe* pCurKey = GetFrame(0);
//	CXR_Anim_Keyframe* pPrevKey = GetFrame(0);
//	int nRot = pCurKey->m_lRotKeys.Len();

	if (m_AbsTimeFlag == false)
	{
		m_Duration = 0.0f;
		for (int iFrm = 0; iFrm < nKeys; iFrm++)
		{
			pCurKey = GetFrame(iFrm);
			// Time is still relative here
			fp32 temp = pCurKey->m_AbsTime;
			pCurKey->m_AbsTime = m_Duration;
			if (iFrm < nKeys-1)
				m_Duration += temp;
		}
		m_AbsTimeFlag = true;
	}

	if (nKeys > 1)
	{
		m_Duration = GetFrame(nKeys-1)->m_AbsTime;
	}
	else
	{	// Single key animations have a duration of 1/20 seconds to avoid division by 0

		m_Duration = 0.05f;

//		m_Duration = SERVER_TIMEPERFRAME;   <--- WTF!??!??!?!?! /mh
	}

	AlignRotations();
}

//--------------------------------------------------------------------------------
bool CXR_Anim_SequenceData::GetFrameAndTimeFraction(fp32 _Time, int& _iFrame, fp32& _tFrac) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_GetFrameAndTimeFraction, 0.0f);

	int nFrames = GetNumKeys();
	if (nFrames == 0)
	{
		return false;
	}

	if (_Time < 0)
	{
		_iFrame = 0;
		_tFrac = 0.0f;
		return true;
	}

	// Note: We never return the last key as it should only be used
	// for interpolation.
	if (_Time >= m_Duration )
	{
		if ((nFrames - 2) >= 0)
		{
			_iFrame = nFrames - 2;
			_tFrac = 0.9999f;
			return true;
		}
		else
		{
			_iFrame = 0;
			_tFrac = 0.9999f;
			return false;
		}
	}
	
	int i,high,low;
	fp32 begin,end;
	// We make an educated guess at where the key is assuming even spacing of keys
	i = TruncToInt((nFrames - 2) * (_Time / m_Duration));
	for (low = (-1), high = nFrames-1; high - low > 1;  )
	{
		begin = GetFrameAbsTime(i);
		end = GetFrameAbsTime(i+1);

		if ((_Time >= begin) && (_Time < end))
		{
			_tFrac = (_Time - begin) / (end - begin);
			_iFrame = i;
			return(true);
		}
		if (_Time < begin)
		{
			high = i;
		}
		else
		{
			low = i;
		}
		i = (high+low) / 2;
	}

	_iFrame = 0;
	return(false);
}

bool CXR_Anim_SequenceData::GetFrameAndTimeFraction(const CMTime& _Time, int& _iFrame, fp32& _tFrac) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_GetFrameAndTimeFraction, 0.0f);

	return GetFrameAndTimeFraction( _Time.GetTime(), _iFrame, _tFrac );
}

//--------------------------------------------------------------------------------

bool CXR_Anim_SequenceData::GetFramesAndTimeFraction(fp32 _Time, int _iFrames[4], fp32& _Fraction) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_GetFramesAndTimeFraction, 0.0f);

	/*
		* Clamps _Time to range (0 - m_Duration).
		* Assumes non looping animation (i.e. does not provide smart loopseems or any such magic).
		* Repeats edge keyframes (i.e. no mirror extrapolation or such magic).
	*/

	if (!GetFrameAndTimeFraction(_Time, _iFrames[1], _Fraction))
		return false;

	// GetFrameAndTimeFraction fails if numkeys == 0
//	int nKeyFrames = GetNumKeys();
//	if (nKeyFrames == 0)
//		return false;

	/* Testing to see if the clamping can be done at a later stage
	_iFrames[0] = Max(0, _iFrames[1] - 1);
	_iFrames[2] = Min(nKeyFrames-1, _iFrames[1] + 1);
	_iFrames[3] = Min(nKeyFrames-1, _iFrames[1] + 2);
	*/

	// This version assumes that the out of range indices will be taken care of further down
	_iFrames[0] = _iFrames[1]-1;
	_iFrames[2] = _iFrames[1]+1;
	_iFrames[3] = _iFrames[1]+2;

	return true;
}

bool CXR_Anim_SequenceData::GetFramesAndTimeFraction(const CMTime& _Time, int _iFrames[4], fp32& _Fraction) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_GetFramesAndTimeFraction, 0.0f);
	MSCOPESHORT(CXR_Anim_SequenceData::GetFramesAndTimeFraction);

	return GetFramesAndTimeFraction( _Time.GetTime(), _iFrames, _Fraction );
/*

	//	* Clamps _Time to range (0 - m_Duration).
	//	* Assumes non looping animation (i.e. does not provide smart loopseems or any such magic).
	//	* Repeats edge keyframes (i.e. no mirror extrapolation or such magic).

	if (!GetFrameAndTimeFraction(_Time, _iFrames[1], _Fraction))
		return false;

	int nKeyFrames = GetNumKeys();
	if (nKeyFrames == 0)
		return false;

	// Testing to see if the clamping can be done at a later stage
	//_iFrames[0] = Max(0, _iFrames[1] - 1);
	//_iFrames[2] = Min(nKeyFrames-1, _iFrames[1] + 1);
	//_iFrames[3] = Min(nKeyFrames-1, _iFrames[1] + 2);
	

	// This version assumes that the out of range indices will be taken care of further down
	_iFrames[0] = _iFrames[1]-1;
	_iFrames[2] = _iFrames[1]+1;
	_iFrames[3] = _iFrames[1]+2;

	return true;
*/
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::EvalMove(const CMTime& _Time, CVec3Dfp32* _pDest, int _nDest) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_EvalMove, MAUTOSTRIP_VOID);

	int nKeys = GetNumKeys();
	int nM = Min(_nDest, (nKeys) ? GetFrame(0)->m_lMoveKeys.Len() : 0);

	if (nKeys > 1)
	{
		int iFrames[4];
		fp32 Fraction;
		if (GetFramesAndTimeFraction(_Time, iFrames, Fraction))
		{
			const CXR_Anim_MoveKey* pM0 = GetMoveFrame(iFrames[0]);
			const CXR_Anim_MoveKey* pM1 = GetMoveFrame(iFrames[1]);
			const CXR_Anim_MoveKey* pM2 = GetMoveFrame(iFrames[2]);
			const CXR_Anim_MoveKey* pM3 = GetMoveFrame(iFrames[3]);

			fp32 Duration01 = GetFrameDuration(iFrames[0]);
			fp32 Duration12 = GetFrameDuration(iFrames[1]);
			fp32 Duration23 = GetFrameDuration(iFrames[2]);

			// These slightly weird looking statements signal VectorSpline that
			// its must extrapolate the NULLed frames
			if (Duration01 == 0)
			{
				pM0 = NULL;
			}

			if (Duration23 == 0)
			{
				pM3 = NULL;
			}

			VectorSpline(pM0, pM1, pM2, pM3,
						 _pDest, Fraction, 
						 Duration01, Duration12, Duration23, nM, 0);
		}
		else
		{
			nM = 0;
		}
	}
	else if (nKeys == 1)
	{	// Just one key
		const CXR_Anim_MoveKey* pKeys = GetMoveFrame(0);
		for(int i = 0; i < nM; i++)
		{
			pKeys[i].GetMove(_pDest[i]);
		}
	}
	else
	{	// No keys
		nM = 0;
	}
	
	// Set undefined slots to 0.
	for(int i = nM; i < _nDest; i++)
		_pDest[i] = 0;
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::EvalRot(const CMTime& _Time, CQuatfp32* _pDest, int _nDest) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_EvalRot, MAUTOSTRIP_VOID);

	int nKeyFrames = GetNumKeys();
	int nQ = Min(_nDest, (nKeyFrames) ? GetFrame(0)->m_lRotKeys.Len() : 0);

	if (nKeyFrames > 1)
	{
		int iFrames[4];
		fp32 Fraction;
		if (GetFramesAndTimeFraction(_Time, iFrames, Fraction))
		{
			const CXR_Anim_RotKey* pR0 = GetRotFrame(iFrames[0]);
			const CXR_Anim_RotKey* pR1 = GetRotFrame(iFrames[1]);
			const CXR_Anim_RotKey* pR2 = GetRotFrame(iFrames[2]);
			const CXR_Anim_RotKey* pR3 = GetRotFrame(iFrames[3]);

			fp32 Duration01 = GetFrameDuration(iFrames[0]);
			fp32 Duration12 = GetFrameDuration(iFrames[1]);
			fp32 Duration23 = GetFrameDuration(iFrames[2]);

			// These slightly weird looking statements signal QuaternionSpline that
			// its must extrapolate the NULLed frames
			if (Duration01 == 0)
			{
				pR0 = NULL;
			}

			if (Duration23 == 0)
			{
				pR3 = NULL;
			}

			QuaternionSpline(pR0, pR1, pR2, pR3,
				_pDest, Fraction, 
				Duration01, Duration12, Duration23, nQ, NULL);
		}
		else
		{
			nQ = 0;
		}
	}
	else if (nKeyFrames == 1)
	{	// Just one key
		const CXR_Anim_RotKey* pKeys = GetRotFrame(0);
		for(int i = 0; i < nQ; i++)
		{
			pKeys[i].GetRot(_pDest[i]);
		}
	}
	else
	{	// No keys
		nQ = 0;
	}

	// Set undefined slots to unit.
	for (int i = nQ; i < _nDest; i++)
		_pDest[i].Unit();
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::Eval(const CMTime& _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_Eval, MAUTOSTRIP_VOID);
	
	Eval(_Time.GetTime(), _pRot, _nRot, _pMove, _nMove, _TrackMask);
/*
	int nKeyFrames = GetNumKeys();
	int nM = Min(_nMove, (nKeyFrames) ? GetFrame(0)->m_lMoveKeys.Len() : 0);
	int nQ = Min(_nRot, (nKeyFrames) ? GetFrame(0)->m_lRotKeys.Len() : 0);

	if (nKeyFrames > 1)
	{
		int iFrames[4];
		fp32 Fraction;
		if (GetFramesAndTimeFraction(_Time, iFrames, Fraction))
		{
			const CXR_Anim_MoveKey* pM0 = GetMoveFrame(iFrames[0]);
			const CXR_Anim_MoveKey* pM1 = GetMoveFrame(iFrames[1]);
			const CXR_Anim_MoveKey* pM2 = GetMoveFrame(iFrames[2]);
			const CXR_Anim_MoveKey* pM3 = GetMoveFrame(iFrames[3]);

			const CXR_Anim_RotKey* pR0 = GetRotFrame(iFrames[0]);
			const CXR_Anim_RotKey* pR1 = GetRotFrame(iFrames[1]);
			const CXR_Anim_RotKey* pR2 = GetRotFrame(iFrames[2]);
			const CXR_Anim_RotKey* pR3 = GetRotFrame(iFrames[3]);

			fp32 Duration01 = GetFrameDuration(iFrames[0]);
			fp32 Duration12 = GetFrameDuration(iFrames[1]);
			fp32 Duration23 = GetFrameDuration(iFrames[2]);

			// These slightly weird looking statements signal QuaternionSpline/VectorSpline that
			// its must extrapolate the NULLed frames
			if (Duration01 == 0)
			{
				pM0 = NULL;
				pR0 = NULL;
			}

			if (Duration23 == 0)
			{
				pM3 = NULL;
				pR3 = NULL;
			}

			VectorSpline(pM0, pM1, pM2, pM3,
				_pMove, Fraction, 
				Duration01, Duration12, Duration23, nM);

			QuaternionSpline(pR0, pR1, pR2, pR3,
				_pRot, Fraction, 
				Duration01, Duration12, Duration23, nQ);
		}
		else
		{
			nM = 0;
			nQ = 0;
		}
	}
	else if (nKeyFrames == 1)
	{	// Just one key
		const CXR_Anim_RotKey* pRotKeys = GetRotFrame(0);
		const CXR_Anim_MoveKey* pMoveKeys = GetMoveFrame(0);
		for(int i = 0; i < nQ; i++)
		{
			pMoveKeys[i].GetMove(_pMove[i]);
			pRotKeys[i].GetRot(_pRot[i]);
		}
	}
	else
	{	// No keys
		nM = 0;
		nQ = 0;
	}

	int i;
	// Set undefined slots to 0.
	for (i = nM; i < _nMove; i++)
		_pMove[i] = 0;

	// Set undefined slots to unit.
	for (i = nQ; i < _nRot; i++)
		_pRot[i].Unit();
*/
}

void CXR_Anim_SequenceData::Eval(fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_Eval, MAUTOSTRIP_VOID);

	int nKeyFrames = GetNumKeys();
	int nM = Min(_nMove, (nKeyFrames) ? GetFrame(0)->m_lMoveKeys.Len() : 0);
	int nQ = Min(_nRot, (nKeyFrames) ? GetFrame(0)->m_lRotKeys.Len() : 0);

	if (nKeyFrames > 1)
	{
		int iFrames[4];
		fp32 Fraction;
		if (GetFramesAndTimeFraction(_Time, iFrames, Fraction))
		{
			const CXR_Anim_MoveKey* pM0 = GetMoveFrame(iFrames[0]);
			const CXR_Anim_MoveKey* pM1 = GetMoveFrame(iFrames[1]);
			const CXR_Anim_MoveKey* pM2 = GetMoveFrame(iFrames[2]);
			const CXR_Anim_MoveKey* pM3 = GetMoveFrame(iFrames[3]);

			const CXR_Anim_RotKey* pR0 = GetRotFrame(iFrames[0]);
			const CXR_Anim_RotKey* pR1 = GetRotFrame(iFrames[1]);
			const CXR_Anim_RotKey* pR2 = GetRotFrame(iFrames[2]);
			const CXR_Anim_RotKey* pR3 = GetRotFrame(iFrames[3]);

			fp32 Duration01 = GetFrameDuration(iFrames[0]);
			fp32 Duration12 = GetFrameDuration(iFrames[1]);
			fp32 Duration23 = GetFrameDuration(iFrames[2]);

			// These slightly weird looking statements signal QuaternionSpline/VectorSpline that
			// its must extrapolate the NULLed frames
			if (Duration01 == 0)
			{
				pM0 = NULL;
				pR0 = NULL;
			}

			if (Duration23 == 0)
			{
				pM3 = NULL;
				pR3 = NULL;
			}

			VectorSpline(pM0, pM1, pM2, pM3,
				_pMove, Fraction, 
				Duration01, Duration12, Duration23, nM, &_TrackMask);

			QuaternionSpline(pR0, pR1, pR2, pR3,
				_pRot, Fraction, 
				Duration01, Duration12, Duration23, nQ, &_TrackMask);
		}
		else
		{
			nM = 0;
			nQ = 0;
		}
	}
	else if (nKeyFrames == 1)
	{	// Just one key
		const CXR_Anim_RotKey* pRotKeys = GetRotFrame(0);
		int i;
		for(i = 0; i < nQ; i++)
			pRotKeys[i].GetRot(_pRot[i]);

		const CXR_Anim_MoveKey* pMoveKeys = GetMoveFrame(0);
		for(i = 0; i < nM; i++)
			_pMove[i] = pMoveKeys[i].GetMove();
	}
	else
	{	// No keys
		nM = 0;
		nQ = 0;
	}

	int i;
	// Set undefined slots to 0.
	for (i = nM; i < _nMove; i++)
		_pMove[i] = M_VConst(0,0,0,1.0f);

	// Set undefined slots to unit.
	for (i = nQ; i < _nRot; i++)
		_pRot[i].Unit();
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::GetTotalTrack0(vec128& _Move0, CQuatfp32& _Rot0) const
{
	int nKeys = GetNumKeys();
	if (nKeys > 0)
	{
		const CXR_Anim_Keyframe* pFirstKey = GetFrame(0);
		const CXR_Anim_Keyframe* pLastKey = GetFrame(nKeys - 1);
		vec128 last = M_VLd_P3_Slow(&pLastKey->m_lMoveKeys[0].m_Move);
		vec128 first = M_VLd_V3_Slow(&pFirstKey->m_lMoveKeys[0].m_Move);
		_Move0 = M_VSub(last,first);

#ifdef USE_QUATERNION16
		pFirstKey->m_lRotKeys[0].m_Rot.GetQuatfp32(_Rot0);
		_Rot0.Inverse();
		CQuatfp32 Rot; pLastKey->m_lRotKeys[0].m_Rot.GetQuatfp32(Rot);
		Rot.Multiply(_Rot0, _Rot0);
#else
		_Rot0 = pFirstKey->m_lRotKeys[0].m_Rot;
		_Rot0.Inverse();
		pLastKey->m_lRotKeys[0].m_Rot.Multiply(_Rot0, _Rot0);
#endif
	}
	else
	{
		_Move0 = M_VConst(0,0,0,1.0f);
		_Rot0.Unit();
	}
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::EvalTrack0(const CMTime& _Time, vec128& _Move0, CQuatfp32& _Rot0) const
{
	int nKeys = GetNumKeys();
	if (nKeys > 1)
	{
		int iFrames[4];
		fp32 Fraction;
		if (GetFramesAndTimeFraction(_Time, iFrames, Fraction))
		{
			const CXR_Anim_MoveKey* pM0 = GetMoveFrame(iFrames[0]);
			const CXR_Anim_MoveKey* pM1 = GetMoveFrame(iFrames[1]);
			const CXR_Anim_MoveKey* pM2 = GetMoveFrame(iFrames[2]);
			const CXR_Anim_MoveKey* pM3 = GetMoveFrame(iFrames[3]);

			const CXR_Anim_RotKey* pR0 = GetRotFrame(iFrames[0]);
			const CXR_Anim_RotKey* pR1 = GetRotFrame(iFrames[1]);
			const CXR_Anim_RotKey* pR2 = GetRotFrame(iFrames[2]);
			const CXR_Anim_RotKey* pR3 = GetRotFrame(iFrames[3]);

			if(pM1 == NULL || pM2 == NULL || pR1 == NULL || pR2 == NULL)
			{	// Broken animation
				_Rot0.Unit();
				_Move0 = M_VConst(0,0,0,1.0f);
				return;
			}

			fp32 Duration01 = GetFrameDuration(iFrames[0]);
			fp32 Duration12 = GetFrameDuration(iFrames[1]);
			fp32 Duration23 = GetFrameDuration(iFrames[2]);

			// These slightly weird looking statements signal QuaternionSpline/VectorSpline that
			// its must extrapolate the NULLed frames
			if (Duration01 == 0)
			{
				pM0 = NULL;
				pR0 = NULL;
			}

			if (Duration23 == 0)
			{
				pM3 = NULL;
				pR3 = NULL;
			}

			VectorSpline(pM0, pM1, pM2, pM3,
				&_Move0, Fraction, 
				Duration01, Duration12, Duration23, 1, NULL);

			QuaternionSpline(pR0, pR1, pR2, pR3,
				&_Rot0, Fraction, 
				Duration01, Duration12, Duration23, 1, NULL);
		}
	}
	else if (nKeys == 1)
	{	// Just one key
		const CXR_Anim_RotKey* pRotKeys = GetRotFrame(0);
		const CXR_Anim_MoveKey* pMoveKeys = GetMoveFrame(0);
		pRotKeys[0].GetRot(_Rot0);
		_Move0=pMoveKeys[0].GetMove();
	}
	else
	{	// Reset, no keys
		_Rot0.Unit();
		_Move0 = M_VConst(0,0,0,1.0f);
	}
	
}

//--------------------------------------------------------------------------------


uint CXR_Anim_SequenceData::FindBreakoutPoints(CXR_Anim_BreakoutPoints& _Points, const CMTime& _AnimTime) const
{
	CBreakoutPoint Points[255];
	uint MaxPoints = 255;
	uint NumAdded = FindBreakoutPoints(Points, MaxPoints,_AnimTime);

	_Points.Create(Points, NumAdded);
	return NumAdded;
}


uint CXR_Anim_SequenceData::FindBreakoutPoints(CBreakoutPoint* _pPoints, uint& _MaxPoints, const CMTime& _AnimTime) const
{
	if (!(m_Flags & ANIM_SEQFLAGS_HASBREAKOUTPOINTS))
		return 0;

	// Find breakoutpoints from given time and forward
	if ((_AnimTime.Compare(CMTime()) < 0) || (_AnimTime.Compare(m_Duration) > 0) || (_MaxPoints <= 0))
		return 0;

	CXR_Anim_DataKey_Sequence Seq = GetDataKeys();
	CXR_Anim_DataKey*const* ppKeys = Seq.GetKeys();
	uint nKeys = Seq.GetNumKeys();
	uint nAdded = 0;
	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		if (nAdded >= _MaxPoints)
			break;

		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if (pKey->m_Type == ANIM_EVENT_TYPE_BREAKOUT)
			_pPoints[nAdded++].m_iBreakout = (uint8)pKey->m_Param;
	}

	// Remove num points added from maxpoints and return the number added 
	_MaxPoints -= nAdded;
	return nAdded;
}


uint CXR_Anim_SequenceData::FindEntryPoints(CXR_Anim_EntryPoints& _Points) const
{
	CEntryPoint Points[255];
	uint MaxPoints = 255;
	uint NumAdded = FindEntryPoints(Points,MaxPoints);

	_Points.Create(Points, NumAdded);
	return NumAdded;
}

uint CXR_Anim_SequenceData::FindEntryPoints(CEntryPoint* _pPoints, uint& _MaxPoints) const
{
	CXR_Anim_DataKey_Sequence Seq = GetDataKeys();
	CXR_Anim_DataKey*const* ppKeys = Seq.GetKeys();

	uint nKeys = Seq.GetNumKeys();
	uint nAdded = 0;

	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		if (nAdded >= _MaxPoints)
			break;

		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if (pKey->m_Type == ANIM_EVENT_TYPE_ENTRY)
			_pPoints[nAdded++].m_iEntry = (uint8)pKey->m_Param;
	}

	// Remove num points added from maxpoints and return the number added 
	_MaxPoints -= nAdded;
	return nAdded;
}

uint CXR_Anim_SequenceData::FindSyncPoints(CXR_Anim_SyncPoints& _Points) const
{
	CSyncPoint Points[255];
	uint MaxPoints = 255;
	uint NumAdded = FindSyncPoints(Points,MaxPoints);

	_Points.Create(Points, NumAdded);
	return NumAdded;
}

uint CXR_Anim_SequenceData::FindSyncPoints(CSyncPoint* _pPoints, uint& _MaxPoints) const
{
	CXR_Anim_DataKey_Sequence Seq = GetDataKeys();
	CXR_Anim_DataKey*const* ppKeys = Seq.GetKeys();

	uint nKeys = Seq.GetNumKeys();
	uint nAdded = 0;

	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		if (nAdded >= _MaxPoints)
			break;

		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if (pKey->m_Type == ANIM_EVENT_TYPE_SYNC)
		{
			_pPoints[nAdded].m_Time = pKey->m_AbsTime;
			_pPoints[nAdded++].m_Type = pKey->m_Param;
		}
	}

	// Remove num points added from maxpoints and return the number added 
	_MaxPoints -= nAdded;
	return nAdded;
}

bool CXR_Anim_SequenceData::HasEvents(uint16 _Mask) const
{
	MSCOPESHORT(CXR_Anim_SequenceData::HasEvents);

	CXR_Anim_DataKey_Sequence DataKeys = GetDataKeys();
	CXR_Anim_DataKey* const* ppKeys = DataKeys.GetKeys();
	uint nKeys = DataKeys.GetNumKeys();

	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if ((1 << pKey->m_Type) & _Mask)
			return true;
	}
	return false;
}


const CXR_Anim_DataKey* CXR_Anim_SequenceData::GetEvents(CMTime& _BeginTime, CMTime _EndTime, uint16 _Mask) const
{
	MSCOPESHORT(CXR_Anim_SequenceData::GetEvents);

	CXR_Anim_DataKey_Sequence DataKeys = GetDataKeys();
	CXR_Anim_DataKey* const* ppKeys = DataKeys.GetKeys();
	uint nKeys = DataKeys.GetNumKeys();

	fp32 BeginTime = _BeginTime.GetTime();
	fp32 EndTime = _EndTime.GetTime();

	// Scan frames for wanted events until _EndTime.		//TODO: Fix this damn linear search
	for (uint iKey = 0; iKey < nKeys; iKey++)
	{
		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if (pKey->m_AbsTime > EndTime)
			return NULL;

		if (((1 << pKey->m_Type) & _Mask) && (((pKey->m_AbsTime - BeginTime) > _FP32_EPSILON) || (pKey->m_AbsTime == 0.0f && BeginTime == 0.0f)))
		{
			// Match found.. Advance _BeginTime towards next frame (between this frame and the next).
			fp32 NewTime = (iKey < nKeys-1) 
				? (pKey->m_AbsTime + ppKeys[iKey+1]->m_AbsTime) * 0.5f 
				: (pKey->m_AbsTime + 1.0f);
			_BeginTime = CMTime::CreateFromSeconds(NewTime); 
			return pKey;
		}
	}
	return NULL;
}

const CXR_Anim_DataKey* CXR_Anim_SequenceData::GetEvents(CMTime _BeginTime, CMTime _EndTime, uint16 _Mask, int16& _iKey) const
{
	MSCOPESHORT(CXR_Anim_SequenceData::GetEvents);

	CXR_Anim_DataKey_Sequence DataKeys = GetDataKeys();
	CXR_Anim_DataKey* const* ppKeys = DataKeys.GetKeys();
	uint nKeys = DataKeys.GetNumKeys();

	fp32 BeginTime = _BeginTime.GetTime();
	fp32 EndTime = _EndTime.GetTime();

	// Scan frames for wanted events from given key until _EndTime.
	for (uint iKey = _iKey; iKey < nKeys; iKey++)
	{
		const CXR_Anim_DataKey* pKey = ppKeys[iKey];
		if (pKey->m_AbsTime > EndTime)
			return NULL;

		if (((1 << pKey->m_Type) & _Mask) && (((pKey->m_AbsTime - BeginTime) >= 0.0f) || (pKey->m_AbsTime == 0.0f && BeginTime == 0.0f)))
		{
			// Match found.. Advance to next key
			_iKey = iKey + 1;
			return pKey;
		}
	}
	return NULL;
}


fp32 CXR_Anim_SequenceData::FindEntryTime(const CXR_Anim_SequenceData* _pOldSeq, const CMTime& _OldTime, int* _pBreakoutID) const
{
	MSCOPESHORT(CXR_Anim_SequenceData::FindEntryTime);

	if (_pBreakoutID)
		*_pBreakoutID = -1;

	CXR_Anim_DataKey_Sequence SrcKeys = _pOldSeq->GetDataKeys();
	CXR_Anim_DataKey_Sequence DstKeys = GetDataKeys();

	uint nSrcKeys = SrcKeys.GetNumKeys();
	uint nDstKeys = DstKeys.GetNumKeys();
	if (!nSrcKeys || ! nDstKeys)
		return 0;

	fp32 StartTime = _OldTime.GetTime();

	CXR_Anim_DataKey* const* ppSrcKeys = SrcKeys.GetKeys();
	CXR_Anim_DataKey* const* ppDstKeys = DstKeys.GetKeys();

	for (uint iSrcKey = 0; iSrcKey < nSrcKeys; iSrcKey++)
	{
		const CXR_Anim_DataKey* pSrcKey = ppSrcKeys[iSrcKey];
		if ((pSrcKey->m_AbsTime > StartTime) && (pSrcKey->m_Type == ANIM_EVENT_TYPE_BREAKOUT))
		{
			uint16 BreakOutID = pSrcKey->m_Param;

			for (int iDestKey = 0; iDestKey < nDstKeys; iDestKey++)
			{
				const CXR_Anim_DataKey* pDestKey = ppDstKeys[iDestKey];
				if ((pDestKey->m_Type == ANIM_EVENT_TYPE_ENTRY) && pDestKey->m_Param == BreakOutID)
				{
					if (_pBreakoutID)
						*_pBreakoutID = BreakOutID;
					return pDestKey->m_AbsTime;
				}
			}
		}
	}
	return 0.0f;
}



//--------------------------------------------------------------------------------

CMTime CXR_Anim_SequenceData::GetLoopedTime(const CMTime& _Time) const
{
	int LoopType = GetLoopType();

	switch (LoopType)
	{

	case ANIM_SEQ_LOOPTYPE_CONTINUOUS:
		{
			return _Time.Modulus(m_Duration);
		}

	case ANIM_SEQ_LOOPTYPE_OSCILLATING:
		{
			// Two forward loop periods equals one pingpong loop period.

			CMTime Duration = CMTime::CreateFromSeconds(m_Duration);
			CMTime DurationTimes2 = Duration + Duration;
			CMTime TimeFrac = _Time.Modulus(DurationTimes2);

			if (TimeFrac.Compare(Duration) > 0)
				return DurationTimes2 - TimeFrac;
			else
				return TimeFrac;
		}
	default:
	case ANIM_SEQ_LOOPTYPE_ONCE:
		{
			return CMTime().Max(_Time.Min(CMTime::CreateFromSeconds(m_Duration)));
		}
	}

	return CMTime();
}

CMTime CXR_Anim_SequenceData::GetTimeCode()
{
	return m_TimeCode;
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::Interpolate(fp32 _Frac, 
	CQuatfp32* _pRot0, CQuatfp32* _pRot1, CQuatfp32* _pRotDst, int _nRot, 
	CVec3Dfp32* _pMove0, CVec3Dfp32* _pMove1, CVec3Dfp32* _pMoveDst, int _nMove)
{
	MAUTOSTRIP(CXR_Anim_SequenceData__Interpolate, MAUTOSTRIP_VOID);
	int i;
	for(i = 0; i < _nRot; i++)
		_pRot0[i].Lerp(_pRot1[i], _Frac, _pRotDst[i]);
	for(i = 0; i < _nMove; i++)
		_pMove0[i].Lerp(_pMove1[i], _Frac, _pMoveDst[i]);
}

bool CXR_Anim_SequenceData::IsPlaying(fp32 _Time) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_IsPlaying, false);
	if ((m_Flags & ANIM_SEQFLAGS_LOOP)/* || (m_Flags & ANIM_SEQFLAGS_HOLDATEND)*/)
		return true;
	else
		return _Time < m_Duration;
}

bool CXR_Anim_SequenceData::IsPlaying(const CMTime& _Time) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_IsPlaying, false);
	if ((m_Flags & ANIM_SEQFLAGS_LOOP)/* || (m_Flags & ANIM_SEQFLAGS_HOLDATEND)*/)
		return true;
	else
		return _Time.Compare(CMTime::CreateFromSeconds(m_Duration))<0;
}

//--------------------------------------------------------------------------------

uint CXR_Anim_SequenceData::ReadData(CCFile* _pF, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_SequenceData_ReadData, MAUTOSTRIP_VOID);
	uint32 Ver;
	_pF->ReadLE(Ver);

	switch(Ver)
	{
	case 0x100:
		{
			m_AbsTimeFlag = false;
			uint32 Flags; _pF->ReadLE(Flags); m_Flags = Flags;
			{
				CStr Name, Comment;
				Name.Read(_pF);
				Comment.Read(_pF);
				if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
#ifndef	PLATFORM_CONSOLE
				if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
				m_Name	= Name;
				m_Comment	= Comment;
#else
				m_NameHash = StringToHash(Name);
#endif
			}
			_pF->ReadLE(m_Duration);
			int32 iRepSFrame; _pF->ReadLE(iRepSFrame);
			int32 iRepEFrame; _pF->ReadLE(iRepEFrame);
			fp32 RepSTime; _pF->ReadLE(RepSTime);
			fp32 RepETime; _pF->ReadLE(RepETime);
		}
		break;

	case 0x101:
		{
			m_AbsTimeFlag = false;
			_pF->ReadLE(m_Flags);
			{
				CStr Name, Comment;
				Name.Read(_pF);
				Comment.Read(_pF);
				if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
#ifndef	PLATFORM_CONSOLE
				if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
				m_Name	= Name;
				m_Comment	= Comment;
#else
				m_NameHash = StringToHash(Name);
#endif
			}
			int16 iRepSFrame; _pF->ReadLE(iRepSFrame);
			int16 iRepEFrame; _pF->ReadLE(iRepEFrame);
		}
		break;

	case 0x102:
		{
			m_AbsTimeFlag = false;
			_pF->ReadLE(m_Flags);
			{
				CStr Name, Comment;
				Name.Read(_pF);
				Comment.Read(_pF);
				if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
#ifndef	PLATFORM_CONSOLE
				if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
				m_Name	= Name;
				m_Comment	= Comment;
#else
				m_NameHash = StringToHash(Name);
#endif
			}
			int16 iRepSFrame; _pF->ReadLE(iRepSFrame);
			int16 iRepEFrame; _pF->ReadLE(iRepEFrame);
		}
		break;

	case 0x103:
	case 0x200:
		{
			_pF->ReadLE(m_AbsTimeFlag);
			_pF->ReadLE(m_Flags);
			{
				CStr Name, Comment;
				Name.Read(_pF);
				Comment.Read(_pF);
				if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
#ifndef	PLATFORM_CONSOLE
				if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
				m_Name	= Name;
				m_Comment	= Comment;
#else
				m_NameHash = StringToHash(Name);
#endif
			}
		}
		break;
	case 0x201:
		{
			_pF->ReadLE(m_AbsTimeFlag);
			_pF->ReadLE(m_Flags);
			m_TimeCode.Read(_pF);
			{
				CStr Name, Comment;
				Name.Read(_pF);
				Comment.Read(_pF);
				if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
#ifndef	PLATFORM_CONSOLE
				if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
				m_Name = Name;
				m_Comment = Comment;
#else
				m_NameHash = StringToHash(Name);
#endif
			}
		}
		break;

	default :
		Error("Read", CStrF("Unsupported file-format version %.4x", Ver));
	}
	return Ver;
}

//--------------------------------------------------------------------------------

void CXR_Anim_SequenceData::WriteData(CCFile* _pF) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_WriteData, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	uint32 Ver = ANIM_SEQUENCE_VERSION;
	_pF->WriteLE(Ver);

	_pF->WriteLE(m_AbsTimeFlag);
	_pF->WriteLE(m_Flags);
	m_TimeCode.Write(_pF);
	m_Name.Write(_pF);
	m_Comment.Write(_pF);
#endif
}

//--------------------------------------------------------------------------------
uint CXR_Anim_SequenceData::ReadDataCompact(CCFile* _pF,int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_SequenceData_ReadDataCompact, MAUTOSTRIP_VOID);
	uint16 Ver;
	_pF->ReadLE(Ver);
	switch(Ver)
	{
	case 0x0100:
	case 0x0101:
		{
			m_AbsTimeFlag = false;
			_pF->ReadLE(m_Flags);
			int16 iRepSFrame; _pF->ReadLE(iRepSFrame);
			int16 iRepEFrame; _pF->ReadLE(iRepEFrame);
		}
		break;

	case 0x0102:
		{
			m_AbsTimeFlag = false;
			_pF->ReadLE(m_Flags);
			int16 iRepSFrame; _pF->ReadLE(iRepSFrame);
			int16 iRepEFrame; _pF->ReadLE(iRepEFrame);
		}
		break;

	default :
		Error("Read", CStrF("Unsupported file-format version %.4x", Ver));
	}
	return Ver;
}

//--------------------------------------------------------------------------------

/*
void CXR_Anim_SequenceData::WriteDataCompact(CCFile* _pF) const
{
	MAUTOSTRIP(CXR_Anim_SequenceData_WriteDataCompact, MAUTOSTRIP_VOID);
	uint16 Ver = ANIM_SEQUENCE_VERSION;
	_pF->WriteLE(Ver);
	_pF->WriteLE(m_Flags);
}
*/

// -------------------------------------------------------------------
CXR_Anim_Sequence::CXR_Anim_Sequence()
{
	MAUTOSTRIP(CXR_Anim_Sequence_ctor, MAUTOSTRIP_VOID);
}

CXR_Anim_Sequence::~CXR_Anim_Sequence()
{
	MAUTOSTRIP(CXR_Anim_Sequence_dtor, MAUTOSTRIP_VOID);
	m_lspKeys.Clear();
}

void CXR_Anim_Sequence::Clear()
{
	m_lspKeys.Clear();
};

int CXR_Anim_Sequence::GetNumKeys() const
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetNumKeys, 0);
	return m_lspKeys.Len();
}

fp32 CXR_Anim_Sequence::GetFrametime(int _iFrame) const
{
	if (_iFrame < 0) {return(0.0f);}
	if (_iFrame >= m_lspKeys.Len()) {return(m_Duration);}

	return(m_lspKeys[_iFrame]->m_AbsTime);
}

void CXR_Anim_Sequence::AddFrame(spCXR_Anim_Keyframe _spFrame)
{
	m_lspKeys.Add(_spFrame);
}

CXR_Anim_Keyframe* CXR_Anim_Sequence::GetFrame(int _iFrm)
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetFrame, NULL);
	int nFrames = m_lspKeys.Len();
	if (_iFrm < 0) _iFrm += nFrames;
	if (_iFrm >= nFrames) _iFrm %= nFrames;
	return m_lspKeys[_iFrm];
}

const CXR_Anim_Keyframe* CXR_Anim_Sequence::GetFrame(int _iFrm) const
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetFrame_2, NULL);
	int nFrames = m_lspKeys.Len();
	if (_iFrm < 0) _iFrm += nFrames;
	if (_iFrm >= nFrames) _iFrm %= nFrames;
	return m_lspKeys[_iFrm];
}

CXR_Anim_RotKey* CXR_Anim_Sequence::GetRotFrame(int _iFrm)
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetRotFrame, NULL);
	return GetFrame(_iFrm)->m_lRotKeys.GetBasePtr();
}

const CXR_Anim_RotKey* CXR_Anim_Sequence::GetRotFrame(int _iFrm) const
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetRotFrame_2, NULL);
	return GetFrame(_iFrm)->m_lRotKeys.GetBasePtr();
}

CXR_Anim_MoveKey* CXR_Anim_Sequence::GetMoveFrame(int _iFrm)
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetMoveFrame, NULL);
	return GetFrame(_iFrm)->m_lMoveKeys.GetBasePtr();
}

const CXR_Anim_MoveKey* CXR_Anim_Sequence::GetMoveFrame(int _iFrm) const
{
	MAUTOSTRIP(CXR_Anim_Sequence_GetMoveFrame_2, NULL);
	return GetFrame(_iFrm)->m_lMoveKeys.GetBasePtr();
}

// -------------------------------------------------------------------
static uint KeysEqual(const CXR_Anim_Keyframe* _pKey0, const CXR_Anim_Keyframe* _pKey1)
{
	// Lengths should never differ, but whatever..
	if (_pKey0->m_lRotKeys.Len() != _pKey1->m_lRotKeys.Len())
		return 0;
	if (_pKey0->m_lMoveKeys.Len() != _pKey1->m_lMoveKeys.Len())
		return 0;

	int movecmp = memcmp(_pKey0->m_lMoveKeys.GetBasePtr(), _pKey1->m_lMoveKeys.GetBasePtr(), _pKey0->m_lMoveKeys.ListSize());
	if (movecmp)
		return 0;
	int rotcmp = memcmp(_pKey0->m_lRotKeys.GetBasePtr(), _pKey1->m_lRotKeys.GetBasePtr(), _pKey0->m_lRotKeys.ListSize());
//	M_TRACEALWAYS("        rcmp %d, movecmp %d\n", rotcmp, movecmp);
	if (rotcmp)
		return 0;
	return 1;
}

void CXR_Anim_Sequence::Read(CCFile* _pF, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Sequence_Read, MAUTOSTRIP_VOID);

//	size_t UsedBefore = MRTC_GetMemoryManager()->GetUsedMem();
	uint nDeleted = 0;
	int nRotKeys = 0;
	int nMoveKeys = 0;
	uint nSeqVersion = ReadData(_pF, _ReadFlags);
	{
		uint32 nEnt; _pF->ReadLE(nEnt);
		m_lspKeys.SetLen(nEnt);
		for(int i = 0; i < nEnt; i++)
		{
			m_lspKeys[i] = MNew(CXR_Anim_Keyframe);
			if (!m_lspKeys[i]) MemError("Read");

			uint32 nVersion;
			_pF->ReadLE(nVersion);
			if ((nVersion & 0xffff) < 0x0200)
			{ // Legacy support
				CXR_Anim_DataKey1 Tmp;
				Tmp.Read(_pF);
				m_lspKeys[i]->Read(_pF, nVersion);
				m_lspKeys[i]->m_AbsTime = Tmp.m_AbsTime;
				m_Events.AddKey(Tmp);
			}
			else
			{
				m_lspKeys[i]->Read(_pF, nVersion);
#ifdef PLATFORM_CONSOLE
				nRotKeys = m_lspKeys[i]->m_lRotKeys.Len();
				nMoveKeys = m_lspKeys[i]->m_lMoveKeys.Len();
				if (i > 1)
				{
					// ToDo: Optimize compares so we don't do 2 per pair.
					if (KeysEqual(m_lspKeys[i-2], m_lspKeys[i-1]) &&
						KeysEqual(m_lspKeys[i-1], m_lspKeys[i]))
					{
						m_lspKeys[i-1] = m_lspKeys[i-2];
						nDeleted++;
					}
				}
#endif
			}
		}
	}
	{ // New DataKey format available?
		if (nSeqVersion >= 0x0200)
		{
			uint16 nDataKeyVersion;
			_pF->ReadLE(nDataKeyVersion);
			m_Events.Read(_pF, nDataKeyVersion);
		}
	}

//	size_t UsedAfter = MRTC_GetMemoryManager()->GetUsedMem();
//	M_TRACEALWAYS("(CXR_Anim_Sequence::Read) Deleted %d keys out of %d, ate %d bytes, nRot %d, nMove %d\n", nDeleted, m_lspKeys.Len(), UsedAfter - UsedBefore, nRotKeys, nMoveKeys);

	CheckSequenceForBreakouts();
}

void CXR_Anim_Sequence::ReadCompact(CCFile* _pF, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Sequence_Read, MAUTOSTRIP_VOID);
	uint nSeqVersion = ReadDataCompact(_pF, _ReadFlags);
	{
		uint32 nEnt; _pF->ReadLE(nEnt);
		m_lspKeys.SetLen(nEnt);
		for(int i = 0; i < nEnt; i++)
		{
			m_lspKeys[i] = MNew(CXR_Anim_Keyframe);
			if (!m_lspKeys[i]) MemError("Read");

			uint32 nVersion;
			_pF->ReadLE(nVersion);
			if (nVersion < 0x0200)
			{ // Legacy support
				CXR_Anim_DataKey1 Tmp;
				Tmp.Read(_pF);
				m_lspKeys[i]->Read(_pF, nVersion);
				m_lspKeys[i]->m_AbsTime = Tmp.m_AbsTime;
				m_Events.AddKey(Tmp);
			}
			else
			{
				m_lspKeys[i]->Read(_pF, nVersion);
			}
		}
	}
	{ // New DataKey format available?
		if (nSeqVersion >= 0x0200)
		{
			uint16 nDataKeyVersion;
			_pF->ReadLE(nDataKeyVersion);
			m_Events.Read(_pF, nDataKeyVersion);
		}
	}
	CheckSequenceForBreakouts();
}

void CXR_Anim_Sequence::Write(CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_Sequence_Write, MAUTOSTRIP_VOID);
	CXR_AnimWriteInfo WriteInfo;
	WriteInfo = _WriteInfo;
	// OK, what this basically does is
	if (WriteInfo.m_RotKeyVersion >= 2 && WriteInfo.m_RotKeyVersion <= 3/* &&
		m_Flags & ANIM_SEQFLAGS_HIGHPRECISIONROT*/)
		WriteInfo.m_RotKeyVersion = 4;

	WriteData(_pF);
	{
		uint32 nEnt = m_lspKeys.Len(); _pF->WriteLE(nEnt);
		for(int i = 0; i < nEnt; i++)
			m_lspKeys[i]->Write(_pF, WriteInfo);
	}
	{
		_pF->WriteLE(uint16(ANIM_DATAKEY_VERSION));
		m_Events.Write(_pF);
	}
}


//
//void CXR_Anim_Sequence::WriteCompact(CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo)
//{
//	MAUTOSTRIP(CXR_Anim_Sequence_WriteCompact, MAUTOSTRIP_VOID);
//	CXR_AnimWriteInfo WriteInfo;
//	WriteInfo = _WriteInfo;
//	if (WriteInfo.m_RotKeyVersion >= 2 && WriteInfo.m_RotKeyVersion <= 3/* &&
//		m_Flags & ANIM_SEQFLAGS_HIGHPRECISIONROT*/)
//		WriteInfo.m_RotKeyVersion = 4;
//
//	WriteDataCompact(_pF);
//
//	{
///*		uint32 nKeys = m_lspKeys.Len(); _pF->WriteLE(nKeys);
//
//		if (nKeys)
//		{
//			uint16 nRot = GetFrame(0)->m_lRotKeys.Len();
//			uint16 nMove = GetFrame(0)->m_lMoveKeys.Len();
//			_pF->WriteLE(nRot);
//			_pF->WriteLE(nMove);
//
//			{ for(int i = 0; i < nKeys; i++) 
//				if (GetFrame(i)->m_lRotKeys.Len() != nRot)
//					Error("WriteCompact", "Rotation track count missmatch.");}
//			{ for(int i = 0; i < nKeys; i++) 
//				if (GetFrame(i)->m_lMoveKeys.Len() != nMove)
//					Error("WriteCompact", "Movement track count missmatch.");}
//
//			
//			{ for(int i = 0; i < nKeys; i++) _pF->WriteLE(GetFrame(i)->m_Data.m_Time); }
//			{ for(int i = 0; i < nKeys; i++) GetFrame(i)->m_Data.m_Sound.Write(_pF); }
//			for(int p = 0; p < ANIM_DATAKEY_NUMPARAMS; p++) 
//				{ for(int i = 0; i < nKeys; i++) _pF->WriteLE(GetFrame(i)->m_Data.m_EventParams[p]); }
//			{ for(int i = 0; i < nKeys; i++) _pF->WriteLE(GetFrame(i)->m_Data.m_iDialogue); }
//			{ for(int i = 0; i < nKeys; i++) _pF->WriteLE(GetFrame(i)->m_Data.m_Volume); }
//			{
//				for(int r = 0; r < nRot; r++)
//					for(int i = 0; i < nKeys; i++) GetRotFrame(i)[r].Write(_pF); 
//			}
//			{
//				for(int m = 0; m < nMove; m++)
//					for(int i = 0; i < nKeys; i++) GetMoveFrame(i)[m].Write(_pF);
//			}
//		}
//*/
//		uint32 nKeys = m_lspKeys.Len(); _pF->WriteLE(nKeys);
//		if (nKeys)
//		{
///*			uint16 nRot = GetFrame(0)->m_lRotKeys.Len();
//			uint16 nMove = GetFrame(0)->m_lMoveKeys.Len();
//			_pF->WriteLE(nRot);
//			_pF->WriteLE(nMove);*/
//
//			for(int i = 0; i < nKeys; i++)
//				m_lspKeys[i]->Write(_pF, WriteInfo);
//		}
//
//	}
//}

void CXR_Anim_Sequence::RenderGraph(const char* _pFileName)
{
	MAUTOSTRIP(CXR_Anim_Sequence_RenderGraph, MAUTOSTRIP_VOID);
/*	CImage Img;
	fp32 h = 512;
	Img.Create(1024, h*2, IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
	Img.Fill(Img.GetClipRect(), 0);
	Img.Line(Img.GetClipRect(), CPnt(0, h), CPnt(640, h), 0x3f3f3f);

	CPixel32 Colors[4];
	Colors[0] = 0xff0000;
	Colors[1] = 0x00ff00;
	Colors[2] = 0x0000ff;
	Colors[3] = 0x3fffff;

	{
		fp32 t = 0;
		for(int i = 0; i < m_lspKeys.Len(); i++)
		{
			Img.Line(Img.GetClipRect(), CPnt(t*100.0, 0), CPnt(t*100.0, h*2), 0x7f7f7f);
			t += m_lspKeys[i]->m_Data.m_Time;
		}
	}

	CQuatfp32 Quat[32];
	for(int x = 0; x < Img.GetWidth(); x++)
	{
		fp32 t = x * 0.01f;
		EvalRot(t, Quat, 32);

		for(int i = 0; i < 4; i++)
		{
			fp32 y = h + Quat[5].k[i] * h;
			Img.SetPixel(Img.GetClipRect(), CPnt(x, y), Colors[i]);
		}
	}

	Img.Write(CRct(0,0,-1,-1), _pFileName);*/
}

void CXR_Anim_Sequence::Initialize()
{
	CXR_Anim_SequenceData::Initialize();
}

void CXR_Anim_Sequence::CalculateTrackmask(CXR_Anim_TrackMask &_TrackMask)
{
	m_TrackMask.Clear();
	if(m_lspKeys.Len() == 0)
		return;

	CQuatfp32 QUnit;
	QUnit.Unit();
	TArrayPtr<spCXR_Anim_Keyframe> lspKeys = m_lspKeys;
	CXR_Anim_Keyframe *pKey0 = m_lspKeys[0];
	int nRot = pKey0->m_lRotKeys.Len();
	for(int i = 0; i < nRot; i++)
	{
		int k;
		for(k = 0; k < lspKeys.Len(); k++)
			if(memcmp(&lspKeys[k]->m_lRotKeys[i].m_Rot, &QUnit, sizeof(QUnit)) != 0)
			{
				m_TrackMask.m_TrackMaskRot.Enable(i);
				break;
			}
	}
	int nMove = pKey0->m_lMoveKeys.Len();
	for(int i = 0; i < nMove; i++)
	{
		int k;
		for(k = 0; k < lspKeys.Len(); k++)
			if(lspKeys[k]->m_lMoveKeys[i].m_Move != CVec3Dfp32(0))
			{
				m_TrackMask.m_TrackMaskMove.Enable(i);
				break;
			}
	}
}

void CXR_Anim_Sequence::operator= (const CXR_Anim_Sequence& _Sequence)
{
	MAUTOSTRIP(CXR_Anim_Sequence_operator_assign, MAUTOSTRIP_VOID);
	m_AbsTimeFlag = _Sequence.m_AbsTimeFlag;
	m_Flags = _Sequence.m_Flags;
	m_TimeCode = _Sequence.m_TimeCode;
#ifndef	PLATFORM_CONSOLE
	m_Name = _Sequence.m_Name;
	m_Comment = _Sequence.m_Comment;
#endif
	m_Duration = _Sequence.m_Duration;
	m_lspKeys.SetLen(_Sequence.m_lspKeys.Len());
	for(int i = 0; i < _Sequence.m_lspKeys.Len(); i++)
		m_lspKeys[i] = _Sequence.m_lspKeys[i]->Duplicate();

	m_Events = _Sequence.m_Events;
	m_iNextAnimSequenceLayer = _Sequence.m_iNextAnimSequenceLayer;
}

spCXR_Anim_Sequence CXR_Anim_Sequence::Duplicate() const
{
	MAUTOSTRIP(CXR_Anim_Sequence_Duplicate, NULL);
	spCXR_Anim_Sequence spSequence = MNew(CXR_Anim_Sequence);

	*spSequence = *this;

	return spSequence;
}


CXR_Anim_DataKey_Sequence CXR_Anim_Sequence::GetDataKeys() const
{
	return CXR_Anim_DataKey_Sequence(m_Events, 0, m_Events.GetKeys().Len());
}


// =========================================================================================
// CXR_Anim_SequenceTracks
// (CXR_Anim_MoveTrack,CXR_Anim_RotTrack)
// =========================================================================================

CXR_Anim_RotTrack::CXR_Anim_RotTrack()
{
}

void CXR_Anim_RotTrack::Clear()
{
	m_lData.Clear();
	m_liTimes.Clear();
	m_liKeys.Clear();
}

const CXR_Anim_RotKey* CXR_Anim_RotTrack::GetKeyAndTimeindex(const int _iKey,int& _iTime) const
{
	/* *** Not needed ***
	if (_iKey < 0)
	{
		_iTime = 0;
		return(NULL);
	}
	else if (_iKey >= m_liKeys.Len())
	{
		uint16 index = m_liKeys[ m_liKeys.Len()-1];
		_iTime = m_liTimes[index];
		return(NULL);
	}
	else
	*/

	{
		uint16 index = m_liKeys[_iKey];
		_iTime = m_liTimes[index];
		return(&(m_lData[index]));
	}
}

// Similar to GetKeyAndTimeindex above but returns a key offset by _Offset
const CXR_Anim_RotKey* CXR_Anim_RotTrack::GetKeyAndTimeindex_Offset(const int _iKey,const int _Offset,int& _iTime) const
{
	int16 index = m_liKeys[_iKey] + _Offset;
	if (index < 0)
	{
		_iTime = m_liTimes[0];
		return(NULL);
	}
	else if (index >= m_liTimes.Len())
	{
		_iTime = m_liTimes[m_liTimes.Len()-1];
		return(NULL);
	}
	else
	{
		_iTime = m_liTimes[index];
		return(&(m_lData[index]));
	}
}

void CXR_Anim_RotTrack::operator= (const CXR_Anim_RotTrack& _Track)
{
	Clear();
	int i;
	m_lData.SetLen( _Track.m_lData.Len() );
	m_liTimes.SetLen( _Track.m_liTimes.Len() );
	m_liKeys.SetLen( _Track.m_liKeys.Len() );

	//TODO: Take BasePtr from the above arrays and use those so we dont use the operator [] for each instance
	for (i = 0; i < _Track.m_lData.Len(); i++)
	{
		m_lData[i]	= _Track.m_lData[i];
	}

	for (i = 0; i < _Track.m_liTimes.Len(); i++)
	{
		m_liTimes[i]	= _Track.m_liTimes[i];
	}

	for (i = 0; i < _Track.m_liKeys.Len(); i++)
	{
		m_liKeys[i]	= _Track.m_liKeys[i];
	}
}

void CXR_Anim_RotTrack::Read(CCFile* _pF, int _ReadFlags)
{
	Clear();
	//	Read version
	int32 ver;
	_pF->ReadLE(ver);
	if (ver != ANIM_ROTTRACKVERSION_100)
	{
		// *** Error("CXR_Anim_RotTrack::Read", "Wrong version");
		return;
	}

	//	Read m_liTimes.Len()
	//	Read m_liTimes
	uint16 nTimes;
	_pF->ReadLE(nTimes);
	m_liTimes.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		_pF->ReadLE(m_liTimes[iTime]);
	}

	//	Read m_liKeys.Len()
	//	Read m_liKeys
	uint16 nKeys;
	_pF->ReadLE(nKeys);
	m_liKeys.SetLen(nKeys);
	for (int iKey = 0; iKey < nKeys; iKey++)
	{
		_pF->ReadLE(m_liKeys[iKey]);
	}

	//	Write m_lData.Len()
	uint16 nData;
	_pF->ReadLE(nData);
	m_lData.SetLen(nData);
	for (int iData = 0; iData < nData; iData++)
	{
		m_lData[iData].Read(_pF,DEFAULT_ROTKEY_VERSION);
	}
}

void CXR_Anim_RotTrack::Write(CCFile* _pF)
{
#ifndef	PLATFORM_CONSOLE
	//	Write version
	_pF->WriteLE(int32(ANIM_ROTTRACKVERSION_100));
	//	Write m_liTimes.Len()
	//	Write m_liTimes
	_pF->WriteLE(uint16(m_liTimes.Len()));
	for (int iTime = 0; iTime < m_liTimes.Len(); iTime++)
	{
		_pF->WriteLE(m_liTimes[iTime]);
	}
	//	Write m_liKeys.Len()
	//	Write m_liKeys
	_pF->WriteLE(uint16(m_liKeys.Len()));
	for (int iKey = 0; iKey < m_liKeys.Len(); iKey++)
	{
		_pF->WriteLE(m_liKeys[iKey]);
	}
	//	Write m_lData.Len()
	_pF->WriteLE(uint16(m_lData.Len()));
	for (int iData = 0; iData < m_lData.Len(); iData++)
	{
		m_lData[iData].Write(_pF,DEFAULT_ROTKEY_VERSION);
	}
#endif
}

CXR_Anim_MoveTrack::CXR_Anim_MoveTrack()
{
}

void CXR_Anim_MoveTrack::Clear()
{
	m_lData.Clear();
	m_liTimes.Clear();
	m_liKeys.Clear();
}

const CXR_Anim_MoveKey* CXR_Anim_MoveTrack::GetKeyAndTimeindex(const int _iKey,int& _iTime) const
{
	/* *** Not needed ***
	if (_iKey < 0)
	{
		_iTime = 0.0f;
		return(NULL);
	}
	else if (_iKey >= m_liKeys.Len())
	{
		uint16 index = m_liKeys[ m_liKeys.Len()-1];
		_iTime = m_liTimes[index];
		return(NULL);
	}
	else
	*/
	{
		uint16 index = m_liKeys[_iKey];
		_iTime = m_liTimes[index];
		return(&(m_lData[index]));
	}
}

// Similar to GetKeyAndTimeindex above but returns a key offset by _Offset
const CXR_Anim_MoveKey* CXR_Anim_MoveTrack::GetKeyAndTimeindex_Offset(const int _iKey,const int _Offset,int& _iTime) const
{
	int16 index = m_liKeys[_iKey] + _Offset;
	if (index < 0)
	{
		_iTime = m_liTimes[0];
		return(NULL);
	}
	else if (index >= m_liTimes.Len())
	{
		_iTime = m_liTimes[m_liTimes.Len()-1];
		return(NULL);
	}
	else
	{
		_iTime = m_liTimes[index];
		return(&(m_lData[index]));
	}
}

void CXR_Anim_MoveTrack::operator= (const CXR_Anim_MoveTrack& _Track)
{
	Clear();
	int i;
	m_lData.SetLen( _Track.m_lData.Len() );
	m_liTimes.SetLen( _Track.m_liTimes.Len() );
	m_liKeys.SetLen( _Track.m_liKeys.Len() );

	//TODO: Take BasePtr from the above arrays and use those so we dont use the operator [] for each instance
	for (i = 0; i < _Track.m_lData.Len(); i++)
	{
		m_lData[i]	= _Track.m_lData[i];
	}

	for (i = 0; i < _Track.m_liTimes.Len(); i++)
	{
		m_liTimes[i]	= _Track.m_liTimes[i];
	}

	for (i = 0; i < _Track.m_liKeys.Len(); i++)
	{
		m_liKeys[i]	= _Track.m_liKeys[i];
	}
}

void CXR_Anim_MoveTrack::Read(CCFile* _pF, int _ReadFlags)
{
	Clear();
	//	Read version
	int32 ver;
	_pF->ReadLE(ver);
	if (ver != ANIM_MOVETRACKVERSION_100)
	{
		// *** Error("CXR_Anim_RotTrack::Read",CStr("Wrong verion"));
		return;
	}

	//	Read m_liTimes.Len()
	//	Read m_liTimes
	uint16 nTimes;
	_pF->ReadLE(nTimes);
	m_liTimes.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		_pF->ReadLE(m_liTimes[iTime]);
	}

	//	Read m_liKeys.Len()
	//	Read m_liKeys
	uint16 nKeys;
	_pF->ReadLE(nKeys);
	m_liKeys.SetLen(nKeys);
	for (int iKey = 0; iKey < nKeys; iKey++)
	{
		_pF->ReadLE(m_liKeys[iKey]);
	}

	//	Write m_lData.Len()
	uint16 nData;
	_pF->ReadLE(nData);
	m_lData.SetLen(nData);
	for (int iData = 0; iData < nData; iData++)
	{
		m_lData[iData].Read(_pF,0x0104);
	}
}

void CXR_Anim_MoveTrack::Write(CCFile* _pF)
{
#ifndef	PLATFORM_CONSOLE
	//	Write version
	_pF->WriteLE(int32(ANIM_MOVETRACKVERSION_100));
	//	Write m_liTimes.Len()
	//	Write m_liTimes
	_pF->WriteLE(uint16(m_liTimes.Len()));
	for (int iTime = 0; iTime < m_liTimes.Len(); iTime++)
	{
		_pF->WriteLE(m_liTimes[iTime]);
	}
	//	Write m_liKeys.Len()
	//	Write m_liKeys
	_pF->WriteLE(uint16(m_liKeys.Len()));
	for (int iKey = 0; iKey < m_liKeys.Len(); iKey++)
	{
		_pF->WriteLE(m_liKeys[iKey]);
	}
	//	Write m_lData.Len()
	_pF->WriteLE(uint16(m_lData.Len()));
	for (int iData = 0; iData < m_lData.Len(); iData++)
	{
		m_lData[iData].Write(_pF);
	}
#endif
}

// =========================================================================================
// CXR_Anim_SequenceTracks
// =========================================================================================
CXR_Anim_SequenceTracks::CXR_Anim_SequenceTracks()
{
}

CXR_Anim_SequenceTracks::~CXR_Anim_SequenceTracks()
{
	Clear();
}

void CXR_Anim_SequenceTracks::Clear()
{
	m_Events.Clear();
	m_lTimes.Clear();

	m_lRotTracks.Clear();
	m_lMoveTracks.Clear();
}

spCXR_Anim_SequenceTracks CXR_Anim_SequenceTracks::Duplicate() const
{
	spCXR_Anim_SequenceTracks spSeq = MNew(CXR_Anim_SequenceTracks);

	spSeq->m_Events = m_Events;

	int nTimes = m_lTimes.Len();
	spSeq->m_lTimes.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		spSeq->m_lTimes[iTime] = m_lTimes[iTime];
	}

	int nRots = m_lRotTracks.Len();
	spSeq->m_lRotTracks.SetLen(nRots);
	for (int iRot = 0; iRot < nRots; iRot++)
	{
		spSeq->m_lRotTracks[iRot] = m_lRotTracks[iRot];
	}

	int nMoves = m_lMoveTracks.Len();
	spSeq->m_lMoveTracks.SetLen(nMoves);
	for (int iMove = 0; iMove < nMoves; iMove++)
	{
		spSeq->m_lMoveTracks[iMove] = m_lMoveTracks[iMove];
	}

	return(spSeq);
}

void CXR_Anim_SequenceTracks::operator= (const CXR_Anim_SequenceTracks& _Seq)
{
	Clear();

	m_Events = _Seq.m_Events;

	int nTimes = _Seq.m_lTimes.Len();
	m_lTimes.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		m_lTimes[iTime] = _Seq.m_lTimes[iTime];
	}

	int nRots = _Seq.m_lRotTracks.Len();
	m_lRotTracks.SetLen(nRots);
	for (int iRot = 0; iRot < nTimes; iRot++)
	{
		m_lRotTracks[iRot] = _Seq.m_lRotTracks[iRot];
	}

	int nMoves = _Seq.m_lMoveTracks.Len();
	m_lMoveTracks.SetLen(nMoves);
	for (int iMove = 0; iMove < nTimes; iMove++)
	{
		m_lMoveTracks[iMove] = _Seq.m_lMoveTracks[iMove];
	}
}

void CXR_Anim_SequenceTracks::Initialize()
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_Initialize, MAUTOSTRIP_VOID);
	int nKeys = GetNumKeys();
	if (nKeys > 0)
	{
		m_Duration = m_lTimes[nKeys - 1];
	}
	else
	{
		m_Duration = 0.0f;
	}

	if((m_Flags & ANIM_SEQFLAGS_LOOPTYPE_MASK) >> ANIM_SEQFLAGS_LOOPTYPE_SHIFT == ANIM_SEQ_LOOPTYPE_CONTINUOUS)
		m_Flags |= ANIM_SEQFLAGS_LOOP;

	AlignRotations();
}

void CXR_Anim_SequenceTracks::AlignRotations()
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_AlignRotations, MAUTOSTRIP_VOID);

	int nRotTracks = m_lRotTracks.Len();
	for (int iTrack = 0; iTrack < nRotTracks ; iTrack++)
	{
		int nRotKeys = m_lRotTracks[iTrack].m_lData.Len();
		CQuatfp32 prev,cur;
		for(int iRot = 0; iRot < nRotKeys-1; iRot++)
		{
			m_lRotTracks[iTrack].m_lData[iRot].GetRot(prev);
			m_lRotTracks[iTrack].m_lData[iRot+1].GetRot(cur);
			if (prev.DotProd(cur) < 0.0f)
			{	
				cur.k[0] = -cur.k[0];
				cur.k[1] = -cur.k[1];
				cur.k[2] = -cur.k[2];
				cur.k[3] = -cur.k[3];
				m_lRotTracks[iTrack].m_lData[iRot+1].SetRot(cur);
			}
		}
	}
}

// ConvertSequence uses the data _Seq to fill its own data structures
void CXR_Anim_SequenceTracks::ConvertSequence(const CXR_Anim_Sequence* _Seq)
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_ConvertSequence, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_SequenceTracks::ConvertSequence);
	Clear();

	// First we copy the respective members of the baseclass
	m_Flags = _Seq->m_Flags;
	m_AbsTimeFlag = _Seq->m_AbsTimeFlag;
	m_TimeCode = _Seq->m_TimeCode;
#ifndef	PLATFORM_CONSOLE
	m_Name = _Seq->m_Name;
	m_Comment = _Seq->m_Comment;
#endif	// PLATFORM_CONSOLE
	m_iNextAnimSequenceLayer = _Seq->m_iNextAnimSequenceLayer;

	int nKeys = _Seq->GetNumKeys();
	// Q: Can there be animations with zero keys to convert?
	// A: Apparantly, we'll handle it but we'll also spam the console 'coz this is a BAD thing
	int nRotTracks = 0;
	int nMoveTracks = 0;
	if (nKeys > 0)
	{
		nRotTracks = _Seq->GetFrame(0)->m_lRotKeys.Len();
		nMoveTracks = _Seq->GetFrame(0)->m_lMoveKeys.Len();
	}
	else
	{	
#ifndef	PLATFORM_CONSOLE
		ConOut(CStr("Old sequence ") + m_Name + CStr(" contains 0 keys!"));
#endif	// PLATFORM_CONSOLE
	}

	int maxTracks = Max(nRotTracks,nMoveTracks);

	m_lRotTracks.SetLen(nRotTracks);
	m_lMoveTracks.SetLen(nMoveTracks);

	int TimeBase = m_lTimes.Len();
//	int EventCount = m_lEvents.Len();
	
	m_lTimes.SetLen( TimeBase + nKeys );
//	m_lEvents.SetLen( EventCount );

	for (int iFrame = 0; iFrame < nKeys; iFrame++)
	{
		const CXR_Anim_Keyframe* pCurKey;
		pCurKey = _Seq->GetFrame(iFrame);
		int iCurTime = TimeBase + iFrame;
		fp32 curTime = pCurKey->m_AbsTime;
		m_lTimes[iCurTime]	= curTime;

		for (int iTrack = 0; iTrack < maxTracks; iTrack++)
		{
			if (iTrack < nRotTracks)
			{
				m_lRotTracks[iTrack].m_lData.SetLen( nKeys );
				m_lRotTracks[iTrack].m_liTimes.SetLen( nKeys );
				m_lRotTracks[iTrack].m_liKeys.SetLen( nKeys );

				const CXR_Anim_RotKey* pRot = &(pCurKey->m_lRotKeys[iTrack]);
				m_lRotTracks[iTrack].m_lData[iFrame]	= *pRot;
				m_lRotTracks[iTrack].m_liTimes[iFrame]	= iCurTime;
				m_lRotTracks[iTrack].m_liKeys[iFrame]	= iFrame;
			}

			if (iTrack < nMoveTracks)
			{
				m_lMoveTracks[iTrack].m_lData.SetLen( nKeys );
				m_lMoveTracks[iTrack].m_liTimes.SetLen( nKeys );
				m_lMoveTracks[iTrack].m_liKeys.SetLen( nKeys );

				const CXR_Anim_MoveKey* pMove = &(pCurKey->m_lMoveKeys[iTrack]);
				m_lMoveTracks[iTrack].m_lData[iFrame]	= *pMove;
				m_lMoveTracks[iTrack].m_liTimes[iFrame]	= iCurTime;
				m_lMoveTracks[iTrack].m_liKeys[iFrame]	= iFrame;
			}
		}
	}

	// Copy events
	m_Events = _Seq->m_Events;

	// Postprocess
}

spCXR_Anim_Sequence CXR_Anim_SequenceTracks::ConvertSequence()
{
	spCXR_Anim_Sequence spSeq = MNew(CXR_Anim_Sequence);
	spSeq->m_Flags = m_Flags;
	spSeq->m_AbsTimeFlag = m_AbsTimeFlag;
	spSeq->m_TimeCode = m_TimeCode;
#ifndef	PLATFORM_CONSOLE
	spSeq->m_Name = m_Name;
	spSeq->m_Comment = m_Comment;
#endif	// PLATFORM_CONSOLE
	spSeq->m_iNextAnimSequenceLayer = m_iNextAnimSequenceLayer;

	int nRotTracks = m_lRotTracks.Len();
	int nMoveTracks = m_lMoveTracks.Len();
	int nTimes = m_lTimes.Len();

	// Pseudo
	// Step through m_lTimes and for each time:
	//	Add keys if available, Eval if not available
	// After all keys have been added/Eval we go through
	// m_lEvents and add keys for every unique time, once again Eval
	// Done(?)

	spSeq->m_lspKeys.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		fp32 curTime = m_lTimes[iTime];
		CXR_Anim_Keyframe* pKeyframe = MNew(CXR_Anim_Keyframe);
		spSeq->m_lspKeys[iTime] = pKeyframe;

		pKeyframe->m_AbsTime = curTime;

		pKeyframe->m_lRotKeys.SetLen(nRotTracks);
		pKeyframe->m_lMoveKeys.SetLen(nMoveTracks);
		int iTrack;
		for (iTrack = 0; iTrack < nRotTracks; iTrack++)
		{
			int curIndex = m_lRotTracks[iTrack].m_liTimes[iTime];
			CQuatfp32 curQ;
			fp32 time = m_lTimes[curIndex];
			if (M_Fabs(time - curTime) < 0.001f)	// *** Epsilon? ***
			{
				m_lRotTracks[iTrack].m_lData[curIndex].GetRot(curQ);
			}
			else
			{	// We must LERP/Slerp a new value here
				EvalRotTrack(curTime,&curQ,iTrack);
			}
			pKeyframe->m_lRotKeys[iTrack].SetRot(curQ);
		}

		for (iTrack = 0; iTrack < nMoveTracks; iTrack++)
		{
			int curIndex = m_lMoveTracks[iTrack].m_liTimes[iTime];
			CVec3Dfp32 curM;
			fp32 time = m_lTimes[curIndex];
			if (M_Fabs(time - curTime) < 0.001f)	// *** Epsilon? ***
			{
				m_lMoveTracks[iTrack].m_lData[curIndex].GetMove(curM);
			}
			else
			{	// We must LERP/Slerp a new value here
				EvalMoveTrack(curTime,&curM,iTrack);
			}
			pKeyframe->m_lMoveKeys[iTrack].SetMove(curM);
		}
	}

	return(spSeq);
}


int CXR_Anim_SequenceTracks::GetNumKeys() const
{
	return(m_lTimes.Len());
}

fp32 CXR_Anim_SequenceTracks::GetFrametime(int _iFrame) const
{
	if (_iFrame < 0) {return(0.0f);}
	if (_iFrame >= m_lTimes.Len()) {return(m_Duration);}

	return(m_lTimes[_iFrame]);
}

/*
void CXR_Anim_SequenceTracks::AddFrame(spCXR_Anim_Keyframe _spFrame)
{
	const CXR_Anim_DataKey* pEvent = &_spFrame->m_Data;
	fp32 curTime = _spFrame->m_Data.m_AbsTime;
	int iCurTime = m_lTimes.Len();
	m_lTimes.SetLen( iCurTime + 1 );	//TODO: This sucks! don't allocate space 1 at a time
//	m_lTimes.Add(curTime );
	m_lTimes[iCurTime]	= curTime;

	// Check for event and add it
	if ((pEvent->m_EventParams[0] != 0)
		||(pEvent->m_EventParams[1] != 0)
		||(pEvent->m_EventParams[2] != 0)
		||(pEvent->m_EventParams[3] != 0)
		||(pEvent->m_iDialogue != 0)
		||(pEvent->m_Sound != ""))
	{
		int EventCount = m_lEvents.Len();
		m_lEvents.SetLen( EventCount + 1 );	//TODO: This sucks! don't allocate space 1 at a time
//		m_lEvents.Add(_spFrame->m_Data);
		m_lEvents[EventCount]	= _spFrame->m_Data;
	}

	int nRotTracks = _spFrame->m_lRotKeys.Len();
	int nMoveTracks = _spFrame->m_lMoveKeys.Len();
	int maxTracks = Max(nRotTracks,nMoveTracks);
	
	int RotBase = m_lRotTracks.Len();
	int MovBase = m_lMoveTracks.Len();
	
	m_lRotTracks.SetLen( RotBase + nRotTracks );
	m_lMoveTracks.SetLen( MovBase + nMoveTracks );
	
	for (int iTrack = 0; iTrack < maxTracks; iTrack++)
	{
		if (iTrack < nRotTracks)
		{
			const CXR_Anim_RotKey* pRot = &(_spFrame->m_lRotKeys[iTrack]);
			m_lRotTracks[iTrack].m_lData[RotBase+iTrack]	= *pRot;
			m_lRotTracks[iTrack].m_liTimes[RotBase+iTrack]	= iCurTime;
			m_lRotTracks[iTrack].m_liKeys[RotBase+iTrack]	= RotBase+iTrack;
//			m_lRotTracks[iTrack].m_lData.Add(*pRot);
//			m_lRotTracks[iTrack].m_liTimes.Add(iCurTime);
//			m_lRotTracks[iTrack].m_liKeys.Add(m_lRotTracks[iTrack].m_lData.Len()-1);
		}

		if (iTrack < nMoveTracks)
		{
			const CXR_Anim_MoveKey* pMove = &(_spFrame->m_lMoveKeys[iTrack]);
			m_lMoveTracks[iTrack].m_lData[MovBase+iTrack]	= *pMove;
			m_lMoveTracks[iTrack].m_liTimes[MovBase+iTrack]	= iCurTime;
			m_lMoveTracks[iTrack].m_liKeys[MovBase+iTrack]	= MovBase+iTrack;
//			m_lMoveTracks[iTrack].m_lData.Add(*pMove);
//			m_lMoveTracks[iTrack].m_liTimes.Add(iCurTime);
//			m_lMoveTracks[iTrack].m_liKeys.Add(m_lRotTracks[iTrack].m_lData.Len()-1);	// Should this _really_ be offseted by rottracks?
		}
	}
}
*/
// As CXR_Anim_SequenceTracks doesn't hold any CXR_Anim_Keyframe we have
// to create one
CXR_Anim_Keyframe* CXR_Anim_SequenceTracks::GetFrame(int _iFrm)
{
	M_ASSERT(false, "Deprecated (couldn't have worked, since the returned value would have been de-allocated at return)");
	Error_static("CXR_Anim_SequenceTracks::GetFrame", "Illegal call!");
	return 0;
}


const CXR_Anim_Keyframe* CXR_Anim_SequenceTracks::GetFrame(int _iFrm) const
{
	return(NULL);
}


bool CXR_Anim_SequenceTracks::GetFrameAndTimeFraction(fp32 _Time, int& _iFrame, fp32& _tFrac) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_GetFrameAndTimeFraction, 0.0f);

	int nFrames = GetNumKeys();
	if (nFrames == 0)
	{
		return false;
	}
	else if (nFrames == 1)
	{
		_iFrame = 0;
		_tFrac = 0.0f;
		return(true);
	}

	if (_Time <= 0.0f)
	{
		_iFrame = 0;
		_tFrac = 0.0f;
		return true;
	}

	// Note: We never return the last key as it should only be used
	// for interpolation.
	if (_Time >= m_Duration)
	{
		if ((nFrames - 2) >= 0)
		{
			_iFrame = nFrames - 2;
			_tFrac = 0.9999f;
			return true;
		}
		else
		{
			_iFrame = 0;
			_tFrac = 0.9999f;
			return false;
		}
	}
	
	int i,high,low;
	fp32 begin,end;
	// We make an educated guess at where the key is assuming even spacing of keys
	i = TruncToInt((nFrames - 2) * _Time / m_Duration);
	for (low = (-1), high = nFrames-1; high - low > 1;  )
	{
		begin = m_lTimes[i];
		end = m_lTimes[i+1];

		if ((begin <= _Time) && (end > _Time))
		{
			_tFrac = (_Time - begin) / (end - begin);
			_iFrame = i;
			return(true);
		}
		if (begin > _Time)
		{
			high = i;
		}
		else
		{
			low = i;
		}
		i = (high+low) / 2;
	}

	_iFrame = 0;
	return(false);
}

fp32 CXR_Anim_SequenceTracks::GetFrameDuration(int _iFrame) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_GetFrameDuration, 0.0f);
	if ((_iFrame >= 0) && (_iFrame < m_lTimes.Len()-1))
	{
		return(m_lTimes[_iFrame+1] - m_lTimes[_iFrame]);
	}
	else
	{
		return(0.0f);
	}
}

// Get the duration of a particular frame of a particular track
// Returns 0 if _iFrame < 0 or the last key+ of the track
fp32 CXR_Anim_SequenceTracks::GetFrameDuration(int _iFrame,int _iTrack,bool _RotTrack)
{
	if (_RotTrack == true)
	{
		if ((_iFrame >= 0) && (_iFrame < m_lRotTracks[_iTrack].m_lData.Len()-1))
		{
			fp32 rt = m_lTimes[m_lRotTracks[_iTrack].m_liTimes[_iFrame+1]]
					- m_lTimes[m_lRotTracks[_iTrack].m_liTimes[_iFrame]];
			return(rt);
		}
		else
		{
			return(0.0f);
		}
	}
	else
	{
		if ((_iFrame >= 0) && (_iFrame < m_lMoveTracks[_iTrack].m_lData.Len()-1))
		{
			fp32 rt = m_lTimes[m_lMoveTracks[_iTrack].m_liTimes[_iFrame+1]]
					- m_lTimes[m_lMoveTracks[_iTrack].m_liTimes[_iFrame]];
			return(rt);
		}
		else
		{
			return(0.0f);
		}
	}
}


void CXR_Anim_SequenceTracks::Eval(fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_Eval, MAUTOSTRIP_VOID);

	int iTime;
	fp32 Fraction;
	int nRotTracks = Min(_nRot,m_lRotTracks.Len());
	int nMoveTracks = Min(_nMove,m_lMoveTracks.Len());
//	int nTracks = Max(nRotTracks,nMoveTracks);

	// Get the time index of the master timetrack
	GetFrameAndTimeFraction(_Time,iTime,Fraction);

	// We do it separately for move and rot tracks
	// (This saves us some branches)
	int iTrack;
	for (iTrack = 0; iTrack < nRotTracks; iTrack++)
	{
		const CXR_Anim_RotTrack* pTrack = &m_lRotTracks[iTrack];
		if (pTrack->m_lData.Len() > 1)
		{
			int it0,it1,it2,it3;
			const CXR_Anim_RotKey* pR1 = pTrack->GetKeyAndTimeindex(iTime,it1);
			const CXR_Anim_RotKey* pR0 = pTrack->GetKeyAndTimeindex_Offset(iTime,-1,it0);
			const CXR_Anim_RotKey* pR2 = pTrack->GetKeyAndTimeindex_Offset(iTime,1,it2);
			const CXR_Anim_RotKey* pR3 = pTrack->GetKeyAndTimeindex_Offset(iTime,2,it3);

			fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
			fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
			fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];

			/* *** Remove when debugged ***
			if (Duration12 <= 0.00001f)
			{
				int nliKeys = pTrack->m_liKeys.Len();
				int nliTimes = pTrack->m_liTimes.Len();
				int nlData = pTrack->m_lData.Len();
				uint16* pliKeys= pTrack->m_liKeys.GetBasePtr();
				bool debug = true;	// ***
			}
			*/

			Fraction = (_Time - m_lTimes[it1]) / (Duration12);
			QuaternionSpline(pR0, pR1, pR2, pR3,&_pRot[iTrack], Fraction, 
					Duration01, Duration12, Duration23, 1, NULL);
		}
		else
		{
			pTrack->m_lData[0].GetRot(_pRot[iTrack]);
		}
	}
	for (iTrack = 0; iTrack < nMoveTracks; iTrack++)
	{
		const CXR_Anim_MoveTrack* pTrack = &m_lMoveTracks[iTrack];
		if (pTrack->m_lData.Len() > 1)
		{
			int it0,it1,it2,it3;
			const CXR_Anim_MoveKey* pM1 = pTrack->GetKeyAndTimeindex(iTime,it1);
			const CXR_Anim_MoveKey* pM0 = pTrack->GetKeyAndTimeindex_Offset(iTime,-1,it0);
			const CXR_Anim_MoveKey* pM2 = pTrack->GetKeyAndTimeindex_Offset(iTime,1,it2);
			const CXR_Anim_MoveKey* pM3 = pTrack->GetKeyAndTimeindex_Offset(iTime,2,it3);
			
			fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
			fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
			fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];

			/* *** Remove when debugged ***
			if (Duration12 <= 0.00001f)
			{
				int nliKeys = pTrack->m_liKeys.Len();
				int nliTimes = pTrack->m_liTimes.Len();
				int nlData = pTrack->m_lData.Len();
				uint16* pliKeys= pTrack->m_liKeys.GetBasePtr();
				bool debug = true;	// ***
			}
			*/

			Fraction = (_Time - m_lTimes[it1]) / (Duration12);
			VectorSpline(pM0, pM1, pM2, pM3,&_pMove[iTrack], Fraction, 
					Duration01, Duration12, Duration23, 1, NULL);
		}
		else
		{
			_pMove[iTrack]=pTrack->m_lData[0].GetMove();
		}
	}

	// Set undefined slots to 0.
	int i;
	for (i = m_lMoveTracks.Len(); i < _nMove; i++)
		_pMove[i] = M_VConst(0,0,0,1);

	// Set undefined slots to unit.
	for (i = m_lRotTracks.Len(); i < _nRot; i++)
		_pRot[i].Unit();
}

void CXR_Anim_SequenceTracks::EvalTrack0(fp32 _Time, vec128& _Move0, CQuatfp32& _Rot0) const
{
	Eval(_Time,&_Rot0,1,&_Move0,1);
}

void CXR_Anim_SequenceTracks::EvalRot(fp32 _Time,CQuatfp32* _pRot,int _nRot) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_EvalRot, MAUTOSTRIP_VOID);

	int iTime;
	fp32 Fraction;
	int nTracks = Min(_nRot,m_lRotTracks.Len());

	// Get the time index of the master timetrack
	GetFrameAndTimeFraction(_Time,iTime,Fraction);

	for (int iTrack = 0; iTrack < nTracks; iTrack++)
	{
		const CXR_Anim_RotTrack* pTrack = &m_lRotTracks[iTrack];
		if (pTrack->m_lData.Len() > 1)
		{
			int it0,it1,it2,it3;
			const CXR_Anim_RotKey* pR1 = pTrack->GetKeyAndTimeindex((int)_Time,it1);
			const CXR_Anim_RotKey* pR0 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,-1,it0);
			const CXR_Anim_RotKey* pR2 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,1,it2);
			const CXR_Anim_RotKey* pR3 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,2,it3);
			
			fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
			fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
			fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];
			Fraction = (_Time - m_lTimes[it1]) / (Duration12);
			QuaternionSpline(pR0, pR1, pR2, pR3,&_pRot[iTrack], Fraction, 
					Duration01, Duration12, Duration23, 1, NULL);
		}
		else
		{
			pTrack->m_lData[0].GetRot(_pRot[iTrack]);
		}
	}

	// Set undefined slots to unit.
	for (int i = m_lRotTracks.Len(); i < _nRot; i++)
		_pRot[i].Unit();
}

void CXR_Anim_SequenceTracks::EvalRotTrack(fp32 _Time,CQuatfp32* _pDest,int _iTrack) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_EvalRotTrack, MAUTOSTRIP_VOID);

	int iTime;
	fp32 Fraction;

	// Get the time index of the master timetrack
	GetFrameAndTimeFraction(_Time,iTime,Fraction);

	const CXR_Anim_RotTrack* pTrack = &m_lRotTracks[_iTrack];
	if (pTrack->m_lData.Len() > 1)
	{
		int it0,it1,it2,it3;
		const CXR_Anim_RotKey* pR1 = pTrack->GetKeyAndTimeindex((int)_Time,it1);
		const CXR_Anim_RotKey* pR0 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,-1,it0);
		const CXR_Anim_RotKey* pR2 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,1,it2);
		const CXR_Anim_RotKey* pR3 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,2,it3);
		
		fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
		fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
		fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];
		Fraction = (_Time - m_lTimes[it1]) / (Duration12);
		QuaternionSpline(pR0, pR1, pR2, pR3,&_pDest[0], Fraction, 
				Duration01, Duration12, Duration23, 1, NULL);
		}
	else
	{
		pTrack->m_lData[0].GetRot(_pDest[0]);
	}
}


void CXR_Anim_SequenceTracks::EvalMove(fp32 _Time,CVec3Dfp32* _pMove,int _nMove) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_Eval, MAUTOSTRIP_VOID);

	int iTime;
	fp32 Fraction;
	int nTracks = Min(_nMove,m_lMoveTracks.Len());

	// Get the time index of the master timetrack
	GetFrameAndTimeFraction(_Time,iTime,Fraction);

	for (int iTrack = 0; iTrack < nTracks; iTrack++)
	{
		const CXR_Anim_MoveTrack* pTrack = &m_lMoveTracks[iTrack];
		if (pTrack->m_lData.Len() > 1)
		{
			int it0,it1,it2,it3;
			const CXR_Anim_MoveKey* pM1 = pTrack->GetKeyAndTimeindex((int)_Time,it1);
			const CXR_Anim_MoveKey* pM0 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,-1,it0);
			const CXR_Anim_MoveKey* pM2 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,1,it2);
			const CXR_Anim_MoveKey* pM3 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,2,it3);
			
			fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
			fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
			fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];
			Fraction = (_Time - m_lTimes[it1]) / (Duration12);
			VectorSpline(pM0, pM1, pM2, pM3,&_pMove[iTrack], Fraction, 
					Duration01, Duration12, Duration23, 1, NULL);
		}
		else
		{
			pTrack->m_lData[0].GetMove(_pMove[iTrack]);
		}
	}

	// Set undefined slots to 0.
	for (int i = m_lMoveTracks.Len(); i < _nMove; i++)
		_pMove[i] = 0;
}

void CXR_Anim_SequenceTracks::EvalMoveTrack(fp32 _Time,CVec3Dfp32* _pDest,int _iTrack) const
{
	MAUTOSTRIP(CXR_Anim_SequenceTracks_EvalMoveTrack, MAUTOSTRIP_VOID);

	int iTime;
	fp32 Fraction;

	// Get the time index of the master timetrack
	GetFrameAndTimeFraction(_Time,iTime,Fraction);

	const CXR_Anim_MoveTrack* pTrack = &m_lMoveTracks[_iTrack];
	if (pTrack->m_lData.Len() > 1)
	{
		int it0,it1,it2,it3;
		const CXR_Anim_MoveKey* pM1 = pTrack->GetKeyAndTimeindex((int)_Time,it1);
		const CXR_Anim_MoveKey* pM0 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,-1,it0);
		const CXR_Anim_MoveKey* pM2 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,1,it2);
		const CXR_Anim_MoveKey* pM3 = pTrack->GetKeyAndTimeindex_Offset((int)_Time,2,it3);
		
		fp32 Duration01 = m_lTimes[it1] - m_lTimes[it0];
		fp32 Duration12 = m_lTimes[it2] - m_lTimes[it1];
		fp32 Duration23 = m_lTimes[it3] - m_lTimes[it2];
		Fraction = (_Time - m_lTimes[it1]) / (Duration12);
		VectorSpline(pM0, pM1, pM2, pM3,&_pDest[0], Fraction, 
				Duration01, Duration12, Duration23, 1, NULL);
	}
	else
	{
		pTrack->m_lData[0].GetMove(_pDest[0]);
	}
}

void CXR_Anim_SequenceTracks::GetTotalTrack0(vec128& _Move0, CQuatfp32& _Rot0) const
{
	int nMoveKeys = m_lMoveTracks[0].m_lData.Len();
	if (nMoveKeys > 0)
	{
		vec128 m0 = M_VLd_P3_Slow(&m_lMoveTracks[0].m_lData[nMoveKeys-1].m_Move);
		vec128 m1 = M_VLd_P3_Slow(&m_lMoveTracks[0].m_lData[0].m_Move);
		_Move0 = M_VSub(m0,m1);
	}
	else
	{
		_Move0 = M_VConst(0,0,0,1.0f);
	}

	int nRotKeys = m_lRotTracks[0].m_lData.Len();
	if (nRotKeys > 0)
	{
#ifdef USE_QUATERNION16
		m_lRotTracks[0].m_lData[0].m_Rot.GetQuatfp32(_Rot0);
		_Rot0.Inverse();
		CQuatfp32 Rot;
		m_lRotTracks[0].m_lData[nRotKeys-1].m_Rot.GetQuatfp32(Rot);
		Rot.Multiply(_Rot0, _Rot0);
#else
		_Rot0 = m_lRotTracks[0].m_lData[0].m_Rot;
		_Rot0.Inverse();
		m_lRotTracks[0].m_lData[nRotKeys-1].m_Rot.Multiply(_Rot0, _Rot0);
#endif
	}
	else
	{
		_Rot0.Unit();
	}
}

void CXR_Anim_SequenceTracks::Read(CCFile* _pF, int _ReadFlags)
{
	ReadData( _pF, _ReadFlags );
}

void CXR_Anim_SequenceTracks::ReadCompact(CCFile* _pF, int _ReadFlags)
{
	ReadData( _pF, _ReadFlags );
}

void CXR_Anim_SequenceTracks::Write(class CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo)
{
	WriteData( _pF );
}

uint CXR_Anim_SequenceTracks::ReadData(CCFile* _pF, int _ReadFlags)
{
	Clear();
	// PSEUDO
	// Read version
	int32 Ver;
	_pF->ReadLE(Ver);
	if (Ver != ANIM_SEQUENCETRACKVERSION_100 || Ver != ANIM_SEQUENCETRACK_VERSION)
	{	
		Error("CXR_Anim_SequenceTracks::ReadData","Wrong animation format");
		return Ver;
	}

	// Read baseclass data
	_pF->ReadLE(m_AbsTimeFlag);
	_pF->ReadLE(m_Flags);
	{
		CStr Name, Comment;
		Name.Read(_pF);
		Comment.Read(_pF);

#ifndef	PLATFORM_CONSOLE
		if (_ReadFlags & ANIM_READ_NONAMES) Name = "";
		if (_ReadFlags & ANIM_READ_NOCOMMENTS) Comment = "";
		m_Name	= Name;
		m_Comment	= Comment;
#endif	// PLATFORM_CONSOLE
	}

	if (Ver == ANIM_SEQUENCETRACKVERSION_100)
	{
		// Read m_lEvents.Len()
		uint16 nEvents;
		_pF->ReadLE(nEvents);
		// Read m_lEvents
		for (int iEvent = 0; iEvent < nEvents; iEvent++)
		{
			CXR_Anim_DataKey1 Tmp;
			Tmp.Read(_pF);
			m_Events.AddKey(Tmp);
		}
	}
	else
	{
		uint16 nDataKeyVersion;
		_pF->ReadLE(nDataKeyVersion);
		m_Events.Read(_pF, nDataKeyVersion);
	}

	CheckSequenceForBreakouts();

	// Read m_lTimes
	// Read m_lTimes.Len()
	uint16 nTimes;
	_pF->ReadLE(nTimes);
	m_lTimes.SetLen(nTimes);
	for (int iTime = 0; iTime < nTimes; iTime++)
	{
		_pF->ReadLE(m_lTimes[iTime]);
	}

	// Read m_lRotTracks.Len()
	// Read m_lRotTracks
	uint16 nRotTracks;
	_pF->ReadLE(nRotTracks);
	m_lRotTracks.SetLen(nRotTracks);
	for (int iRotTrack = 0; iRotTrack < nRotTracks; iRotTrack++)
	{
		m_lRotTracks[iRotTrack].Read(_pF);
	}

	// Read m_lMoveTracks.Len()
	// Read m_lMoveTracks
	uint16 nMoveTracks;
	_pF->ReadLE(nMoveTracks);
	m_lMoveTracks.SetLen(nMoveTracks);
	for (int iMoveTrack = 0; iMoveTrack < nMoveTracks; iMoveTrack++)
	{
		m_lMoveTracks[iMoveTrack ].Read(_pF);
	}
	return Ver;
}

void CXR_Anim_SequenceTracks::WriteData(CCFile* _pF)
{
#ifndef	PLATFORM_CONSOLE
	// PSEUDO
	// Write version
	_pF->WriteLE(int32(ANIM_SEQUENCETRACK_VERSION));

	// Write baseclass data
	_pF->WriteLE(m_AbsTimeFlag);
	_pF->WriteLE(m_Flags);
	m_Name.Write(_pF);
	m_Comment.Write(_pF);

	// Write Data keys
	_pF->WriteLE(uint16(ANIM_DATAKEY_VERSION));
	m_Events.Write(_pF);

	// Write m_lTimes.Len()
	// Write m_lTimes
	_pF->WriteLE(uint16(m_lTimes.Len()));
	for (int i = 0; i < m_lTimes.Len(); i++)
	{
		_pF->WriteLE(m_lTimes[i]);
	}

	// Write m_lRotTracks.Len()
	// Write m_lRotTracks
	_pF->WriteLE(uint16(m_lRotTracks.Len()));
	for (int i = 0; i < m_lRotTracks.Len(); i++)
	{
		m_lRotTracks[i].Write(_pF);
	}

	// Write m_lMoveTracks.Len()
	// Write m_lMoveTracks
	_pF->WriteLE(uint16(m_lMoveTracks.Len()));
	for (int i = 0; i < m_lMoveTracks.Len(); i++)
	{
		m_lMoveTracks[i].Write(_pF);
	}
#endif
}


CXR_Anim_DataKey_Sequence CXR_Anim_SequenceTracks::GetDataKeys() const
{
	return CXR_Anim_DataKey_Sequence(m_Events, 0, m_Events.GetKeys().Len());
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Base
|__________________________________________________________________________________________________
\*************************************************************************************************/

// -------------------------------------------------------------------
CXR_Anim_Base::CXR_Anim_Base()
{
	MAUTOSTRIP(CXR_Anim_Base_ctor, MAUTOSTRIP_VOID);
}

CXR_Anim_Base::~CXR_Anim_Base()
{
	MAUTOSTRIP(CXR_Anim_Base_dtor, MAUTOSTRIP_VOID);
	m_lspSequences.Clear();
}

void CXR_Anim_Base::AddSequence(spCXR_Anim_SequenceData _Seq)
{
	int Count = m_lspSequences.Len();
	m_lspSequences.SetLen( Count + 1 );
	m_lspSequences[Count]	= _Seq;
//	m_lspSequences.Add(_Seq);
}

spCXR_Anim_SequenceData CXR_Anim_Base::GetSequence(int _iSeq)
{
	MAUTOSTRIP(CXR_Anim_Base_GetSequence, NULL);
	return m_lspSequences.ValidPos(_iSeq) ? (m_lspSequences[_iSeq]) : spCXR_Anim_SequenceData ();
}

const spCXR_Anim_SequenceData CXR_Anim_Base::GetSequence(int _iSeq) const
{
	MAUTOSTRIP(CXR_Anim_Base_GetSequence_const, NULL);
	return m_lspSequences.ValidPos(_iSeq) ? (m_lspSequences[_iSeq]) : spCXR_Anim_SequenceData();
}


int CXR_Anim_Base::Read(const char* _pFileName, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_Read_2, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_pFileName);
	int Res = Read(&DFile, _ReadFlags);
	DFile.Close();
	m_ContainerName = _pFileName;
	return Res;
}

int CXR_Anim_Base::Read(CDataFile* _pDFile, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_Read, 0);
	while(1)
	{
		_pDFile->PushPosition();
		CStr NextEntry = _pDFile->GetNext();

		if (NextEntry == "")
		{
			break;
		}
		else if (NextEntry == "ANIMATIONTRACKSSET")
		{
			_pDFile->PopPosition();
			ReadTracks(_pDFile, _ReadFlags);
			return 3;
		}
		else if (NextEntry == "ANIMATIONSET")
		{
			_pDFile->PopPosition();
			ReadAnim(_pDFile, _ReadFlags);
			return 2;
		}
		else if (NextEntry == "COMPRESSEDANIMATIONSET")
		{
			_pDFile->PopPosition();
			ReadCompressed(_pDFile, _ReadFlags);
			return 1;
		}
		_pDFile->PopPosition();
		_pDFile->GetNext();
	}

	Error("ReadMultiFormat", CStrF("Invalid animation file, FileName %s ", _pDFile->GetFile()->GetFileName().Str()));
	return 0;
}

spCXR_Anim_Base CXR_Anim_Base::StripSequences(const uint8* _pSeqMask, int _nSeqMask)
{
	return NULL;
}

void CXR_Anim_Base::ReadTracks(CDataFile* _pDFile, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_ReadTracks, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_Base::ReadTracks);

	if (!_pDFile->GetNext("ANIMATIONTRACKSSET"))
	{
		Error("CXR_Anim::ReadTracks", "ANIMATIONTRACKSSET-entry not found.");
	}

	m_lspSequences.SetLen(_pDFile->GetUserData());
	if (!_pDFile->GetSubDir())
	{
		Error("CXR_Anim::ReadTracks","Invalid ANIMATIONTRACKSSET-entry.");
	}

	for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
	{
		if (!_pDFile->GetNext("SEQUENCE"))
		{
			Error("CXR_Anim::ReadTracks","SEQUENCE-entry expected.");
		}

		// Read sequence
		spCXR_Anim_SequenceTracks spSeq = MNew(CXR_Anim_SequenceTracks);
		if (!spSeq)
			MemError("Read");
		spSeq->Read(_pDFile->GetFile(), _ReadFlags);
		spSeq->Initialize();
		m_lspSequences[iSeq] = spSeq;
	}

	_pDFile->GetParent();
}

void CXR_Anim_Base::ReadAnim(CDataFile* _pDFile, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_ReadAnim, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_Base::ReadAnim);

	if (!_pDFile->GetNext("ANIMATIONSET"))
	{
		Error("CXR_Anim::ReadAnim", "ANIMATIONSET-entry not found.");
	}
	
	m_lspSequences.SetLen(_pDFile->GetUserData());

	if (!_pDFile->GetSubDir())
	{
		Error("CXR_Anim::ReadAnim","Invalid ANIMATIONSET-entry.");
	}

	for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
	{
		if (!_pDFile->GetNext("SEQUENCE"))
		{
			Error("CXR_Anim::ReadAnim","SEQUENCE-entry expected.");
		}

		spCXR_Anim_Sequence spSeq = MNew(CXR_Anim_Sequence);
		if (!spSeq) MemError("Read");
		spSeq->Read(_pDFile->GetFile(), _ReadFlags);
//	#endif
		spSeq->Initialize();
		m_lspSequences[iSeq] = spSeq;
	}

	_pDFile->GetParent();
}

void CXR_Anim_Base::ReadCompressed(CDataFile* _pDFile, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_ReadCompressed, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_Base::ReadCompressed);

	if (!_pDFile->GetNext("COMPRESSEDANIMATIONSET"))
	{
		Error("CXR_Anim::ReadCompressed", "COMPRESSEDANIMATIONSET-entry not found.");
	}

	m_lspSequences.SetLen(_pDFile->GetUserData());
	for(int iSeq = 0; iSeq < m_lspSequences.Len(); iSeq++)
	{
		// Read compressed sequence
		TArray<uint8> lCompressedSequence;
		uint32 Len = 0;
		_pDFile->GetFile()->ReadLE(Len);
		lCompressedSequence.SetLen(Len);
		_pDFile->GetFile()->Read(lCompressedSequence.GetBasePtr(),Len);
		spCCFile spFile = CDiskUtil::DecompressToMemoryFile(lCompressedSequence);

		spCXR_Anim_Sequence spSeq = MNew(CXR_Anim_Sequence); //Tracks;
		if (!spSeq) 
			MemError("CXR_Anim::Read");
		spSeq->ReadCompact(spFile, 0);
		spSeq->Initialize();
		m_lspSequences[iSeq] = spSeq;
	}

//if (m_lspSequences.Len() > 0x26) m_lspSequences[0x26]->RenderGraph();

	_pDFile->GetParent();
}

void CXR_Anim_Base::Write(CDataFile* _pDFile, const CXR_AnimWriteInfo _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_Base_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	_pDFile->BeginEntry("ANIMATIONSET");
	_pDFile->EndEntry(m_lspSequences.Len());

	_pDFile->BeginSubDir();
	for(int i = 0; i < m_lspSequences.Len(); i++)
	{
		_pDFile->BeginEntry("SEQUENCE");
		CXR_Anim_SequenceTracks* pSeq = TDynamicCast<CXR_Anim_SequenceTracks>((CXR_Anim_SequenceData*)m_lspSequences[i]);
		if (pSeq)
			pSeq->Write(_pDFile->GetFile(), _WriteInfo);
		else
		{
			CXR_Anim_Sequence* pSeq = TDynamicCast<CXR_Anim_Sequence>((CXR_Anim_SequenceData*)m_lspSequences[i]);
			if (pSeq)
				pSeq->Write(_pDFile->GetFile(), _WriteInfo);
			else
				Error("Write", "Internal error.");
		}

		// _pDFile->EndEntry(m_lspSequences[i]->m_lspKeys.Len());
		_pDFile->EndEntry(m_lspSequences[i]->GetNumKeys());
	}

	_pDFile->EndSubDir();
#endif	// PLATFORM_CONSOLE
}

void CXR_Anim_Base::Write(const char* _pFileName, const CXR_AnimWriteInfo _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_Base_Write_2, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CDataFile DFile;
	DFile.Create(_pFileName);
	Write(&DFile, _WriteInfo);
	DFile.Close();
#endif	// PLATFORM_CONSOLE
}

//
//void CXR_Anim::WriteCompressed(CDataFile* _pDFile, const CXR_AnimWriteInfo _WriteInfo)
//{
//	MAUTOSTRIP(CXR_Anim_WriteCompressed, MAUTOSTRIP_VOID);
//	_pDFile->BeginEntry("COMPRESSEDANIMATIONSET");
//
///*	int SumLZW = 0;
//	int SumLZWGIF = 0;
//	int SumLZSS = 0;*/
//	int SumUncompressed = 0;
//	int SumLZSSHuff = 0;
//
//	int iSeq = 0;
//	while(iSeq < m_lspSequences.Len())
//	{
//		TArray<uint8> lSeqFile;
//		spCCFile spFile = CDiskUtil::CreateCCFile(lSeqFile, CFILE_WRITE | CFILE_BINARY);
//
////		while(spFile->Length() < 4096 && iSeq < m_lspSequences.Len())
//		{
//			m_lspSequences[iSeq]->WriteCompact(spFile, _WriteInfo);
//			iSeq++;
//		}
//		lSeqFile.SetLen(spFile->Length());
///*		TArray<uint8> lSeqFileComprLZW = CDiskUtil::Compress(lSeqFile, LZW);
//		TArray<uint8> lSeqFileComprLZWGIF; // = CDiskUtil::Compress(lSeqFile, LZW_GIF);
//		TArray<uint8> lSeqFileComprLZSS = CDiskUtil::Compress(lSeqFile, LZSS);*/
//		TArray<uint8> lSeqFileComprLZSSHuff = CDiskUtil::Compress(lSeqFile, LZSS_HUFFMAN);
//
////LogFile(CStrF("Seq %d, %d => %d", iSeq, lSeqFile.Len(), lSeqFileComprLZSSHuff.Len()));
//
////LogFile(CStrF("Seq %d, %d => %d, %d, %d, %d", iSeq, lSeqFile.Len(), 
////	lSeqFileComprLZW.Len(), lSeqFileComprLZWGIF.Len(), lSeqFileComprLZSS.Len(), lSeqFileComprLZSSHuff.Len()));
//
///*	SumLZW += lSeqFileComprLZW.Len();
//	SumLZWGIF += lSeqFileComprLZWGIF.Len();
//	SumLZSS += lSeqFileComprLZSS.Len();*/
//
//		SumLZSSHuff += lSeqFileComprLZSSHuff.Len();
//		SumUncompressed += lSeqFile.Len();
//
//		uint32 Len = lSeqFileComprLZSSHuff.Len();
//		_pDFile->GetFile()->WriteLE(Len);
//		_pDFile->GetFile()->Write(lSeqFileComprLZSSHuff.GetBasePtr(), lSeqFileComprLZSSHuff.Len());
//	}
//	_pDFile->EndEntry(m_lspSequences.Len());
//
////LogFile(CStrF("TOTAL %d => %d", SumUncompressed, SumLZSSHuff));
//
//}

//void CXR_Anim::WriteCompressed(const char* _pFileName, const CXR_AnimWriteInfo _WriteInfo)
//{
//	MAUTOSTRIP(CXR_Anim_WriteCompressed_2, MAUTOSTRIP_VOID);
//	CDataFile DFile;
//	DFile.Create(_pFileName);
//	WriteCompressed(&DFile, _WriteInfo);
//	DFile.Close();
//}

void CXR_Anim_Base::operator= (const CXR_Anim_Base& _Anim)
{
	MAUTOSTRIP(CXR_Anim_Base_operator_assign, MAUTOSTRIP_VOID);
	m_ContainerName = _Anim.m_ContainerName;

//	m_lspSequences.SetLen(_Anim.m_lspSequences.Len());
//	for(int i = 0; i < m_lspSequences.Len(); i++)
//		m_lspSequences[i] = _Anim.m_lspSequences[i]->Duplicate();
}

spCXR_Anim_Base CXR_Anim_Base::Duplicate() const
{
	MAUTOSTRIP(CXR_Anim_Base_Duplicate, NULL);
	spCXR_Anim_Base spAnim = MNew(CXR_Anim_Base);

	*spAnim = *this;

	return spAnim;
}

const CXR_Anim_DataKey* CXR_Anim_Base::GetEvents(int _iSeq, CMTime &_BeginTime, const CMTime& _EndTime, int _Mask)
{
	MAUTOSTRIP(CXR_Anim_Base_GetEvents, NULL);
	MSCOPESHORT(CXR_Anim_Base::GetEvents);
	CXR_Anim_SequenceData *pSeq = GetSequence(_iSeq);
	if(!pSeq)
		return NULL;

	return pSeq->GetEvents(_BeginTime, _EndTime, _Mask);
}

#include "XRAnimCompressed.h"

spCXR_Anim_Base CXR_Anim_Base::ReadMultiFormat(class CDataFile* _pDFile, int _ReadFlags, const uint8* _pSeqMask, int _nSeqMask)
{
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("ANIMATIONSET"))
		{
			_pDFile->PopPosition();
			ReadAnim(_pDFile, _ReadFlags);
			return this;
		}
		_pDFile->PopPosition();
	}

	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("ANIMATIONTRACKSET"))
		{
			_pDFile->PopPosition();
			ReadTracks(_pDFile, _ReadFlags);
			return this;
		}
		_pDFile->PopPosition();
	}

	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("COMPRESSEDANIMATIONSET"))
		{
			_pDFile->PopPosition();
			ReadCompressed(_pDFile, _ReadFlags);
			return this;
		}
		_pDFile->PopPosition();
	}

	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("COMPRESSEDANIMATIONSET2"))
		{
			_pDFile->PopPosition();
			spCXR_Anim_Base spAnim = MNew(CXR_Anim_Compressed);
			spAnim->Read(_pDFile, _ReadFlags);

//			const uint8 lSeqMask[] = { 0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa };

			spCXR_Anim_Base spAnimStrip = spAnim->StripSequences(_pSeqMask, _nSeqMask);
//			spCXR_Anim_Base spAnimStrip = spAnim->StripSequences(lSeqMask, 32);

			if (spAnimStrip != NULL)
				spAnim = spAnimStrip;
			return spAnim;
		}
		_pDFile->PopPosition();
	}

	return NULL;
}

spCXR_Anim_Base CXR_Anim_Base::ReadMultiFormat(const char* _pFileName, int _ReadFlags, const uint8* _pSeqMask, int _nSeqMask)
{
	MAUTOSTRIP(CXR_Anim_Base_Read_2, MAUTOSTRIP_VOID);

	m_ContainerName = _pFileName;

	CDataFile DFile;
	DFile.Open(_pFileName);
	
	spCXR_Anim_Base spAnim = ReadMultiFormat(&DFile, _ReadFlags, _pSeqMask, _nSeqMask);
	DFile.Close();
	return spAnim;
}


//--------------------------------------------------------------------------------
void CXR_Anim_BreakoutPoints::Create(CBreakoutPoint* _lPoints, int32 _NumPoints)
{
	if ((_lPoints == NULL) || (_NumPoints <= 0))
		return;

	m_lPoints.SetLen(_NumPoints);

	// Beh, should sort this ?
	for (int32 i = 0; i < _NumPoints; i++)
	{
		m_lPoints[i] = _lPoints[i];
	}
}

void Sort(CEntryPoint* _lPoints, int32 _iStart, int32 _iEnd)
{
	//Check for final case
	if (_iStart >= _iEnd)
		return;

	//Get pivot value
	uint8 Pivot = _iStart + _lPoints[(_iEnd - _iStart) / 2].m_iEntry;

	//Loop through list until indices cross
	int iStart = _iStart;
	int iEnd = _iEnd;
	while (iStart <= iEnd)
	{
		// Find the first value that is greater than or equal to the pivot .
		while ((iStart < _iEnd ) &&	(_lPoints[iStart].m_iEntry < Pivot))
			iStart++;

		// Find the last value that is smaller than or equal to the pivot .
		while ((iEnd > _iStart) && 	(_lPoints[iEnd].m_iEntry > Pivot))
			iEnd--;

		// If the indexes have not crossed, swap stuff
		if (iStart <= iEnd) 
		{
			CEntryPoint Temp = _lPoints[iStart];
			_lPoints[iStart] = _lPoints[iEnd];
			_lPoints[iEnd] = Temp;
			iStart++;
			iEnd--;
		}
	}

	//Sort left partition if end index hasn't reached start
	if (_iStart < iEnd)
		Sort(_lPoints, _iStart, iEnd);

	//Sort right partition if start index hasn't reached end
	if (iStart < _iEnd)
		Sort(_lPoints, iStart, _iEnd);
}

//--------------------------------------------------------------------------------
void CXR_Anim_SyncPoints::Create(CSyncPoint* _lPoints, int32 _NumPoints)
{
	if ((_lPoints == NULL) || (_NumPoints <= 0))
		return;

	m_lPoints.SetLen(_NumPoints);

	for (int32 i = 0; i < _NumPoints; i++)
	{
		m_lPoints[i] = _lPoints[i];
	}
}

#define CXR_ANIM_ENTRYPOINT_SORT
// Create, (and sort entry points?)
void CXR_Anim_EntryPoints::Create(CEntryPoint* _lPoints, int32 _NumPoints)
{
	if ((_lPoints == NULL) || (_NumPoints <= 0))
		return;

#ifdef CXR_ANIM_ENTRYPOINT_SORT
	// Sort the list
	Sort(_lPoints,0,_NumPoints-1);
#endif

	m_lPoints.SetLen(_NumPoints);

	for (int32 i = 0; i < _NumPoints; i++)
	{
		m_lPoints[i] = _lPoints[i];
	}
}

int32 Find(const CEntryPoint* _lPoints, int32 _iStart, int32 _iEnd, uint8 _Value)
{
	if (_iStart >= _iEnd)
	{
		if (_lPoints[_iStart].m_iEntry == _Value)
			return _iStart;
		return -1;
	}

	int32 iVal = _iStart + (_iEnd - _iStart) / 2;

	if (_lPoints[iVal].m_iEntry == _Value)
		return iVal;
	
	if (_Value < _lPoints[iVal].m_iEntry)
		return (_iStart < iVal ? Find(_lPoints, _iStart, iVal, _Value) : -1);
	else
		return Find(_lPoints, iVal+1, _iEnd, _Value);
}

bool CXR_Anim_EntryPoints::FindMatchingPoint(const CXR_Anim_BreakoutPoints& _Points, 
											 CEntryPoint& _Point) const
{
#ifdef CXR_ANIM_ENTRYPOINT_SORT
	// If entry array is sorted
	int32 BreakoutLen = _Points.m_lPoints.Len();
	int32 EntryLen = m_lPoints.Len();
	const CEntryPoint* lPoints = m_lPoints.GetBasePtr();
	if (lPoints == NULL)
		return false;
	for (int32 iB = 0; iB < BreakoutLen; iB++)
	{
		uint8 Current = _Points.m_lPoints[iB].m_iBreakout;

		int iF = Find(lPoints, 0, EntryLen-1, Current);
		if (iF != -1)
		{
			_Point = m_lPoints[iF];
			return true;
		}
	}
#else
	// Brute force search
	int32 BreakoutLen = _Points.m_lPoints.Len();
	int32 EntryLen = m_lPoints.Len();
	for (int32 iB = 0; iB < BreakoutLen; iB++)
	{
		uint8 iCurrent = _Points.m_lPoints[iB].m_iBreakout;
		for (int32 iE = 0; iE < EntryLen; iE++)
		{
			if (m_lPoints[iE].m_iEntry == iCurrent)
			{
				// Found a match, use it and return
				_Point = m_lPoints[iE];
				return true;
			}
		}
	}
#endif
	return false;
}

