
#ifndef _INC_MREGISTRY
#define _INC_MREGISTRY

#include "../SysInc.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CRegistry
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios 1999,2000,2001

 	Creation Date:	1999-06-07

	Contents:		CRegistry
					
	Comments:

		UNICODE NOTE: 
			(2001-07-26)
			Key names can only be ANSI strings. (8-bit char strings)
			String key-values can be either ANSI _or_ UNICODE.

		MEMORY NOTE:

			Beware of cross-module memory references when adding 
			CStr and TArray keys to registries.

			If there's reason to believe a registry will persist after
			a module has been unloaded, add CStr values as char* and
			TArray values as uint8*.

			Update 2000-10-01:

			Strings are now always feed to CRegistry as const char*.
			Hence, CRegistry is always the creator of it's CStr and the
			cross-module memory problems mentioned above does not apply
			to CStr keys.

		DATA-REGISTRY NOTE:

			When adding TArray values please note that the registry will
			by default reference the supplied registry rather than 
			copying the data to a new array. As an effect, you can alter 
			the content of a data-registry by modifying a shared TArray.


		MEMORY CONSUMPTION:

			An empty CRegistry object is 56 bytes.
			Key-name and string-value consume 
				((16+7) & ~7) + 8 bytes for CStrData.(*)
				((StrLen+1+7) & ~7) + 8 for the string data.(*)
			Integer value is free.
			Float value is free.
			Data-value consume 
				((24+7) & ~7) + 8 bytes for CArrayData.(*)
				((Size+7) & ~7) + 8 for the data.(*)

			Possible remedies:
				- Remove inheritance from CReferenceCount for CStrBase, saving vtbl ptr for all CStr:s. (8 bytes total)
				- Remove inheritance from CReferenceCount for TArray<>, saving vtbl ptr for all TArrays. (4 bytes total)
				- Make union of integer and float value vars. (4 bytes total)
				- Move m_ldValue, m_lspChildren & m_spHash to an external struct. (12-4 to 20-4 bytes total)

			(*) Memory block overhead numbers assume Win98SE, but should be identical on all win32 platforms.

  			Update 2001-07-26:

			CStr memory consumption is slightly different than mentionened above.
			This is due to the fact that CStr was rewritten a few months ago to
			only allocated a single block of memory that is used for both string
			information and the string data.


		FUNCTION POSTFIXES

			i = Integer
			f = float, fp32
			d = data, TArray<uint8>
			nothing = string

					
\*____________________________________________________________________________________________*/

class CRegistry;
typedef TPtr2<CRegistry, CVirtualRefCount> spCRegistry;

typedef TArray<spCRegistry> lspCRegistry;
typedef TPtr<lspCRegistry> splspCRegistry;

enum
{
	EGetKFFlags_VNormal = 0,
	EGetKFFlags_VLoopEnd = 1,
	EGetKFFlags_VSeqEnd = 2,
	EGetKFFlags_VAssign = 3,
	EGetKFFlags_TypeShift = 30,
	EGetKFFlags_Type = 3 << EGetKFFlags_TypeShift,
	EGetKFFlags_Value = ~EGetKFFlags_Type,
};

enum
{
	REGISTRY_TYPE_VOID			= 0,
	REGISTRY_TYPE_STR			= 1,
	REGISTRY_TYPE_DATA			= 2,
	REGISTRY_TYPE_UINT8			= 3,
	REGISTRY_TYPE_INT16			= 4,
	REGISTRY_TYPE_INT32			= 5,
	REGISTRY_TYPE_FP32			= 6,
	REGISTRY_TYPE_FP2			= 7,
	REGISTRY_TYPE_UINT32		= 8,
	REGISTRY_TYPE_MAX,

	REGISTRY_ANIMFLAGS_LOOP				= 1,
	REGISTRY_ANIMFLAGS_LOOP_PINGPONG	= 2,
	REGISTRY_ANIMFLAGS_CONTIGUOUS		= 4,

	REGISTRY_ANIMIP_NONE			= 0,
	REGISTRY_ANIMIP_LINEAR,
	REGISTRY_ANIMIP_CUBIC,
	REGISTRY_ANIMIP_QUADRIC,
	REGISTRY_ANIMIP_CARDINAL,		// Parameters: Per point tension, Constant tension
	REGISTRY_ANIMIP_CARDINAL_TIMEDISCONNECTED,		// Parameters: Per point tension, Constant tension
	REGISTRY_ANIMIP_LINEAR_VEC3QUAT,
	REGISTRY_ANIMIP_CUBIC_VEC3QUAT,
	REGISTRY_ANIMIP_MAX,

	REGISTRY_MAX_DIMENSIONS = 32,

	REGISTRY_BINARY_VERSION			= 0x204
};


class CRegistry_ParseContext
{
public:
	CStr m_ThisFileName;
	TArray<CStr> m_lDefines;
};

typedef void (FRegistryTypeDestructor)(void *_pData, int _nDim);
typedef void (FRegistryTypeConstructor)(void *_pData, int _nDim);
typedef void (FRegistryTypeConvert)(const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst);



// -------------------------------------------------------------------
class SYSTEMDLLEXPORT CRegistry : public CVirtualRefCount
{
	MRTC_DECLARE;
public:
/*
	DECLARE_OPERATOR_NEW
*/
// Interface

	virtual void operator= (const CRegistry& _Reg) pure;

	virtual void Clear() pure;

	virtual spCRegistry MakeThreadSafe() pure;
	virtual void SetChild(int _iChild, CRegistry*_pReg) pure;

	virtual CRegistry *GetParent() const pure;
	virtual void SetParent(CRegistry *) pure;

	virtual bint ValidChild(int _iChild) const pure;

	virtual int GetNumChildren() const pure;
	virtual void SetNumChildren(int _nChildren) pure;
	virtual CRegistry* GetChild(int _iChild) pure;
	virtual const CRegistry* GetChild(int _iChild) const pure;
	virtual const CRegistry* GetChildUnsafe(int _iChild) const pure;


	virtual int GetType() const pure;
	virtual int GetDimensions() const pure;	
	virtual void ConvertToType(int _Type, int _nDim = -1) pure;

