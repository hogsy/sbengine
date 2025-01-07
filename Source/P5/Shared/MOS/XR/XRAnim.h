#ifndef _INC_XRAMIM
#define _INC_XRAMIM

//#include "MCC.h"
#include "MRTC.h"
#include "MMath.h"
#include "MDA.h"

#define ANIM_READ_NONAMES		1
#define ANIM_READ_NOCOMMENTS	2

#ifdef	PLATFORM_CONSOLE
#define USE_QUATERNION16		1
#endif

#define ANIM_MAXROTTRACKS		288
#define ANIM_MAXMOVETRACKS		128


#ifdef PLATFORM_CONSOLE
	#define ANIM_TRACKS_ARRAYTYPE TThinArray
	#define ANIM_BASE_ARRAYTYPE TThinArray
#else
	#define ANIM_TRACKS_ARRAYTYPE TArray
	#define ANIM_BASE_ARRAYTYPE TArray
#endif



/*
// This union simplifies treating a float as an uint32 and vice versa
typedef union
{
	int32 i;
	fp4 f;
} INT32ORFP4;

// Converts a float into an 11 bit signed pseudo float
// Cannot convert values higher than 1.000 or lower than -1.000
inline int16 FloatToInt16(float _v)
{
	INT32ORFP4 v;
	INT32ORFP4 bias;
	
	bias.i = ((23 - 14 + 127) << 23) + (1 << 22);

	v.f = _v;
	v.f += bias.f;
	v.i -= bias.i;
	int16 i = v.i;

	return(i);
}

// Converts values created by FloatToInt16 back into float
// with some loss of precision.
inline float Int16ToFloat(int16 _v)
{
	INT32ORFP4 v;
	INT32ORFP4 bias;

	bias.i = ((23 - 14 + 127) << 23) + (1 << 22);

	v.i = _v;
	v.i += bias.i;
	v.f -= bias.f;
	
	return(v.f);
}

class CQuaternion16
{
public:
	CQuaternion16() {};
	CQuaternion16(const CQuatfp4& _q) { Set(_q); }
	void Unit();
	void Create(const CMat4Dfp4& _Mat);
	void CreateMatrix(CMat4Dfp4& _Mat) const;
	void Set(const CQuatfp4& _q);
	const void Get(CQuatfp4& _q) const; 
	void operator=(const CQuatfp4& _q) { Set(_q); }
	operator CQuatfp4() { CQuatfp4 q; Get(q); return q; }

private:
	int16 k[4];
};
*/

// -------------------------------------------------------------------
template<int TNumTracks>
class TAnim_TrackMask
{
public:
	uint32 m_lMask[TNumTracks / 32];

	void Clear()
	{
		FillChar(&m_lMask, sizeof(m_lMask), 0);
	}

	void Fill()
	{
		FillChar(&m_lMask, sizeof(m_lMask), 0xff);
	}

	int IsEnabled(int _iTrack) const { return m_lMask[_iTrack >> 5] & (1 << (_iTrack & 31)); };
	void Enable(int _iTrack) { m_lMask[_iTrack >> 5] |= (1 << (_iTrack & 31)); };

	void Or(const TAnim_TrackMask& _Mask)
	{
		for(int i = 0; i < TNumTracks / 32; i++)
			m_lMask[i] |= _Mask.m_lMask[i];
	}

	void And(const TAnim_TrackMask& _Mask)
	{
		for(int i = 0; i < TNumTracks / 32; i++)
			m_lMask[i] &= _Mask.m_lMask[i];
	}

	void AndNot(const TAnim_TrackMask& _Mask)
	{
		for(int i = 0; i < TNumTracks / 32; i++)
			m_lMask[i] &= ~_Mask.m_lMask[i];
	}
};

typedef TAnim_TrackMask<ANIM_MAXROTTRACKS> CXR_Anim_TrackMaskRot;
typedef TAnim_TrackMask<ANIM_MAXMOVETRACKS> CXR_Anim_TrackMaskMove;

class CXR_Anim_TrackMask : public CReferenceCount
{
public:
	CXR_Anim_TrackMaskRot m_TrackMaskRot;
	CXR_Anim_TrackMaskMove m_TrackMaskMove;

	void Clear()
	{
		m_TrackMaskRot.Clear();
		m_TrackMaskMove.Clear();
	}

	void Fill()
	{
		m_TrackMaskRot.Fill();
		m_TrackMaskMove.Fill();
	}
	void Copy(const CXR_Anim_TrackMask& _Source)
	{
		m_TrackMaskRot = _Source.m_TrackMaskRot;
		m_TrackMaskMove = _Source.m_TrackMaskMove;
	}

	int IsEnabledRot(int _iTrack) const { return m_TrackMaskRot.IsEnabled(_iTrack); };
	int IsEnabledMove(int _iTrack) const { return m_TrackMaskMove.IsEnabled(_iTrack); };

	void Or(const CXR_Anim_TrackMask& _Mask)
	{
		m_TrackMaskRot.Or(_Mask.m_TrackMaskRot);
		m_TrackMaskMove.Or(_Mask.m_TrackMaskMove);
	}

