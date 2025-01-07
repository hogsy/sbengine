#ifndef _INC_XRAMIMCOMPRESSED
#define _INC_XRAMIMCOMPRESSED

#define XRANIM_DEFUALTCOMPRESSION 0.1f

#include "XRAnim.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_SequenceCompressed
|__________________________________________________________________________________________________
\*************************************************************************************************/


class CXR_Anim_SequenceCompressed : public CXR_Anim_SequenceData
{
/*	
MRTC_DECLARE;
*/
protected:
	bool GetFramesAndTimeFraction(fp32 _Time, int _iFrames[4], fp32& _Fraction) const;
	bool GetFrameAndTimeFraction(fp32 _Time, int& _iFrame, fp32& _Fraction) const;

friend class CXR_Anim_Compressed;
public:
	uint16 m_iiTracksRot;			// Index to first CompressedTrack
	uint16 m_iiTracksMove;			// Index to first CompressedTrack
	uint8 m_nTracksRot;				// Number of used rot-tracks
	uint8 m_nTracksMove;			// Number of used move-tracks
	uint16 m_iKeyTimes;				// Index to first keyframe time
	uint16 m_nKeys;					// Number of keyframes
	uint16 m_iDataKeys;				// Index to first datakey
	uint16 m_nDataKeys;				// Number of datakeys

	uint16 m_iSeq;
	class CXR_Anim_Compressed* m_pContainer;

	CXR_Anim_SequenceCompressed(class CXR_Anim_Compressed* _pContainer, int _iSeq);
	void Clear();

	virtual void Read(class CCFile* _pF, int _ReadFlags = 0);
	virtual void Write(class CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo);

	virtual int GetNumKeys() const { return m_nKeys; };
	virtual fp32 GetFrametime(int _iFrame) const { return 0; };

	virtual CXR_Anim_Keyframe* GetFrame(int _iFrm) { Error("GetFrameData", "Illegal call."); return 0;};

	virtual const CXR_Anim_Keyframe* GetFrame(int _iFrm) const { Error("GetFrame", "Illegal call."); return 0;};
	virtual fp32 GetFrameAbsTime(int _iFrm) const;

