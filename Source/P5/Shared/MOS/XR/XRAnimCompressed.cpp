#include "PCH.h"
#include "../MSystem/MSystem.h"
#include "MFloat.h"
#include "XRAnimCompressed.h"
#include "XRAnimOptimizer.h"
#include "MMath_Vec128.h"

// MRTC_IMPLEMENT_DYNAMIC(CXR_Anim_SequenceCompressed, CReferenceCount);

//#define ANIM_COMPRESSED_VERSION			0x0100
#define ANIM_COMPRESSED_VERSION				0x0101			// Added support for multiple quat-palettes
#define ANIM_COMPRESSEDSEQUENCE_VERSION		0x0101

// #define XRANIM_OLDEVALDATA
// #define XRANIM_EVALDATAMARK
// #define XRANIM_CHECKCOMPATIBILITY


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Compressed
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Anim_Quatint16::CXR_Anim_Quatint16()
{
}

void CXR_Anim_Quatint16::Read(CCFile* _pF, int _Version)
{
	_pF->ReadLE(k, 4);
}

void CXR_Anim_Quatint16::Write(CCFile* _pF) const
{
#ifndef	PLATFORM_CONSOLE
	_pF->WriteLE(k, 4);
#endif	// PLATFORM_CONSOLE
}

#ifndef CPU_LITTLEENDIAN
void CXR_Anim_Quatint16::SwapLE()
{
	::SwapLE(k[0]);
	::SwapLE(k[1]);
	::SwapLE(k[2]);
	::SwapLE(k[3]);
}
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_SequenceCompressed
|__________________________________________________________________________________________________
\*************************************************************************************************/
/*
void CXR_Anim_SequenceCompressed::operator= (const CXR_Anim_SequenceCompressed& _Seq)
{
	m_iiTracksRot = _Seq.m_iiTracksRot;
	m_iiTracksMove = _Seq.m_iiTracksMove;
	m_nTracksRot = _Seq.m_nTracksRot;
	m_nTracksMove = _Seq.m_nTracksMove;;
	m_iKeyTimes = _Seq.m_iKeyTimes;
	m_nKeys = _Seq.m_nKeys;
	m_iDataKeys = _Seq.m_iDataKeys;
	m_nDataKeys = _Seq.m_nDataKeys;

	m_Duration = _Seq.m_Duration;
	m_Flags = _Seq.m_Flags;
	m_AbsTimeFlag = _Seq.m_AbsTimeFlag;
	m_Name = _Seq.m_Name;
	m_Comment = _Seq.m_Comment;
}
*/

CXR_Anim_SequenceCompressed::CXR_Anim_SequenceCompressed(class CXR_Anim_Compressed* _pContainer, int _iSeq)
{
	Clear();
	m_iSeq = _iSeq;
	m_pContainer = _pContainer;
}

void CXR_Anim_SequenceCompressed::Clear()
{
	CXR_Anim_SequenceData::Clear();
	m_iiTracksRot = 0;
	m_iiTracksMove = 0;
	m_nTracksRot = 0;
	m_nTracksMove = 0;
	m_iKeyTimes = 0;
	m_nKeys = 0;
	m_iDataKeys = 0;
	m_nDataKeys = 0;
}

void CXR_Anim_SequenceCompressed::Read(CCFile* _pF, int _ReadFlags)
{
	ReadData(_pF);

	uint16 Ver;
	_pF->ReadLE(Ver);

	switch(Ver)
	{
	case 0x0100 :
	case 0x0101 :
		{
			_pF->ReadLE(m_iiTracksRot);
			_pF->ReadLE(m_iiTracksMove);
			_pF->ReadLE(m_nTracksRot);
			_pF->ReadLE(m_nTracksMove);
			_pF->ReadLE(m_iKeyTimes);
			_pF->ReadLE(m_nKeys);
			_pF->ReadLE(m_iDataKeys);
			_pF->ReadLE(m_nDataKeys);
			if(Ver == 0x0101)
				_pF->ReadLE(m_iNextAnimSequenceLayer);
			break;
		}

	default :
		Error("Read", CStrF("Unsupported sequence version %.4x", Ver));
	}
}


fp32 M_INLINE CXR_Anim_SequenceCompressed::GetFrameAbsTime(int _iFrm) const
{
	return m_pContainer->GetFrameAbsTime(this, _iFrm);
}

void M_INLINE CXR_Anim_SequenceCompressed::EvalRot(const CMTime& _Time, CQuatfp32* _pDest, int _nDest) const
{
	m_pContainer->EvalRot(this, _Time, _pDest, _nDest);
}

void M_INLINE CXR_Anim_SequenceCompressed::EvalMove(const CMTime& _Time, CVec3Dfp32* _pDest, int _nDest) const
{
	m_pContainer->EvalMove(this, _Time, _pDest, _nDest);
}