	void And(const CXR_Anim_TrackMask& _Mask)
	{
		m_TrackMaskRot.And(_Mask.m_TrackMaskRot);
		m_TrackMaskMove.And(_Mask.m_TrackMaskMove);
	}

	void AndNot(const CXR_Anim_TrackMask& _Mask)
	{
		m_TrackMaskRot.AndNot(_Mask.m_TrackMaskRot);
		m_TrackMaskMove.AndNot(_Mask.m_TrackMaskMove);
	}
};

typedef TPtr<CXR_Anim_TrackMask> spCXR_Anim_TrackMask;

// -------------------------------------------------------------------
class CXR_Anim_Quatint16
{
public:
	int16 k[4];

	void Create(const CQuatfp4& _Quat)
	{
		// Codewarrior is _really_ bad at removing stalls so we do it manually
		int16 a, b, c, d;
		a = RoundToInt(_Quat.k[0] * 32767.0f);
		b = RoundToInt(_Quat.k[1] * 32767.0f);
		c = RoundToInt(_Quat.k[2] * 32767.0f);
		d = RoundToInt(_Quat.k[3] * 32767.0f);
		k[0] = a;
		k[1] = b;
		k[2] = c;
		k[3] = d;
	}

	CXR_Anim_Quatint16();

	CXR_Anim_Quatint16(const CXR_Anim_Quatint16& _Quat)
	{
		k[0] = _Quat.k[0]; k[1] = _Quat.k[1]; k[2] = _Quat.k[2]; k[3] = _Quat.k[3];
	}

	CXR_Anim_Quatint16(const CQuatfp4& _Quat)
	{
		Create(_Quat);
	}

	void GetQuatfp4(CQuatfp4& _Quat) const
	{
		// Codewarrior is _really_ bad at removing stalls so we do it manually
		fp4 a, b, c, d;
		a = fp4(k[0]) * (1.0f/32767.0f);
		b = fp4(k[1]) * (1.0f/32767.0f);
		c = fp4(k[2]) * (1.0f/32767.0f);
		d = fp4(k[3]) * (1.0f/32767.0f);
		_Quat.k[0] = a;
		_Quat.k[1] = b;
		_Quat.k[2] = c;
		_Quat.k[3] = d;
	}

	bool AlmostEqual(const CXR_Anim_Quatint16& _Quat, int _Epsilon) const
	{
		if (Abs(_Quat.k[0] - k[0]) > _Epsilon ||
			Abs(_Quat.k[1] - k[1]) > _Epsilon ||
			Abs(_Quat.k[2] - k[2]) > _Epsilon ||
			Abs(_Quat.k[3] - k[3]) > _Epsilon)
			return false;
		return true;
	}

	void Read(class CCFile* _pF, int _Version);
	void Write(class CCFile* _pF) const;
#ifndef CPU_LITTLEENDIAN
	void SwapLE();
#endif
};

// -------------------------------------------------------------------
class CBreakoutPoint
{
public:
	// What entry point index we should jump to
	uint8	m_iBreakout;
};

// -------------------------------------------------------------------
class CEntryPoint
{
public:
	// Entry point index
	uint8 m_iEntry;
	// Keyframe time (should rather have keyframe index later (where we should jump to))
	fp4 m_Time;
};

// -------------------------------------------------------------------
// Mmmkay then, hold breakout/entry points
class CXR_Anim_BreakoutPoints
{
public:
	TThinArray<CBreakoutPoint> m_lPoints;

	void Create(CBreakoutPoint* _lPoints, int32 _NumPoints);
};

// Sync points
class CSyncPoint
{
public:
	// Sync point time
	fp4 m_Time;
	int8 m_Type;
};

// Sync points
class CXR_Anim_SyncPoints
{
public:
	TThinArray<CSyncPoint> m_lPoints;

	void Create(CSyncPoint* _lPoints, int32 _NumPoints);
};

// -------------------------------------------------------------------
class CXR_Anim_EntryPoints
{
public:
	TThinArray<CEntryPoint> m_lPoints;
	
	// Create, (and sort entry points?)
	void Create(CEntryPoint* _lPoints, int32 _NumPoints);

	// Find a matching entry point (if any)
	virtual bool FindMatchingPoint(const CXR_Anim_BreakoutPoints& _Points, CEntryPoint& _Point) const;
};