	virtual void SimulateRegistryCompiled(bint _Enable) {}
	virtual bint SimulateRegistryCompiled() {return 0;}

	// Sort register
	virtual void Sort(bint _bReversedOrder = false, bint _bSortByName = true) pure;

	// Search children
	virtual int FindIndex(const char* _pKey) const pure;
	virtual int FindIndex(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true) const pure;

	virtual CRegistry* CreateDir(const char* _pPath) pure;
	virtual bint DeleteDir(const char* _pPath) pure;
	virtual void CopyDir(const CRegistry* _pReg, bint _bRecursive = true) pure;

	virtual void CopyValue(const CRegistry* _pReg) pure;

	virtual void AddReg(spCRegistry _spReg) pure;

	virtual CRegistry *CreateNewChild() pure;

	virtual void DeleteKey(int _iKey) pure;
	virtual void Hash_Invalidate() pure;

	// Setting
	virtual void SetThisName(const char* _pName) pure;

	virtual void SetThisValue(const char* _pValue) pure;

	virtual void SetThisValueConvert(CStr _Value, int _nDim, int _Type) pure;

	virtual void SetThisValue(const wchar* _pValue) pure;
	virtual void SetThisValue(CStr _Value) pure;
	virtual void SetThisValuei(int32 _Value, int _StoreType = REGISTRY_TYPE_INT32) pure;
	virtual void SetThisValuef(fp32 _Value, int _StoreType = REGISTRY_TYPE_FP32) pure;
	virtual void SetThisValued(const uint8* _pValue, int _Size, bint _bQuick = true) pure;
	virtual void SetThisValued(TArray<uint8> _lValue, bint _bReference = true) pure;

	virtual void SetThisValuea(int _nDim, const CStr *_Value) pure;
	virtual void SetThisValuead(int _nDim, const TArray<uint8> *_lValue, bint _bReference = true) pure;
	virtual void SetThisValueai(int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32) pure;
	virtual void SetThisValueaf(int _nDim, const fp32 *_Value, int _StoreType = REGISTRY_TYPE_FP32) pure;


	/////////////////////////////////////////////////////////////////
	// Animation
	////////////////////////////////////////////////////////////////

	virtual void Anim_ThisSetAnimated(bint _bEnable) pure;
	virtual bint Anim_ThisGetAnimated() const pure;

	virtual void Anim_ThisSetDisableAutoDemote(bint _bEnable) pure;
	virtual bint Anim_ThisGetDisableAutoDemote() const pure;

	virtual void Anim_ThisSetEnableTimed(bint _bEnable) pure;
	virtual bint Anim_ThisGetEnableTimed() const pure;

	virtual void Anim_ThisSetFlags(uint32 _Flags) pure;
	virtual uint32 Anim_ThisGetFlags() const pure;

	virtual void Anim_ThisSetInterpolate(uint32 _InterpolateType, const fp32 *_pParams, int _nParams) pure;
	virtual uint32 Anim_ThisGetInterpolate(fp32 *_pParams, int &_nParams) const pure;

	virtual bint Anim_ThisIsValidSequenceKeyframe(int _iSec, int _iKF) const pure;

	virtual int Anim_ThisAddSeq() pure;
	virtual int Anim_ThisInsertSeq(int _iSeqAfter) pure;
	virtual void Anim_ThisDeleteSeq(int _iSeq) pure;
	virtual void Anim_ThisSetNumSeq(int _nSeq) pure;
	virtual int Anim_ThisGetNumSeq() const pure;

	virtual fp32 Anim_ThisGetSeqLoopStart(int _iSeq) const pure;
	virtual fp32 Anim_ThisGetSeqLoopEnd(int _iSeq) const pure;
	virtual fp32 Anim_ThisGetSeqLength(int _iSeq) const pure;

	virtual void Anim_ThisSetSeqLoopStart(int _iSeq, fp32 _Time) pure;
	virtual void Anim_ThisSetSeqLoopEnd(int _iSeq, fp32 _Time) pure;