void M_INLINE CXR_Anim_SequenceCompressed::Eval(const CMTime& _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const
{
	m_pContainer->Eval(this, _Time.GetTime(), _pRot, _nRot, _pMove, _nMove, _TrackMask);
}

void M_INLINE CXR_Anim_SequenceCompressed::Eval(fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const
{
	m_pContainer->Eval(this, _Time, _pRot, _nRot, _pMove, _nMove, _TrackMask);
}

void M_INLINE CXR_Anim_SequenceCompressed::EvalTrack0(const CMTime& _Time, vec128& _Move0, CQuatfp32& _Rot0) const
{
	m_pContainer->EvalTrack0(this, _Time, _Move0, _Rot0);
}

void M_INLINE CXR_Anim_SequenceCompressed::GetTotalTrack0(vec128& _Move0, CQuatfp32& _Rot0) const
{
	m_pContainer->GetTotalTrack0(this, _Move0, _Rot0);
}

bool CXR_Anim_SequenceCompressed::GetFramesAndTimeFraction(const CMTime& _Time, int _iFrames[4], fp32& _Fraction) const
{
	MSCOPESHORT( CXR_Anim_SequenceCompressed::GetFramesAndTimeFraction );
	return GetFramesAndTimeFraction( _Time.GetTime(), _iFrames, _Fraction );
}

bool M_INLINE CXR_Anim_SequenceCompressed::GetFramesAndTimeFraction(fp32 _Time, int _iFrames[4], fp32& _Fraction) const
{
	MSCOPESHORT(CXR_Anim_SequenceCompressed::GetFramesAndTimeFraction);
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

bool CXR_Anim_SequenceCompressed::GetFrameAndTimeFraction(fp32 _Time, int& _iFrame, fp32& _tFrac) const
{
	MAUTOSTRIP(CXR_Anim_SequenceCompressed_GetFrameAndTimeFraction, false);

	int nFrames = CXR_Anim_SequenceCompressed::GetNumKeys();
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

	TAP_RCD<fp32> pKeyTimes = m_pContainer->m_lKeyTimes;
	int iKeyTimes = m_iKeyTimes;
//	_pSeq->m_pContainer->m_lKeyTimes[_pSeq->m_iKeyTimes + _iFrm]

	i = TruncToInt((nFrames - 2) * (_Time / m_Duration));
	for (low = (-1), high = nFrames-1; high - low > 1;  )
	{
//		begin = CXR_Anim_SequenceCompressed::GetFrameAbsTime(i);
//		end = CXR_Anim_SequenceCompressed::GetFrameAbsTime(i+1);
		begin = pKeyTimes[iKeyTimes + i];
		end = pKeyTimes[iKeyTimes + i+1];

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


void CXR_Anim_SequenceCompressed::Write(CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_SequenceCompressed_Write, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_CONSOLE
	CXR_AnimWriteInfo WriteInfo;
	WriteInfo = _WriteInfo;
	// OK, what this basically does is
	if (WriteInfo.m_RotKeyVersion >= 2 && WriteInfo.m_RotKeyVersion <= 3/* &&
		m_Flags & ANIM_SEQFLAGS_HIGHPRECISIONROT*/)
		WriteInfo.m_RotKeyVersion = 4;

	WriteData(_pF);

	uint16 Ver = ANIM_COMPRESSEDSEQUENCE_VERSION;
	_pF->WriteLE(Ver);

	_pF->WriteLE(m_iiTracksRot);
	_pF->WriteLE(m_iiTracksMove);
	_pF->WriteLE(m_nTracksRot);
	_pF->WriteLE(m_nTracksMove);
	_pF->WriteLE(m_iKeyTimes);
	_pF->WriteLE(m_nKeys);
	_pF->WriteLE(m_iDataKeys);
	_pF->WriteLE(m_nDataKeys);
	_pF->WriteLE(m_iNextAnimSequenceLayer);
#endif	// PLATFORM_CONSOLE
}


CXR_Anim_DataKey_Sequence CXR_Anim_SequenceCompressed::GetDataKeys() const
{
	M_ASSERT((m_iDataKeys + m_nDataKeys) <= m_pContainer->GetDataKeys().GetKeys().Len(), "Index out of range!");
	return CXR_Anim_DataKey_Sequence(m_pContainer->GetDataKeys(), m_iDataKeys, m_nDataKeys);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_QuatHash
|__________________________________________________________________________________________________
\*************************************************************************************************/

int CXR_Anim_QuatHash::GetHashIndex(const CXR_Anim_Quatint16& _Quat)
{
	return
		((uint32(_Quat.k[0]) >> 13) << 0) +
		((uint32(_Quat.k[1]) >> 13) << 3) +
		((uint32(_Quat.k[2]) >> 13) << 6) +
		((uint32(_Quat.k[3]) >> 13) << 9);
}

void CXR_Anim_QuatHash::Create(int _nIndices, int _HashShiftSize)
{
	m_HashAnd = (1 << _HashShiftSize) - 1;
	THash<int32, CQuatHashElem>::Create(_nIndices, (1 << _HashShiftSize), false);
}

void CXR_Anim_QuatHash::Insert(int _iQuat, const CXR_Anim_Quatint16& _Quat)
{
	THash<int32, CQuatHashElem>::Remove(_iQuat);
	THash<int32, CQuatHashElem>::Insert(_iQuat, GetHashIndex(_Quat) & m_HashAnd);
}

int CXR_Anim_QuatHash::GetIndex(const CXR_Anim_Quatint16& _Quat, CXR_Anim_Quatint16* _pQuats)
{
	int ID = m_pHash[GetHashIndex(_Quat) & m_HashAnd];
	while(ID != -1)
	{
		if (_pQuats[ID].AlmostEqual(_Quat, 1))
			return ID;
		ID = m_pIDInfo[ID].m_iNext;
	}

	return -1;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Compressed
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Anim_TrackMask CXR_Anim_Compressed::ms_TrackMaskAll;

spCXR_Anim_SequenceData CXR_Anim_Compressed::GetSequence(int _iSeq)
{
	MAUTOSTRIP(CXR_Anim_Compressed_GetSequence, NULL);
	return m_lspSequences.ValidPos(_iSeq) ? (m_lspSequences[_iSeq]) : spCXR_Anim_SequenceData ();
}

const spCXR_Anim_SequenceData CXR_Anim_Compressed::GetSequence(int _iSeq) const
{
	MAUTOSTRIP(CXR_Anim_Compressed_GetSequence_const, NULL);
	return m_lspSequences.ValidPos(_iSeq) ? (m_lspSequences[_iSeq]) : spCXR_Anim_SequenceData ();
}


CXR_Anim_SequenceCompressed* CXR_Anim_Compressed::GetSequenceCompressed(int _iSeq)
{
	MAUTOSTRIP(CXR_Anim_Compressed_GetSequenceCompressed, NULL);
	return m_lspSequences.ValidPos(_iSeq) ? (TDynamicCast<CXR_Anim_SequenceCompressed>((CXR_Anim_SequenceData*)m_lspSequences[_iSeq])) : NULL;
}

M_INLINE fp32 CXR_Anim_Compressed::GetFrameAbsTime(const CXR_Anim_SequenceCompressed* _pSeq, int _iFrm) const
{
	return _pSeq->m_pContainer->m_lKeyTimes[_pSeq->m_iKeyTimes + _iFrm];
}

M_INLINE void CVec3Dfp32_CustomLerp( const CVec3Dfp32& _Src1, const CVec3Dfp32& _Src2, CVec3Dfp32& _Dest, const fp32 _Val )
{
	_Src1.Lerp( _Src2, _Val, _Dest);
}

// This function Interpolates between 2 quats and then checks the sign on the second source and destination to make sure it's positive
M_INLINE void CQuatfp32_CustomLerp( const CQuatfp32& _Src1, const CQuatfp32& _Src2, CQuatfp32& _Dest, const fp32 _Val )
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	_Src1.Lerp(_Src2, 2.0f, _Dest);
	fp32 dot = _Src2.DotProd(_Dest);
	fp32 k0 = _Dest.k[0];
	fp32 k1 = _Dest.k[1];
	fp32 k2 = _Dest.k[2];
	fp32 k3 = _Dest.k[3];
	_Dest.k[0] = __fsel(dot, k0, -k0);
	_Dest.k[1] = __fsel(dot, k1, -k1);
	_Dest.k[2] = __fsel(dot, k2, -k2);
	_Dest.k[3] = __fsel(dot, k3, -k3);
#else
	_Src1.Lerp(_Src2, 2.0f, _Dest);
	if (_Src2.DotProd(_Dest) < 0.0f)
	{
		_Dest.k[0] = -_Dest.k[0];
		_Dest.k[1] = -_Dest.k[1];
		_Dest.k[2] = -_Dest.k[2];
		_Dest.k[3] = -_Dest.k[3];
	}
#endif
}

// This function Interpolates between 2 quats and then checks the sign on the second source and destination to make sure it's positive
#ifndef XRANIM_OLDEVALDATA

M_FORCEINLINE void CQuatfp32_CustomLerp2( const CQuatfp32& _Src1, const CQuatfp32& _Src2, CQuatfp32& _Dest, const fp32 _Val )
{
	vec128 dst = M_VAdd(_Src1.v, M_VMul(M_VSub(_Src2.v, _Src1.v), M_VScalar(2.0f)));
	vec128 dot = M_VDp4(_Src2.v, dst);
	_Dest.v = M_VSel(dot, dst, M_VNeg(dst));
}

M_FORCEINLINE vec128 GetQuatfp32(const CXR_Anim_Quatint16& _Quat)
{
/*	__m64 a = *((__m64*)&_Quat);
	__m64  ext_val = _mm_cmpgt_pi16(_mm_setzero_si64(), a);
	__m128 tmp = _mm_cvtpi32_ps(_mm_setzero_ps(), _mm_unpackhi_pi16(a, ext_val));
	vec128 q = _mm_cvtpi32_ps(_mm_movelh_ps(tmp, tmp), _mm_unpacklo_pi16(a, ext_val));

//	vec128 q = _mm_cvtpi16_ps(*((__m64*)&_Quat));
	vec128 r = M_VMul(q, M_VScalar(1.0f / 32767.0f));

	return r;
*/
	vec128 a = M_VLd64(&_Quat);
	vec128 b = M_VScalar_u16(32768);
	vec128 s = M_VScalar(1.0f / 32767);
	vec128 mrg = M_VMrgL_u16_u32(M_VAdd_u16(a, b), M_VZero());
	vec128 af = M_VCnv_i32_f32(mrg);

	vec128 res = M_VAdd(M_VMul(af, s), M_VScalar(-32768.0f/32767.0f));
	return res;

/*	CQuatfp32 Q;
	_Quat.GetQuatfp32(Q);
	return Q.v;
*/
/*	fp32 a, b, c, d;
	a = fp32(_Quat.k[0]) * (1.0f/32767.0f);
	b = fp32(_Quat.k[1]) * (1.0f/32767.0f);
	c = fp32(_Quat.k[2]) * (1.0f/32767.0f);
	d = fp32(_Quat.k[3]) * (1.0f/32767.0f);
	CQuatfp32 Quat;
	Quat.k[0] = a;
	Quat.k[1] = b;
	Quat.k[2] = c;
	Quat.k[3] = d;
	return Quat.v;*/
}
#endif

void CXR_Anim_Compressed::EvalData(
	const uint16* _piQTracks,
	const uint16* _piMTracks,
	int _nQ, int _nMove,
	int _iFrm0, int _iFrm1, int _iFrm2, int _iFrm3, 
	CQuatfp32* _pDest, vec128* _pDestMove, fp32 _tFrac,
	fp32 _t01,
	fp32 _t12,
	fp32 _t23, const CXR_Anim_TrackMask& _TrackMask) const
{
	MAUTOSTRIP(CXR_Anim_Compressed_EvalData, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Anim_Compressed::EvalData);

#ifdef XRANIM_EVALDATAMARK
	CMTime TimeQuat, TimeMove;
	TimeQuat.Start();
#endif


	fp32 tSqr = Sqr(_tFrac);
	fp32 tCube = tSqr * _tFrac;

	const fp32 k = 0.5f;
	fp32 tsA0 = (_t01 != 0.0f) ? (k * _t12 / _t01) : 0.0f;
	fp32 tsA1 = k;
	fp32 tsB0 = k;
	fp32 tsB1 = (_t23 != 0.0f) ? (k * _t12 / _t23) : 0.0f;

#ifdef XRANIM_EVALDATAMARK
	int nEvalRot = 0;
	int nEvalMove = 0;
#endif

	const CXR_Anim_CompressedTrack* pTracks = m_lTracks.GetBasePtr();
	const uint16* piIndices = m_liQuats.GetBasePtr();

//	vec128 UnitQuat = M_VConst(0.0f, 0.0f, 0.0f, 1.0f);
#if defined(XRANIM_CHECKCOMPATIBILITY) || defined(XRANIM_OLDEVALDATA)
	int nQuatErrors = 0;
	{
		TAP_RCD<const CQuatPalette> pQuatPalettes = m_lQuatPalettes;


#ifdef	PLATFORM_PS2
		CQuatfp32* Rot = (CQuatfp32*)ScratchPad_Alloc( sizeof(CQuatfp32)*4 );;
#else
		CQuatfp32 Rot[4];
#endif

		for(int iQ = 0; iQ < _nQ; iQ++)
		{
			if (!_TrackMask.IsEnabledRot(iQ))
			{
//				_pDest[iQ].v = UnitQuat;
//#ifndef M_RTM
//				MemSetD(&_pDest[iQ], 0x7Fc00000, 4);	// Fill with QNaN
//#endif
				continue;
			}

#ifdef XRANIM_EVALDATAMARK
			nEvalRot++;
#endif

			// Get track and check if it is constant or unit.
			const CXR_Anim_CompressedTrack& Track = pTracks[_piQTracks[iQ]];
			uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);

			if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;
				pQuats[piIndices[iiQuat]].GetQuatfp32(_pDest[iQ]);
				continue;
			}
			else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				_pDest[iQ].Unit();
				continue;
			}

			const uint16* piTQ = &piIndices[iiQuat];
			const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;


			// Handle sequence endpoint splining
			pQuats[piTQ[_iFrm1]].GetQuatfp32(Rot[1]);
			pQuats[piTQ[_iFrm2]].GetQuatfp32(Rot[2]);

			// pRot[1] = &_pRot1[iQ];
			// pRot[2] = &_pRot2[iQ];

			if (_iFrm0 == -1)
			{
				CQuatfp32_CustomLerp(Rot[2],Rot[1], Rot[0], 2.0f);
			}
			else
			{
				pQuats[piTQ[_iFrm0]].GetQuatfp32(Rot[0]);
				// pRot[0] = &_pRot0[iQ];
			}
				
			if (_iFrm3 == -1)
			{
				CQuatfp32_CustomLerp(Rot[1],Rot[2], Rot[3], 2);
			}
			else
			{
				pQuats[piTQ[_iFrm3]].GetQuatfp32(Rot[3]);
				// pRot[3] = &_pRot3[iQ];
			}

			// Spline it
			for(int i = 0; i < 4; i++)
			{
				const fp32 p0 = Rot[1].k[i];
				const fp32 p1 = Rot[2].k[i];
				const fp32 p1mp0 = p1 - p0;
				const fp32 v0 = (p0 - Rot[0].k[i]) * tsA0 + p1mp0 * tsA1;
				const fp32 v1 = p1mp0 * tsB0 + (Rot[3].k[i] - p1) * tsB1; 
				const fp32 B = 3.0f*(p1mp0) - (2.0f*v0) - v1;
				_pDest[iQ].k[i] = (-(2.0f * B + v0 - v1) *(1.0f/ 3.0f))*tCube + B*tSqr + v0*_tFrac + p0;
			}

			// FIXME: These sqrt's can be combined for SIMD execution. Not a huge win, just a possible improvement.
			_pDest[iQ].Normalize();
		}
	}
#endif
#ifndef XRANIM_OLDEVALDATA
	{
		TAP_RCD<const CQuatPalette> pQuatPalettes = m_lQuatPalettes;

#ifdef	PLATFORM_PS2
		CQuatfp32* Rot = (CQuatfp32*)ScratchPad_Alloc( sizeof(CQuatfp32)*4 );;
#else
//		CQuatfp32 Rot[4];
		CQuatfp32 Rot0, Rot1, Rot2, Rot3;
#endif

		vec128 vtCubeOver3 = M_VMul(M_VScalar(-1.0f/3.0f), M_VLdScalar(tCube));
		vec128 vtSqr = M_VLdScalar(tSqr);
		vec128 vtFrac = M_VLdScalar(_tFrac);
		vec128 vtsA0 = M_VLdScalar(tsA0);
		vec128 vtsA1 = M_VLdScalar(tsA1);
		vec128 vtsB0 = M_VLdScalar(tsB0);
		vec128 vtsB1 = M_VLdScalar(tsB1);

		if (_iFrm0 == -1 || _iFrm3 == -1)
		{
			for(int iQ = 0; iQ < _nQ; iQ++)
			{
				if (!_TrackMask.IsEnabledRot(iQ))
				{
//					_pDest[iQ].v = UnitQuat;
					continue;
				}
	#ifdef XRANIM_EVALDATAMARK
				nEvalRot++;
	#endif

				// Get track and check if it is constant or unit.
				const CXR_Anim_CompressedTrack& Track = pTracks[_piQTracks[iQ]];
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);

				if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
				{
					const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;
					_pDest[iQ].v = GetQuatfp32(pQuats[piIndices[iiQuat]]);
	//				pQuats[piIndices[iiQuat]].GetQuatfp32(_pDest[iQ]);
					continue;
				}
				else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
				{
					_pDest[iQ].Unit();
					continue;
				}

				const uint16* piTQ = &piIndices[iiQuat];
				const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;


				// Handle sequence endpoint splining
				Rot1.v = GetQuatfp32(pQuats[piTQ[_iFrm1]]);
				Rot2.v = GetQuatfp32(pQuats[piTQ[_iFrm2]]);

				if (_iFrm0 == -1)
				{
					CQuatfp32_CustomLerp2(Rot2,Rot1, Rot0, 2.0f);
				}
				else
				{
					Rot0.v = GetQuatfp32(pQuats[piTQ[_iFrm0]]);
				}
					
				if (_iFrm3 == -1)
				{
					CQuatfp32_CustomLerp2(Rot1,Rot2, Rot3, 2.0f);
				}
				else
				{
					Rot3.v = GetQuatfp32(pQuats[piTQ[_iFrm3]]);
				}

				// Spline it
				vec128 p0 = Rot1.v;
				vec128 p1 = Rot2.v;
				vec128 p1mp0 = M_VSub(p1, p0);
				vec128 v0 = M_VAdd(M_VMul(M_VSub(p0, Rot0.v), vtsA0), M_VMul(p1mp0, vtsA1));
				vec128 v1 = M_VAdd(M_VMul(p1mp0, vtsB0), M_VMul(M_VSub(Rot3.v, p1), vtsB1));
				vec128 B = M_VSub(M_VSub(M_VMul(M_VScalar(3.0f), p1mp0), M_VMul(v0, M_VScalar(2.0f))), v1);
				vec128 QDest = 
					M_VAdd(
						M_VAdd(M_VMul(vtCubeOver3, M_VAdd(M_VMul(B, M_VScalar(2.0f)), M_VSub(v0, v1))),
								M_VMul(vtSqr, B)),
						M_VAdd(M_VMul(v0, vtFrac),
							p0)
						);

#ifdef XRANIM_CHECKCOMPATIBILITY
				QDest = M_VNrm4(QDest);
				vec128 facit = _pDest[iQ].v;
				if (M_VCmpAnyGE(M_VAbs(M_VSub(facit, QDest)), M_VScalar(0.00001f)))
//				if (!M_VCmpAllEq(facit, QDest))
				{
					nQuatErrors++;
				}
#else
				_pDest[iQ].v = QDest;
#endif
			}
		}
		else
		{
			for(int iQ = 0; iQ < _nQ; iQ++)
			{
				if (!_TrackMask.IsEnabledRot(iQ))
				{
	//#ifndef M_RTM
	//				MemSetD(&_pDest[iQ], 0x7Fc00000, 4);	// Fill with QNaN
	//#endif
					continue;
				}

	#ifdef XRANIM_EVALDATAMARK
				nEvalRot++;
	#endif

				// Get track and check if it is constant or unit.
				const CXR_Anim_CompressedTrack& Track = pTracks[_piQTracks[iQ]];
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);

				if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
				{
					const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;
					_pDest[iQ].v = GetQuatfp32(pQuats[piIndices[iiQuat]]);
	//				pQuats[piIndices[iiQuat]].GetQuatfp32(_pDest[iQ]);
					continue;
				}
				else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
				{
					_pDest[iQ].Unit();
					continue;
				}

				const uint16* piTQ = &piIndices[iiQuat];
				const CXR_Anim_Quatint16* pQuats = pQuatPalettes[iPalette].m_pQuats;


				// Handle sequence endpoint splining
				const CXR_Anim_Quatint16* pQ0 = &pQuats[piTQ[_iFrm0]];
				M_PRECACHE128(0, (const char*)pQ0);
				const CXR_Anim_Quatint16* pQ1 = &pQuats[piTQ[_iFrm1]];
				M_PRECACHE128(0, (const char*)pQ1);
				const CXR_Anim_Quatint16* pQ2 = &pQuats[piTQ[_iFrm2]];
				M_PRECACHE128(0, (const char*)pQ2);
				const CXR_Anim_Quatint16* pQ3 = &pQuats[piTQ[_iFrm3]];
				M_PRECACHE128(0, (const char*)pQ3);

				Rot0.v = GetQuatfp32(*pQ0);
				Rot1.v = GetQuatfp32(*pQ1);
				Rot2.v = GetQuatfp32(*pQ2);
				Rot3.v = GetQuatfp32(*pQ3);

				// Spline it

				vec128 p0 = Rot1.v;
				vec128 p1 = Rot2.v;
				vec128 p1mp0 = M_VSub(p1, p0);
				vec128 v0 = M_VAdd(M_VMul(M_VSub(p0, Rot0.v), vtsA0), M_VMul(p1mp0, vtsA1));
				vec128 v1 = M_VAdd(M_VMul(p1mp0, vtsB0), M_VMul(M_VSub(Rot3.v, p1), vtsB1));
				vec128 B = M_VSub(M_VSub(M_VMul(M_VScalar(3.0f), p1mp0), M_VMul(v0, M_VTwo())), v1);
				vec128 QDest = 
					M_VAdd(
						M_VAdd(M_VMul(vtCubeOver3, M_VAdd(M_VMul(B, M_VTwo()), M_VSub(v0, v1))),
								M_VMul(vtSqr, B)),
						M_VAdd(M_VMul(v0, vtFrac),
							p0)
						);

#ifdef XRANIM_CHECKCOMPATIBILITY
				QDest = M_VNrm4(QDest);
				vec128 facit = _pDest[iQ].v;
				if (M_VCmpAnyGE(M_VAbs(M_VSub(facit, QDest)), M_VScalar(0.00001f)))
//				if (!M_VCmpAllEq(facit, QDest))
				{
					nQuatErrors++;
				}
#else
				_pDest[iQ].v = QDest;
#endif


	/*			for(int i = 0; i < 4; i++)
				{
					const fp32 p0 = Rot[1].k[i];
					const fp32 p1 = Rot[2].k[i];
					const fp32 p1mp0 = p1 - p0;
					const fp32 v0 = (p0 - Rot[0].k[i]) * tsA0 + p1mp0 * tsA1;
					const fp32 v1 = p1mp0 * tsB0 + (Rot[3].k[i] - p1) * tsB1; 
					const fp32 B = 3.0f*(p1mp0) - (2.0f*v0) - v1;
					QDest.k[i] = (-(2.0f * B + v0 - v1) *(1.0f/ 3.0f))*tCube + B*tSqr + v0*_tFrac + p0;
				}
	*/
				// FIXME: These sqrt's can be combined for SIMD execution. Not a huge win, just a possible improvement.
	//			_pDest[iQ].v = M_VNrm4(QDest);
	//			_pDest[iQ].Normalize();
			}
		}

#ifndef XRANIM_CHECKCOMPATIBILITY
		CQuatfp32* pNrm = _pDest;
		int iN = _nQ;
		for(; iN > 3; iN -= 4)
		{
			M_VNrm4x4(pNrm[0].v, pNrm[1].v, pNrm[2].v, pNrm[3].v);
			pNrm += 4;
		}
		for(; iN > 0; iN -= 1)
		{
			pNrm[0].v = M_VNrm4(pNrm[0].v);
			pNrm += 1;
		}
#endif

#ifdef	PLATFORM_PS2
		ScratchPad_Free( sizeof( CQuatfp32 ) * 4 );
#endif
	}
#endif // XRANIM_OLDEVALDATA

#ifdef XRANIM_EVALDATAMARK
	TimeQuat.Stop();
	TimeMove.Start();
#endif

	{
		TAP_RCD<const CVecPalette> pMovePalettes = m_lMovePalettes;

#ifdef	PLATFORM_PS2
		CVec3Dfp32* lMoves = (CVec3Dfp32*)ScratchPad_Alloc( sizeof(CVec3Dfp32)*4 );
#else
		CVec3Dfp32 lMoves[4];
#endif

		for(int iM = 0; iM < _nMove; iM++)
		{
			if (!_TrackMask.IsEnabledMove(iM))
			{
//#ifndef M_RTM
//				MemSetD(&_pDestMove[iM], 0x7Fc00000, 3);	// Fill with QNaN
//#endif
				continue;
			}

#ifdef XRANIM_EVALDATAMARK
			nEvalMove++;
#endif
			// Get track and check if it is constant or unit.
			const CXR_Anim_CompressedTrack& Track = pTracks[_piMTracks[iM]];
			uint iiMove = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);

			if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				const CVec3Dfp32* pMoves = pMovePalettes[iPalette].m_pMoves;
				_pDestMove[iM] = M_VLd_P3_Slow(&pMoves[piIndices[iiMove]]);
				continue;
			}
			else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				_pDestMove[iM] =  M_VConst(0,0,0,1.0f);
				continue;
			}

			const uint16* piTM = &piIndices[iiMove];
			const CVec3Dfp32* pMoves = pMovePalettes[iPalette].m_pMoves;


			// Handle sequence endpoint splining
			lMoves[1] = pMoves[piTM[_iFrm1]];
			lMoves[2] = pMoves[piTM[_iFrm2]];

			if (_iFrm0 == -1)
			{	
				// Extrapolate before first frame
				CVec3Dfp32_CustomLerp( lMoves[2], lMoves[1], lMoves[0], 2.0f );
			}
			else
			{
				lMoves[0] = pMoves[piTM[_iFrm0]];
			}

			if (_iFrm3 == -1)
			{	
				// Extrapolate beyond last frame
				CVec3Dfp32_CustomLerp( lMoves[1], lMoves[2], lMoves[3], 2.0f );
			}
			else
			{
				lMoves[3] = pMoves[piTM[_iFrm3]];
			}

			// CVec3Dfp32 dMA = (*pMove[1] - *pMove[0]) * tsA0;
			// dMA += (*pMove[2] - *pMove[1]) * tsA1;

			CVec3Dfp32 dMA;
			dMA[0] = (lMoves[1][0] - lMoves[0][0]) * tsA0 + (lMoves[2][0] - lMoves[1][0]) * tsA1;
			dMA[1] = (lMoves[1][1] - lMoves[0][1]) * tsA0 + (lMoves[2][1] - lMoves[1][1]) * tsA1;
			dMA[2] = (lMoves[1][2] - lMoves[0][2]) * tsA0 + (lMoves[2][2] - lMoves[1][2]) * tsA1;


			// CVec3Dfp32 dMB = (*pMove[2] - *pMove[1]) * tsB0;
			// dMB += (*pMove[3] - *pMove[2]) * tsB1;
			CVec3Dfp32 dMB;
			dMB[0] = (lMoves[2][0] - lMoves[1][0]) * tsB0 + (lMoves[3][0] - lMoves[2][0]) * tsB1;
			dMB[1] = (lMoves[2][1] - lMoves[1][1]) * tsB0 + (lMoves[3][1] - lMoves[2][1]) * tsB1;
			dMB[2] = (lMoves[2][2] - lMoves[1][2]) * tsB0 + (lMoves[3][2] - lMoves[2][2]) * tsB1;

			// Spline it
			CVec4Dfp32 dest;
			for(int i = 0; i < 3; i++)
			{
				const fp32 v0 = dMA.k[i];
				const fp32 v1 = dMB.k[i];
				const fp32 p0 = lMoves[1][i];
				const fp32 p1 = lMoves[2][i];
				const fp32 D = p0;
				const fp32 C = v0;
				const fp32 B = 3.0f*(p1 - p0) - (2.0f*v0) - v1;
				const fp32 A = -(2.0f * B + v0 - v1) * (1.0f/3.0f);
				dest.k[i] = A*tCube + B*tSqr + C*_tFrac + D;
			}
			dest.k[3]=1.0f;
			_pDestMove[iM]=dest.v;

			// _pDestMove[iM] = 0;
		}