// -------------------------------------------------------------------
#define DEFAULT_ROTKEY_VERSION	0x03
class CXR_AnimWriteInfo
{
public:
	int m_RotKeyVersion;
	/*
		00, Quaternion float				(4 x float = 16 bytes)
		01, Quaternion float				(4 x float = 16 bytes)
		02, Euler angles 10,10,11 bit		(32 bit = 4 bytes)
		03, Euler angles 10,11,11 bit		(32 bit = 4 bytes)
		04, Euler angles 16,16,16 bit		(3 x 16 bit = 6 bytes)
		05, Quaternion16					(4 x 16 bit = 8 bytes)
	*/
	CXR_AnimWriteInfo()
	{
		MAUTOSTRIP(CXR_AnimWriteInfo_ctor, MAUTOSTRIP_VOID);
		m_RotKeyVersion = DEFAULT_ROTKEY_VERSION;
	};
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Keyframe
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_RotKey
{
public:
#ifdef USE_QUATERNION16
	CXR_Anim_Quatint16	m_Rot;
#else
	CQuatfp4		m_Rot;
#endif

	CXR_Anim_RotKey()
	{
	}

	void GetRot(CQuatfp4& _r);
	void GetRot(CQuatfp4& _r) const;
	void SetRot(const CQuatfp4& _q);

	void Read(class CCFile* _pF, int _Version);
	void Write(class CCFile* _pF, int _Version);
};

// -------------------------------------------------------------------
class CXR_Anim_MoveKey
{
public:
	CVec3Dfp4 m_Move;

	CXR_Anim_MoveKey()
	{
		MAUTOSTRIP(CXR_Anim_MoveKey_ctor, MAUTOSTRIP_VOID);
		m_Move = 0;
	};
	CXR_Anim_MoveKey(CVec3Dfp4 _Move, fp4 _Vel = 1.0f)
	{
		m_Move = _Move;
	}

	void GetMove(CVec3Dfp4& _m);
	void GetMove(CVec3Dfp4& _m) const;
	void SetMove(const CVec3Dfp4& _m);

	void Read(class CCFile* _pF, int _Version);
	void Write(class CCFile* _pF) const;
};

// -------------------------------------------------------------------


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKey1; // old format
class CXR_Anim_DataKey;
class CXR_Anim_DataKeys;
class CXR_Anim_DataKey_Edit;
class CXR_Anim_DataKeys_Edit;
class CXR_Anim_DataKey_Sequence;

enum
{
	ANIM_EVENT_TYPE_SOUND,
	ANIM_EVENT_TYPE_DIALOGUE,
	ANIM_EVENT_TYPE_BREAKOUT,
	ANIM_EVENT_TYPE_ENTRY,
	ANIM_EVENT_TYPE_SYNC,
	ANIM_EVENT_TYPE_EFFECT,
	ANIM_EVENT_TYPE_IK,
	ANIM_EVENT_TYPE_GAMEPLAY,
	ANIM_EVENT_TYPE_AISCRIPT,
	ANIM_EVENT_TYPE_INVALID = 255,

	ANIM_EVENT_TYPE_IK_LEFT_FOOT_DOWN = 0,
	ANIM_EVENT_TYPE_IK_LEFT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_RIGHT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_RIGHT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_LEFT_FRONT_FOOT_UP,
	ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_DOWN,
	ANIM_EVENT_TYPE_IK_RIGHT_FRONT_FOOT_UP,


	ANIM_EVENT_MASK_SOUND		= DBit(ANIM_EVENT_TYPE_SOUND),
	ANIM_EVENT_MASK_DIALOGUE	= DBit(ANIM_EVENT_TYPE_DIALOGUE),
	ANIM_EVENT_MASK_BREAKOUT	= DBit(ANIM_EVENT_TYPE_BREAKOUT),
	ANIM_EVENT_MASK_ENTRY		= DBit(ANIM_EVENT_TYPE_ENTRY),
	ANIM_EVENT_MASK_SYNC		= DBit(ANIM_EVENT_TYPE_SYNC),
	ANIM_EVENT_MASK_EFFECT		= DBit(ANIM_EVENT_TYPE_EFFECT),
	ANIM_EVENT_MASK_GAMEPLAY	= DBit(ANIM_EVENT_TYPE_GAMEPLAY),
	ANIM_EVENT_MASK_IK			= DBit(ANIM_EVENT_TYPE_IK),
	ANIM_EVENT_MASK_AISCRIPT	= DBit(ANIM_EVENT_TYPE_AISCRIPT),

	ANIM_EVENT_EFFECTTYPE_WEAPON = 0,
};

class CXR_Anim_DataKey
{
public:
	fp4    m_AbsTime;		// Time since start of sequence 
	uint8  m_Type;			// ANIM_DATAKEY_TYPE_...
	uint8  m_nSize;			// Size in bytes (sizeof struct + extra data)
	uint16 m_Param;			// parameter. if more is needed, it is put in extra data

	CXR_Anim_DataKey() : m_Type(ANIM_EVENT_TYPE_INVALID) { }

	CXR_Anim_DataKey(uint8 _Type, fp4 _AbsTime) 
		: m_AbsTime(_AbsTime)
		, m_Type(_Type)
		, m_nSize(sizeof(CXR_Anim_DataKey))
		, m_Param(0)
	{ }

	// Extra Data may follow directly after the struct
	uint8 DataSize() const { return m_nSize - sizeof(CXR_Anim_DataKey); }
	const char* Data() const { return (char*)(this + 1); }
	      char* Data()       { return (char*)(this + 1); }