	virtual void Anim_ThisGetKF(int _iSeq, fp32 _Time, fp32 &_Fraction, int *_pKeys, fp32 *_pTimeDeltas, int _nPre, int _nPost) const pure;
	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp32 &_Fraction, int *_pKeys, fp32 *_pTimeDeltas, int _nPre, int _nPost) const pure;
	virtual void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp32 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const pure;

	virtual void Anim_ThisSetNumKF(int _iSeq, int _nKF) pure;
	virtual int Anim_ThisGetNumKF(int _iSeq = 0) const pure;

	virtual void Anim_ThisDeleteKF(int _iSeq, int _iKF) pure;

	virtual int Anim_ThisSetKFTime(int _iSeq, int _iKF, fp32 _Time)  pure;
	virtual fp32 Anim_ThisGetKFTime(int _iSeq, int _iKF) const pure;

	virtual fp32 Anim_ThisGetWrappedTime(const CMTime &_Time, int _iSeq = 0) const pure;

	// Adds
	virtual int Anim_ThisAddKF(int _iSeq, CStr _Val, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFi(int _iSeq, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFf(int _iSeq, fp32 _Val, int _StoreType = REGISTRY_TYPE_FP32, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFd(int _iSeq, const uint8* _pValue, int _Size, bint _bQuick = true, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFd(int _iSeq, TArray<uint8> _lValue, bint _bReference = true, fp32 _Time = -1) pure;

	virtual int Anim_ThisAddKFa(int _iSeq, int _nDim, const CStr *_pVal, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFai(int _iSeq, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFaf(int _iSeq, int _nDim, const fp32 *_pVal, int _StoreType = REGISTRY_TYPE_FP32, fp32 _Time = -1) pure;
	virtual int Anim_ThisAddKFad(int _iSeq, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true, fp32 _Time = -1) pure;

	// Set
	virtual void Anim_ThisSetKFValueConvert(int _iSeq, int _iKF, CStr _Val, int _nDim, int _StoreType, fp32 _Time) pure;

	virtual void Anim_ThisSetKFValue(int _iSeq, int _iKF, CStr _Val) pure;
	virtual void Anim_ThisSetKFValuei(int _iSeq, int _iKF, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32) pure;
	virtual void Anim_ThisSetKFValuef(int _iSeq, int _iKF, fp32 _Val, int _StoreType = REGISTRY_TYPE_FP32) pure;
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, const uint8* _pValue, int _Size, bint _bQuick = true) pure;
	virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, TArray<uint8> _lValue, bint _bReference = true) pure;

	virtual void Anim_ThisSetKFValuea(int _iSeq, int _iKF, int _nDim, const CStr *_pVal) pure;
	virtual void Anim_ThisSetKFValueai(int _iSeq, int _iKF, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32) pure;
	virtual void Anim_ThisSetKFValueaf(int _iSeq, int _iKF, int _nDim, const fp32 *_pVal, int _StoreType = REGISTRY_TYPE_FP32) pure;
	virtual void Anim_ThisSetKFValuead(int _iSeq, int _iKF, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true) pure;

	// Get Value
	virtual CStr Anim_ThisGetKFValue(int _iSeq, int _iKF) const pure;
	virtual int32 Anim_ThisGetKFValuei(int _iSeq, int _iKF) const pure;
	virtual fp32 Anim_ThisGetKFValuef(int _iSeq, int _iKF) const pure;
	virtual TArray<uint8> Anim_ThisGetKFValued(int _iSeq, int _iKF) const pure;

	virtual void Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest) const pure;
	virtual void Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest) const pure;
	virtual void Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp32 *_pDest) const pure;
	virtual void Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const pure;


	/////////////////////////////
	///////////////////////////////
	/////////////////////////////

	// Getting value from this

	// Get/Set user flags, user flags are 8-bit
	virtual void SetThisUserFlags(uint32 _Value) pure;
	virtual uint32 GetThisUserFlags() const pure;

	virtual CStr GetThisName() const pure;
	virtual uint32 GetThisNameHash() const pure;
	virtual const char* GetThisNameUnsafe() const pure;

	virtual CStr GetThisValue() const pure;
	virtual int32 GetThisValuei() const pure;
	virtual fp32 GetThisValuef() const pure;
	virtual const TArray<uint8> GetThisValued() const pure;
	virtual TArray<uint8> GetThisValued() pure;

	virtual void GetThisValuea(int _nDim, CStr *_pDest) const pure;
	virtual void GetThisValueai(int _nDim, int32 *_pDest) const pure;
	virtual void GetThisValueaf(int _nDim, fp32 *_pDest) const pure;
	virtual void GetThisValuead(int _nDim, TArray<uint8> *_pDest) const pure;

	////////////////// IO
	
	virtual int XRG_Parse(char* _pOrgData, char* _pData, int _Size, CRegistry_ParseContext& _ParseContext) pure;
	virtual int XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CRegistry_ParseContext& _ParseContext) pure;
	virtual void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) pure;	// Unicode detection is automatic.
	virtual void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) pure;	// Unicode detection is automatic.
	virtual void XRG_Read(const CStr& _Filename) pure;								// XRG_Read(_FileName, TArray<CStr>::TArray<CStr>(), true)

	virtual void ReadSimple(CCFile* _pFile) pure;									// Unicode detection is automatic.
	virtual void ReadSimple(CStr _FileName) pure;									// Unicode detection is automatic.
	virtual void ReadRegistryDir(CStr _Dir, CCFile* _pFile) pure;
	virtual void ReadRegistryDir(CStr _Dir, CStr _Filename) pure;

	// -------------------------------------------------------------------
	// Binary IO
	virtual void Read(CCFile* _pFile, int _Version) pure;
	virtual void Write(CCFile* _pFile) const pure;

	// Read/Write from data-file entry.
	virtual bint Read(CDataFile* _pDFile, const char* _pEntryName) pure;
	virtual void Write(CDataFile* _pDFile, const char* _pEntryName) const pure;

	// Read/Write datafile.
	virtual void Read(const char* _pFileName) pure;
	virtual void Write(const char* _pFileName) const pure;

	/////////////////////////////////
	// Override possible
	/////////////////////////////////

	virtual spCRegistry Duplicate() const;

	/////////////////////////////////
	// Implemented in CRegistry
	/////////////////////////////////

	CRegistry* GetDir(const char* _pDir);
	const CRegistry* GetDir(const char* _pDir) const;
	int operator== (const CRegistry& _Reg);
	int operator!= (const CRegistry& _Reg);

	int GetNumNodes_r() const;		// Nodes in entire tree including this

	// Get/Set user flags, user flags are 8-bit
	void SetUserFlags(int _iKey, uint32 _Value);
	uint32 GetUserFlags(int _iKey) const;
	void SetUserFlags(const char* _pName, uint32 _Value);
	uint32 GetUserFlags(const char* _pName) const;

	CStr GetThisValuesAsStr(int _ReqLevel) const;

	CRegistry* FindChild(const char* _pKey);
	const CRegistry* FindChild(const char* _pKey) const;
	CRegistry* FindChild(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true);
	const CRegistry* FindChild(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true) const;

	// Search with pathname  (ex. pSys->GetRegistry()->Find("SORCERY\\USER\\NAME");   )
	CRegistry* Find(const char* _pKey);
	const CRegistry* Find(const char* _pKey) const;

	spCRegistry EvalTemplate_r(const CRegistry* _pReg, bool _bRecursive = true) const;

	//// Set

	void SetThisKey(const char* _pName, const char* _pValue);
	void SetThisKey(const char* _pName, const wchar* _pValue);
	void SetThisKey(const char* _pName, CStr _Value);
	void SetThisKeyi(const char* _pName, int32 _Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetThisKeyf(const char* _pName, fp32 _Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetThisKeyd(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick = true);
	void SetThisKeyd(const char* _pName, TArray<uint8> _lValue, bint _bReference = true);

	void SetThisKeya(const char* _pName, int _nDim, const CStr *_Value);
	void SetThisKeyai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetThisKeyaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetThisKeyad(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);

	// Adding keys (NOTE: AddKey always adds a new key. It does NOT modify any existing keys with the same name.)
	void AddKey(const char* _pName, const char* _pValue);
	void AddKey(const char* _pName, const wchar* _pValue);
	void AddKey(const char* _pName, CStr _Value);
	void AddKeyi(const char* _pName, int32 _Value, int _StoreType = REGISTRY_TYPE_INT32);
	void AddKeyf(const char* _pName, fp32 _Value, int _StoreType = REGISTRY_TYPE_FP32);
	void AddKeyd(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick = true);
	void AddKeyd(const char* _pName, TArray<uint8> _lValue, bint _bReference = true);
	void AddKey(const CRegistry* _pReg);

	void AddKeya(const char* _pName, int _nDim, const CStr *_Value);
	void AddKeyai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32);
	void AddKeyaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType = REGISTRY_TYPE_FP32);
	void AddKeyad(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);

	// Setting keys (Checks for matching key and modifies it's value, otherwise a new key is created.)
	void SetValue(const char* _pName, const char* _pValue);
	void SetValue(const char* _pName, const wchar* _pValue);
	void SetValue(const char* _pName, CStr _Value);
	void SetValuei(const char* _pName, int32 _Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetValuef(const char* _pName, fp32 _Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetValued(const char* _pName, const uint8* _pValue, int _Size, bint _bQuick = true);
	void SetValued(const char* _pName, TArray<uint8> _lValue, bint _bReference = true);

	void SetValuea(const char* _pName, int _nDim, const CStr *_Value);
	void SetValueai(const char* _pName, int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetValueaf(const char* _pName, int _nDim, const fp32 *_Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetValuead(const char* _pName, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);

	// Special
	void SetValue(const CRegistry* _pReg);

	// Change value of a key specified by index.
	void SetValue(int _iKey, const char* _pValue);
	void SetValue(int _iKey, const wchar* _pValue);
	void SetValue(int _iKey, CStr _Value);
	void SetValuei(int _iKey, int32 _Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetValuef(int _iKey, fp32 _Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetValued(int _iKey, const uint8* _pValue, int _Size, bint _bQuick = true);
	void SetValued(int _iKey, TArray<uint8> _lValue, bint _bReference = true);

	void SetValuea(int _iKey, int _nDim, const CStr *_Value);
	void SetValueai(int _iKey, int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32);
	void SetValueaf(int _iKey, int _nDim, const fp32 *_Value, int _StoreType = REGISTRY_TYPE_FP32);
	void SetValuead(int _iKey, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true);

	// Special
	void SetValue(int _iKey, const CRegistry* _pReg);

	void RenameKey(const char* _pName, const char* _pNewName);
	void RenameKey(int _iKey, const char* _pNewName);


	///////////////////////////////////////////////////////////////////
	// Animation
	///////////////////////////////////////////////////////////////////

	///////////////////////
	// This
	///////////////////////

	// Get Value
	CStr Anim_ThisGetKFValue(int _iSeq, int _iKF, CStr _Default) const;
	int32 Anim_ThisGetKFValuei(int _iSeq, int _iKF, int32 _Default) const;
	fp32 Anim_ThisGetKFValuef(int _iSeq, int _iKF, fp32 _Default) const;
	TArray<uint8> Anim_ThisGetKFValued(int _iSeq, int _iKF, const TArray<uint8> &_Default) const;

	void Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	// Get Interpolated Value
	CStr Anim_ThisGetIPValue(int _iSeq, const CMTime &_Time, CStr _Default) const;
	int32 Anim_ThisGetIPValuei(int _iSeq, const CMTime &_Time, int32 _Default) const;
	fp32 Anim_ThisGetIPValuef(int _iSeq, const CMTime &_Time, fp32 _Default) const;
	TArray<uint8> Anim_ThisGetIPValued(int _iSeq, const CMTime &_Time, const TArray<uint8> &_Default) const;

	void Anim_ThisGetIPValuea(int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_ThisGetIPValueai(int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_ThisGetIPValueaf(int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_ThisGetIPValuead(int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_ThisGetIPValue(int _iSeq, fp32 _Time, CStr _Default) const;
	int32 Anim_ThisGetIPValuei(int _iSeq, fp32 _Time, int32 _Default) const;
	fp32 Anim_ThisGetIPValuef(int _iSeq, fp32 _Time, fp32 _Default) const;
	TArray<uint8> Anim_ThisGetIPValued(int _iSeq, fp32 _Time, const TArray<uint8> &_Default) const;

	void Anim_ThisGetIPValuea(int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_ThisGetIPValueai(int _iSeq, fp32 _Time, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_ThisGetIPValueaf(int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_ThisGetIPValuead(int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_ThisGetIPValue(int _iSeq, const CMTime &_Time) const;
	int32 Anim_ThisGetIPValuei(int _iSeq, const CMTime &_Time) const;
	fp32 Anim_ThisGetIPValuef(int _iSeq, const CMTime &_Time) const;
	TArray<uint8> Anim_ThisGetIPValued(int _iSeq, const CMTime &_Time) const;

	void Anim_ThisGetIPValuea(int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;
	void Anim_ThisGetIPValueai(int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest) const;
	void Anim_ThisGetIPValueaf(int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest) const;
	void Anim_ThisGetIPValuead(int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest) const;

	CStr Anim_ThisGetIPValue(int _iSeq, fp32 _Time) const;
	int32 Anim_ThisGetIPValuei(int _iSeq, fp32 _Time) const;
	fp32 Anim_ThisGetIPValuef(int _iSeq, fp32 _Time) const;
	TArray<uint8> Anim_ThisGetIPValued(int _iSeq, fp32 _Time) const;

	void Anim_ThisGetIPValuea(int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;
	void Anim_ThisGetIPValueai(int _iSeq, fp32 _Time, int _nDim, int32 *_pDest) const;
	void Anim_ThisGetIPValueaf(int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest) const;
	void Anim_ThisGetIPValuead(int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest) const;

	///////////////////////
	// Key ID
	///////////////////////

	// Get Value
	CStr Anim_GetKFValue(int _iKey, int _iSeq, int _iKF) const;
	int32 Anim_GetKFValuei(int _iKey, int _iSeq, int _iKF) const;
	fp32 Anim_GetKFValuef(int _iKey, int _iSeq, int _iKF) const;
	TArray<uint8> Anim_GetKFValued(int _iKey, int _iSeq, int _iKF) const;

	void Anim_GetKFValuea(int _iKey, int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;
	void Anim_GetKFValueai(int _iKey, int _iSeq, int _iKF, int _nDim, int32 *_pDest) const;
	void Anim_GetKFValueaf(int _iKey, int _iSeq, int _iKF, int _nDim, fp32 *_pDest) const;
	void Anim_GetKFValuead(int _iKey, int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const;

	// Get Value
	CStr Anim_GetKFValue(int _iKey, int _iSeq, int _iKF, CStr _Default) const;
	int32 Anim_GetKFValuei(int _iKey, int _iSeq, int _iKF, int32 _Default) const;
	fp32 Anim_GetKFValuef(int _iKey, int _iSeq, int _iKF, fp32 _Default) const;
	TArray<uint8> Anim_GetKFValued(int _iKey, int _iSeq, int _iKF, const TArray<uint8> &_Default) const;

	void Anim_GetKFValuea(int _iKey, int _iSeq, int _iKF, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetKFValueai(int _iKey, int _iSeq, int _iKF, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_GetKFValueaf(int _iKey, int _iSeq, int _iKF, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_GetKFValuead(int _iKey, int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	// Get Interpolated Value
	CStr Anim_GetIPValue(int _iKey, int _iSeq, const CMTime &_Time, CStr _Default) const;
	int32 Anim_GetIPValuei(int _iKey, int _iSeq, const CMTime &_Time, int32 _Default) const;
	fp32 Anim_GetKFValuef(int _iKey, int _iSeq, const CMTime &_Time, fp32 _Default) const;
	TArray<uint8> Anim_GetIPValued(int _iKey, int _iSeq, const CMTime &_Time, const TArray<uint8> &_Default) const;

	void Anim_GetIPValuea(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetIPValueai(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_GetKFValueaf(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_GetIPValuead(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_GetIPValue(int _iKey, int _iSeq, fp32 _Time, CStr _Default) const;
	int32 Anim_GetIPValuei(int _iKey, int _iSeq, fp32 _Time, int32 _Default) const;
	fp32 Anim_GetIPValuef(int _iKey, int _iSeq, fp32 _Time, fp32 _Default) const;
	TArray<uint8> Anim_GetIPValued(int _iKey, int _iSeq, fp32 _Time, const TArray<uint8> &_Default) const;

	void Anim_GetIPValuea(int _iKey, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetIPValueai(int _iKey, int _iSeq, fp32 _Time, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void Anim_GetIPValueaf(int _iKey, int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void Anim_GetIPValuead(int _iKey, int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_GetIPValue(int _iKey, int _iSeq, const CMTime &_Time) const;
	int32 Anim_GetIPValuei(int _iKey, int _iSeq, const CMTime &_Time) const;
	fp32 Anim_GetIPValuef(int _iKey, int _iSeq, const CMTime &_Time) const;
	TArray<uint8> Anim_GetIPValued(int _iKey, int _iSeq, const CMTime &_Time) const;

	void Anim_GetIPValuea(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueai(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, int32 *_pDest) const;
	void Anim_GetIPValueaf(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, fp32 *_pDest) const;
	void Anim_GetIPValuead(int _iKey, int _iSeq, const CMTime &_Time, int _nDim, TArray<uint8> *_pDest) const;

	CStr Anim_GetIPValue(int _iKey, int _iSeq, fp32 _Time) const;
	int32 Anim_GetIPValuei(int _iKey, int _iSeq, fp32 _Time) const;
	fp32 Anim_GetIPValuef(int _iKey, int _iSeq, fp32 _Time) const;
	TArray<uint8> Anim_GetIPValued(int _iKey, int _iSeq, fp32 _Time) const;

	void Anim_GetIPValuea(int _iKey, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueai(int _iKey, int _iSeq, fp32 _Time, int _nDim, int32 *_pDest) const;
	void Anim_GetIPValueaf(int _iKey, int _iSeq, fp32 _Time, int _nDim, fp32 *_pDest) const;
	void Anim_GetIPValuead(int _iKey, int _iSeq, fp32 _Time, int _nDim, TArray<uint8> *_pDest) const;

	///////////////////////
	// Key Name
	///////////////////////

	// Get Value
	CStr Anim_GetKFValue(const char* _pName, int _iSeq, int _iKF) const;
	int32 Anim_GetKFValuei(const char* _pName, int _iSeq, int _iKF) const;
	fp32 Anim_GetKFValuef(const char* _pName, int _iSeq, int _iKF) const;
	TArray<uint8> Anim_GetKFValued(const char* _pName, int _iSeq, int _iKF) const;

	void Anim_GetKFValuea(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;
	void Anim_GetKFValueai(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;
	void Anim_GetKFValueaf(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;
	void Anim_GetKFValuead(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest) const;

	// Get Value
	CStr Anim_GetKFValue(const char* _pName, int _iSeq, int _iKF, CStr _Default) const;
	int32 Anim_GetKFValuei(const char* _pName, int _iSeq, int _iKF, int32 _Default) const;
	fp32 Anim_GetKFValuef(const char* _pName, int _iSeq, int _iKF, fp32 _Default) const;
	TArray<uint8> Anim_GetKFValued(const char* _pName, int _iSeq, int _iKF, const TArray<uint8> &_Default) const;

	void Anim_GetKFValuea(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetKFValueai(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest, const int32 *_Default) const;
	void Anim_GetKFValueaf(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest, const fp32 *_Default) const;
	void Anim_GetKFValuead(const char* _pName, int _iSeq, int _iKF, int _nDim, CStr *_pDest, const TArray<uint8> *_Default) const;

	// Get Interpolated Value
	CStr Anim_GetIPValue(const char* _pName, int _iSeq, const CMTime &_Time, CStr _Default) const;
	int32 Anim_GetIPValuei(const char* _pName, int _iSeq, const CMTime &_Time, int32 _Default) const;
	fp32 Anim_GetIPValuef(const char* _pName, int _iSeq, const CMTime &_Time, fp32 _Default) const;
	TArray<uint8> Anim_GetIPValued(const char* _pName, int _iSeq, const CMTime &_Time, const TArray<uint8> &_Default) const;

	void Anim_GetIPValuea(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetIPValueai(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const int32 *_Default) const;
	void Anim_GetIPValueaf(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const fp32 *_Default) const;
	void Anim_GetIPValuead(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_GetIPValue(const char* _pName, int _iSeq, fp32 _Time, CStr _Default) const;
	int32 Anim_GetIPValuei(const char* _pName, int _iSeq, fp32 _Time, int32 _Default) const;
	fp32 Anim_GetIPValuef(const char* _pName, int _iSeq, fp32 _Time, fp32 _Default) const;
	TArray<uint8> Anim_GetIPValued(const char* _pName, int _iSeq, fp32 _Time, const TArray<uint8> &_Default) const;

	void Anim_GetIPValuea(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void Anim_GetIPValueai(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const int32 *_Default) const;
	void Anim_GetIPValueaf(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const fp32 *_Default) const;
	void Anim_GetIPValuead(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest, const TArray<uint8> *_Default) const;

	CStr Anim_GetIPValue(const char* _pName, int _iSeq, const CMTime &_Time) const;
	int32 Anim_GetIPValuei(const char* _pName, int _iSeq, const CMTime &_Time) const;
	fp32 Anim_GetIPValuef(const char* _pName, int _iSeq, const CMTime &_Time) const;
	TArray<uint8> Anim_GetIPValued(const char* _pName, int _iSeq, const CMTime &_Time) const;

	void Anim_GetIPValuea(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueai(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueaf(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValuead(const char* _pName, int _iSeq, const CMTime &_Time, int _nDim, CStr *_pDest) const;

	CStr Anim_GetIPValue(const char* _pName, int _iSeq, fp32 _Time) const;
	int32 Anim_GetIPValuei(const char* _pName, int _iSeq, fp32 _Time) const;
	fp32 Anim_GetIPValuef(const char* _pName, int _iSeq, fp32 _Time) const;
	TArray<uint8> Anim_GetIPValued(const char* _pName, int _iSeq, fp32 _Time) const;

	void Anim_GetIPValuea(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueai(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValueaf(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;
	void Anim_GetIPValuead(const char* _pName, int _iSeq, fp32 _Time, int _nDim, CStr *_pDest) const;

	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Getting value from a child
	CStr GetName(int _iKey) const;
	const char* GetNameUnsafe(int _iKey) const;

	CStr GetValue(int _iKey) const;
	int32 GetValuei(int _iKey) const;
	fp32 GetValuef(int _iKey) const;
	const TArray<uint8> GetValued(int _iKey) const;
	TArray<uint8> GetValued(int _iKey);

	void GetValuea(int _iKey, int _nDim, CStr *_pDest) const;
	void GetValueai(int _iKey, int _nDim, int32 *_pDest) const;
	void GetValueaf(int _iKey, int _nDim, fp32 *_pDest) const;
	void GetValuead(int _iKey, int _nDim, TArray<uint8> *_pDest) const;

	// Named Key

	CStr GetName(const char* _pName) const;
	const char* GetNameUnsafe(const char* _pName) const;

	CStr GetValue(const char* _pName) const;
	int32 GetValuei(const char* _pName) const;
	fp32 GetValuef(const char* _pName) const;
	const TArray<uint8> GetValued(const char* _pName) const;
	TArray<uint8> GetValued(const char* _pName);

	void GetValuea(const char* _pName, int _nDim, CStr *_pDest) const;
	void GetValueai(const char* _pName, int _nDim, int32 *_pDest) const;
	void GetValueaf(const char* _pName, int _nDim, fp32 *_pDest) const;
	void GetValuead(const char* _pName, int _nDim, TArray<uint8> *_pDest) const;

	CStr GetValue(const char* _pName, CStr _Default) const;
	int32 GetValuei(const char* _pName, int32 _Default) const;
	fp32 GetValuef(const char* _pName, fp32 _Default) const;
	const TArray<uint8> GetValued(const char* _pName, const TArray<uint8> &_Default) const;
	TArray<uint8> GetValued(const char* _pName, TArray<uint8> &_Default);

	void GetValuea(const char* _pName, int _nDim, CStr *_pDest, const CStr *_Default) const;
	void GetValueai(const char* _pName, int _nDim, int32 *_pDest, const int32 *_Default) const;
	void GetValueaf(const char* _pName, int _nDim, fp32 *_pDest, const fp32 *_Default) const;
	void GetValuead(const char* _pName, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default);

	CStr GetValue(const char* _pName, CStr _Default, bint _bAddValue);
	int32 GetValuei(const char* _pName, int32 _Default, bint _bAddValue);
	fp32 GetValuef(const char* _pName, fp32 _Default, bint _bAddValue);
	TArray<uint8> GetValued(const char* _pName, TArray<uint8> &_Default, bint _bAddValue);

	void GetValue(const char* _pName, int _nDim, CStr *_pDest, const CStr *_Default, bint _bAddValue);
	void GetValuei(const char* _pName, int _nDim, int32 *_pDest, const int32 *_Default, bint _bAddValue);
	void GetValuef(const char* _pName, int _nDim, fp32 *_pDest, const fp32 *_Default, bint _bAddValue);
	void GetValued(const char* _pName, int _nDim, TArray<uint8> *_pDest, const TArray<uint8> *_Default, bint _bAddValue);

	// Destroying keys
	void DeleteKey(const char* _pName);

	// -------------------------------------------------------------------
	// Text based IO
//	static CStr XRG_GetWord(char* _pData, int _Size, int& _Pos);
	void XRG_LogDump(int _Level) const;

	void XRG_WriteChildren(CCFile* _pFile) const;					// Writes in unicode format.
	void XRG_Write_r(CCFile* _pFile, int _ReqLevel) const;					// Writes in unicode format.
	void XRG_Write(const CStr& _Filename) const;							// Writes in unicode format.

	void WriteSimple(CCFile* _pFile) const;									// Writes in unicode format

	void WriteSimple(const CStr& _Filename) const;							// Writes in unicode format

	void WriteRegistryDir(CStr _Dir, CCFile* _pFile) const;
	
	void WriteRegistryDir(CStr _Dir, CStr _Filename) const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| 
|__________________________________________________________________________________________________
\*************************************************************************************************/

template<class T>
static int FindSeq(const T* p, int _Len, const T* pSeq, int _SeqLen)
{
	int i;
	for( i = 0; i < _Len-_SeqLen; i++)
	{
		int j;
		for(j = 0; j < _SeqLen; j++)
			if (p[i+j] != pSeq[j]) break;
		if (j == _SeqLen) return i;
	}
	return -1;
}

template<class T>
static int FindChar(const T* p, int _Len, char ch)
{
	for(int i = 0; i < _Len; i++)
		if (p[i] == ch) return i;
	return -1;
}

template<class T>
static int GetLineNumber(const T* pData, const T* pPos, int _Len)
{
	if (pPos - pData > _Len)
		Error_static("::GetLineNumber", "Position is outside scope.");

	int iLine = 0;
	int Pos = 0;
	while(Pos < _Len)
	{
		int End = FindChar(pData + Pos, _Len - Pos, 10);
		if (End < 0) return iLine;
		Pos += End+1;
		iLine++;
	}

	return iLine;
}

template<class T>
static void Parse_WhiteSpace(const T* p, int& _Pos, int _Len)
{
	while((_Pos < _Len) && CStr::IsWhiteSpace(p[_Pos])) _Pos++;
}

template<class T>
static void ParseComment_SlashAstrix(const char* p, int& _Pos, int _Len, T * pTemp)
{
	int End = FindSeq(p + _Pos, _Len - _Pos, "*/", 2);
	if (End < 0)
	{
		Error_static("ParseQuote", "Unexpected end-of-file in /*  */ comment.");
	}
	else
		_Pos += End+2;
}


template<class T>
void ParseComment_SlashAstrix(const wchar* p, int& _Pos, int _Len, T * pTemp)
{
	int End = FindSeq(p + _Pos, _Len - _Pos, WTEXT("*/"), 2);
	if (End < 0)
	{
		Error_static("ParseQuote", "Unexpected end-of-file in /*  */ comment.");
	}
	else
		_Pos += End+2;
}


template<class T>
static void ParseComment_SlashSlash(const T* p, int& _Pos, int _Len)
{
	int End = FindChar(p + _Pos, _Len - _Pos, 10);	// EOL can be 13, 10 or just 10.
	if (End < 0)
		_Pos = _Len;
	else
		_Pos += End+1;
}

template<class T>
CStr ParseEscSeqIntact(const T* p, int& _Pos, int _Len, bint _bUseSlash = true)
{
	CStr Ret;
	int nRet = 0;
	T *Return;
	if (sizeof(T) == 1)
		Return = (T *)Ret.GetBuffer(_Len+1);
	else
		Return = (T *)Ret.GetBufferW(_Len+1);

	Return[nRet++] = '"';

	while(_Pos < _Len)
	{
		if (_bUseSlash && p[_Pos] == '\\')
		{
			Return[nRet++] = p[_Pos];
			_Pos++;
			if (_Pos >= _Len) Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

			switch(p[_Pos])
			{
			case '\\' :
			case '"' :
			case 'n' :
			case 'r' :
				Return[nRet++] = p[_Pos];
				break;

			default :
				Error_static("::ParseEscSeq", CStrF("Invalid escape-code. (%i: %s)", _Pos, &p[_Pos]));
			}
			_Pos++;
		}
		else if (p[_Pos] == '"')
		{
			Return[nRet++] = p[_Pos];
			Return[nRet++] = 0;
			Ret.Trim();
			_Pos++;
			return Ret;
		}
		else 
			Return[nRet++] = p[_Pos++];
	}

	Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (2).");
	return "";
}

template<class T>
CStr ParseEscSeq(const T* p, int& _Pos, int _Len, bint _bUseSlash = true)
{
	CStr Ret;
	int nRet = 0;
	T *Return;
	if (sizeof(T) == 1)
		Return = (T *)Ret.GetBuffer(_Len+1);
	else
		Return = (T *)Ret.GetBufferW(_Len+1);

	while(_Pos < _Len)
	{
		if (_bUseSlash && p[_Pos] == '\\')
		{
			_Pos++;
			if (_Pos >= _Len) Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

			switch(p[_Pos])
			{
			case '\\' :
			case '"' :
				Return[nRet++] = p[_Pos];
				break;
			case 'n' :
				Return[nRet++] = '\n';
				break;
			case 'r' :
				Return[nRet++] = '\r';
				break;

			default :
				Error_static("::ParseEscSeq", CStrF("Invalid escape-code. (%i: %s)", _Pos, &p[_Pos]));
			}
			_Pos++;
		}
		else if (p[_Pos] == '"')
		{
			_Pos++;
			Return[nRet++] = 0;
			Ret.Trim();
			return Ret;
		}
		else 
			Return[nRet++] = p[_Pos++];

	}

	Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (2).");
	return "";
}


template<typename t_CChar>
static CStr ParseEsqSeqCompatible(const t_CChar* &_pParse, int _Len, bint _bUseSlash = true, ch8 *_pStopChars = NULL)
{
	const t_CChar * &pParse = _pParse;
	CStr Ret;
	t_CChar *pDst;
	if (sizeof(t_CChar) == 1)
		pDst = (t_CChar *)Ret.GetBuffer(_Len);
	else
		pDst = (t_CChar *)Ret.GetBufferW(_Len);

	int Mode = 0;
	while(*pParse)
	{
		if (Mode == 1)
		{
			if (_bUseSlash && *pParse == '\\')
			{
				++pParse;
				if (!(*pParse)) 
					Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

				switch(*pParse)
				{
				case '\\' :
				case '"' :
					*pDst = *pParse;
					++pDst;
					break;
				case 'n' :
					*pDst = '\n';
					++pDst;
					break;
				case 'r' :
					*pDst = '\r';
					++pDst;
					break;
				default :
					Error_static("::ParseEscSeq", CStr("Invalid escape-code"));
				}
				++pParse;
			}
			else if (*pParse == '"')
			{
				pParse++;
				Mode = 2;
			}
			else 
			{
				*pDst = *pParse;
				++pDst;
				++pParse;
			}
		}
		else
		{
			Mode = 0;
			if (_pStopChars)
			{
				ch8 *pStop = _pStopChars;
				while (*pStop)
				{
					if (*pStop == *pParse)
					{
						++pParse;
						*pDst = 0;
						Ret.Trim();
						return Ret;
					}
					++pStop;
				}
			}

			if (*pParse == '"')
			{
				++pParse;
				Mode = 1;
			}
			else
			{
				*pDst = *pParse;
				++pDst;
				++pParse;
			}
		}
	}

	if (Mode == 1)
		Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (2).");

	*pDst = 0;
	if (Mode == 0)
		Ret.Trim();
	return Ret;
}



template<typename t_CChar>
static CStr ParseEsqSeqIntactCompatible(const t_CChar* &_pParse, int _Len, bint _bUseSlash = true, ch8 *_pStopChars = NULL)
{
	const t_CChar * &pParse = _pParse;
	CStr Ret;
	t_CChar *pDst;
	if (sizeof(t_CChar) == 1)
		pDst = (t_CChar *)Ret.GetBuffer(_Len);
	else
		pDst = (t_CChar *)Ret.GetBufferW(_Len);

	int Mode = 0;
	while(*pParse)
	{
		if (Mode == 1)
		{
			if (_bUseSlash && *pParse == '\\')
			{
				*(pDst++) = *pParse;
				++pParse;
				if (!(*pParse)) 
					Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (1).");

				switch(*pParse)
				{
				case '\\' :
				case '"' :
				case 'n' :
				case 'r' :
					*(pDst++) = *pParse;
					break;
				default :
					Error_static("::ParseEscSeq", CStr("Invalid escape-code"));
				}
				++pParse;
			}
			else if (*pParse == '"')
			{
				*(pDst++) = *pParse;
				pParse++;
				Mode = 0;
			}
			else 
			{
				*pDst = *pParse;
				++pDst;
				++pParse;
			}
		}
		else
		{
			if (_pStopChars)
			{
				ch8 *pStop = _pStopChars;
				while (*pStop)
				{
					if (*pStop == *pParse)
					{
						++pParse;
						*pDst = 0;
						Ret.Trim();
						return Ret;
					}
					++pStop;
				}
			}
			if (*pParse == '"')
			{
				*(pDst++) = *pParse;
				++pParse;
				Mode = 1;
			}
			else
			{
				*pDst = *pParse;
				++pDst;
				++pParse;
			}
		}
	}

	if (Mode == 1)
		Error_static("::ParseEscSeq", "Unexpected end-of-file in escape-sequence (2).");

	Ret.Trim();
	return Ret;
}

CStr ParseEscSeq_Char(const char* p, int& _Pos, int _Len, bint _bUseSlash = true);

template<class T>
CStr CreateEscSeq(const T* p, int _Len)
{
	T Return[8192];
	int nRet = 0;

	int Pos = 0;
	while(Pos < _Len)
	{
		if (p[Pos] == '"' || p[Pos] == '\\')
		{
			Return[nRet++] = '\\';
			Return[nRet++] = p[Pos];
		}
		else if (p[Pos] == '\n')
		{
			Return[nRet++] = '\\';
			Return[nRet++] = 'n';
		}
		else if (p[Pos] == '\r')
		{
			Return[nRet++] = '\\';
			Return[nRet++] = 'r';
		}
		else
			Return[nRet++] = p[Pos];

		Pos++;
	}

	CStr Ret;
	Ret.Capture(Return, nRet);
	return Ret;
}

template<class T>
bint XRG_IsReservedChar(T _ch)
{
	return
		_ch == '"' ||
		_ch == '/' ||
		_ch == '*' ||
		_ch == '{' ||
		_ch == '}';
}


template<class T>
bint XRG_IsReservedCharEscapeNeed(T _ch)
{
	return
		_ch == '<' ||
		_ch == '>' ||
		_ch == ',' ||
		_ch == ';' ||
		_ch == '/' ||
		_ch == '*' ||
		_ch == '{' ||
		_ch == '}';
}

template<class T>
bint XRG_IsEscapeChar(T _ch)
{
	return
		_ch == '\r' ||
		_ch == '\n' ||
		_ch == '\t' ||
		_ch == '"' ||
		_ch == '\\';
}

template<class T>
bint XRG_NeedEscSeq(const T* p, int _Len)
{
	for(int i = 0; i < _Len; i++)
		if (XRG_IsEscapeChar(p[i]) ||
			XRG_IsReservedCharEscapeNeed(p[i]) ||
			CStr::IsWhiteSpace(p[i])) return true;
	return false;
}


template<class T>
CStr XRG_ParseWord(const T* p, int& _Pos, int _Len)
{
	int WordStart = _Pos;
	while(_Pos < _Len)
	{
		if (CStr::IsWhiteSpace(p[_Pos])) break;
		if (::XRG_IsReservedChar(p[_Pos])) break;
		_Pos++;
	}

	CStr Ret;
	Ret.Capture(p + WordStart, _Pos - WordStart);
	return Ret;
}

template<class T>
CStr XRG_ParseLine(const T* p, int& _Pos, int _Len)
{
	Parse_WhiteSpace(p, _Pos, _Len);
	if (_Pos >= _Len)
		return CStr();

	int LineStart = _Pos;
	while(_Pos < _Len)
	{
		if (CStr::IsWhiteSpace(p[_Pos])) break;
		if (::XRG_IsReservedChar(p[_Pos])) break;
		if (p[_Pos] == 13) break;
		if (p[_Pos] == 10) break;
		_Pos++;
	}

	CStr Ret;
	Ret.Capture(p + LineStart, _Pos - LineStart);
	return Ret;
}


#include "MRegistry_Dynamic.h"

#define REGISTRY_CREATE DNew(CRegistry_Dynamic) CRegistry_Dynamic

#endif // _INC_MREGISTRY