#ifdef	PLATFORM_PS2
		ScratchPad_Free( sizeof( CVec3Dfp32 ) * 4 );
#endif
	}
#ifdef XRANIM_EVALDATAMARK
	TimeMove.Stop();
#ifdef XRANIM_CHECKCOMPATIBILITY
	M_TRACEALWAYS("EvalData: Q %.4f ms, M %.4f ms (%d/%d quats, %d moves), nErrors %d\n", TimeQuat.GetTime() * 1000.0f, TimeMove.GetTime() * 1000.0f, nEvalRot, _nQ, _nMove, nQuatErrors);
#else
	M_TRACEALWAYS("EvalData: Q %.4f ms, M %.4f ms (%d/%d quats, %d moves)\n", TimeQuat.GetTime() * 1000.0f, TimeMove.GetTime() * 1000.0f, nEvalRot, _nQ, _nMove);
#endif
//	ConOut(CStrF("Rot %d, Move %d", nEvalRot, nEvalMove));
#endif

//	ConOut(CStrF("Rot %d, Move %d", nEvalRot, nEvalMove));
}


void CXR_Anim_Compressed::EvalRot(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, CQuatfp32* _pDest, int _nDest) const
{
	Error("EvalRot", "WTF is this used for?");

//	for(int i = 0; i < _nDest; i++)
//		_pDest[i].Unit();
}

void CXR_Anim_Compressed::EvalMove(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, CVec3Dfp32* _pDest, int _nDest) const
{
	Error("EvalMove", "WTF is this used for?");

//	for(int i = 0; i < _nDest; i++)
//		_pDest[i] = 0;
}

void CXR_Anim_Compressed::Eval(const CXR_Anim_SequenceCompressed* _pSeq, fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const
{
	MSCOPESHORT(CXR_Anim_Compressed::Eval);

/*	if (FloatIsInvalid(_Time))
	{
		ConOut("(CXR_Anim_Compressed::Eval) You sucked ass and sent an invalid float as time to this function.");
		return;
	}*/

	int iFrames[4] = {0};
	fp32 Fraction = 0;

	int nEvalRot = 0;
	int nEvalMove = 0;

	//if (!FloatIsInvalid(_Time))
	if (_pSeq->GetFramesAndTimeFraction(_Time, iFrames, Fraction))
	{
		const fp32* pKeyTimes = &m_lKeyTimes[_pSeq->m_iKeyTimes];
		int nKeys = _pSeq->m_nKeys;

		fp32 dt01 = 0.0f;
		fp32 dt12 = pKeyTimes[iFrames[2]] - pKeyTimes[iFrames[1]];
		fp32 dt23 = 0.0f;

		if (iFrames[0] < 0)
			iFrames[0] = -1;
		else
			dt01 = pKeyTimes[iFrames[1]] - pKeyTimes[iFrames[0]];

		if (iFrames[3] >= nKeys)
			iFrames[3] = -1;
		else
			dt23 = pKeyTimes[iFrames[3]] - pKeyTimes[iFrames[2]];

		const uint16* piQTracks = (_pSeq->m_nTracksRot) ? &m_liTracks[_pSeq->m_iiTracksRot] : NULL;
		const uint16* piMTracks = (_pSeq->m_nTracksMove) ? &m_liTracks[_pSeq->m_iiTracksMove] : NULL;
//		const uint16* piMTracks = NULL;

		nEvalRot = MinMT(_pSeq->m_nTracksRot, _nRot);
		nEvalMove = MinMT(_pSeq->m_nTracksMove, _nMove);

//ConOut(CStrF("%f, Frac %f, iFrm %d, %d, %d, %d", _Time, Fraction, iFrames[0], iFrames[1], iFrames[2], iFrames[3]));
		EvalData(piQTracks, piMTracks, nEvalRot, nEvalMove, iFrames[0], iFrames[1], iFrames[2], iFrames[3], _pRot, _pMove, Fraction, dt01, dt12, dt23, _TrackMask);
	}

	for(int i = nEvalRot; i < _nRot; i++)
		_pRot[i].Unit();

	for(int j = nEvalMove; j < _nMove; j++)
		_pMove[j] = M_VConst(0,0,0,1.0f);
}

void CXR_Anim_Compressed::EvalTrack0(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, vec128& _Move0, CQuatfp32& _Rot0) const
{
	MSCOPESHORT(CXR_Anim_Compressed::EvalTrack0);

	Eval(_pSeq, _Time.GetTime(), &_Rot0, 1, &_Move0, 1, ms_TrackMaskAll);
//	_Move0 = 0;
//	_Rot0.Unit();
}

void CXR_Anim_Compressed::GetTotalTrack0(const CXR_Anim_SequenceCompressed* _pSeq, vec128& _Move0, CQuatfp32& _Rot0) const
{
	int nKeys = _pSeq->m_nKeys;
	if (nKeys > 1)
	{
		if (_pSeq->m_nTracksRot > 0)
		{
			const uint16* piQTracks = &m_liTracks[_pSeq->m_iiTracksRot];
			const CXR_Anim_CompressedTrack& Track = m_lTracks[piQTracks[0]];
			if (Track.m_iiQuat & (ANIM_COMPRESSEDTRACK_UNIT | ANIM_COMPRESSEDTRACK_CONSTANT))
				_Rot0.Unit();
			else
			{
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);
				const uint16* piQuats = &m_liQuats[iiQuat];
				const CXR_Anim_Quatint16* pQuats = m_lQuatPalettes[iPalette].m_pQuats;

				CQuatfp32 QuatFirst, QuatLast;
				pQuats[piQuats[0]].GetQuatfp32(QuatFirst);
				pQuats[piQuats[nKeys-1]].GetQuatfp32(QuatLast);
				QuatFirst.Inverse();
				QuatLast.Multiply(QuatFirst, _Rot0);
			}
		}
		else
			_Rot0.Unit();

		if (_pSeq->m_nTracksMove > 0)
		{
			const uint16* piMTracks = &m_liTracks[_pSeq->m_iiTracksMove];
			const CXR_Anim_CompressedTrack& Track = m_lTracks[piMTracks[0]];
			if (Track.m_iiQuat & (ANIM_COMPRESSEDTRACK_UNIT | ANIM_COMPRESSEDTRACK_CONSTANT))
				_Move0 = M_VConst(0,0,0,1.0f);
			else
			{
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);
				const uint16* piMoves = &m_liQuats[iiQuat];
				const CVec3Dfp32* pMoves = m_lMovePalettes[iPalette].m_pMoves;
				vec128 m0=M_VLd_P3_Slow(&pMoves[piMoves[nKeys-1]]);
				vec128 m1=M_VLd_V3_Slow(&pMoves[piMoves[0]]);
				_Move0 = M_VSub(m0,m1);
			}
		}
		else
			_Move0 = M_VConst(0,0,0,1.0f);

	}
	else
	{
		_Move0 = M_VConst(0,0,0,1.0f);
		_Rot0.Unit();
	}
}


#ifndef PLATFORM_CONSOLE