	virtual CXR_Anim_RotKey* GetRotFrame(int _iFrm) { return NULL; };
	virtual const CXR_Anim_RotKey* GetRotFrame(int _iFrm) const { return NULL; };
	virtual CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) { return NULL; };
	virtual const CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) const { return NULL; };

	virtual bool GetFramesAndTimeFraction(const CMTime& _Time, int _iFrames[4], fp32& _Fraction) const;

	virtual void EvalRot(const CMTime& _Time, CQuatfp32* _pDest, int _nDest) const;
	virtual void EvalMove(const CMTime& _Time, CVec3Dfp32* _pDest, int _nDest) const;
	virtual void Eval(const CMTime& _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const;
	virtual void Eval(fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const;

	virtual void EvalTrack0(const CMTime& _Time, vec128& _Move0, CQuatfp32& _Rot0) const;
	virtual void GetTotalTrack0(vec128& _Move0, CQuatfp32& _Rot0) const;

	virtual CXR_Anim_DataKey_Sequence GetDataKeys() const;
};

typedef TPtr<CXR_Anim_SequenceCompressed> spCXR_Anim_SequenceCompressed;


enum
{
	ANIM_COMPRESSEDTRACK_INDEXBITS		= 22,	// max ~4M indices
	ANIM_COMPRESSEDTRACK_PALETTEBITS	= 8,	// max 256 palettes		

	ANIM_COMPRESSEDTRACK_INDEXMASK		= DBitRange(0, 21),
	ANIM_COMPRESSEDTRACK_PALETTEMASK	= DBitRange(22, 29),
	ANIM_COMPRESSEDTRACK_UNIT			= M_Bit(30),
	ANIM_COMPRESSEDTRACK_CONSTANT		= M_Bit(31),
};
#define ANIM_COMPRESSEDTRACK_GETINDEX(iiQuat)   (iiQuat & ANIM_COMPRESSEDTRACK_INDEXMASK)
#define ANIM_COMPRESSEDTRACK_GETPALETTE(iiQuat) ((iiQuat & ANIM_COMPRESSEDTRACK_PALETTEMASK) >> ANIM_COMPRESSEDTRACK_INDEXBITS)

class CXR_Anim_CompressedTrack
{
public:
	uint32 m_iiQuat;

	CXR_Anim_CompressedTrack()
	{
	}

	CXR_Anim_CompressedTrack(uint32 _iiQuat) : m_iiQuat(_iiQuat)
	{
	}

#ifndef CPU_LITTLEENDIAN
	void SwapLE()
	{
		::SwapLE(m_iiQuat);
	}
#endif
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_QuatHash
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CQuatHashElem
{
public:
	uint8 m_Dummy;
};

class CXR_Anim_QuatHash : public THash<int32, CQuatHashElem>
{
	int GetHashIndex(const CXR_Anim_Quatint16& _Quat);
	int m_HashAnd;

public:
	void Create(int _nIndices, int _HashShiftSize = 8);
	void Insert(int _iQuat, const CXR_Anim_Quatint16& _Quat);
	int GetIndex(const CXR_Anim_Quatint16& _Quat, CXR_Anim_Quatint16* _pQuats);
};

typedef TPtr<CXR_Anim_QuatHash> spCXR_Anim_QuatHash;




/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Compressed
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifdef PLATFORM_CONSOLE
	#define ANIM_COMPRESSED_ARRAYTYPE TThinArray
#else
	#define ANIM_COMPRESSED_ARRAYTYPE TArray
#endif

typedef TPtr<class CXR_Anim_Compressed>	spCXR_Anim_Compressed;

class CXR_Anim_Compressed : public CXR_Anim_Base
{
protected:
	struct CQuatPalette
	{
		ANIM_COMPRESSED_ARRAYTYPE<CXR_Anim_Quatint16> m_lQuats;
		CXR_Anim_Quatint16* m_pQuats; // not valid during compression!
	};
	struct CVecPalette
	{
		ANIM_COMPRESSED_ARRAYTYPE<CVec3Dfp32> m_lMoves;
		CVec3Dfp32* m_pMoves; // not valid during compression!
	};

	ANIM_COMPRESSED_ARRAYTYPE<CQuatPalette> m_lQuatPalettes;
	ANIM_COMPRESSED_ARRAYTYPE<CVecPalette> m_lMovePalettes;
	ANIM_COMPRESSED_ARRAYTYPE<uint16> m_liTracks;
	ANIM_COMPRESSED_ARRAYTYPE<uint16> m_liQuats;
	ANIM_COMPRESSED_ARRAYTYPE<fp32> m_lKeyTimes;
	ANIM_COMPRESSED_ARRAYTYPE<CXR_Anim_CompressedTrack> m_lTracks;
	CXR_Anim_DataKeys m_DataKeys;

#ifndef PLATFORM_CONSOLE
	spCXR_Anim_QuatHash m_spQuatHash;
#endif
//	ANIM_COMPRESSED_ARRAYTYPE<spCXR_Anim_SequenceCompressed> m_lspSequences;
	static CXR_Anim_TrackMask ms_TrackMaskAll;

	//"Next" pointer
	spCXR_Anim_Compressed	m_spNext;

	//Sequence count
	uint16	m_nSequences;
	uint16	m_nLayeredSequences;

	uint16 CountNodes() const
	{
		const CXR_Anim_Compressed * pItr = (const CXR_Anim_Compressed*)m_spNext.p;
		uint16 ret = 1;
		while( pItr )
		{
			ret++;
			pItr = (const CXR_Anim_Compressed*)pItr->m_spNext.p;
		}
		return ret;
	}

#ifndef PLATFORM_CONSOLE
	//Copy compressed data from one object to the other
	void CopyCompressedData(const CXR_Anim_Compressed & _pSrc);
#endif

	void ReadQuats(uint _iPalette, CDataFile* _pDFile);
	void ReadMoves(uint _iPalette, CDataFile* _pDFile);

public:
	void Init();
	virtual const spCXR_Anim_SequenceData GetSequence(int _iSeq) const;
	virtual       spCXR_Anim_SequenceData GetSequence(int _iSeq);

	CXR_Anim_SequenceCompressed* GetSequenceCompressed(int _iSeq);

	fp32 GetFrameAbsTime(const CXR_Anim_SequenceCompressed* _pSeq, int _iFrm) const;

	void EvalData(const uint16* _piQTracks, const uint16* _piMTracks, int _nQ, int _nMove,
	int _iFrm0, int _iFrm1, int _iFrm2, int _iFrm3, 
	CQuatfp32* _pDest, vec128* _pDestMove, fp32 _tFrac,
	fp32 _t01, fp32 _t12, fp32 _t23, const CXR_Anim_TrackMask& _TrackMask) const;

	void EvalRot(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, CQuatfp32* _pDest, int _nDest) const;
	void EvalMove(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, CVec3Dfp32* _pDest, int _nDest) const;
//	void Eval(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove) const;
	void Eval(const CXR_Anim_SequenceCompressed* _pSeq, const fp32 _Time, CQuatfp32* _pRot, int _nRot, vec128* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const;

	void EvalTrack0(const CXR_Anim_SequenceCompressed* _pSeq, const CMTime& _Time, vec128& _Move0, CQuatfp32& _Rot0) const;
	void GetTotalTrack0(const CXR_Anim_SequenceCompressed* _pSeq, vec128& _Move0, CQuatfp32& _Rot0) const;

	CXR_Anim_DataKeys & GetDataKeys() { return m_DataKeys; }

#ifndef PLATFORM_CONSOLE
	int    Compress_AddQuat(const CXR_Anim_Quatint16& _Quat, TArray<int32>* _pCleared);
	uint32 Compress_AddQuatIndices(const uint16* _piQuat, int _niQuats);
	uint32 Compress_AddQuatIndex(uint16 _iQuat) { return Compress_AddQuatIndices(&_iQuat, 1); }
	int    Compress_AddMove(const CVec3Dfp32& _Move);
	uint16 Compress_AddTrack(const CXR_Anim_CompressedTrack& _Track);
	void Compress(CXR_Anim_Base* _pAnim, const fp32 *_pCompression = NULL, bool _bSkipCulling = false);
	void Write(class CDataFile* _pDFile, const CXR_AnimWriteInfo _WriteInfo);
	void Write(const char* _pFileName, const CXR_AnimWriteInfo _WriteInfo);
#endif
	virtual void Initialize();
	void CalculateTrackmask(int _iSeq, CXR_Anim_TrackMask &_TrackMask);

	virtual int Read(const char* _pFileName, int _ReadFlags = 0);
	virtual int Read(class CDataFile* _pDFile, int _ReadFlags = 0);

	virtual spCXR_Anim_Base StripSequences(const uint8* _pSeqMask, int _nSeqMask);
	spCXR_Anim_Base StripSequences_i(const uint8* _pSeqMask, int _nSeqMask, CXR_Anim_Compressed* _pRoot = NULL, uint32 _iSeqStart = 0, CXR_Anim_Compressed* _pContainer = NULL);

	friend class CXR_Anim_SequenceCompressed;
};



#endif // _INC_XRAMIMCOMPRESSED