	void SwapLE();
	void SwapLE_Data();
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKeys
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKeys
{
protected:
	TThinArray<uint8> m_lData;
	TThinArray<CXR_Anim_DataKey*> m_lpKeys;	// pointers into m_lData[] or m_lpKeys[]

	void RebuildIndex();

public:
	void Read(CCFile* _pFile, uint _nVersion);
	void Write(CCFile* _pFile) const;
	void AddFromSequence(const CXR_Anim_DataKey_Sequence& _Seq);

	CXR_Anim_DataKeys& operator= (const CXR_Anim_DataKeys& _DataKeys);

	const TThinArray<CXR_Anim_DataKey*>& GetKeys() const { return m_lpKeys; }
	uint GetDataSize() const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey_Edit
| This struct directly contains its extra data, to make life easier for tools code etc..
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKey_Edit : public CXR_Anim_DataKey
{
public:
	char m_Data[64];

	CXR_Anim_DataKey_Edit() { m_Data[0] = 0; }
	CXR_Anim_DataKey_Edit(uint8 _Type, fp4 _AbsTime);
	CXR_Anim_DataKey_Edit(const CXR_Anim_DataKey& _Key);

	CXR_Anim_DataKey_Edit& operator= (const CXR_Anim_DataKey& _Key);

	void SetData(const char* _pData, uint8 _nSize);
	void SetData(const CStr& _Data);

	void Read(CCFile* _pFile, uint _nVer);
	void Write(CCFile* _pFile) const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKeys_Edit
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKeys_Edit : public CXR_Anim_DataKeys
{
	TArray<CXR_Anim_DataKey_Edit> m_lKeys;

	void RebuildIndex();

public:
	CXR_Anim_DataKeys_Edit();
	~CXR_Anim_DataKeys_Edit();

	CXR_Anim_DataKeys_Edit& operator= (const CXR_Anim_DataKeys_Edit& _DataKeys);
	
	CXR_Anim_DataKey_Edit &GetKey(int _iKey) { return m_lKeys[_iKey]; }
	int GetNumKeys() { return m_lKeys.Len(); }

	void Read(CCFile* _pFile, uint _nVer);
	void Write(CCFile* _pFile) const;

	uint GetNumEntries(fp4 _AbsTime) const;
	int  FindKeyIndex(fp4 _AbsTime, uint _iEntry) const;

	void Clear();
	bool RemoveKey(fp4 _AbsTime, uint _iEntry);
	void RemoveKey(int _iKey);

	void AddKey(const CXR_Anim_DataKey_Edit& _Key);
	uint AddKey(const CXR_Anim_DataKey1& _OldKey);
	void AddFromSequence(const CXR_Anim_DataKey_Sequence& _Seq);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey_Sequence - helper class to aid working on specified ranges of a data key container
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKey_Sequence
{
	const CXR_Anim_DataKeys& m_Container;
	uint16 m_iStart;
	uint16 m_nKeys;

public:
	CXR_Anim_DataKey_Sequence(const CXR_Anim_DataKeys& _Container, uint16 _iStart, uint16 _nKeys)
		: m_Container(_Container), m_iStart(_iStart), m_nKeys(_nKeys) { }

	uint GetNumKeys() const { return m_nKeys; }
	CXR_Anim_DataKey* const* GetKeys() const { return m_Container.GetKeys().GetBasePtr() + m_iStart; }
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_DataKey1 - used to keep support for old file versions
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_DataKey1
{
public:
	enum
	{
		ANIM_EVENT0TYPE_BREAKOUT	= 1,
		ANIM_EVENT0TYPE_ENTRY		= 2,

		ANIM_DATAKEY_NUMPARAMS      = 4,
	};

	fp4		m_AbsTime;
	CStr	m_Sound;
	uint16	m_EventParams[ANIM_DATAKEY_NUMPARAMS];
	uint16	m_iDialogue;

	void Read(CCFile* _pF);
};




// -------------------------------------------------------------------
class CXR_Anim_Keyframe;
typedef TPtr<CXR_Anim_Keyframe> spCXR_Anim_Keyframe;

class CXR_Anim_Keyframe : public CReferenceCount
{
	MRTC_DECLARE;

public:
	spCXR_Anim_Keyframe Duplicate() const;
	virtual void operator= (const CXR_Anim_Keyframe& _Keyframe);

	fp4 m_AbsTime;

	TArray<CXR_Anim_RotKey> m_lRotKeys;
	TArray<CXR_Anim_MoveKey> m_lMoveKeys;

//	int16 GetFlags() { return m_Data.m_EventParams[0]; }
//	void SetFlags(int16 _Flags) { m_Data.m_EventParams[0] = _Flags; }

	void Read(class CCFile* _pF, uint _nVersion);
	void Write(class CCFile* _pF, const CXR_AnimWriteInfo& _WriteInfo);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Sequence
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	ANIM_SEQFLAGS_LOOP = 1,
	ANIM_SEQFLAGS_NEVERFULLBODY = 2,
	ANIM_SEQFLAGS_ALWAYSFULLBODY = 4,
	ANIM_SEQFLAGS_FROMSPINE = 8,
	ANIM_SEQFLAGS_NOZROTATE = 16,
	ANIM_SEQFLAGS_NOOBJECTROTATION = 32,
	ANIM_SEQFLAGS_HIGHPRECISIONROT = 64,
	ANIM_SEQFLAGS_NOOBJECTMOVE = 128,
	ANIM_SEQFLAGS_APPLYTIMESCALE = 256,
	//ANIM_SEQFLAGS_HASBREAKOUTPOINTS = 512,
	//ANIM_SEQFLAGS_HASENTRYPOINTS = 1 << 12,
	ANIM_SEQFLAGS_HASBREAKOUTPOINTS = 1 << 12,
//	ANIM_SEQFLAGS_HOLDATEND = 512,
	
	ANIM_SEQFLAGS_LOOPTYPE_SHIFT = 9,
	ANIM_SEQFLAGS_LOOPTYPE_MASK = 3 << ANIM_SEQFLAGS_LOOPTYPE_SHIFT,
	
	ANIM_SEQ_LOOPTYPE_ONCE = 0,
	ANIM_SEQ_LOOPTYPE_CONTINUOUS = 1,
	ANIM_SEQ_LOOPTYPE_OSCILLATING = 2,
	
	ANIM_SEQFLAGS_AIMING_MASK = 8192 | 16384 | 32768,
	ANIM_SEQFLAGS_AIMING_SHIFT = 13,

	ANIM_SEQ_AIMING_NORMAL = 0,
	ANIM_SEQ_AIMING_BODY = 1,
	ANIM_SEQ_AIMING_NONE = 2,
	ANIM_SEQ_AIMING_HEAD = 3,
	ANIM_SEQ_AIMING_FULLBODY = 4,
	AIMING_TYPE_RIOTGUARD = 5,
	AIMING_TYPE_HEAVYGUARD = 6,
	AIMING_TYPE_MINIGUN = 7,
	AIMING_TYPE_EXOSKELETON = 8,
	ANIM_SEQ_AIMING_CROUCHBODY = 9,
	ANIM_SEQ_AIMING_CARRYBODY = 10,
};

// -------------------------------------------------------------------
class CXR_Anim_SequenceData : public CReferenceCount
{
protected:
	fp4 m_Duration;

	bool GetFramesAndTimeFraction(fp4 _Time, int _iFrames[4], fp4& _Fraction) const;
	bool GetFrameAndTimeFraction(fp4 _Time, int& _iFrame, fp4& _Fraction) const;

	void EvalRot(fp4 _Time, CQuatfp4* _pDest, int _nDest) const;
	void EvalMove(fp4 _Time, CVec3Dfp4* _pDest, int _nDest) const;

	void EvalTrack0(fp4 _Time, CVec3Dfp4& _Move0, CQuatfp4& _Rot0) const;

	bool IsPlaying(fp4 _Time) const;
public:
	uint16 m_Flags;
	uint16 m_AbsTimeFlag;
	CMTime m_TimeCode;
	CXR_Anim_TrackMask m_TrackMask;

	int16 m_iNextAnimSequenceLayer; // Sequence-index in same container that has next part of the animation (when body/face is split up for example)
	CXR_Anim_SequenceData *m_pNextAnimSequenceLayer; // Direct pointer to next sequence

#ifndef	PLATFORM_CONSOLE
	CStr m_Name;
	CStr m_Comment;
#else
	uint32 m_NameHash;
#endif
	CXR_Anim_SequenceData() { Clear();}
	~CXR_Anim_SequenceData() {};

	void Clear();
	virtual fp4 GetDuration() const
	{
		return m_Duration;
	}
	virtual void SetDuration(fp4 _Duration)
	{
		m_Duration = _Duration;
	};

	// Returns the duration of the frame
	// 0 is returned for frames before 0 and for the last and subsequent frames
	virtual fp4 GetFrameDuration(int _iFrm) const;

	virtual int GetNumKeys() const = 0;
	virtual fp4 GetFrametime(int _iFrame) const = 0;

	virtual CXR_Anim_Keyframe* GetFrame(int _iFrm) = 0;

	virtual const CXR_Anim_Keyframe* GetFrame(int _iFrm) const = 0;
	virtual fp4 GetFrameAbsTime(int _iFrm) const { return GetFrame(_iFrm)->m_AbsTime; }

	virtual CXR_Anim_RotKey* GetRotFrame(int _iFrm) = 0;
	virtual const CXR_Anim_RotKey* GetRotFrame(int _iFrm) const = 0;
	virtual CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) = 0;
	virtual const CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) const = 0;

	virtual void Initialize();
	virtual void AlignRotations();
	virtual bool GetFramesAndTimeFraction(const CMTime& _Time, int _iFrames[4], fp4& _Fraction) const;
	virtual bool GetFrameAndTimeFraction(const CMTime& _Time, int& _iFrame, fp4& _Fraction) const;

	virtual void EvalRot(const CMTime& _Time, CQuatfp4* _pDest, int _nDest) const;
	virtual void EvalMove(const CMTime& _Time, CVec3Dfp4* _pDest, int _nDest) const;
	virtual void Eval(const CMTime& _Time, CQuatfp4* _pRot, int _nRot, CVec3Dfp4* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const;
	virtual void Eval(fp4 _Time, CQuatfp4* _pRot, int _nRot, CVec3Dfp4* _pMove, int _nMove, const CXR_Anim_TrackMask& _TrackMask) const;

	virtual void EvalTrack0(const CMTime& _Time, CVec3Dfp4& _Move0, CQuatfp4& _Rot0) const;
	virtual void GetTotalTrack0(CVec3Dfp4& _Move0, CQuatfp4& _Rot0) const;

	virtual int GetLoopType() const { return ((m_Flags & ANIM_SEQFLAGS_LOOPTYPE_MASK) >> ANIM_SEQFLAGS_LOOPTYPE_SHIFT); };
	virtual void SetLoopType(int _LoopType) { m_Flags = (m_Flags & ~ANIM_SEQFLAGS_LOOPTYPE_MASK) | (_LoopType<< ANIM_SEQFLAGS_LOOPTYPE_SHIFT); };
	virtual CMTime GetLoopedTime(const CMTime& _Time) const;

	virtual CMTime GetTimeCode();

	bool HasEvents(uint16 _Mask) const;
	const CXR_Anim_DataKey* GetEvents(CMTime& _BeginTime, CMTime _EndTime = CMTime::CreateInvalid(), uint16 _Mask = -1) const;
	const CXR_Anim_DataKey* GetEvents(CMTime _BeginTime, CMTime _EndTime, uint16 _Mask, int16& _iKey) const;

	void  CheckSequenceForBreakouts();
	uint FindBreakoutPoints(CXR_Anim_BreakoutPoints& _Points, const CMTime& _AnimTime) const;
	uint FindBreakoutPoints(CBreakoutPoint* _pPoints, uint& _MaxPoints, const CMTime& _AnimTime) const;
	uint FindSyncPoints(CXR_Anim_SyncPoints& _Points) const;
	uint FindSyncPoints(CSyncPoint* _pPoints, uint& _MaxPoints) const;
	uint FindEntryPoints(CXR_Anim_EntryPoints& _Points) const;
	uint FindEntryPoints(CEntryPoint* _pPoints, uint& _MaxPoints) const;
	fp4  FindEntryTime(const CXR_Anim_SequenceData* _pOldSeq, const CMTime& _OldTime, int* _pBreakoutID = NULL) const;

	static void Interpolate(fp4 _Frac, 
		CQuatfp4* _pRot0, CQuatfp4* _pRot1, CQuatfp4* _pRotDst, int _nRot, 
		CVec3Dfp4* _pMove0, CVec3Dfp4* _pMove1, CVec3Dfp4* _pMoveDst, int _nMove);

	virtual bool IsPlaying(const CMTime& _Time) const;
	virtual uint ReadData(class CCFile* _pF, int _ReadFlags = 0);
	virtual void WriteData(class CCFile* _pF) const;
	virtual uint ReadDataCompact(class CCFile* _pF, int _ReadFlags = 0);

	virtual CXR_Anim_DataKey_Sequence GetDataKeys() const = 0;
};

typedef TPtr<CXR_Anim_SequenceData> spCXR_Anim_SequenceData;

// -------------------------------------------------------------------
class CXR_Anim_Sequence;
typedef TPtr<CXR_Anim_Sequence> spCXR_Anim_Sequence;

class CXR_Anim_Sequence : public CXR_Anim_SequenceData
{
	MRTC_DECLARE;

public:
	TArray<spCXR_Anim_Keyframe>		m_lspKeys;
	CXR_Anim_DataKeys_Edit          m_Events;    // List of data keys

	CXR_Anim_Sequence();
	~CXR_Anim_Sequence();

	virtual spCXR_Anim_Sequence Duplicate() const;
	virtual void operator= (const CXR_Anim_Sequence& _Sequence);

	virtual void Clear();
	virtual int GetNumKeys() const;
	virtual fp4 GetFrametime(int _iFrame) const;

	// GetFlags,SetFlags: Reads param 0 of the first event. Method moved from CXR_Anim_Keyframe
	// as it will be removed later on.
	virtual void AddFrame(spCXR_Anim_Keyframe _spFrame);

	virtual CXR_Anim_Keyframe* GetFrame(int _iFrm);
	virtual const CXR_Anim_Keyframe* GetFrame(int _iFrm) const;

	virtual CXR_Anim_RotKey* GetRotFrame(int _iFrm);
	virtual const CXR_Anim_RotKey* GetRotFrame(int _iFrm) const;
	virtual CXR_Anim_MoveKey* GetMoveFrame(int _iFrm);
	virtual const CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) const;

	virtual CXR_Anim_DataKey_Sequence GetDataKeys() const;

	virtual void Read(class CCFile* _pF, int _ReadFlags = 0);
	virtual void ReadCompact(class CCFile* _pF, int _ReadFlags = 0);
	virtual void Write(class CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo);
	// virtual void WriteCompact(class CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo);
	virtual void Initialize();
	virtual void CalculateTrackmask(CXR_Anim_TrackMask &_TrackMask);

	virtual void RenderGraph(const char* _pFileName);
};

// ==========================================================================================
// CXR_Anim_SequenceTracks
// ==========================================================================================
// CXR_Anim_SequenceTracks
// CXR_Anim_SequenceTracks will replace CXR_Anim_Sequence in storing animation sequences
// Goals
// Eventually reduce the memory demand by allowing CAnim_TracksOptimizer to cull
// keys independently for each track. Note that if no animation optimization is performed
// CXR_Anim_SequenceTracks will INCREASE memory requirements by 25% for rotkeys and 50%(?)
// for movekeys. It should only marginally reduce execution speed.
//
// m_lEvents
// =========
// A list of CXR_Anim_DataKey. This event track is entirely separate from the move/rot
// tracks to save on memory. This also makes it easier to reimport animation data without clobbering
// the event data. Individual events are looked up using GetEvents() using binary search.
// m_lTimes
// ========
// A list of absolute time values used by the rota and moves.
// m_lRotTracks, m_lMoveTracks
// ===========================
// Lists of CXR_Anim_RotTrack,CXR_Anim_MoveTrack, one for each animation track of the sequence.
// The two classes are identical aside from the contents of their m_lData members
// who hold an CXR_Anim_RotKey or CXR_Anim_MoveKey respectively.
// m_lData: List holding the actual animation data (positions/quaternions).
// m_liTimes: List of indices into the m_lTimes.
// m_liKeys: List of indices into m_lData and m_liTimes
// 
// When we want to evaluate the sequence at a specific time t we do as follows:
// We find the actual time index ti using GetFrameAndTimeFraction(). We use ti as an index
// into m_lKeys to get another index which we use as index into m_liTimes and m_lData.
// The value we get from m_liTimes is used as index into m_lTimes to get the time value.
//  Still didn't get it? OK; I'll try again.
// TBD
// Q: Why not store time values inside m_lData?
// A: Many keys share the same time and as we store time m_liTimes as uint16 (2 bytes) compared to
// m_lTimes fp4 (4 bytes). We win 2 bytes for every track with coincident time values.

class CXR_Anim_RotTrack;
typedef TPtr<CXR_Anim_RotTrack> spCXR_Anim_RotTrack;
class CXR_Anim_RotTrack
{
public:
	CXR_Anim_RotTrack();
	void Clear();
	const CXR_Anim_RotKey* GetKeyAndTimeindex(const int _iKey,int& _iTime) const;
	const CXR_Anim_RotKey* GetKeyAndTimeindex_Offset(const int _iKey,const int _Offset,int& _iTime) const;
	void operator= (const CXR_Anim_RotTrack& _Track);
	void Read(class CCFile* _pF, int _ReadFlags = 0);
	void Write(class CCFile* _pF);

	ANIM_TRACKS_ARRAYTYPE<CXR_Anim_RotKey>	m_lData;
	ANIM_TRACKS_ARRAYTYPE<uint16>			m_liTimes;
	ANIM_TRACKS_ARRAYTYPE<uint16>			m_liKeys;
};

class CXR_Anim_MoveTrack;
typedef TPtr<CXR_Anim_MoveTrack> spCXR_Anim_MoveTrack;
class CXR_Anim_MoveTrack
{
public:
	CXR_Anim_MoveTrack();
	void Clear();
	const CXR_Anim_MoveKey* GetKeyAndTimeindex(const int _iKey,int& _iTime) const;
	const CXR_Anim_MoveKey* GetKeyAndTimeindex_Offset(const int _iKey,const int _Offset,int& _iTime) const;
	void operator= (const CXR_Anim_MoveTrack& _Track);
	void Read(class CCFile* _pF, int _ReadFlags = 0);
	void Write(class CCFile* _pF);

	ANIM_TRACKS_ARRAYTYPE<CXR_Anim_MoveKey>	m_lData;
	ANIM_TRACKS_ARRAYTYPE<uint16>			m_liTimes;
	ANIM_TRACKS_ARRAYTYPE<uint16>			m_liKeys;
};



class CXR_Anim_SequenceTracks;
typedef TPtr<CXR_Anim_SequenceTracks> spCXR_Anim_SequenceTracks;

class CXR_Anim_SequenceTracks : public CXR_Anim_SequenceData
{
	MRTC_DECLARE;

friend class CAnim_TracksOptimizer;

//protected:
public:
	CXR_Anim_DataKeys_Edit							m_Events;	// List of data keys
	ANIM_TRACKS_ARRAYTYPE<fp4>						m_lTimes;	// t0,t1,t2 etc

	ANIM_TRACKS_ARRAYTYPE<CXR_Anim_RotTrack>		m_lRotTracks;
	ANIM_TRACKS_ARRAYTYPE<CXR_Anim_MoveTrack>		m_lMoveTracks;
public:
	CXR_Anim_SequenceTracks();
	~CXR_Anim_SequenceTracks();
	
	// Clears out all lists
	virtual void Clear();
	virtual spCXR_Anim_SequenceTracks Duplicate() const;
	virtual void operator= (const CXR_Anim_SequenceTracks& _Seq);

	// ConvertSequence uses the data _Seq to fill its own data structures
	virtual void Initialize();
	virtual void AlignRotations();
	virtual void ConvertSequence(const CXR_Anim_Sequence* _Seq);
	spCXR_Anim_Sequence ConvertSequence();

	virtual int GetNumKeys() const;
	virtual fp4 GetFrametime(int _iFrame) const;
	//virtual void AddFrame(spCXR_Anim_Keyframe _spFrame);
	void EvalRot(fp4 _Time, CQuatfp4* _pDest, int _nDest) const;
	void EvalRotTrack(fp4 _Time,CQuatfp4* _pDest,int _iTrack) const;
	void EvalMove(fp4 _Time, CVec3Dfp4* _pDest, int _nDest) const;
	void EvalMoveTrack(fp4 _Time,CVec3Dfp4* _pDest,int _iTrack) const;
	void Eval(fp4 _Time, CQuatfp4* _pRot, int _nRot, CVec3Dfp4* _pMove, int _nMove) const;
	virtual void EvalTrack0(fp4 _Time, CVec3Dfp4& _Move0, CQuatfp4& _Rot0) const;
	virtual void GetTotalTrack0(CVec3Dfp4& _Move0, CQuatfp4& _Rot0) const;
	virtual bool GetFrameAndTimeFraction(fp4 _Time, int& _iFrame, fp4& _tFrac) const;
	virtual fp4 GetFrameDuration(int _iFrame) const;
	virtual fp4 GetFrameDuration(int _iFrame,int _iTrack,bool _RotTrack);
	virtual CXR_Anim_DataKey_Sequence GetDataKeys() const;

	virtual uint ReadData(class CCFile* _pF, int _ReadFlags = 0);
	virtual void WriteData(class CCFile* _pF);
	
	virtual void Read(class CCFile* _pF, int _ReadFlags = 0);
	virtual void ReadCompact(class CCFile* _pF, int _ReadFlags = 0);
	virtual void Write(class CCFile* _pF, const CXR_AnimWriteInfo _WriteInfo);

	virtual CXR_Anim_Keyframe* GetFrame(int _iFrm);
	virtual const CXR_Anim_Keyframe* GetFrame(int _iFrm) const;

	virtual CXR_Anim_RotKey* GetRotFrame(int _iFrm)
	{
		return(NULL);
	}

	virtual const CXR_Anim_RotKey* GetRotFrame(int _iFrm) const
	{
		return(NULL);
	}
	virtual CXR_Anim_MoveKey* GetMoveFrame(int _iFrm)
	{
		return(NULL);
	}
	virtual const CXR_Anim_MoveKey* GetMoveFrame(int _iFrm) const
	{
		return(NULL);
	}
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Anim_Base
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Anim_Base;
typedef TPtr<CXR_Anim_Base> spCXR_Anim_Base;
class CXR_Anim_Base : public CReferenceCount
{
	MRTC_DECLARE;
	
public:
	ANIM_BASE_ARRAYTYPE<spCXR_Anim_SequenceData> m_lspSequences;

	CStr m_ContainerName;

	CXR_Anim_Base();
	~CXR_Anim_Base();

	virtual spCXR_Anim_Base Duplicate() const;
	virtual void operator= (const CXR_Anim_Base& _Anim);

	void AddSequence(spCXR_Anim_SequenceData _Seq);
	virtual const spCXR_Anim_SequenceData GetSequence(int _iSeq) const;
	virtual       spCXR_Anim_SequenceData GetSequence(int _iSeq);
	int GetNumSequences() const { return m_lspSequences.Len(); }

	virtual int Read(const char* _pFileName, int _ReadFlags = 0);
	virtual int Read(class CDataFile* _pDFile, int _ReadFlags = 0);
	virtual spCXR_Anim_Base StripSequences(const uint8* _pSeqMask, int _nSeqMask);

	void ReadTracks(class CDataFile* _pDFile, int _ReadFlags = 0);
	void ReadAnim(class CDataFile* _pDFile, int _ReadFlags = 0);
	void ReadCompressed(class CDataFile* _pDFile, int _ReadFlags = 0);
	void Write(class CDataFile* _pDFile, const CXR_AnimWriteInfo _WriteInfo);
	void Write(const char* _pFileName, const CXR_AnimWriteInfo _WriteInfo);
	const CXR_Anim_DataKey* GetEvents(int _iSeq, CMTime &_BeginTime, const CMTime& _EndTime, int _Mask = -1);

	virtual spCXR_Anim_Base ReadMultiFormat(const char* _pFileName, int _Flags, const uint8* _pSeqMask = NULL, int _nSeqMask = 0);
	virtual spCXR_Anim_Base ReadMultiFormat(class CDataFile *_pDFile, int _Flags, const uint8* _pSeqMask = NULL, int _nSeqMask = 0);
};



#endif // _INC_XRAMIM