int CXR_Anim_Compressed::Compress_AddQuat(const CXR_Anim_Quatint16& _Quat, TArray<int32> * _pCleared)
{
	if (!m_spQuatHash)
		Error("Compress_AddQuat", "Hash not initialized.");

	int iPalette = m_lQuatPalettes.Len() - 1;
	CQuatPalette& QP = m_lQuatPalettes[iPalette];

	int iQ = m_spQuatHash->GetIndex(_Quat, QP.m_lQuats.GetBasePtr());
	if (iQ < 0)
	{
		if (QP.m_lQuats.Len() >= 65535)
			return -1; // force restart of track, with a new palette

		iQ = QP.m_lQuats.Add(_Quat);
		m_spQuatHash->Insert(iQ, _Quat);
		if (_pCleared) 
			_pCleared->Add(iQ);
	}
	return iQ;
}


int CXR_Anim_Compressed::Compress_AddMove(const CVec3Dfp32& _Move)
{
	int iPalette = m_lMovePalettes.Len() - 1;
	CVecPalette& VP = m_lMovePalettes[iPalette];
	const CVec3Dfp32* pM = VP.m_lMoves.GetBasePtr();
	int nM = VP.m_lMoves.Len();

	for (int i = 0; i < nM; i++)
		if (_Move.AlmostEqual(pM[i], 0.001f))
			return i;

	if (nM >= 65535)
		return -1; // force restart of track, with new palette

	return VP.m_lMoves.Add(_Move);
}


uint32 CXR_Anim_Compressed::Compress_AddQuatIndices(const uint16* _piQuat, int _niQuats)
{
	uint16* piQ = m_liQuats.GetBasePtr();
	int niQ = m_liQuats.Len();

	// Loop through existing indices to see if we can reuse an entire index range.
	for (int i = 0; i <= niQ-_niQuats; i++)
	{
		int j = 0;
		for(; j < _niQuats; j++)
			if (piQ[i+j] != _piQuat[j])
				break;

		if (j == _niQuats)
			return i;
	}

	int iRet = m_liQuats.Len();
	for(int j = 0; j < _niQuats; j++)
		m_liQuats.Add(_piQuat[j]);

	if (m_liQuats.Len() & ~ANIM_COMPRESSEDTRACK_INDEXMASK)
		Error("Compress_AddQuatIndices", "Too many quaternion indices.");

//	if (m_liQuats.Len() >= 65536)
//		Error("Compress_AddQuatIndices", "Too many quaternion indices.");

	return iRet;
}


uint16 CXR_Anim_Compressed::Compress_AddTrack(const CXR_Anim_CompressedTrack& _Track)
{
	const CXR_Anim_CompressedTrack* pT = m_lTracks.GetBasePtr();
	int nT = m_lTracks.Len();

	for(int i = 0; i < nT; i++)
		if (_Track.m_iiQuat == pT[i].m_iiQuat)
			return i;
/*
	if (m_lTracks.Len() >= 16383)
		Error("Compress_AddTrack", "Too many tracks.");
//*/
	return m_lTracks.Add(_Track);
}

static spCXR_Anim_QuatHash CreateQuatHash()
{
	spCXR_Anim_QuatHash spQuatHash = MNew(CXR_Anim_QuatHash);
	if (!spQuatHash)
		MemError_static("Compress");
	spQuatHash->Create(65536, 12);
	return spQuatHash;
}

void CXR_Anim_Compressed::Compress(CXR_Anim_Base* _pAnim, const fp32* _pCompression, bool _bSkipCulling)
{
	m_lQuatPalettes.Clear();
	m_lMovePalettes.Clear();
	m_liTracks.SetGrow(32768);
	m_liTracks.QuickSetLen(0);
	m_liQuats.SetGrow(32768);
	m_liQuats.QuickSetLen(0);
	m_lKeyTimes.SetGrow(8192);
	m_lKeyTimes.QuickSetLen(0);
	m_lTracks.SetGrow(8192);
	m_lTracks.QuickSetLen(0);
	m_lspSequences.SetGrow(256);
	m_lspSequences.QuickSetLen(0);

	m_spQuatHash = CreateQuatHash();

	int nCulledKeys = 0;
	int nKeys = 0;
	int nQuatsOrg = 0;
	int nMovesOrg = 0;
	int niQuatOrg = 0;
	int nTrackOrg = 0;
	int nTrackUnit = 0;
	int nTrackConst = 0;
	int nMoveTrackOrg = 0;
	int nMoveTrackUnit = 0;
	int nMoveTrackConst = 0;

	bool bSeqError = false;

	// Init optimizer
	TArray<fp32> lWeights;
	lWeights.Add(3.0f);
	lWeights.Add(2.0f);
	lWeights.Add(1.0f);
	
	CAnimationOptimizer animOpt;
	// *** FIXME ***


	//LogFile(CStrF("Optimizing animation container %s...", _pAnim->m_ContainerName.Str()));
	uint nSeq = _pAnim->GetNumSequences();

	TArray<spCXR_Anim_SequenceTracks> lspSeqTracks;
	lspSeqTracks.SetLen(nSeq);

	{
		for(int i = 0; i < nSeq; i++)
		{
			spCXR_Anim_SequenceData spSeqData = _pAnim->GetSequence(i);
			spCXR_Anim_SequenceTracks spSeq;
			int nCulled = 0;
			CXR_Anim_Sequence *pOrgSeq = TDynamicCast<CXR_Anim_Sequence>((CXR_Anim_SequenceData*)spSeqData);
			if (pOrgSeq)
			{
				fp32 Compression = XRANIM_DEFUALTCOMPRESSION;
				if (_pCompression && _pCompression[i] != -1)
					Compression = _pCompression[i];

				if (_bSkipCulling || (Compression < 0.000001f))
				{
					spSeq = MNew(CXR_Anim_SequenceTracks);
					spSeq->ConvertSequence(pOrgSeq);
				}
				else
				{
					spCXR_Anim_Sequence spSeqOpt = pOrgSeq->Duplicate();
					animOpt.Setup(spSeqOpt);
					animOpt.Optimize(lWeights);
					fp32 MaxError = animOpt.Cull(Compression);

					spSeq = MNew(CXR_Anim_SequenceTracks);
					spSeq->ConvertSequence(spSeqOpt);
					nCulled = spSeqData->GetNumKeys() - spSeqOpt->GetNumKeys();
				}
			}
			else
				spSeq = TDynamicCast<CXR_Anim_SequenceTracks>((CXR_Anim_SequenceData*)spSeqData);
			nCulledKeys += nCulled;

			//LogFile(CStrF("  %s:  Culled %i  Remaining %i", spSeq->m_Name.Str(), nCulled, spSeq->GetNumKeys()));
			lspSeqTracks[i] = spSeq;

			MRTC_IncProgress();
		}
	}

	//LogFile("Done");

	CXR_Anim_Compressed	* pCurrent = this;
	pCurrent->m_nSequences = 0;

	TArray<int32>	AddedQuaternions;
	AddedQuaternions.SetGrow(0x4FFF);

	uint nCurrQuatPaletteOrigSize = 0, nCurrMovePaletteOrigSize = 0;
	bool bRestartRot = true, bRestartMove = true;

	for (int i = 0; i < _pAnim->GetNumSequences(); i++)
	{
		spCXR_Anim_SequenceCompressed spSeqCompr = MNew2(CXR_Anim_SequenceCompressed, pCurrent, i);
		spSeqCompr->Clear();

		spCXR_Anim_SequenceTracks spSeq = lspSeqTracks[i];

		if (spSeq)
		{
//				TArray<CXR_Anim_DataKey>			m_lEvents;	// List of CXR_Anim_DataKey
//				TArray<fp32>							m_lTimes;	// t0,t1,t2 etc
//				TArray<CXR_Anim_RotTrack>			m_lRotTracks;
//				TArray<CXR_Anim_MoveTrack>			m_lMoveTracks;

			spSeqCompr->m_nKeys = spSeq->m_lTimes.Len();
			spSeqCompr->m_nTracksRot = spSeq->m_lRotTracks.Len();
			spSeqCompr->m_Flags = spSeq->m_Flags;
			spSeqCompr->m_AbsTimeFlag = spSeq->m_AbsTimeFlag;
			spSeqCompr->m_TimeCode = spSeq->m_TimeCode;
			spSeqCompr->m_Name = spSeq->m_Name;
			spSeqCompr->m_Comment = spSeq->m_Comment;
			spSeqCompr->m_iNextAnimSequenceLayer = spSeq->m_iNextAnimSequenceLayer;

			nKeys += spSeq->m_lTimes.Len();

			//Cache the current state
			bool bIsOverflowed = false;
			CXR_Anim_Compressed	Cache;

			do 
			{
				Cache.CopyCompressedData(*pCurrent);
				AddedQuaternions.QuickSetLen(0);

				//We need to set this here since pCurrent need to be initialized
				spSeqCompr->m_iiTracksRot = (spSeqCompr->m_nTracksRot) ? pCurrent->m_liTracks.Len() : 0;

				spSeqCompr->m_iKeyTimes = pCurrent->m_lKeyTimes.Len();
				pCurrent->m_lKeyTimes.Add(&spSeq->m_lTimes);

				uint nRotTracks = spSeq->m_lRotTracks.Len();
				for (uint iTrack = 0; iTrack < nRotTracks; iTrack++)
				{
					nTrackOrg++;

RestartQuat:
					if (bRestartRot)
					{
						if (pCurrent->m_lQuatPalettes.Len())
						{
							if (nCurrQuatPaletteOrigSize == 0)
								Error("Compress", CStrF("Rotation data overflow in sequence %i!", m_lspSequences.Len()));

							// remove data added from unfinished track
							pCurrent->m_lQuatPalettes[pCurrent->m_lQuatPalettes.Len()-1].m_lQuats.QuickSetLen(nCurrQuatPaletteOrigSize);
							M_TRACE("Seq %d: Out of quaternions! Restarting track %d with new palette.\n", i, iTrack);
						}
						CQuatPalette Tmp;
						Tmp.m_lQuats.SetGrow(8192);
						pCurrent->m_lQuatPalettes.Add(Tmp);
						pCurrent->m_spQuatHash = CreateQuatHash();
						bRestartRot = false;
					}
					uint iCurrPalette = pCurrent->m_lQuatPalettes.Len() - 1;
					nCurrQuatPaletteOrigSize = pCurrent->m_lQuatPalettes[iCurrPalette].m_lQuats.Len();

					CXR_Anim_RotTrack& Rot = spSeq->m_lRotTracks[iTrack];
					if (!Rot.m_liKeys.Len() || !Rot.m_lData.Len())
					{
						// Unit
						pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(ANIM_COMPRESSEDTRACK_UNIT)));
						nTrackUnit++;
					}
					else if (Rot.m_lData.Len() == 1)
					{
						// Const
						CQuatfp32 Quat;
						Rot.m_lData[0].GetRot(Quat);
						int iQuat = pCurrent->Compress_AddQuat(CXR_Anim_Quatint16(Quat), &AddedQuaternions);
						if (iQuat < 0)
							{ bRestartRot = true; goto RestartQuat; }

						uint32 iiQuat = pCurrent->Compress_AddQuatIndex(iQuat);
						iiQuat |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
						niQuatOrg += 1;
						pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiQuat | ANIM_COMPRESSEDTRACK_CONSTANT)));
						nQuatsOrg++;
						nTrackConst++;
					}
					else
					{
						TArray<uint16> liQuats;
						liQuats.SetLen(spSeqCompr->m_nKeys);

						bool bConst = true;
						for(int iKey = 0; iKey < spSeqCompr->m_nKeys; iKey++)
						{
							int iKey2 = Min(iKey, Rot.m_liKeys.Len()-1);
							int iRot = Rot.m_liKeys[iKey2];

							CQuatfp32 Quat;
							Rot.m_lData[iRot].GetRot(Quat);
							int iQuat = pCurrent->Compress_AddQuat(CXR_Anim_Quatint16(Quat),&AddedQuaternions);
							if (iQuat < 0)
								{ bRestartRot = true; goto RestartQuat; }

							liQuats[iKey] = iQuat;
							if (liQuats[iKey] != liQuats[0])
								bConst = false;
						}

						if (bConst)
						{
							uint32 iiQuat = pCurrent->Compress_AddQuatIndices(liQuats.GetBasePtr(), 1);
							iiQuat |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
							niQuatOrg += 1;
							pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiQuat | ANIM_COMPRESSEDTRACK_CONSTANT)));
							nTrackConst++;
						}
						else
						{
							uint32 iiQuat = pCurrent->Compress_AddQuatIndices(liQuats.GetBasePtr(), liQuats.Len());
							iiQuat |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
							niQuatOrg += liQuats.Len();
							pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiQuat)));
						}
						nQuatsOrg += Rot.m_lData.Len();
					}
				}

				spSeqCompr->m_nTracksMove = spSeq->m_lMoveTracks.Len();
				spSeqCompr->m_iiTracksMove = (spSeqCompr->m_nTracksMove) ? pCurrent->m_liTracks.Len() : 0;

				uint nMoveTracks = spSeq->m_lMoveTracks.Len();
				for (uint iTrackMove = 0; iTrackMove < nMoveTracks; iTrackMove++)
				{
					nTrackOrg++;

RestartMove:
					if (bRestartMove)
					{
						if (pCurrent->m_lMovePalettes.Len())
						{
							if (nCurrMovePaletteOrigSize == 0)
								Error("Compress", CStrF("Move data overflow in sequence %i!", m_lspSequences.Len()));

							// remove data added from unfinished track
							pCurrent->m_lMovePalettes[pCurrent->m_lMovePalettes.Len()-1].m_lMoves.QuickSetLen(nCurrMovePaletteOrigSize);
							M_TRACE("Seq %d: Out of vectors! Restarting track %d with new palette.\n", i, iTrackMove);
						}
						CVecPalette Tmp;
						Tmp.m_lMoves.SetGrow(8192);
						pCurrent->m_lMovePalettes.Add(Tmp);
						bRestartMove = false;
					}
					uint iCurrPalette = pCurrent->m_lMovePalettes.Len() - 1;
					nCurrMovePaletteOrigSize = pCurrent->m_lMovePalettes[iCurrPalette].m_lMoves.Len();

					CXR_Anim_MoveTrack& Move = spSeq->m_lMoveTracks[iTrackMove];
					if (!Move.m_liKeys.Len() || !Move.m_lData.Len())
					{
						// Unit
						pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(ANIM_COMPRESSEDTRACK_UNIT)));
						nMoveTrackUnit++;
					}
					else if (Move.m_lData.Len() == 1)
					{
						// Const
						int iMove = pCurrent->Compress_AddMove(Move.m_lData[0].m_Move);
						if (iMove < 0)
							{ bRestartMove = true; goto RestartMove; }

						uint32 iiMove = pCurrent->Compress_AddQuatIndex(iMove);
						iiMove |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
						niQuatOrg += 1;
						pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiMove | ANIM_COMPRESSEDTRACK_CONSTANT)));
						nMovesOrg++;
						nMoveTrackConst++;
					}
					else
					{
						TArray<uint16> liMoves;
						liMoves.SetLen(spSeqCompr->m_nKeys);

						bool bConst = true;
						for(int iKey = 0; iKey < spSeqCompr->m_nKeys; iKey++)
						{
							int iKey2 = Min(iKey, Move.m_liKeys.Len()-1);
							int iMove = Move.m_liKeys[iKey2];

							int iAdded = pCurrent->Compress_AddMove(Move.m_lData[iMove].m_Move);
							if (iAdded < 0)
								{ bRestartMove = true; goto RestartMove; }

							liMoves[iKey] = iAdded;
							if (liMoves[iKey] != liMoves[0])
								bConst = false;
						}

						if (bConst)
						{
							uint32 iiMove = pCurrent->Compress_AddQuatIndex(liMoves[0]);
							iiMove |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
							niQuatOrg += 1;
							pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiMove | ANIM_COMPRESSEDTRACK_CONSTANT)));
							nMoveTrackConst++;
						}
						else
						{
							uint32 iiMove = pCurrent->Compress_AddQuatIndices(liMoves.GetBasePtr(), liMoves.Len());
							iiMove |= (iCurrPalette << ANIM_COMPRESSEDTRACK_INDEXBITS);
							niQuatOrg += liMoves.Len();
							pCurrent->m_liTracks.Add(pCurrent->Compress_AddTrack(CXR_Anim_CompressedTrack(iiMove)));
						}
						nMovesOrg += Move.m_lData.Len();
					}
				}

				//Check overflow
		//		bIsOverflowed = ((pCurrent->m_lQuats.Len() >= 65535) || (pCurrent->m_lMoves.Len() >= 65535) || (pCurrent->m_lTracks.Len() >= 16383));
				bIsOverflowed = (pCurrent->m_lQuatPalettes.Len() > 255) || (pCurrent->m_lMovePalettes.Len() > 255) || (pCurrent->m_lTracks.Len() >= 16383);

				if (bIsOverflowed)
				{
					//If we didn't have any sequences, a single one is too large
					if (!pCurrent->m_nSequences)
						Error("Compress", CStrF("Data overflow in sequence %i!", m_lspSequences.Len()));

					//Restore to previous state
					pCurrent->CopyCompressedData(Cache);
					for(int i = 0;i < AddedQuaternions.Len();i++)
					{
						if( AddedQuaternions[i] < 65536 ) pCurrent->m_spQuatHash->Remove(AddedQuaternions[i]);
					}

					//Create new
					pCurrent->m_spNext = MNew(CXR_Anim_Compressed);
					if (!pCurrent->m_spNext)
						MemError("Compress");
					pCurrent = (CXR_Anim_Compressed*)pCurrent->m_spNext;
					pCurrent->m_liTracks.SetGrow(32768);
					pCurrent->m_liQuats.SetGrow(32768);
					pCurrent->m_lKeyTimes.SetGrow(8192);
					pCurrent->m_lTracks.SetGrow(8192);
					pCurrent->m_nSequences = 0;
					pCurrent->m_spQuatHash = CreateQuatHash();

					spSeqCompr->m_pContainer = pCurrent;
					bRestartRot = true;
					bRestartMove = true;
				}
			}
			while(bIsOverflowed);

