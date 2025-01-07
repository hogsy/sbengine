#ifndef __WOBJ_POSHISTORY_H
#define __WOBJ_POSHISTORY_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Helper class for accessing static matrix-animations

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		TBitField
					CWO_PosHistory
\*____________________________________________________________________________________________*/

#include "MRTC.h"
#include "MDA.h"
#include "MMath.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Position History
					
	Author:			Jens Andersson
					
	Copyright:		Starbreeze Studios AB
					
	Contents:		CWO_PosHistory
\*____________________________________________________________________________________________*/

#ifndef PLATFORM_CONSOLE
struct CPosHistory_EditData
{
	TArray<TArray<CMat4Dfp32> >	m_lPath;
	TArray<TArray<fp32> >		m_lTimes;
	TArray<TArray<uint8> >		m_lFlags;
	TArray<fp32> m_lTotTime;

	void SetNumSequences(int _nSeq)
	{
		m_lPath.SetLen(_nSeq);
		m_lTimes.SetLen(_nSeq);
		m_lFlags.SetLen(_nSeq);
		m_lTotTime.SetLen(_nSeq);
	}

	void SetNumKeyframes(int _iSeq, int _nKeyframes)
	{
		m_lPath[_iSeq].SetLen(_nKeyframes);
		m_lTimes[_iSeq].SetLen(_nKeyframes);
		m_lFlags[_iSeq].SetLen(_nKeyframes);
	}

	void SetKeyframe(int _iSeq, int _iKeyframe, CMat4Dfp32 &_Mat, fp32 _Time, uint8 _Flags = 0)
	{
		m_lPath[_iSeq][_iKeyframe] = _Mat;
		m_lTimes[_iSeq][_iKeyframe] = _Time;
		m_lFlags[_iSeq][_iKeyframe] = _Flags;
	}

	void SetKeyframe(	int _iSeq, int _iKeyframe, CQuatfp32 &_Rot, CVec3Dfp32 &_Pos,
		fp32 _Time, uint8 _Flags=0)
	{
		CMat4Dfp32 Mat;
		_Rot.CreateMatrix(Mat);
		_Pos.SetMatrixRow(Mat, 3);
		SetKeyframe(_iSeq, _iKeyframe, Mat, _Time, _Flags);
	}

	void Init()
	{
		for(int s = 0; s < m_lTotTime.Len(); s++)
		{
			m_lTotTime[s] = m_lTimes[s][m_lTimes[s].Len() - 1];
		}
	}

	int Load(const void* _pData); // returns version
	bool Save(TArray<uint8>& _Data, bool _bLodded) const; // returns true if succesful

	int SetMatrix(int _iSequence, const CMat4Dfp32& _Mat, fp32 _Time, uint8 _Flags = 0);
	void LodDynamics(fp32 _Time);
};
#endif // PLATFORM_CONSOLE




enum
{
	POSHISTORY_RESOURCEID = 1000,
	POSHISTORY_PACKED_RESOURCEID = 1001,
	
	POSHISTORY_IDMASK = 0xffff,
	POSHISTORY_FLAGS = 0x10000,
};

template<int _Bytes>
class TBitField
{
protected:
	unsigned char m_Data[_Bytes];
	
public:
	unsigned int GetUInt(int _Bits, int &_Pos) const
	{
		int Res = 0;

		int i = _Pos >> 3;
		int Shift = (_Pos & 7);
		int Mask = (1 << _Bits) - 1;
		_Pos += _Bits;
		
		if(Shift > 0)
		{
			Res |= m_Data[i++] >> Shift;
			Mask >>= 8 - Shift;
			Shift = 8 - Shift;
		}
		
		while(Mask)
		{
			Res |= int(m_Data[i++] & Mask) << Shift;
			Shift += 8;
			Mask >>= 8;
		}
		return Res;
	}

	int GetInt(int _Bits, int &_Pos) const
	{
		int Res = GetUInt(_Bits, _Pos);
		if(Res & (1 << (_Bits - 1)))
			Res |= ~(1 << _Bits);
		return Res;
	}
	
	fp32 GetFloat(int _Bits, int &_Pos) const
	{
		int IntRes = GetUInt(_Bits, _Pos) << (32 - _Bits);
		return *(fp32 *)&IntRes;
	}

	void AddInt(unsigned int _Integer, int _Bits, int &_Pos)
	{
		int i = _Pos >> 3;
		int Shift = (_Pos & 7);
		int Mask = (1 << _Bits) - 1;
		_Pos += _Bits;
		
		if(Shift > 0)
		{
			m_Data[i] &= ~(Mask << Shift);
			m_Data[i++] |= _Integer << Shift;
			_Integer >>= 8 - Shift;
			Mask >>= 8 - Shift;
		}
		
		while(Mask)
		{
			m_Data[i++] = (_Integer & Mask);
			Mask >>= 8;
			_Integer >>= 8;
		}
	}
	