//Uncut

			{
				CXR_Anim_DataKey_Sequence DKSeq = spSeq->GetDataKeys();
				spSeqCompr->m_nDataKeys = DKSeq.GetNumKeys();
				spSeqCompr->m_iDataKeys = DKSeq.GetNumKeys() ? pCurrent->m_DataKeys.GetKeys().Len() : 0;
				pCurrent->m_DataKeys.AddFromSequence(DKSeq);
			}
		}
		else
		{
			bSeqError = true;
		}

		m_lspSequences.Add((CXR_Anim_SequenceData*)spSeqCompr);
		pCurrent->m_nSequences++;
	}

	if (bSeqError)
		LogFile("Animation set contained one or more unknown sequence types.");

	uint32 nQuats = 0, nQuatBytes = 0, nQuatPalettes = 0,
		   nMoves = 0, nMoveBytes = 0, nMovePalettes = 0,
		   niTracks = 0,niTrackBytes = 0,niQuats = 0,niQuatBytes = 0,
		   nKeyTimes = 0,nKeyTimeBytes = 0,nTracks = 0,nTrackBytes = 0,
		   nDataKeys = 0,nDataKeyBytes = 0;
	CXR_Anim_Compressed * pItr = this;
	do 
	{
		TAP<CQuatPalette> pQuatPalettes = pItr->m_lQuatPalettes;
		for (uint i = 0; i < pQuatPalettes.Len(); i++)
		{
			pQuatPalettes[i].m_lQuats.OptimizeMemory();
			nQuats += pQuatPalettes[i].m_lQuats.Len();
			nQuatBytes += pQuatPalettes[i].m_lQuats.ListSize();
			nQuatPalettes++;
		}
		TAP<CVecPalette> pMovePalettes = pItr->m_lMovePalettes;
		for (uint i = 0; i < pMovePalettes.Len(); i++)
		{
			pMovePalettes[i].m_lMoves.OptimizeMemory();
			nMoves += pMovePalettes[i].m_lMoves.Len();
			nMoveBytes += pMovePalettes[i].m_lMoves.ListSize();
			nMovePalettes++;
		}
		pItr->m_liTracks.OptimizeMemory();
		niTracks += pItr->m_liTracks.Len();
		niTrackBytes += pItr->m_liTracks.ListSize();
		pItr->m_liQuats.OptimizeMemory();
		niQuats += pItr->m_liQuats.Len();
		niQuatBytes += pItr->m_liQuats.ListSize();
		pItr->m_lKeyTimes.OptimizeMemory();
		nKeyTimes += pItr->m_lKeyTimes.Len();
		nKeyTimeBytes += pItr->m_lKeyTimes.ListSize();
		pItr->m_lTracks.OptimizeMemory();
		nTracks += pItr->m_lTracks.Len();
		nTrackBytes += pItr->m_lTracks.ListSize();
		nDataKeys += pItr->m_DataKeys.GetKeys().Len();
		nDataKeyBytes += pItr->m_DataKeys.GetDataSize();
		pItr = (CXR_Anim_Compressed*)pItr->m_spNext;
	} 
	while(pItr);
	m_lspSequences.OptimizeMemory();

	LogFile(CStrF("Sequences %d, Keyframes %d, CulledKeys %d", _pAnim->GetNumSequences(), nKeys, nCulledKeys));
	LogFile(CStrF("       %d/%d Quats, %d bytes (%d palettes)", nQuats, nQuatsOrg, nQuatBytes, nQuatPalettes));
	LogFile(CStrF("       %d Moves, %d bytes (%d palettes)", nMoves, nMoveBytes, nMovePalettes));
	LogFile(CStrF("       %d iTracks, %d bytes", niTracks, niTrackBytes));
	LogFile(CStrF("       %d/%d iQuats, %d bytes", niQuats, niQuatOrg, niQuatBytes));
	LogFile(CStrF("       %d KeyTimes, %d bytes", nKeyTimes, nKeyTimeBytes));
	LogFile(CStrF("       %d Tracks, %d bytes (%d org, %d unit, %d const)", nTracks, nTrackBytes, nTrackOrg, nTrackUnit, nTrackConst));
	LogFile(CStrF("       %d Sequences, %d bytes", m_lspSequences.Len(), m_lspSequences.ListSize()));
	LogFile(CStrF("       %d Data keys, %d bytes", nDataKeys, nDataKeyBytes));
}


void CXR_Anim_Compressed::Write(CDataFile* _pDFile, const CXR_AnimWriteInfo _WriteInfo)
{
	uint16 nNodes = CountNodes();

	MAUTOSTRIP(CXR_Anim_Base_Write, MAUTOSTRIP_VOID);
	_pDFile->BeginEntry("COMPRESSEDANIMATIONSET2");
	_pDFile->EndEntry(m_lspSequences.Len(), nNodes);

	CCFile* pF = _pDFile->GetFile();

	_pDFile->BeginSubDir();

	CXR_Anim_Compressed* pItr = this;
	uint16	iNode = 0;

	while (pItr)
	{
		if (nNodes > 1)
		{
			_pDFile->BeginEntry(CStrF("NODE%i",iNode));
			_pDFile->EndEntry(pItr->m_nSequences);
			_pDFile->BeginSubDir();
		}

		_pDFile->BeginEntry("QUATPALETTES");
		uint nQuatPalettes = pItr->m_lQuatPalettes.Len();
		_pDFile->EndEntry(nQuatPalettes, 0x0100);
		_pDFile->BeginSubDir();
		{
			for (uint iQP = 0; iQP < nQuatPalettes; iQP++)
			{
				const CQuatPalette& QP = pItr->m_lQuatPalettes[iQP];
				_pDFile->BeginEntry("QUATERNIONS");
				for (uint i = 0; i < QP.m_lQuats.Len(); i++)
					QP.m_lQuats[i].Write(pF);
				_pDFile->EndEntry(QP.m_lQuats.Len(), 0x0100);
			}
		}
		_pDFile->EndSubDir();

		_pDFile->BeginEntry("MOVEPALETTES");
		uint nMovePalettes = pItr->m_lMovePalettes.Len();
		_pDFile->EndEntry(nMovePalettes, 0x0100);
		_pDFile->BeginSubDir();
		{
			for (uint iMP = 0; iMP < nMovePalettes; iMP++)
			{
				const CVecPalette& MP = pItr->m_lMovePalettes[iMP];
				_pDFile->BeginEntry("MOVES");
				for (uint i = 0; i < MP.m_lMoves.Len(); i++)
					MP.m_lMoves[i].Write(pF);
				_pDFile->EndEntry(MP.m_lMoves.Len(), 0x0100);
			}
		}
		_pDFile->EndSubDir();

		_pDFile->BeginEntry("TRACKINDICES");
		pF->WriteLE(pItr->m_liTracks.GetBasePtr(), pItr->m_liTracks.Len());
		_pDFile->EndEntry(pItr->m_liTracks.Len(), 0x0100);

		_pDFile->BeginEntry("QUATERNIONINDICES");
		pF->WriteLE(pItr->m_liQuats.GetBasePtr(), pItr->m_liQuats.Len());
		_pDFile->EndEntry(pItr->m_liQuats.Len(), 0x0100);

		_pDFile->BeginEntry("KEYFRAMETIMES");
		pF->WriteLE(pItr->m_lKeyTimes.GetBasePtr(), pItr->m_lKeyTimes.Len());
		_pDFile->EndEntry(pItr->m_lKeyTimes.Len(), 0x0100);

		_pDFile->BeginEntry("TRACKS");
		pF->Write(pItr->m_lTracks.GetBasePtr(), pItr->m_lTracks.ListSize());
		_pDFile->EndEntry(pItr->m_lTracks.Len(), 0x0100);

		_pDFile->BeginEntry("DATAKEYS");
		pItr->m_DataKeys.Write(_pDFile->GetFile());
		_pDFile->EndEntry(pItr->m_DataKeys.GetKeys().Len(), 0x0200);

		if (nNodes > 1)
		{
			_pDFile->EndSubDir();
		}

		pItr = (CXR_Anim_Compressed*)pItr->m_spNext;
		iNode++;
	}

	_pDFile->BeginEntry("SEQUENCES");
	{
		for(int i = 0; i < m_lspSequences.Len(); i++)
		{
			GetSequenceCompressed(i)->Write(pF, _WriteInfo);
		}
	}
	_pDFile->EndEntry(m_lspSequences.Len(), ANIM_COMPRESSED_VERSION);

	_pDFile->EndSubDir();
}

void CXR_Anim_Compressed::Write(const char* _pFileName, const CXR_AnimWriteInfo _WriteInfo)
{
	MAUTOSTRIP(CXR_Anim_Base_Write_2, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Create(_pFileName);
	Write(&DFile, _WriteInfo);
	DFile.Close();
}

#endif	// PLATFORM_CONSOLE

void CXR_Anim_Compressed::Initialize()
{
	FillChar(&ms_TrackMaskAll, sizeof(ms_TrackMaskAll), -1);

	for(int i = 0; i < m_lspSequences.Len(); i++)
	{
		CXR_Anim_SequenceCompressed* pSeq = GetSequenceCompressed(i);
		if (pSeq->m_nKeys > 1)
			pSeq->SetDuration(GetFrameAbsTime(pSeq, pSeq->m_nKeys-1));
		else
			pSeq->SetDuration(0.05f);
	
		CalculateTrackmask(i, pSeq->m_TrackMask);
		if(pSeq->m_iNextAnimSequenceLayer >= 0 && m_lspSequences.Len() >= pSeq->m_iNextAnimSequenceLayer)
			pSeq->m_pNextAnimSequenceLayer = m_lspSequences[pSeq->m_iNextAnimSequenceLayer];
	}

	TAP<CQuatPalette> pQP = m_lQuatPalettes;
	for (int iQP = 0; iQP < pQP.Len(); iQP++)
		pQP[iQP].m_pQuats = pQP[iQP].m_lQuats.GetBasePtr();

	TAP<CVecPalette> pMP = m_lMovePalettes;
	for (int iMP = 0; iMP < pMP.Len(); iMP++)
		pMP[iMP].m_pMoves = pMP[iMP].m_lMoves.GetBasePtr();
}

void CXR_Anim_Compressed::CalculateTrackmask(int _iSeq, CXR_Anim_TrackMask &_TrackMask)
{
	CXR_Anim_SequenceCompressed* pSeq = GetSequenceCompressed(_iSeq);
	pSeq->m_TrackMask.Clear();
	if(!pSeq->m_nTracksRot || !pSeq->m_nTracksMove)
		return;

	const CXR_Anim_CompressedTrack* pTracks = pSeq->m_pContainer->m_lTracks.GetBasePtr();
	const uint16* piIndices = pSeq->m_pContainer->m_liQuats.GetBasePtr();
	TAP_RCD<const CQuatPalette> pQuatPalettes = pSeq->m_pContainer->m_lQuatPalettes;
	TAP_RCD<const CVecPalette> pMovePalettes = pSeq->m_pContainer->m_lMovePalettes;

	CQuatfp32 QUnit;
	QUnit.Unit();
	const uint16* piQTracks = &pSeq->m_pContainer->m_liTracks[pSeq->m_iiTracksRot];
	for (uint i = 0; i < pSeq->m_nTracksRot; i++)
	{
		const CXR_Anim_CompressedTrack& Track = pSeq->m_pContainer->m_lTracks[piQTracks[i]];
		if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT && Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			continue;
		if ((Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT))
		{
			uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);
			const uint16* piQuats = &pSeq->m_pContainer->m_liQuats[iiQuat];
			CQuatfp32 Q;
			pQuatPalettes[iPalette].m_lQuats[piQuats[0]].GetQuatfp32(Q);
			if (memcmp(&Q, &QUnit, sizeof(Q)) != 0)
				pSeq->m_TrackMask.m_TrackMaskRot.Enable(i);
			else
			{
				// One might hope that this would never happen. Remove this whole check if still unit while constant when fixed. -JA
			}
		}
		else
			pSeq->m_TrackMask.m_TrackMaskRot.Enable(i);
	}

	const uint16* piMTracks = &pSeq->m_pContainer->m_liTracks[pSeq->m_iiTracksMove];
	for (uint i = 0; i < pSeq->m_nTracksMove; i++)
	{
		const CXR_Anim_CompressedTrack& Track = pSeq->m_pContainer->m_lTracks[piMTracks[i]];
		if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT && Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			continue;
		if ((Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT))
		{
			uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			uint iPalette = ANIM_COMPRESSEDTRACK_GETPALETTE(Track.m_iiQuat);
			const uint16* piMoves = &pSeq->m_pContainer->m_liQuats[iiQuat];
			if (pMovePalettes[iPalette].m_lMoves[piMoves[0]].LengthSqr() > 0.01f)
				pSeq->m_TrackMask.m_TrackMaskMove.Enable(i);
			else
			{
				// One might hope that this would never happen. Remove this whole check if still unit while constant when fixed. -JA
			}
		}
		else
			pSeq->m_TrackMask.m_TrackMaskMove.Enable(i);
	}
}


int CXR_Anim_Compressed::Read(const char* _pFileName, int _ReadFlags)
{
	MAUTOSTRIP(CXR_Anim_Base_Write_2, false);
	CDataFile DFile;
	DFile.Open(_pFileName);
	Read(&DFile, _ReadFlags);
	DFile.Close();
	return true;
}


void CXR_Anim_Compressed::ReadQuats(uint _iPalette, CDataFile* _pDFile)
{
	if (!_pDFile->GetNext("QUATERNIONS")) Error("Read", "No QUATERNIONS entry.");
	CCFile* pFile = _pDFile->GetFile();

	uint nElem = _pDFile->GetUserData();
	CQuatPalette& QP = m_lQuatPalettes[_iPalette];
	QP.m_lQuats.SetLen(nElem);
	QP.m_pQuats = QP.m_lQuats.GetBasePtr();
	pFile->Read(QP.m_pQuats, QP.m_lQuats.ListSize());

#ifndef CPU_LITTLEENDIAN
	for (uint i = 0; i < nElem; i++)
		QP.m_pQuats[i].SwapLE();
#endif
}

void CXR_Anim_Compressed::ReadMoves(uint _iPalette, CDataFile* _pDFile)
{
	if (!_pDFile->GetNext("MOVES")) Error("Read", "No MOVES entry.");
	CCFile* pFile = _pDFile->GetFile();

	uint nElem = _pDFile->GetUserData();
	CVecPalette& MP = m_lMovePalettes[_iPalette];
	MP.m_lMoves.SetLen(nElem);
	MP.m_pMoves = MP.m_lMoves.GetBasePtr();
	pFile->Read(MP.m_pMoves, MP.m_lMoves.ListSize());

#ifndef CPU_LITTLEENDIAN
	for (uint i = 0; i < nElem; i++)
		MP.m_pMoves[i].SwapLE();
#endif
}


int CXR_Anim_Compressed::Read(CDataFile* _pDFile, int _ReadFlags)
{
	MSCOPESHORT(CXR_Anim_Compressed::Read);

	if (!_pDFile->GetNext("COMPRESSEDANIMATIONSET2"))
		Error("Read", "No COMPRESSEDANIMATIONSET2 entry.");

	uint16 nNodes = _pDFile->GetUserData2();
	if (nNodes == 0) nNodes = 1;

	if (!_pDFile->GetSubDir())
		Error("Read", "Invalid COMPRESSEDANIMATIONSET2 entry.");

	CCFile* pFile = _pDFile->GetFile();

	TThinArray<uint> lnDataKeyVersion;
	TThinArray<TThinArray<int16> > llNumDataKeys; // needed to adjust offsets (each old datakey may contain more/less than one new datakey)

	lnDataKeyVersion.SetLen(nNodes);
	llNumDataKeys.SetLen(nNodes);
	
	CXR_Anim_Compressed* pCurrent = this;
	for (uint i = 0; i < nNodes; i++)
	{
		if (nNodes > 1)
		{
			if (_pDFile->GetNext(CStrF("NODE%i",i)) == NULL) Error("Read", "missing NODE entry.");
			pCurrent->m_nSequences = _pDFile->GetUserData();
			if (!_pDFile->GetSubDir()) Error("Read", "Missing node information");
		}

		{
			MSCOPE(m_lQuatPalettes, XR_ANIM);

			_pDFile->PushPosition();
			if (_pDFile->GetNext("QUATPALETTES"))
			{
				uint nQP = _pDFile->GetUserData();
				if (!_pDFile->GetSubDir()) Error("Read", "Invalid QUATPALETTES entry");
				pCurrent->m_lQuatPalettes.SetLen(nQP);
				for (uint iQP = 0; iQP < nQP; iQP++)
					pCurrent->ReadQuats(iQP, _pDFile);
				_pDFile->GetParent();
				_pDFile->PopPosition();
			}
			else
			{ // Old format
				_pDFile->PopPosition();
				pCurrent->m_lQuatPalettes.SetLen(1);
				pCurrent->ReadQuats(0, _pDFile);
			}
		}

		{
			MSCOPE(m_lMovePalettes, XR_ANIM);

			_pDFile->PushPosition();
			if (_pDFile->GetNext("MOVEPALETTES"))
			{
				uint nMP = _pDFile->GetUserData();
				if (!_pDFile->GetSubDir()) Error("Read", "Invalid MOVEPALETTES entry");
				pCurrent->m_lMovePalettes.SetLen(nMP);
				for (uint iMP = 0; iMP < nMP; iMP++)
					pCurrent->ReadMoves(iMP, _pDFile);
				_pDFile->GetParent();
				_pDFile->PopPosition();
			}
			else
			{ // Old format
				_pDFile->PopPosition();
				pCurrent->m_lMovePalettes.SetLen(1);
				pCurrent->ReadMoves(0, _pDFile);
			}
		}

		{
			MSCOPE(m_liTracks, XR_ANIM);

			if (!_pDFile->GetNext("TRACKINDICES")) Error("Read", "No TRACKINDICES entry.");
			int nElem = _pDFile->GetUserData();
			pCurrent->m_liTracks.SetLen(nElem);
			uint16* pElems = pCurrent->m_liTracks.GetBasePtr();
			pFile->Read(pElems, pCurrent->m_liTracks.ListSize());

			SwitchArrayLE_uint16(pElems, pCurrent->m_liTracks.Len());
		}

		{
			MSCOPE(m_liQuats, XR_ANIM);

			if (!_pDFile->GetNext("QUATERNIONINDICES")) Error("Read", "No QUATERNIONINDICES entry.");
			int nElem = _pDFile->GetUserData();
			pCurrent->m_liQuats.SetLen(nElem);
			uint16* pElems = pCurrent->m_liQuats.GetBasePtr();
			pFile->Read(pElems, pCurrent->m_liQuats.ListSize());

			SwitchArrayLE_uint16(pElems,pCurrent->m_liQuats.Len());
		}

		{
			MSCOPE(m_lKeyTimes, XR_ANIM);

			if (!_pDFile->GetNext("KEYFRAMETIMES")) Error("Read", "No KEYFRAMETIMES entry.");
			int nElem = _pDFile->GetUserData();
			pCurrent->m_lKeyTimes.SetLen(nElem);
			fp32* pElems = pCurrent->m_lKeyTimes.GetBasePtr();
			pFile->Read(pElems, pCurrent->m_lKeyTimes.ListSize());

			SwitchArrayLE_fp32(pElems, pCurrent->m_lKeyTimes.Len());
		}

		{
			MSCOPE(m_lTracks, XR_ANIM);

			if (!_pDFile->GetNext("TRACKS")) Error("Read", "No KEYFRAMETIMES entry.");
			int nElem = _pDFile->GetUserData();
			pCurrent->m_lTracks.SetLen(nElem);
			CXR_Anim_CompressedTrack* pElems = pCurrent->m_lTracks.GetBasePtr();
			pFile->Read(pElems, pCurrent->m_lTracks.ListSize());

#ifndef CPU_LITTLEENDIAN
			for(int i = 0; i < nElem; i++)
				pElems[i].SwapLE();
#endif
		}

		{
			MSCOPE(pCurrent->m_lDataKeys, XR_ANIM);

			if (!_pDFile->GetNext("DATAKEYS")) Error("Read", "No KEYFRAMETIMES entry.");
			lnDataKeyVersion[i] = _pDFile->GetUserData2();
			if (lnDataKeyVersion[i] == 0x0100)
			{
				// Fallback reading...
				CXR_Anim_DataKeys_Edit TmpContainer;
				uint nKeys = _pDFile->GetUserData();
				llNumDataKeys[i].SetLen(nKeys);
				for (uint j = 0; j < nKeys; j++)
				{
					CXR_Anim_DataKey1 Tmp;
					Tmp.Read(pFile);
					uint nAdded = TmpContainer.AddKey(Tmp);
					llNumDataKeys[i].GetBasePtr()[j] = nAdded;
				}
				if (nKeys)
					pCurrent->m_DataKeys.AddFromSequence( CXR_Anim_DataKey_Sequence(TmpContainer, 0, TmpContainer.GetKeys().Len()) );
			}
			else
			{
				pCurrent->m_DataKeys.Read(_pDFile->GetFile(), lnDataKeyVersion[i]);
			}
		}

		if (nNodes > 1)
		{
			_pDFile->GetParent();
			if( i+1 < nNodes ) pCurrent->m_spNext = MNew(CXR_Anim_Compressed);
			pCurrent = (CXR_Anim_Compressed*)pCurrent->m_spNext;
		}
	}

	{
		MSCOPE(m_lspSequences, XR_ANIM);

		if (!_pDFile->GetNext("SEQUENCES")) Error("Read", "No KEYFRAMETIMES entry.");
		int nElem = _pDFile->GetUserData();
		m_lspSequences.SetLen(nElem);

		if( nNodes < 2 )
		{
			m_nSequences = nElem;
		}

		M_TRY
		{
			pCurrent = this;
			uint16 iNode = 0;

			uint nDataKeyAdjust = 0;
			const int16* pNumAdded = llNumDataKeys[0].GetBasePtr();

			for (int i = 0, iSeq = 0; i < nElem; i++, iSeq++)
			{
				if (iSeq == pCurrent->m_nSequences)
				{
					iSeq = 0;
					pCurrent = (CXR_Anim_Compressed*)pCurrent->m_spNext;
					iNode++;

					if (lnDataKeyVersion[iNode] == 0x0100)
						pNumAdded = llNumDataKeys[iNode].GetBasePtr();
				}

				spCXR_Anim_SequenceCompressed spSeq = MNew2(CXR_Anim_SequenceCompressed, pCurrent, i);
				if (!spSeq)
					MemError("Read");
				spSeq->Read(pFile, _ReadFlags);

				if (lnDataKeyVersion[iNode] == 0x0100)
				{	// We need to adjust offsets...
					int nAdjust = 0;
					uint16 iDataKeys = spSeq->m_iDataKeys;
					uint16 nDataKeys = spSeq->m_nDataKeys;
					for (uint i = 0; i < nDataKeys; i++)
						nAdjust += (pNumAdded[iDataKeys + i] - 1);

					if (spSeq->m_iDataKeys)
						spSeq->m_iDataKeys += nDataKeyAdjust;

					spSeq->m_nDataKeys += nAdjust;
					nDataKeyAdjust += nAdjust;
				}

				m_lspSequences[i] = spSeq;
				spSeq->CheckSequenceForBreakouts();
			}
		}
		M_CATCH(
		catch(CCException)
		{
			m_lspSequences.Clear();
			throw;
		}
		)
	}

	Initialize();

	return true;
}