	void AddFloat(fp32 _Float, int _Bits, int &_Pos)
	{
		int Val = (*((int *)&_Float)) >> (32 - _Bits);
		AddInt(Val, _Bits, _Pos);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_PosHistory
|__________________________________________________________________________________________________
\*************************************************************************************************/

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Position History
						
	Comments:		Handles resource data used by Engine_Path and other
					classes, which haves numerous sequences of position-
					keyframes.
\*____________________________________________________________________*/

class CWO_PosHistory : public CReferenceCount
{
	class CSequence
	{
	public:
		const uint32* m_pData;
		int m_Version;

		int GetVersion() const;
		fp32 GetTime(int _iKeyframe) const;
		int GetNumKeyframes() const;
		fp32 GetDuration() const;
		int GetTimeIndex(fp32 _Time) const;
		fp32 GetKeyframeLength(int _iKeyframe) const;
		bool GetMatrix(fp32 _Time, bool _bLoop, int _iInterpolateType, CMat4Dfp32 &_Result) const;
		void GetMatrix(int _iKeyframe, CMat4Dfp32 &_Result) const;
		int GetFlags(int _iKeyframe) const;
		
		CVec3Dfp32 GetPos(int _iKeyframe) const;
		CQuatfp32 GetRot(int _iKeyframe) const;
	};

	CMat4Dfp32 m_Transform;
	bool m_bTransform;

public:

#if defined(PLATFORM_PS2) && defined( COMPILER_CODEWARRIOR )
#pragma push
#pragma pack(1)
#else
#pragma pack( push, 1 )
#endif
	class CFileKeyframe
	{
	private:
		fp32 m_Time;
		CVec3Dfp32 m_Pos;
		CQuatfp32 m_Rot;
	public:
#ifdef CPU_LITTLEENDIAN
		fp32              GetTime() const { return m_Time; }
		const CVec3Dfp32& GetPos() const  { return m_Pos; }
		const CQuatfp32&  GetRot() const  { return m_Rot; }
#else
		fp32              GetTime() const { fp32 Tmp = m_Time; SwapLE(Tmp); return Tmp; }
		const CVec3Dfp32  GetPos() const  { CVec3Dfp32 Tmp = m_Pos; Tmp.SwapLE(); return Tmp; }
		const CQuatfp32   GetRot() const  { CQuatfp32 Tmp = m_Rot; Tmp.SwapLE(); return Tmp; }
#endif	

#ifndef M_RTM
		void Create(fp32 _Time, const CVec3Dfp32 &_Pos, const CQuatfp32 &_Rot)
		{
			m_Time = _Time;
			m_Pos = _Pos;
			memcpy(&m_Rot, &_Rot, sizeof(m_Rot));
			if(m_Rot.k[3] < 0)
			{
				m_Rot.k[0] = -m_Rot.k[0];
				m_Rot.k[1] = -m_Rot.k[1];
				m_Rot.k[2] = -m_Rot.k[2];
				m_Rot.k[3] = -m_Rot.k[3];
			}
		}
#endif
	};

	class CFileKeyframe_Packed
	{
private:
		uint8 m_Time[2];
		TBitField<10> m_Pos;
		TBitField<4> m_Rot;
public:
#ifndef M_RTM
		void Create(fp32 _Time, const CVec3Dfp32 &_Pos, const CQuatfp32 &_Rot)
		{
			int Ticks = RoundToInt(_Time * 20);
			m_Time[0] = Ticks >> 8;
			m_Time[1] = Ticks & 0xff;

			int Pos = 0;
			m_Pos.AddFloat(_Pos[0], 27, Pos);
			m_Pos.AddFloat(_Pos[1], 27, Pos);
			m_Pos.AddFloat(_Pos[2], 26, Pos);

			Pos = 0;

			CQuatfp32 Q = _Rot;
			if(Q.k[3] < 0)
			{
				Q.k[0] = -Q.k[0];
				Q.k[1] = -Q.k[1];
				Q.k[2] = -Q.k[2];
				Q.k[3] = -Q.k[3];
			}
			
			m_Rot.AddInt(RoundToInt((Q.k[0] + 1) * (1 << 10)), 11, Pos);
			m_Rot.AddInt(RoundToInt((Q.k[1] + 1) * (1 << 10)), 11, Pos);
			m_Rot.AddInt(RoundToInt((Q.k[2] + 1) * (1 << 9)), 10, Pos);
		}
#endif

		fp32 GetTime() const
		{
			int Ticks = (int(m_Time[0]) << 8) | m_Time[1];
			return fp32(Ticks) * (1.0f / 20.0f);
		}
		
		const CVec3Dfp32 GetPos() const
		{
			CVec3Dfp32 V;
			int Pos = 0;
			V[0] = m_Pos.GetFloat(27, Pos);
			V[1] = m_Pos.GetFloat(27, Pos);
			V[2] = m_Pos.GetFloat(26, Pos);
			return V;
		}
		
		const CQuatfp32 GetRot() const
		{
			CQuatfp32 Q;
			int Pos = 0;
			Q.k[0] = (fp32(m_Rot.GetUInt(11, Pos)) * (1.0f / ((1 << 10)))) - 1;
			Q.k[1] = (fp32(m_Rot.GetUInt(11, Pos)) * (1.0f / ((1 << 10)))) - 1;
			Q.k[2] = (fp32(m_Rot.GetUInt(10, Pos)) * (1.0f / ((1 << 9)))) - 1;
			Q.k[3] = M_Sqrt(1.0f - Min(Sqr(Q.k[0]) + Sqr(Q.k[1]) + Sqr(Q.k[2]), 1.0f));

			return Q;
		}
	};
#if defined(PLATFORM_PS2) && defined(COMPILER_CODEWARRIOR)
#pragma pop
#else
#pragma pack( pop )
#endif

	TArray<CSequence> m_lSequences;
	TArray<TArray<CMat4Dfp32> >	m_lCache;

	CWO_PosHistory();
	
	bool IsTransformed();
	CMat4Dfp32& GetTransformation();

	void LoadPath(const void* _pData, const CMat4Dfp32 &_Transform);
	bool IsValid();

	bool GetMatrix(int _iSeq, fp32 _Time, bool _bLoop, int _iInterpolateType, CMat4Dfp32 &_Result);
	bool GetCacheMatrix(int _iSeq, fp32 _Time, CMat4Dfp32& _Pos);
	void GenerateCache(fp32 _NumSamples);
};

#endif