#define SET_BIT(pArray, iBit) pArray[(iBit) >> 3] |= 1 << ((iBit) & 7)

#define ISBITSET(pArray, iBit) (pArray[(iBit) >> 3] & 1 << ((iBit) & 7))

static int CreateRemapTable(const uint8* _pUseTags, uint16* _pRemap, int _nElem)
{
	int nUsed = 0;
	for(int i = 0; i < _nElem; i++)
	{
		if (_pUseTags[i >> 3] & (1 << (i & 7)))
			_pRemap[i] = nUsed++;
		else
			_pRemap[i] = 0xffff;
	}

	return nUsed;
}

static int CreateRemapTable(const uint8* _pUseTags, uint32* _pRemap, int _nElem)
{
	int nUsed = 0;
	for(int i = 0; i < _nElem; i++)
	{
		if (_pUseTags[i >> 3] & (1 << (i & 7)))
			_pRemap[i] = nUsed++;
		else
			_pRemap[i] = 0xffffffff;
	}

	return nUsed;
}

template<class TElem>
void RemapArray(const uint8* _pUseTags, const ANIM_COMPRESSED_ARRAYTYPE<TElem>& _lOld, ANIM_COMPRESSED_ARRAYTYPE<TElem>& _lNew, int _nNew)
{
	_lNew.SetLen(_nNew);

	const TElem* pOld = _lOld.GetBasePtr();
	TElem* pNew = _lNew.GetBasePtr();

	int nOld = _lOld.Len();
	int nUsed = 0;
	for(int i = 0; i < nOld; i++)
	{
		if (_pUseTags[i >> 3] & (1 << (i & 7)))
		{
			pNew[nUsed] = pOld[i];
			nUsed++;
		}
	}

	M_ASSERT(nUsed == _nNew, "!");
}

template<class TElem>
void RemapArray2(const uint8* _pUseTags, const ANIM_COMPRESSED_ARRAYTYPE<TElem>& _lOld, TElem* _pNew, int _nNew)
{
	const TElem* pOld = _lOld.GetBasePtr();

	int nOld = _lOld.Len();
	int nUsed = 0;
	for(int i = 0; i < nOld; i++)
	{
		if (_pUseTags[i >> 3] & (1 << (i & 7)))
		{
			_pNew[nUsed] = pOld[i];
			nUsed++;
		}
	}

	M_ASSERT(nUsed == _nNew, "!");
}


spCXR_Anim_Base CXR_Anim_Compressed::StripSequences(const uint8* _pSeqMask, int _nSeqMask)
{
	if (m_lQuatPalettes.Len() != 1 || m_lMovePalettes.Len() != 1)
	{
		ConOutL("NOTE: will not strip sequences from multi-palette animation. (TODO: fix support for this!)");
		M_TRACE("NOTE: will not strip sequences from multi-palette animation. (TODO: fix support for this!)\n");
		return this;
	}

	if (!_nSeqMask)
		return NULL;

	return StripSequences_i(_pSeqMask, _nSeqMask, this, 0, NULL);
}


spCXR_Anim_Base CXR_Anim_Compressed::StripSequences_i(const uint8* _pSeqMask, int _nSeqMask, CXR_Anim_Compressed* _pRoot, 
                                                      uint32 _iSeqStart, CXR_Anim_Compressed* _pContainer)
{
	ANIM_COMPRESSED_ARRAYTYPE<CXR_Anim_Quatint16>& lQuats = m_lQuatPalettes[0].m_lQuats;
	ANIM_COMPRESSED_ARRAYTYPE<CVec3Dfp32>& lMoves = m_lMovePalettes[0].m_lMoves;

//	if (_nSeqMask >> 3 < m_lspSequences.Len() && !_pSeqMask)
//		return NULL;

	int nUseTagQuats = (lQuats.Len() + 7) >> 3;
	int nUseTagMoves = (lMoves.Len() + 7) >> 3;
	int nUseTagiTracksQ = (m_liTracks.Len() + 7) >> 3;
	int nUseTagiTracksM = (m_liTracks.Len() + 7) >> 3;
	int nUseTagiQuats = (m_liQuats.Len() + 7) >> 3;
	int nUseTagiMoves = (m_liQuats.Len() + 7) >> 3;
	int nUseTagKeyTimes = (m_lKeyTimes.Len() + 7) >> 3;
	int nUseTagTracksQ = (m_lTracks.Len() + 7) >> 3;
	int nUseTagTracksM = (m_lTracks.Len() + 7) >> 3;

	int nUseTags = nUseTagQuats + nUseTagMoves + nUseTagiTracksQ + nUseTagiTracksM + nUseTagiQuats + nUseTagiMoves + nUseTagKeyTimes + nUseTagTracksQ + nUseTagTracksM /*+ nUseTagDataKeys*/;
	TArray<uint8> lUseTagHeap;
	lUseTagHeap.SetLen(nUseTags+4);
	FillChar(lUseTagHeap.GetBasePtr(), lUseTagHeap.ListSize(), 0);
	lUseTagHeap[nUseTags] = 0xfd;
	lUseTagHeap[nUseTags+1] = 0xfd;
	lUseTagHeap[nUseTags+2] = 0xfd;
	lUseTagHeap[nUseTags+3] = 0xfd;

	int niRemap = 2 + lQuats.Len() + lMoves.Len() + m_liTracks.Len()*2 + m_liQuats.Len()*2 + m_liQuats.Len()*2 + m_lKeyTimes.Len() + m_lTracks.Len() * 2/* + m_lDataKeys.Len()*/;
	TArray<uint16> liRemapHeap;
	liRemapHeap.SetLen(niRemap+4);
	FillChar(liRemapHeap.GetBasePtr(), liRemapHeap.ListSize(), 0);
	liRemapHeap[niRemap] = 0xfdfd;
	liRemapHeap[niRemap+1] = 0xfdfd;
	liRemapHeap[niRemap+2] = 0xfdfd;
	liRemapHeap[niRemap+3] = 0xfdfd;

	uint8* pUse = lUseTagHeap.GetBasePtr();
	uint8* pUTQuats = pUse;				pUse += nUseTagQuats;
	uint8* pUTMoves = pUse;				pUse += nUseTagMoves;
	uint8* pUTiTracksQ = pUse;			pUse += nUseTagiTracksQ;
	uint8* pUTiTracksM = pUse;			pUse += nUseTagiTracksM;
	uint8* pUTiQuats = pUse;			pUse += nUseTagiQuats;
	uint8* pUTiMoves = pUse;			pUse += nUseTagiMoves;
	uint8* pUTKeyTimes = pUse;			pUse += nUseTagKeyTimes;
	uint8* pUTTracksQ = pUse;			pUse += nUseTagTracksQ;
	uint8* pUTTracksM = pUse;			pUse += nUseTagTracksM;
	pUse = NULL;

	uint16* piRemap = liRemapHeap.GetBasePtr();
	uint16* piRemapQuats = piRemap;		piRemap += lQuats.Len();
	uint16* piRemapMoves = piRemap;		piRemap += lMoves.Len();
	uint16* piRemapiTracksQ = piRemap;	piRemap += m_liTracks.Len();
	uint16* piRemapiTracksM = piRemap;	piRemap += m_liTracks.Len();

	// Make sure memory is properly aligned
	piRemap = (uint16*)((((mint)piRemap)+3)&~3);
	uint32* piRemapiQuats = (uint32*)piRemap;	piRemap += m_liQuats.Len() * 2;
	piRemap = (uint16*)((((mint)piRemap)+3)&~3);
	uint32* piRemapiMoves = (uint32*)piRemap;	piRemap += m_liQuats.Len() * 2;

	uint16* piRemapKeyTimes = piRemap;	piRemap += m_lKeyTimes.Len();
	uint16* piRemapTracksQ = piRemap;	piRemap += m_lTracks.Len();
	uint16* piRemapTracksM = piRemap;	piRemap += m_lTracks.Len();
	piRemap = NULL;

	int iSeq;
	for(iSeq = _iSeqStart; iSeq < _iSeqStart + m_nSequences; iSeq++)
	{
		if ((iSeq >= (_nSeqMask << 3)) || !ISBITSET(_pSeqMask, iSeq))
			continue;

		CXR_Anim_SequenceCompressed* pS = _pRoot->GetSequenceCompressed(iSeq);
		for(int iRot = 0; iRot < pS->m_nTracksRot; iRot++)
		{
			SET_BIT(pUTiTracksQ, pS->m_iiTracksRot + iRot);
			SET_BIT(pUTTracksQ, m_liTracks[pS->m_iiTracksRot + iRot]);
			const CXR_Anim_CompressedTrack& Track = m_lTracks[m_liTracks[pS->m_iiTracksRot + iRot]];

			if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				SET_BIT(pUTiQuats, iiQuat);
				SET_BIT(pUTQuats, m_liQuats[iiQuat]);
				continue;
			}
			else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				continue;
			}

			uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			const uint16* piTQ = &m_liQuats[iiQuat];
			for(int i = 0; i < pS->m_nKeys; i++)
			{
				SET_BIT(pUTiQuats, iiQuat+i);
				SET_BIT(pUTQuats, piTQ[i]);
			}

		}

		for(int iMove = 0; iMove < pS->m_nTracksMove; iMove++)
		{
			SET_BIT(pUTiTracksM, pS->m_iiTracksMove + iMove);
			SET_BIT(pUTTracksM, m_liTracks[pS->m_iiTracksMove + iMove]);
			const CXR_Anim_CompressedTrack& Track = m_lTracks[m_liTracks[pS->m_iiTracksMove + iMove]];

			if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
				SET_BIT(pUTiMoves, iiQuat);
				SET_BIT(pUTMoves, m_liQuats[iiQuat]);
				continue;
			}
			else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				continue;
			}

			uint iiQuat = ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat);
			const uint16* piTQ = &m_liQuats[iiQuat];
			for(int i = 0; i < pS->m_nKeys; i++)
			{
				SET_BIT(pUTiMoves, iiQuat+i);
				SET_BIT(pUTMoves, piTQ[i]);
			}

		}

		for(int iKey = 0; iKey < pS->m_nKeys; iKey++)
		{
			SET_BIT(pUTKeyTimes, pS->m_iKeyTimes + iKey);
		}
	}

	int nNewQuats = CreateRemapTable(pUTQuats, piRemapQuats, lQuats.Len());
	int nNewMoves = CreateRemapTable(pUTMoves, piRemapMoves, lMoves.Len());
	int nNewiTracksQ = CreateRemapTable(pUTiTracksQ, piRemapiTracksQ, m_liTracks.Len());
	int nNewiTracksM = CreateRemapTable(pUTiTracksM, piRemapiTracksM, m_liTracks.Len());
	int nNewiQuats = CreateRemapTable(pUTiQuats, piRemapiQuats, m_liQuats.Len());
	int nNewiMoves = CreateRemapTable(pUTiMoves, piRemapiMoves, m_liQuats.Len());
	int nNewKeyTimes = CreateRemapTable(pUTKeyTimes, piRemapKeyTimes, m_lKeyTimes.Len());
	int nNewTracksQ = CreateRemapTable(pUTTracksQ, piRemapTracksQ, m_lTracks.Len());
	int nNewTracksM = CreateRemapTable(pUTTracksM, piRemapTracksM, m_lTracks.Len());

	TPtr<CXR_Anim_Compressed> spAnim = MNew(CXR_Anim_Compressed);
	if (!spAnim)
		MemError("StripSequences");

	if(this == _pRoot)
	{
		spAnim->m_lspSequences.SetLen(m_lspSequences.Len());
		_pContainer = spAnim;
	}

	spAnim->m_lQuatPalettes.SetLen(1);
	spAnim->m_lMovePalettes.SetLen(1);

	ANIM_COMPRESSED_ARRAYTYPE<CXR_Anim_Quatint16>& lDestQuats = spAnim->m_lQuatPalettes[0].m_lQuats;
	ANIM_COMPRESSED_ARRAYTYPE<CVec3Dfp32>& lDestMoves = spAnim->m_lMovePalettes[0].m_lMoves;

	RemapArray(pUTQuats, lQuats, lDestQuats, nNewQuats);
	RemapArray(pUTMoves, lMoves, lDestMoves, nNewMoves);
	RemapArray(pUTKeyTimes, m_lKeyTimes, spAnim->m_lKeyTimes, nNewKeyTimes);
	spAnim->m_DataKeys = m_DataKeys;

	spAnim->m_lQuatPalettes[0].m_pQuats = lDestQuats.GetBasePtr();
	spAnim->m_lMovePalettes[0].m_pMoves = lDestMoves.GetBasePtr();

	spAnim->m_liTracks.SetLen(nNewiTracksQ + nNewiTracksM);
	RemapArray2(pUTiTracksQ, m_liTracks, spAnim->m_liTracks.GetBasePtr(), nNewiTracksQ);
	if (nNewiTracksM)
		RemapArray2(pUTiTracksM, m_liTracks, &spAnim->m_liTracks[nNewiTracksQ], nNewiTracksM);

	spAnim->m_lTracks.SetLen(nNewTracksQ + nNewTracksM);
	RemapArray2(pUTTracksQ, m_lTracks, spAnim->m_lTracks.GetBasePtr(), nNewTracksQ);
	if (nNewTracksM)
		RemapArray2(pUTTracksM, m_lTracks, &spAnim->m_lTracks[nNewTracksQ], nNewTracksM);

	spAnim->m_liQuats.SetLen(nNewiQuats + nNewiMoves);
	RemapArray2(pUTiQuats, m_liQuats, spAnim->m_liQuats.GetBasePtr(), nNewiQuats);
	if (nNewiMoves)
		RemapArray2(pUTiMoves, m_liQuats, &spAnim->m_liQuats[nNewiQuats], nNewiMoves);

	for(iSeq = _iSeqStart; iSeq < _iSeqStart + m_nSequences; iSeq++)
	{
		spCXR_Anim_SequenceCompressed spSeq = MNew2(CXR_Anim_SequenceCompressed, spAnim, iSeq);
		if (!spSeq)
			MemError("StripSequences");

		CXR_Anim_SequenceCompressed* pS = _pRoot->GetSequenceCompressed(iSeq);

		_pContainer->m_lspSequences[iSeq] = spSeq;
		if (((iSeq >= (_nSeqMask << 3)) || !ISBITSET(_pSeqMask, iSeq)) || !pS->m_nKeys)
			continue;

		spSeq->m_Duration = pS->m_Duration;
		spSeq->m_Flags = pS->m_Flags;
		spSeq->m_AbsTimeFlag = pS->m_AbsTimeFlag;
		spSeq->m_TimeCode = pS->m_TimeCode;
#ifndef	PLATFORM_CONSOLE
		spSeq->m_Name = pS->m_Name;
		spSeq->m_Comment = pS->m_Comment;
#endif

		M_ASSERT(!pS->m_nTracksRot || ISBITSET(pUTiTracksQ, pS->m_iiTracksRot), "!");
		M_ASSERT(!pS->m_nTracksMove || ISBITSET(pUTiTracksM, pS->m_iiTracksMove), "!");
		spSeq->m_iiTracksRot = piRemapiTracksQ[pS->m_iiTracksRot];
		spSeq->m_iiTracksMove = piRemapiTracksM[pS->m_iiTracksMove] + nNewiTracksQ;
		M_ASSERT(!pS->m_nKeys || ISBITSET(pUTKeyTimes, pS->m_iKeyTimes), "!");
		spSeq->m_iKeyTimes = piRemapKeyTimes[pS->m_iKeyTimes];
		spSeq->m_iDataKeys = pS->m_iDataKeys;
		spSeq->m_nTracksRot = pS->m_nTracksRot;
		spSeq->m_nTracksMove = pS->m_nTracksMove;
		spSeq->m_nKeys = pS->m_nKeys;
		spSeq->m_nDataKeys = pS->m_nDataKeys;
	}

	// Remap quaternion track indices
	uint16* piTracksNew = spAnim->m_liTracks.GetBasePtr();
	for(int iiTrack = 0; iiTrack < nNewiTracksQ; iiTrack++)
	{
		M_ASSERT(ISBITSET(pUTTracksQ, piTracksNew[iiTrack]), "!");
		piTracksNew[iiTrack] = piRemapTracksQ[piTracksNew[iiTrack]];
	}

	// Remap move track indices
	if (nNewiTracksM)
	{
		uint16* piTracksNew = &spAnim->m_liTracks[nNewiTracksQ];
		for(int iiTrack = 0; iiTrack < nNewiTracksM; iiTrack++)
		{
			M_ASSERT(ISBITSET(pUTTracksM, piTracksNew[iiTrack]), "!");
			piTracksNew[iiTrack] = nNewTracksQ + piRemapTracksM[piTracksNew[iiTrack]];
		}
	}

	// Remap quaternion tracks
	CXR_Anim_CompressedTrack* pTrackNew = spAnim->m_lTracks.GetBasePtr();
	for(int iTrack = 0; iTrack < nNewTracksQ; iTrack++)
	{
		CXR_Anim_CompressedTrack& Track = pTrackNew[iTrack];

		if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
		{
			M_ASSERT(ISBITSET(pUTiQuats, ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)), "!");
			Track.m_iiQuat = piRemapiQuats[ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)] | ANIM_COMPRESSEDTRACK_CONSTANT;
			continue;
		}
		else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
		{
			continue;
		}

		M_ASSERT(ISBITSET(pUTiQuats, ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)), "!");
		Track.m_iiQuat = piRemapiQuats[ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)];
	}

	// Remap move tracks
	if (nNewTracksM)
	{
		CXR_Anim_CompressedTrack* pTrackNew = &spAnim->m_lTracks[nNewTracksQ];
		for(int iTrack = 0; iTrack < nNewTracksM; iTrack++)
		{
			CXR_Anim_CompressedTrack& Track = pTrackNew[iTrack];

			if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				M_ASSERT(ISBITSET(pUTiMoves, ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)), "!");
				Track.m_iiQuat = nNewiQuats + piRemapiMoves[ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)] | ANIM_COMPRESSEDTRACK_CONSTANT;
				continue;
			}
			else if (Track.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				continue;
			}

			M_ASSERT(ISBITSET(pUTiMoves, ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)), "!");
			Track.m_iiQuat = nNewiQuats + piRemapiMoves[ANIM_COMPRESSEDTRACK_GETINDEX(Track.m_iiQuat)];
		}
	}

	// Remap quaternion indices
	uint16* piQuatNew = spAnim->m_liQuats.GetBasePtr();
	for(int iiQ = 0; iiQ < nNewiQuats; iiQ++)
	{
		M_ASSERT(ISBITSET(pUTQuats, piQuatNew[iiQ]), "!");
		M_ASSERT(piRemapQuats[piQuatNew[iiQ]] < nNewQuats, "!");
		piQuatNew[iiQ] = piRemapQuats[piQuatNew[iiQ]];
	}

	// Remap move indices
	if (nNewiMoves)
	{
		uint16* piQuatNew = &spAnim->m_liQuats[nNewiQuats];
		for(int iiQ = 0; iiQ < nNewiMoves; iiQ++)
		{
			M_ASSERT(ISBITSET(pUTMoves, piQuatNew[iiQ]), "!");
			M_ASSERT(piRemapMoves[piQuatNew[iiQ]] < nNewMoves, "!");
			piQuatNew[iiQ] = piRemapMoves[piQuatNew[iiQ]];
		}
	}

/*	M_TRACEALWAYS(m_ContainerName.Str());
	M_TRACEALWAYS(":");
	for(int iMask = 0; iMask < _nSeqMask; iMask++)
		M_TRACEALWAYS("%.2x,", _pSeqMask[iMask]);
	M_TRACEALWAYS("  \n");
	M_TRACEALWAYS(CStrF("        %d / %d Quats", lDestQuats.Len(), lQuats.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d Quats", lDestMoves.Len(), lMoves.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d iQuats", spAnim->m_liQuats.Len(), m_liQuats.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d Tracks", spAnim->m_lTracks.Len(), m_lTracks.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d iTracks", spAnim->m_liTracks.Len(), m_liTracks.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d DataKeys", spAnim->m_lDataKeys.Len(), m_lDataKeys.Len()).Str());
	M_TRACEALWAYS(CStrF("        %d / %d KeyTimes\n", spAnim->m_lKeyTimes.Len(), m_lKeyTimes.Len()).Str());
*/
	if (lUseTagHeap[nUseTags] != 0xfd ||
		lUseTagHeap[nUseTags+1] != 0xfd ||
		lUseTagHeap[nUseTags+2] != 0xfd ||
		lUseTagHeap[nUseTags+3] != 0xfd)
	{
		M_ASSERT(0, "!");
	}

	if (liRemapHeap[niRemap] != 0xfdfd ||
		liRemapHeap[niRemap+1] != 0xfdfd ||
		liRemapHeap[niRemap+2] != 0xfdfd ||
		liRemapHeap[niRemap+3] != 0xfdfd)
	{
		M_ASSERT(0, "!");
	}

#ifndef M_RTM
	for(iSeq = _iSeqStart; iSeq < m_nSequences + _iSeqStart; iSeq++)
	{
		if (((iSeq >= (_nSeqMask << 3)) || !ISBITSET(_pSeqMask, iSeq)))
			continue;

		CXR_Anim_SequenceCompressed* pS0 = _pRoot->GetSequenceCompressed(iSeq);
		CXR_Anim_SequenceCompressed* pS1 = _pContainer->GetSequenceCompressed(iSeq);

		M_ASSERT(pS0->m_nKeys == pS1->m_nKeys, "!");
		M_ASSERT(pS0->m_nDataKeys == pS1->m_nDataKeys, "!");
		M_ASSERT(pS0->m_nTracksRot == pS1->m_nTracksRot, "!");
		M_ASSERT(pS0->m_nTracksMove == pS1->m_nTracksMove, "!");

		for(int iKey = 0; iKey < pS0->m_nKeys; iKey++)
		{
			M_ASSERT(m_lKeyTimes[pS0->m_iKeyTimes + iKey] == spAnim->m_lKeyTimes[pS1->m_iKeyTimes + iKey], "!");
		}

		for(int iRot = 0; iRot < pS0->m_nTracksRot; iRot++)
		{
			const CXR_Anim_CompressedTrack& Track0 = m_lTracks[m_liTracks[pS0->m_iiTracksRot + iRot]];
			const CXR_Anim_CompressedTrack& Track1 = spAnim->m_lTracks[spAnim->m_liTracks[pS1->m_iiTracksRot + iRot]];

			M_ASSERT((Track0.m_iiQuat & ~ANIM_COMPRESSEDTRACK_INDEXMASK) == (Track1.m_iiQuat & ~ANIM_COMPRESSEDTRACK_INDEXMASK), "!");

			if (Track0.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				int iiQuat0 = ANIM_COMPRESSEDTRACK_GETINDEX(Track0.m_iiQuat);
				int iiQuat1 = ANIM_COMPRESSEDTRACK_GETINDEX(Track1.m_iiQuat);
				M_ASSERT(lQuats[m_liQuats[iiQuat0]].AlmostEqual(lDestQuats[spAnim->m_liQuats[iiQuat1]], 0), "!");
				continue;
			}
			else if (Track0.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				continue;
			}

			int iiQuat0 = ANIM_COMPRESSEDTRACK_GETINDEX(Track0.m_iiQuat);
			int iiQuat1 = ANIM_COMPRESSEDTRACK_GETINDEX(Track1.m_iiQuat);
			const uint16* piTQ0 = &m_liQuats[iiQuat0];
			const uint16* piTQ1 = &spAnim->m_liQuats[iiQuat1];
			for(int i = 0; i < pS0->m_nKeys; i++)
			{
				M_ASSERT(lQuats[piTQ0[i]].AlmostEqual(lDestQuats[piTQ1[i]], 0), "!");
			}
		}

		for(int iMove = 0; iMove < pS0->m_nTracksMove; iMove++)
		{
			const CXR_Anim_CompressedTrack& Track0 = m_lTracks[m_liTracks[pS0->m_iiTracksMove + iMove]];
			const CXR_Anim_CompressedTrack& Track1 = spAnim->m_lTracks[spAnim->m_liTracks[pS1->m_iiTracksMove + iMove]];

			M_ASSERT((Track0.m_iiQuat & ~ANIM_COMPRESSEDTRACK_INDEXMASK) == (Track1.m_iiQuat & ~ANIM_COMPRESSEDTRACK_INDEXMASK), "!");

			if (Track0.m_iiQuat & ANIM_COMPRESSEDTRACK_CONSTANT)
			{
				int iiQuat0 = ANIM_COMPRESSEDTRACK_GETINDEX(Track0.m_iiQuat);
				int iiQuat1 = ANIM_COMPRESSEDTRACK_GETINDEX(Track1.m_iiQuat);
				M_ASSERT(lMoves[m_liQuats[iiQuat0]] == lDestMoves[spAnim->m_liQuats[iiQuat1]], "!");
				continue;
			}
			else if (Track0.m_iiQuat & ANIM_COMPRESSEDTRACK_UNIT)
			{
				continue;
			}

			int iiQuat0 = ANIM_COMPRESSEDTRACK_GETINDEX(Track0.m_iiQuat);
			int iiQuat1 = ANIM_COMPRESSEDTRACK_GETINDEX(Track1.m_iiQuat);
			const uint16* piTQ0 = &m_liQuats[iiQuat0];
			const uint16* piTQ1 = &spAnim->m_liQuats[iiQuat1];
			for(int i = 0; i < pS0->m_nKeys; i++)
			{
				M_ASSERT(lMoves[piTQ0[i]] == lDestMoves[piTQ1[i]], "!");
			}
		}
	}
#endif

	//Recurse
	if( m_spNext )
	{
		spAnim->m_spNext = (CXR_Anim_Compressed*)m_spNext->StripSequences_i(_pSeqMask, _nSeqMask, _pRoot, _iSeqStart + m_nSequences, _pContainer).p;
	}

	return (CXR_Anim_Base*)spAnim;
}

#ifndef PLATFORM_CONSOLE

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Copies the compressed data in the object		

Parameters:		
_pSrc:		The source object

Comments:	Used for caching and restoring of the cache
\*____________________________________________________________________*/ 
void CXR_Anim_Compressed::CopyCompressedData(const CXR_Anim_Compressed& _Src)
{
	m_lQuatPalettes = _Src.m_lQuatPalettes;
	m_lMovePalettes = _Src.m_lMovePalettes;
	m_liTracks = _Src.m_liTracks;
	m_liQuats = _Src.m_liQuats;
	m_lKeyTimes = _Src.m_lKeyTimes;
	m_lTracks = _Src.m_lTracks;
}

#endif
