
#include "PCH.h"
#include "MRegistry_Compiled.h"
#include "../Classes/Miscellaneous/MMd5.h"
//#include "MRegistry_Shared.h"

#ifdef M_ENABLE_REGISTRYCOMPILED
MRTC_IMPLEMENT( CRegistryCompiled, CReferenceCount);




//#define M_ENABLE_REGISTRYCOMPILEDVTABLEHACK

class CRegistryCompiledInternal
{
public:

	class CCompiledData;

	class CRegistry_Const : public CRegistry
	{
	public:
		CRegistry_Const()
		{
		}
		~CRegistry_Const()
		{
		}
		int MRTC_AddRef()               
		{ 
			return 100; 
		}

		int MRTC_DelRef()               
		{ 
			return 100; 
		}
		int MRTC_ReferenceCount() const 
		{ 
			return 100; 
		}

#ifdef M_ENABLE_REGISTRYCOMPILEDVTABLEHACK
#else
		CCompiledData *m_pCompiledData;
		const CCompiledData *Private_GetData() const
		{
			return m_pCompiledData;
		}
		CCompiledData *Private_GetData()
		{
			return m_pCompiledData;
		}
#endif
		uint32 m_Data0;


		/*
			uint32 m_bHasChildren : 1; 0
			uint32 m_iNameStr : 15; 1 
			uint32 m_iValue : 16; 16

			uint32 m_bChildrenHasChildren : 1; 0
			uint32 m_nChildren : 15; 1
			uint32 m_iNodeChildren : 16; 16 
		*/

		uint32 Private_Get_HasChildren() const
		{
			return (m_Data0 & DBitRangeTyped(0,0,uint32)) >> 0;
		}
		void Private_Set_HasChildren(uint32 _Value)
		{
			uint32 Mask = DBitRangeTyped(0,0,uint32);
			M_ASSERT(!(_Value & (~(Mask>>0))), "Value too large");
			m_Data0 = (m_Data0 & (~Mask)) | (((_Value) & Mask>>0) << 0);
		}

		uint32 Private_Get_NameDataIndex() const
		{
			return (m_Data0 & DBitRangeTyped(1,15,uint32)) << 1; // 4 Byte aligned
		}
		void Private_Set_NameDataIndex(uint32 _Value)
		{
			uint32 Mask = DBitRangeTyped(1,15,uint32);
			M_ASSERT(!(_Value & (~(Mask >> 1))), "Value too large");
			m_Data0 = (m_Data0 & (~Mask)) | (((_Value) & (Mask>>1)) << 1);
		}

		uint32 Private_Get_ValueIndex() const
		{
			return (m_Data0 & DBitRangeTyped(16,31,uint32)) >> 15; // 2 Byte aligned
		}
		void Private_Set_ValueIndex(uint32 _Value)
		{
			uint32 Mask = DBitRangeTyped(16,31,uint32);
			M_ASSERT(!(_Value & (~(Mask>>16))), "Value too large");
			m_Data0 = (m_Data0 & (~Mask)) | (((_Value) & (Mask>>16)) << 16);
		}


		uint32 Private_Get_ChildrenHasChildren() const
		{
			if (Private_Get_HasChildren())
				return ((*((uint32 *)(this + 1))) & DBitRangeTyped(0,0,uint32)) >> 0;
			else
				return 0;
		}
		void Private_Set_ChildrenHasChildren(uint32 _Value)
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			uint32 Mask = DBitRangeTyped(0,0,uint32);
			M_ASSERT(!(_Value & (~(Mask>>0))), "Value too large");
			(*((uint32 *)(this + 1))) = ((*((uint32 *)(this + 1))) & (~Mask)) | (((_Value & (Mask>>0)) << 0));
		}

		uint32 Private_Get_NumChildren() const
		{
			if (Private_Get_HasChildren())
				return ((*((uint32 *)(this + 1))) & DBitRangeTyped(1,15,uint32)) >> 1;
			else
				return 0;
		}
		void Private_Set_NumChildren(uint32 _Value)
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			uint32 Mask = DBitRangeTyped(1,15,uint32);
			M_ASSERT(!(_Value & (~(Mask>>1))), "Value too large");
			(*((uint32 *)(this + 1))) = ((*((uint32 *)(this + 1))) & (~Mask)) | (((_Value & (Mask>>1)) << 1));
		}

		uint32 Private_Get_ChildNodeStart() const
		{
			if (Private_Get_HasChildren())
				return ((*((uint32 *)(this + 1))) & (uint32)DBitRangeTyped(16,31, uint32)) >> 16;
			else
				return 0;
		}
		void Private_Set_ChildNodeStart(uint32 _Value)
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			uint32 Mask = DBitRangeTyped(16,31, uint32);
			M_ASSERT(!(_Value & (~(Mask>>16))), "Value too large");
			(*((uint32 *)(this + 1))) = ((*((uint32 *)(this + 1))) & (~Mask)) | (((_Value & (Mask>>16)) << 16));
		}

		const CRegistry_Const *Private_GetChild(mint _Stride, const uint32 *_pNodes, int _iChild) const
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			M_ASSERT(_iChild >= 0 && _iChild < Private_Get_NumChildren(), "Access error");
			return (const CRegistry_Const *)(_pNodes + _Stride * _iChild);
		}

		void Private_GetChildOptInit(mint &_Stride, const uint32 *&_pNodes) const
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			int nChildren = Private_Get_NumChildren();
			bint bChildrenHasChildren = Private_Get_ChildrenHasChildren();
			_Stride = (sizeof(CRegistry_Const) + (bChildrenHasChildren ? sizeof(uint32) : 0)) / sizeof(uint32);

			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			_pNodes = pData->GetNodePtr(Private_Get_ChildNodeStart());
			_pNodes += (nChildren + 1) >> 1;
		}

		
		CRegistry_Const *Private_GetChild(int _iChild)
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			int nChildren = Private_Get_NumChildren();
			M_ASSERT(_iChild >= 0 && _iChild < nChildren, "Access error");
			bint bChildrenHasChildren = Private_Get_ChildrenHasChildren();
			mint Stride = (sizeof(CRegistry_Const) + (bChildrenHasChildren ? sizeof(uint32) : 0)) / sizeof(uint32);

			CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();

			const uint32 *pNodes = pData->GetNodePtr(Private_Get_ChildNodeStart());
			pNodes += (nChildren + 1) >> 1;

#ifndef M_RTM
			if ((pNodes + Stride * _iChild) > pData->m_lRegistryNodes.GetBasePtr() + pData->m_lRegistryNodes.Len())
				M_BREAKPOINT;
#endif

			return (CRegistry_Const *)(pNodes + Stride * _iChild);

		}

		const CRegistry_Const *Private_GetChild(int _iChild) const
		{
			M_ASSERT(Private_Get_HasChildren(), "Cannot write here");
			int nChildren = Private_Get_NumChildren();
			M_ASSERT(_iChild >= 0 && _iChild < nChildren, "Access error");
			bint bChildrenHasChildren = Private_Get_ChildrenHasChildren();
			mint Stride = (sizeof(CRegistry_Const) + (bChildrenHasChildren ? sizeof(uint32) : 0)) / sizeof(uint32);

			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();

			const uint32 *pNodes = pData->GetNodePtr(Private_Get_ChildNodeStart());
			pNodes += (nChildren + 1) >> 1;

#ifndef M_RTM
			if ((pNodes + Stride * _iChild) > pData->m_lRegistryNodes.GetBasePtr() + pData->m_lRegistryNodes.Len())
				M_BREAKPOINT;
#endif

			return (const CRegistry_Const *)(pNodes + Stride * _iChild);

		}

		// Don't do anything
		virtual CRegistry *GetParent() const {return NULL;};
		virtual void Hash_Invalidate() {};

		// Implement this

		virtual bint ValidChild(int _iChild) const 
		{
			int nChildren = Private_Get_NumChildren();
			return _iChild >= 0 && _iChild < nChildren;
		}

		virtual int GetNumChildren() const
		{
			int nChildren = Private_Get_NumChildren();
			return nChildren;
		}
		virtual CRegistry* GetChild(int _iChild)
		{
			int nChildren = Private_Get_NumChildren();
			if (_iChild < 0 || _iChild >= nChildren)
				Error_static(M_FUNCTION, "Child out of range");
			return Private_GetChild(_iChild);
		}
		virtual const CRegistry* GetChild(int _iChild) const
		{
			int nChildren = Private_Get_NumChildren();
			if (_iChild < 0 || _iChild >= nChildren)
				Error_static(M_FUNCTION, "Child out of range");
			return Private_GetChild(_iChild);
		}
		virtual const CRegistry* GetChildUnsafe(int _iChild) const
		{
			return Private_GetChild(_iChild);
		}
		virtual int GetType() const
		{
			uint32 ValIndex = Private_Get_ValueIndex();

			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			//bint bAnimated = Header & 1;
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			//uint32 nDim = (Header & DBitRange(5,9)) >> 5;

			return Type;
		}
		virtual int GetDimensions() const	
		{
			uint32 ValIndex = Private_Get_ValueIndex();

			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			//bint bAnimated = Header & 1;
			//uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			return nDim;
		}

		// Search children
			
		int FindIndexInternal(const char* _pKey) const
		{
			int nChildren = Private_Get_NumChildren();
			if (nChildren < 1)
				return -1;

			bint bChildrenHasChildren = Private_Get_ChildrenHasChildren();
			mint ChildStride = (sizeof(CRegistry_Const) + (bChildrenHasChildren ? sizeof(uint32) : 0)) / sizeof(uint32);

			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint32 *pChildNodes = pData->GetNodePtr(Private_Get_ChildNodeStart());
			const uint16 * pChildrenHash = (const uint16 *)pChildNodes;
			pChildNodes += (nChildren + 1) >> 1;

			if (nChildren == 1)
			{
				const CRegistry_Const *pChild = Private_GetChild(ChildStride, pChildNodes, 0);
				const CRegistryCompiledInternal::CCompiledData *pChildData = pChild->Private_GetData();
				uint32 Index = pChild->Private_Get_NameDataIndex();
				const ch8 * pName = pChildData->GetStringPtr(Index);
				if (CStrBase::stricmp(pName, _pKey) == 0)
					return 0;
				else
					return -1;
			}



			// Do a binary search for the hash value
			CFStr Upper;
			Upper.Capture(_pKey);
			Upper = Upper.UpperCase();
			uint16 Hash = StringToHash(_pKey) & 0xffff;
			int iIndexSearch = 0;

			int Low = 0;
			int High = nChildren;
			while(Low < High)
			{
				iIndexSearch = (Low + High) >> 1;

				uint32 CurrHash = pChildrenHash[iIndexSearch];

				if (CurrHash < Hash)
					Low = iIndexSearch + 1;
				else if (CurrHash > Hash)
					High = iIndexSearch;
				else
					break;
			}

			if (iIndexSearch < nChildren)
			{
				if (pChildrenHash[iIndexSearch] == Hash)
				{
					while (iIndexSearch >= 0 && pChildrenHash[iIndexSearch] == Hash)
						--iIndexSearch;

					++iIndexSearch;
					while (iIndexSearch < nChildren && pChildrenHash[iIndexSearch] == Hash)
					{
						const CRegistry_Const *pChild = Private_GetChild(ChildStride, pChildNodes, iIndexSearch);
						int iNameIndex = pChild->Private_Get_NameDataIndex();
						const CRegistryCompiledInternal::CCompiledData *pChildData = pChild->Private_GetData();
						const ch8 * pChildName = pChildData->GetStringPtr(iNameIndex);

						if (CStrBase::stricmp(pChildName, Upper.Str()) == 0)
						{
							return iIndexSearch;
						}
						++iIndexSearch;
					}
					return -1;
				}
				else
					return -1;
			}
			return -1;
		}

		virtual int FindIndex(const char* _pKey) const
		{
			return FindIndexInternal(_pKey);
		}
		virtual int FindIndex(const char* _pKey, const char* _pValue, bint _bCaseSensitiveValue = true) const
		{
			int iIndex = FindIndexInternal(_pKey);
			if (iIndex < 0)
				return -1;

			CFStr Upper;
			Upper.Capture(_pKey);
			Upper = Upper.UpperCase();
			
			int nChildren = Private_Get_NumChildren();

			bint bChildrenHasChildren = Private_Get_ChildrenHasChildren();
			mint ChildStride = (sizeof(CRegistry_Const) + (bChildrenHasChildren ? sizeof(uint32) : 0)) / sizeof(uint32);
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint32 *pChildNodes = pData->GetNodePtr(Private_Get_ChildNodeStart());
			pChildNodes += (nChildren + 1) >> 1;

			while (iIndex < nChildren)
			{
				const CRegistry_Const *pChild = Private_GetChild(ChildStride, pChildNodes, iIndex);
				int iNameIndex = pChild->Private_Get_NameDataIndex();
				const CRegistryCompiledInternal::CCompiledData *pChildData = pChild->Private_GetData();
				const ch8 * pChildName = pChildData->GetStringPtr(iNameIndex);

				if (CStrBase::stricmp(pChildName, Upper.Str()) != 0)
					return -1;

				if (_bCaseSensitiveValue)
				{ 
					if (pChild->CRegistry_Const::GetThisValue() == _pValue) 
						return iIndex;
				}
				else
				{ 
					if (pChild->CRegistry_Const::GetThisValue().CompareNoCase(_pValue) == 0) 
						return iIndex;
				}

				++iIndex;
			}
			return -1;
		}
		// Getting value from this

		virtual CStr GetThisName() const
		{
			uint32 Index = Private_Get_NameDataIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			return pData->GetString(Index);
		}
		virtual uint32 GetThisNameHash() const
		{
			uint32 Index = Private_Get_NameDataIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const ch8 *pStr = pData->GetStringPtr(Index);
			return StringToHash(pStr);
		}
		virtual const char* GetThisNameUnsafe() const
		{
			uint32 Index = Private_Get_NameDataIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			return pData->GetStringPtr(Index);
		}

		virtual CStr GetThisValue() const
		{
			CStr Ret;
			CRegistry_Const::GetThisValuea(1, &Ret);
			return Ret;
		}
		virtual int32 GetThisValuei() const
		{
			int32 Ret;
			CRegistry_Const::GetThisValueai(1, &Ret);
			return Ret;
		}
		virtual fp4 GetThisValuef() const
		{
			fp4 Ret;
			CRegistry_Const::GetThisValueaf(1, &Ret);
			return Ret;
		}
		virtual const TArray<uint8> GetThisValued() const
		{
			TArray<uint8> Ret;
			CRegistry_Const::GetThisValuead(1, &Ret);
			return Ret;
		}
		virtual TArray<uint8> GetThisValued()
		{
			TArray<uint8> Ret;
			CRegistry_Const::GetThisValuead(1, &Ret);
			return Ret;
		}

		virtual void GetThisValuea(int _nDim, CStr *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (bAnimated)
				return CRegistry_Const::Anim_ThisGetKFValuea(0,0,_nDim,_pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const void *pDataPtr2 = pData->GetData((*((uint16 *)(pDataPtr + 2))) << 1);
			CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_STR](this, pDataPtr2, nDim, _pDest, _nDim);
		}

		virtual void GetThisValueai(int _nDim, int32 *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (bAnimated)
				return CRegistry_Const::Anim_ThisGetKFValueai(0,0,_nDim,_pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const void *pDataPtr2 = pData->GetData((*((uint16 *)(pDataPtr + 2))) << 1);
			CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_INT32](this, pDataPtr2, nDim, _pDest, _nDim);
		}
		virtual void GetThisValueaf(int _nDim, fp4 *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (bAnimated)
				return CRegistry_Const::Anim_ThisGetKFValueaf(0,0,_nDim,_pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const void *pDataPtr2 = pData->GetData((*((uint16 *)(pDataPtr + 2))) << 1);
			CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_FP4](this, pDataPtr2, nDim, _pDest, _nDim);
		}
		virtual void GetThisValuead(int _nDim, TArray<uint8> *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (bAnimated)
				return CRegistry_Const::Anim_ThisGetKFValuead(0,0,_nDim,_pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const void *pDataPtr2 = pData->GetData((*((uint16 *)(pDataPtr + 2))) << 1);
			CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_DATA](this, pDataPtr2, nDim, _pDest, _nDim);
		}

		/////////////////////////////////////////////////////////////////
		// Animation
		////////////////////////////////////////////////////////////////


		virtual bint Anim_ThisGetAnimated() const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			return bAnimated;
		}
		virtual bint Anim_ThisGetEnableTimed() const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			if (AnimHeader0 & 1)
				return true;
			return false;
		}

		virtual uint32 Anim_ThisGetFlags() const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return 0;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			return (AnimHeader0 >> 1) & DBitRange(0,6);
		}

		virtual uint32 Anim_ThisGetInterpolate(fp4 *_pParams, int &_nParams) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return 0;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
//			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nData = AnimHeader1 >> 8;
			int ToCopy = Min(nData, _nParams);
			_nParams = nData;
			if (ToCopy)
			{
				const fp4 *pParams = (const fp4 *)pData->GetData((*((uint16 *)(pDataPtr2))) << 2);
				memcpy(_pParams, pParams, ToCopy * sizeof(fp4));
			}
			return AnimHeader1 & DBitRange(0,7);
		}
		/*
			AnimHeader0:1 bTimed;
			AnimHeader0: |= (AnimFlags & DBitRange(0,6)) << 1;
			AnimHeader0 |= (nSeq & DBitRange(0,7)) << 8;
			uint16 AnimHeader1 = 0;
			AnimHeader1 |= (Interpolate & DBitRange(0,7)) << 0;
			AnimHeader1 |= (nParams & DBitRange(0,7)) << 8;
*/
		virtual int Anim_ThisGetNumSeq() const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return 0;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
//			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			return AnimHeader0 >> 8;
		}

		virtual bint Anim_ThisIsValidSequenceKeyframe(int _iSeq, int _iKF) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
			uint16 nFK = *((uint16 *)pSequenceData);

			if (_iKF < 0 || _iKF >= nFK)
				return false;

			return true;		

		}

		virtual int Anim_ThisGetNumKF(int _iSeq = 0) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
			return *((uint16 *)pSequenceData);
		}

		virtual fp4 Anim_ThisGetSeqLoopStart(int _iSeq) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
//			int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16);
			pSequenceData += sizeof(uint16);
			const fp4 *pLoopData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);
			return pLoopData[0];
		}

		virtual fp4 Anim_ThisGetSeqLoopEnd(int _iSeq) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
//			int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16);
			pSequenceData += sizeof(uint16);
			const fp4 *pLoopData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);
			return pLoopData[1];
		}

		virtual fp4 Anim_ThisGetSeqLength(int _iSeq) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
			int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

			if (AnimHeader0 & 1)
			{
				const fp4 *pTimedData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 1);
				return pTimedData[nKF - 1];                
			}
			else
			{
				return nKF;
			}
		}

		virtual fp4 Anim_ThisGetKFTime(int _iSeq, int _iKF) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return false;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return false;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
			int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

			if (_iKF < 0 || _iKF >= nKF)
				return 0;

			if (AnimHeader0 & 1)
			{
				const fp4 *pTimedData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);
				return pTimedData[_iKF];                
			}
			else
			{
				return _iKF;
			}
		}

		// Get Value

		virtual void Anim_ThisGetKFValuea(int _iSeq, int _iKF, int _nDim, CStr *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return CRegistry_Const::GetThisValuea(_nDim, _pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				goto ReturnNull;
			{
				if (AnimHeader1 >> 8)
					pDataPtr2 += sizeof(uint16);
				const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
				int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

				if (_iKF < 0 || _iKF >= nKF)
					goto ReturnNull;

				{
					if (AnimHeader0 & 1)
						pSequenceData += sizeof(uint16);

					const uint8 *pPtr = (const uint8 *)pData->GetData(((uint16 *)pSequenceData)[_iKF] << 1);

					CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_STR](this, pPtr, nDim, _pDest, _nDim);
				}
				return;
			}
ReturnNull:
            for (int i = 0; i < _nDim; ++i)
			{
				_pDest[i] = CStr();
			}
		}

		virtual void Anim_ThisGetKFValueai(int _iSeq, int _iKF, int _nDim, int32 *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return CRegistry_Const::GetThisValueai(_nDim, _pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				goto ReturnNull;
			{
				if (AnimHeader1 >> 8)
					pDataPtr2 += sizeof(uint16);
				const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
				int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

				if (_iKF < 0 || _iKF >= nKF)
					goto ReturnNull;

				{
					if (AnimHeader0 & 1)
						pSequenceData += sizeof(uint16);

					const uint8 *pPtr = (const uint8 *)pData->GetData(((uint16 *)pSequenceData)[_iKF] << 1);

					CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_INT32](this, pPtr, nDim, _pDest, _nDim);
					return;
				}
			}
ReturnNull:
            for (int i = 0; i < _nDim; ++i)
			{
				_pDest[i] = 0;
			}
		}

		virtual void Anim_ThisGetKFValueaf(int _iSeq, int _iKF, int _nDim, fp4 *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return CRegistry_Const::GetThisValueaf(_nDim, _pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				goto ReturnNull;
			{
				if (AnimHeader1 >> 8)
					pDataPtr2 += sizeof(uint16);
				const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
				int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

				if (_iKF < 0 || _iKF >= nKF)
					goto ReturnNull;

				{
					if (AnimHeader0 & 1)
						pSequenceData += sizeof(uint16);

					const uint8 *pPtr = (const uint8 *)pData->GetData(((uint16 *)pSequenceData)[_iKF] << 1);

					CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_FP4](this, pPtr, nDim, _pDest, _nDim);
					return;
				}
			}
ReturnNull:
            for (int i = 0; i < _nDim; ++i)
			{
				_pDest[i] = 0;
			}
		}

		virtual void Anim_ThisGetKFValuead(int _iSeq, int _iKF, int _nDim, TArray<uint8> *_pDest) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return CRegistry_Const::GetThisValuead(_nDim, _pDest);
			uint32 Type = (Header & DBitRange(1,4)) >> 1;
			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				goto ReturnNull;
			{
				if (AnimHeader1 >> 8)
					pDataPtr2 += sizeof(uint16);
				const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
				int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16) * 2; // Jump over loop daat

				if (_iKF < 0 || _iKF >= nKF)
					goto ReturnNull;

				{
					if (AnimHeader0 & 1)
						pSequenceData += sizeof(uint16);

					const uint8 *pPtr = (const uint8 *)pData->GetData(((uint16 *)pSequenceData)[_iKF] << 1);

					CRegistryCompiledInternal::ms_lTypeConvert[Type][REGISTRY_TYPE_DATA](this, pPtr, nDim, _pDest, _nDim);
					return;
				}
			}
ReturnNull:
            for (int i = 0; i < _nDim; ++i)
			{
				_pDest[i] = TArray<uint8>();
			}
		}

		virtual CStr Anim_ThisGetKFValue(int _iSeq, int _iKF) const
		{
			CStr Ret;
			CRegistry_Const::Anim_ThisGetKFValuea(_iSeq, _iKF, 1, &Ret);
			return Ret;
		}
		virtual int32 Anim_ThisGetKFValuei(int _iSeq, int _iKF) const
		{
			int32 Ret;
			CRegistry_Const::Anim_ThisGetKFValueai(_iSeq, _iKF, 1, &Ret);
			return Ret;
		}
		virtual fp4 Anim_ThisGetKFValuef(int _iSeq, int _iKF) const
		{
			fp4 Ret;
			CRegistry_Const::Anim_ThisGetKFValueaf(_iSeq, _iKF, 1, &Ret);
			return Ret;
		}
		virtual TArray<uint8> Anim_ThisGetKFValued(int _iSeq, int _iKF) const
		{
			TArray<uint8> Ret;
			CRegistry_Const::Anim_ThisGetKFValuead(_iSeq, _iKF, 1, &Ret);
			return Ret;
		}

		// Get kf times

		virtual fp4 Anim_ThisGetWrappedTime(const CMTime &_Time, int _iSeq = 0) const
		{
			return 0;
		}


		int Private_Anim_FindKeyByTime(fp4 _Time, const fp4 *_pTimedData, int nKF) const
		{
			if (_pTimedData)
			{
				int iIndexSearch = 0;

				fp4 Hash = _Time;

				int Low = 0;
				int High = nKF;
				while(Low < High)
				{
					iIndexSearch = (Low + High) >> 1;

					fp4 CurrHash = _pTimedData[iIndexSearch];

					if (CurrHash < Hash)
						Low = iIndexSearch + 1;
					else if (CurrHash > Hash)
						High = iIndexSearch;
					else
						break;
				}

				if (iIndexSearch > High)
					iIndexSearch = High;
				if (iIndexSearch < Low)
					iIndexSearch = Low;
				if (iIndexSearch >= nKF)
					iIndexSearch = nKF-1;
				if (iIndexSearch < 0)
					iIndexSearch = 0;

				while (iIndexSearch < (nKF-1) && _pTimedData[iIndexSearch] < Hash)
					++iIndexSearch;
				while (iIndexSearch > 0 && _pTimedData[iIndexSearch] > Hash)
					--iIndexSearch;
				return iIndexSearch;
			}
			else
			{
				return Max(Min((int)_Time,nKF-1), 0);
			}
		}

		static fp4 Private_Anim_GetKFTime(int _iKF, const fp4 *_pTimedData)
		{
			if (_pTimedData)
				return _pTimedData[_iKF];
			else
				return _iKF;
		}

		static fp4 Private_Anim_GetKFDelta(uint32 _Calc0, uint32 _Calc1, fp4 _SecLen, fp4 _LoopEnd, fp4 _LoopStart, const fp4 *_pTimedData)
		{
			fp4 Duration = 0;
			if (_Calc0 & EGetKFFlags_Type)
			{
				uint32 Value0 = _Calc0 & EGetKFFlags_Value;
				uint32 Type0 = _Calc0 >> EGetKFFlags_TypeShift;
				uint32 Value1 = _Calc1 & EGetKFFlags_Value;
				switch(Type0)
				{
				case EGetKFFlags_VSeqEnd:
					Duration = (_SecLen - Private_Anim_GetKFTime(Value0, _pTimedData)) + (Private_Anim_GetKFTime(Value1, _pTimedData) - _LoopStart);
					break;
				case EGetKFFlags_VLoopEnd:
					Duration = (_LoopEnd - Private_Anim_GetKFTime(Value0, _pTimedData)) + (Private_Anim_GetKFTime(Value1, _pTimedData) - _LoopStart);
					break;
				case EGetKFFlags_VAssign:
					Duration = Private_Anim_GetKFTime(Value0, _pTimedData);
					break;
				}				
			}
			else
			{
				Duration = Private_Anim_GetKFTime(_Calc0, _pTimedData) - Private_Anim_GetKFTime(_Calc1, _pTimedData);
			}
			return Duration;
		}

		void Private_Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
		{
			if (_pTimeDeltas)
			{
				uint32 ValIndex = Private_Get_ValueIndex();
				const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
				const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
				uint16 Header = *((uint16 *)pDataPtr);
				bint bAnimated = Header & 1;
				if (!bAnimated)
					return;
//				uint32 Type = (Header & DBitRange(1,4)) >> 1;
//				uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
				const uint8 *pDataPtr2 = pDataPtr + 2;
				uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
				uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
				int nSeq = AnimHeader0 >> 8;
				if (_iSeq < 0 || _iSeq >= nSeq)
					return;
				if (AnimHeader1 >> 8)
					pDataPtr2 += sizeof(uint16);
				const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
				int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16); // Jump over loop daat
				const fp4 *pLoopData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);pSequenceData += sizeof(uint16);

				const fp4 *pTimedData = NULL;
				fp4 SeqLen;
				if (AnimHeader0 & 1)
				{
					pTimedData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);
					pSequenceData += sizeof(uint16);
					SeqLen = pTimedData[nKF-1];
				}
				else
					SeqLen = nKF;

				fp4 LoopStart = (pLoopData[0] < 0 ? 0 : pLoopData[0]);
				fp4 LoopEnd = (pLoopData[1] < 0 ? SeqLen : pLoopData[1]);
				M_ASSERT(LoopEnd > LoopStart, "");
				if (LoopEnd <= LoopStart)
					return;

				uint32 DeltasCalc[256];
				int nTimeDeltas = _nPre+_nPost;
				
				if (nTimeDeltas > 128)
				{
					fp4 *pDeltas = new fp4[nTimeDeltas * 2];
					
					Private_Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, pDeltas, _nPre, _nPost);
					for (int i = 0; i < nTimeDeltas; ++i)
					{
						_pTimeDeltas[i] = Private_Anim_GetKFDelta((uint32)pDeltas[i*2], (uint32)pDeltas[i*2+1], SeqLen, LoopEnd, LoopStart, pTimedData);
					}

					delete [] pDeltas;
				}
				else
				{
					Private_Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, DeltasCalc, _nPre, _nPost);
					for (int i = 0; i < nTimeDeltas; ++i)
					{
						_pTimeDeltas[i] = Private_Anim_GetKFDelta(DeltasCalc[i*2], DeltasCalc[i*2+1], SeqLen, LoopEnd, LoopStart, pTimedData);
					}
				}
			}
			else
			{
				Private_Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, (uint32 *)NULL, _nPre, _nPost);
			}
		}

		void Private_Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const
		{
			uint32 ValIndex = Private_Get_ValueIndex();
			const CRegistryCompiledInternal::CCompiledData *pData = Private_GetData();
			const uint8 * pDataPtr = (const uint8 * )pData->GetData(ValIndex);
			uint16 Header = *((uint16 *)pDataPtr);
			bint bAnimated = Header & 1;
			if (!bAnimated)
				return;
//			uint32 Type = (Header & DBitRange(1,4)) >> 1;
//			uint32 nDim = ((Header & DBitRange(5,9)) >> 5) + 1;
			const uint8 *pDataPtr2 = pDataPtr + 2;
			uint16 AnimHeader0 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			uint16 AnimHeader1 = *((uint16 *)pDataPtr2); pDataPtr2 += 2;
			int nSeq = AnimHeader0 >> 8;
			if (_iSeq < 0 || _iSeq >= nSeq)
				return;
			if (AnimHeader1 >> 8)
				pDataPtr2 += sizeof(uint16);
			const uint8 *pSequenceData = (const uint8 *)pData->GetData(((uint16 *)pDataPtr2)[_iSeq] << 1);
			int nKF = *((uint16 *)pSequenceData);pSequenceData += sizeof(uint16); // Jump over loop daat
			const fp4 *pLoopData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);pSequenceData += sizeof(uint16);

			const fp4 *pTimedData = NULL;
			fp4 SeqLen;
			if (AnimHeader0 & 1)
			{
				pTimedData = (const fp4 *)pData->GetData((*((uint16 *)pSequenceData)) << 2);
				pSequenceData += sizeof(uint16);
				SeqLen = pTimedData[nKF-1];
			}
			else
				SeqLen = nKF;

			uint32 AnimFlags = (AnimHeader0 >> 1) & DBitRange(0,6);

			fp4 Time = 0;
			int nLoops = 0;
			fp4 LoopStart;
			fp4 LoopEnd = 0;
			fp4 LoopLength;
			fp4 LoopTime = -1;
			if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP)
			{
				LoopStart = (pLoopData[0] < 0 ? 0 : pLoopData[0]);
				LoopEnd = (pLoopData[1] < 0 ? SeqLen : pLoopData[1]);
				M_ASSERT(LoopEnd > LoopStart, "");
				if (LoopEnd <= LoopStart)
					return;
				LoopLength = LoopEnd - LoopStart;
				if (_Time.Compare(SeqLen) > 0)
				{

					CMTime WrapTime;
					if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
					{
						WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen + (SeqLen - LoopEnd)));
					}
					else
						WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen));

					LoopTime = WrapTime.GetTimeModulus(LoopLength);
					nLoops = WrapTime.GetNumModulus(LoopLength) + 1;

					if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
					{
						if (WrapTime.Compare(0) < 0)
						{
							Time = SeqLen - WrapTime.GetTime();
						}
						else
						{
							if (nLoops & 1)
								Time = LoopEnd - LoopTime;
							else
								Time = LoopStart + LoopTime;
						}
					}
					else
						Time = LoopStart + LoopTime;
				}
				else
					Time = _Time.GetTime();
			}
			else if (AnimFlags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
			{
				LoopStart = (pLoopData[0] < 0 ? 0 : pLoopData[0]);
				LoopEnd = (pLoopData[1] < 0 ? SeqLen : pLoopData[1]);
				M_ASSERT(LoopEnd > LoopStart, "");
				if (LoopEnd <= LoopStart)
					return;
				LoopLength = LoopEnd - LoopStart;
					if (_Time.Compare(SeqLen) > 0)
					{
						CMTime WrapTime;
						WrapTime = _Time - (CMTime::CreateFromSeconds(SeqLen));
						LoopTime = WrapTime.GetTimeModulus(LoopLength);
						nLoops = WrapTime.GetNumModulus(LoopLength) + 1;
						Time = LoopStart + LoopTime;
					}
					else
						Time = _Time.GetTime();
			}
			else
			{
				if (_Time.Compare(SeqLen) > 0)
					Time = SeqLen;
				else
					Time = _Time.GetTime();
			}

			fp4 LowTime = Private_Anim_GetKFTime(0, pTimedData);
			if (Time < LowTime)
				Time = LowTime;

			int iStart = Private_Anim_FindKeyByTime(Time, pTimedData, nKF);
			_pKeys[_nPre] = iStart;

			if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP)
			{
				int iLoopStart = Private_Anim_FindKeyByTime(LoopStart, pTimedData, nKF);
				int iLoopEnd = Private_Anim_FindKeyByTime(LoopEnd, pTimedData, nKF);
				while (iLoopStart < (nKF-1) && Private_Anim_GetKFTime(iLoopStart, pTimedData) < LoopStart)
					++iLoopStart;

				_Fraction = 0;

				int iLoopStartIndex = iLoopStart;

				int iCur = iStart;
				int iLoops = nLoops;
				// Walk backwards
				int iPre = _nPre;
				if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
				{
					if (AnimFlags & REGISTRY_ANIMFLAGS_CONTIGUOUS && !iLoops)
					{
						iLoops = 2;
					}
					if (iLoops && iLoops & 1)
					{
						++iStart;
						if (iStart > iLoopEnd)
							iStart = iLoopEnd;
						_pKeys[_nPre] = iCur = iStart;
					}
					else if (!iLoops && LoopTime >= 0)
					{
						++iStart;
						if (iStart >= nKF)
							iStart = nKF-1;
						_pKeys[_nPre] = iCur = iStart;
					}
		//			LoopStartTime = Private_Anim_GetKFTime(pSeq, iLoopStart) - LoopStart;
					// Walk backwards through loops
					while (iLoops && iPre)
					{
						--iPre;
						if (iLoops & 1)
						{
							if (iCur == iLoopEnd)
							{
								--iLoops;
								if (iLoops)
								{
									_pDeltasCalc[iPre*2] = iCur;
									iCur = iCur - 1;
									if (iCur < iLoopStart)
										iCur = iLoopStart;
									_pDeltasCalc[iPre*2+1] = iCur;
								}
								else
								{
									++iPre;
									break;
								}

							}
							else
							{
								_pDeltasCalc[iPre*2+1] = iCur;
								iCur = iCur + 1;
								if (iCur > iLoopEnd)
									iCur = iLoopEnd;
								_pDeltasCalc[iPre*2] = iCur;
							}
						}
						else
						{
							if (iCur == iLoopStart)
							{
								--iLoops;
								_pDeltasCalc[iPre*2+1] = iCur;
								iCur = iCur + 1;
								if (iCur > iLoopEnd)
									iCur = iLoopEnd;
								_pDeltasCalc[iPre*2] = iCur;
							}
							else
							{
								_pDeltasCalc[iPre*2] = iCur;
								iCur = iCur - 1;
								if (iCur < iLoopStart)
									iCur = iLoopStart;
								_pDeltasCalc[iPre*2+1] = iCur;
							}
						}
						_pKeys[iPre] = iCur;

					}
					if (iPre)
					{
						// Walk towards end
						if (LoopTime >= 0)
						{
							while (iPre && iCur < (nKF-1))
							{
								--iPre;
								_pDeltasCalc[iPre*2+1] = iCur;
								iCur = iCur + 1;
								_pDeltasCalc[iPre*2] = iCur;
								_pKeys[iPre] = iCur;
							}
						}

						// Walk towards beginning
						while (iPre)
						{
							--iPre;
							_pDeltasCalc[iPre*2] = iCur;
							iCur = Max(iCur - 1, 0);
							_pDeltasCalc[iPre*2+1] = iCur;
							_pKeys[iPre] = iCur;
						}
					}
				}
				else
				{
					if (AlmostEqual(Private_Anim_GetKFTime(iLoopStartIndex, pTimedData), LoopStart, 0.0001f))
					{
						++iLoopStartIndex;
						if (iLoopStartIndex > iLoopEnd)
							iLoopStartIndex = iLoopEnd;
					}
					if (!iLoops && AnimFlags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
						iLoops = 1;
					if (iLoops && iStart < iLoopStartIndex)
					{	
						if (iLoops == 1)
							iCur = iStart = nKF-1;
						else
							iCur = iStart = iLoopEnd;
						_pKeys[_nPre] = iStart;
					}

					// Walk backwards through loops
					while (iLoops && iPre)
					{
						--iPre;
						if (iCur == iLoopStartIndex)
						{
							if (iLoops == 1)
							{
								iCur = nKF-1;
								_pDeltasCalc[iPre*2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
								_pDeltasCalc[iPre*2+1] = iLoopStartIndex;
							}
							else
							{
								iCur = iLoopEnd;
								_pDeltasCalc[iPre*2] = iLoopEnd | (EGetKFFlags_VLoopEnd << EGetKFFlags_TypeShift);
								_pDeltasCalc[iPre*2+1] = iLoopStartIndex;
							}
							--iLoops;
						}
						else
						{
					//		M_ASSERT(iCur > iLoopStartIndex, "");
							_pDeltasCalc[iPre*2] = iCur;
							iCur = iCur - 1;
							_pDeltasCalc[iPre*2+1] = iCur;
						}
						_pKeys[iPre] = iCur;
					}
					if (iPre)
					{
						// Walk towards beginning
						{
							while (iPre)
							{
								--iPre;
								_pDeltasCalc[iPre*2] = iCur;
								iCur = Max(iCur - 1, 0);
								_pDeltasCalc[iPre*2+1] = iCur;
								_pKeys[iPre] = iCur;
							}
						}
					}
				}

				// Calc fraction
				iCur = iStart;
				iLoops = nLoops;
				// Walk forward
				int nPost = _nPost;
				int iPost = _nPre+1;
				if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
				{
					if (!iLoops)
					{
						// Walk to end of sequence
						while (nPost)
						{
							if (iCur == (nKF - 1))
							{
								++iLoops;
								_pDeltasCalc[iPost*2-2] = iCur;
								iCur = iCur - 1;
								if (iCur < iLoopStart)
									iCur = iLoopStart;
								_pDeltasCalc[iPost*2-1] = iCur;
								_pKeys[iPost++] = iCur;
								--nPost;
								break;
							}
							_pDeltasCalc[iPost*2-1] = iCur;
							iCur = iCur + 1;
							_pDeltasCalc[iPost*2-2] = iCur;
							_pKeys[iPost++] = iCur;
							--nPost;
						}

						// Walk backwards to loop end
						while (nPost && iCur > iLoopEnd)
						{
							_pDeltasCalc[iPost*2-2] = iCur;
							iCur = iCur - 1;
							_pDeltasCalc[iPost*2-1] = iCur;
							_pKeys[iPost++] = iCur;
							--nPost;
						}
					}

					// Walk through loop
					while (nPost)
					{
						if (iLoops & 1)
						{
							if (iCur <= iLoopStart)
							{
								++iLoops;
								_pDeltasCalc[iPost*2-1] = iCur;
								iCur = iCur + 1;
								if (iCur >= nKF)
									iCur = nKF-1;
								_pDeltasCalc[iPost*2-2] = iCur;
							}
							else
							{
								_pDeltasCalc[iPost*2-2] = iCur;
								iCur = iCur - 1;
								_pDeltasCalc[iPost*2-1] = iCur;
							}
							_pKeys[iPost++] = iCur;
							--nPost;
						}
						else
						{
							if (iCur >= iLoopEnd)
							{
								++iLoops;
								_pDeltasCalc[iPost*2-2] = iCur;
								iCur = iCur - 1;
								if (iCur < 0)
									iCur = 0;
								_pDeltasCalc[iPost*2-1] = iCur;
							}
							else
							{
								_pDeltasCalc[iPost*2-1] = iCur;
								iCur = iCur + 1;
								_pDeltasCalc[iPost*2-2] = iCur;
							}
							_pKeys[iPost++] = iCur;
							--nPost;
						}
					}
				}
				else
				{
					if (!iLoops)
					{
						// Walk to end of sequence
						while (nPost)
						{
							if (iCur == (nKF - 1))
							{
								++iLoops;

								_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
								_pDeltasCalc[iPost*2-1] = iLoopStartIndex;

								iCur = iLoopStartIndex;
								_pKeys[iPost++] = iCur;
								--nPost;
								break;
							}
							_pDeltasCalc[iPost*2-1] = iCur;
							iCur = iCur + 1;
							_pDeltasCalc[iPost*2-2] = iCur;
							_pKeys[iPost++] = iCur;
							--nPost;
						}
					}

					// Walk through loop
					while (nPost)
					{
						if (iCur >= iLoopEnd)
						{
							if (iCur == iLoopEnd)
							{
								_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VLoopEnd << EGetKFFlags_TypeShift);
								_pDeltasCalc[iPost*2-1] = iLoopStartIndex;

							}
							else
							{
								_pDeltasCalc[iPost*2-2] = iCur | (EGetKFFlags_VSeqEnd << EGetKFFlags_TypeShift);
								_pDeltasCalc[iPost*2-1] = iLoopStartIndex;
							}
							iCur = iLoopStartIndex;
							++iLoops;
						}
						else
						{
							int iLast = iCur;
							iCur = iCur + 1;
							_pDeltasCalc[iPost*2-2] = iCur;
							_pDeltasCalc[iPost*2-1] = iLast;
						}
						_pKeys[iPost++] = iCur;
						--nPost;
					}
				}

				fp4 LowTime = Private_Anim_GetKFTime(iStart, pTimedData);
				if (_nPost)
				{
					fp4 Duration = Private_Anim_GetKFDelta(_pDeltasCalc[_nPre*2], _pDeltasCalc[_nPre*2+1], SeqLen, LoopEnd, LoopStart, pTimedData);

					if (AnimFlags & REGISTRY_ANIMFLAGS_LOOP_PINGPONG)
					{
						if (Duration > 0)
						{
							fp4 Time0 = Private_Anim_GetKFTime(_pKeys[_nPre], pTimedData);
							fp4 Time1 = Private_Anim_GetKFTime(_pKeys[_nPre+1], pTimedData);
							if (Time1 > Time0)
							{
								_Fraction = (Time - Time0) / Duration;
							}
							else
							{
								_Fraction = (Time0 - Time) / Duration;
							}
						}
						else
							_Fraction = 0;
					}
					else
					{
						if (Duration > 0)
						{
							if (LowTime > Time)
							{
								if (iStart == iLoopEnd)
									_Fraction = ((Time - LoopStart) - (LowTime - LoopEnd)) / Duration;
								else
									_Fraction = ((Time - LoopStart) - (LowTime - SeqLen)) / Duration;
							}
							else
								_Fraction = (Time - LowTime) / Duration;
						}
						else
							_Fraction = 0;
					}
				}
				else
				{
					fp4 HighTime = Private_Anim_GetKFTime(Min(iStart + 1, nKF - 1), pTimedData);
					Time = Min(Max(LowTime, Time), HighTime);
					if (HighTime != LowTime)
						_Fraction = (Time - LowTime) / (HighTime - LowTime);
				}
			}
			else if (AnimFlags & REGISTRY_ANIMFLAGS_CONTIGUOUS)
			{
				_Fraction = 0;
				int iCur = iStart;
				if (iCur == 0)
					_pKeys[_nPre] = iStart = iCur = nKF - 1;
				// Walk backwards
				int iPre = _nPre;
				while (iPre)
				{
					--iPre;
					iCur = iCur - 1;
					if (iCur <= 0)
					{
						_pDeltasCalc[iPre*2] = (iCur+1) | (EGetKFFlags_VAssign << EGetKFFlags_TypeShift);
						_pDeltasCalc[iPre*2+1] = 0;
						iCur = nKF - 1;
					}
					else
					{
						_pDeltasCalc[iPre*2] = iCur+1;
						_pDeltasCalc[iPre*2+1] = iCur;
					}
					_pKeys[iPre] = iCur;
				}
				iCur = iStart;

				// Walk forward
				for (int i = 0; i < _nPost; ++i)
				{
					iCur = iCur + 1;
					if (iCur >= nKF)
					{
						iCur = 1;
						if (iCur >= nKF)
							iCur = 0;

						_pDeltasCalc[(_nPre+i)*2] = (iCur) | (EGetKFFlags_VAssign << EGetKFFlags_TypeShift);
						_pDeltasCalc[(_nPre+i)*2+1] = 0;
					}
					else
					{
						_pDeltasCalc[(_nPre+i)*2] = iCur;
						_pDeltasCalc[(_nPre+i)*2+1] = iCur-1;
					}
					_pKeys[_nPre+i+1] = iCur;
				}

				fp4 LowTime = Private_Anim_GetKFTime(iStart, pTimedData);
				if (_nPost)
				{
					fp4 Duration = Private_Anim_GetKFDelta(_pDeltasCalc[_nPre*2], _pDeltasCalc[_nPre*2+1], SeqLen, LoopEnd, LoopStart, pTimedData);
					if (Duration > 0)
					{
						if (LowTime > Time)
						{
							_Fraction = (Time - (LowTime - SeqLen)) / Duration;
						}
						else
							_Fraction = (Time - LowTime) / Duration;
					}
					else
						_Fraction = 0;
				}
				else
				{
					fp4 HighTime = Private_Anim_GetKFTime(Min(iStart + 1, nKF - 1), pTimedData);
					Time = Min(Max(LowTime, Time), HighTime);
					if (HighTime != LowTime)
						_Fraction = (Time - LowTime) / (HighTime - LowTime);
				}
			}
			else
			{
				_Fraction = 0;
				int iCur = iStart;
				// Walk backwards
				int iPre = _nPre;
				while (iPre)
				{
					--iPre;
					iCur = Max(iCur - 1, 0);
					_pKeys[iPre] = iCur;
				}
				iCur = iStart;

				fp4 LowTime = Private_Anim_GetKFTime(iStart, pTimedData);
				fp4 HighTime = Private_Anim_GetKFTime(Min(iStart + 1, nKF - 1), pTimedData);
				Time = Min(Max(LowTime, Time), HighTime);
				if (HighTime != LowTime)
					_Fraction = (Time - LowTime) / (HighTime - LowTime);

				// Walk forward
				for (int i = 0; i < _nPost; ++i)
				{
					iCur = Min(iCur + 1, nKF - 1);
					_pKeys[_nPre+i+1] = iCur;
				}
				
				if (_pDeltasCalc)
				{
					int nDeltas = _nPre+_nPost;
					for (int i = 0; i < nDeltas; ++i)
					{
						_pDeltasCalc[i*2] = _pKeys[i+1];
						_pDeltasCalc[i*2+1] = _pKeys[i];
					}
				}
			}
		}

		void Anim_ThisGetKF(int _iSeq, fp4 _Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
		{
			return Private_Anim_ThisGetKF(_iSeq, CMTime::CreateFromSeconds(_Time), _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost);
		}

		void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, fp4 *_pTimeDeltas, int _nPre, int _nPost) const
		{
			return Private_Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, _pTimeDeltas, _nPre, _nPost);
		}

		void Anim_ThisGetKF(int _iSeq, const CMTime &_Time, fp4 &_Fraction, int *_pKeys, uint32 *_pDeltasCalc, int _nPre, int _nPost) const
		{
			return Private_Anim_ThisGetKF(_iSeq, _Time, _Fraction, _pKeys, _pDeltasCalc, _nPre, _nPost);
		}


		//////////////////////////////     ///////////////////////////////////
		/////////////                                    /////////////////////
		//////////////////////////    NOIMP   ////////////////////////////////
		/////////////                                    /////////////////////
		//////////////////////////////     ///////////////////////////////////

		virtual uint32 GetThisUserFlags() const 
		{
			return 0;
		}
		virtual bint Anim_ThisGetDisableAutoDemote() const
		{
			return 0;
		}

#ifndef M_RTM
#define NOIMPVOID {M_ASSERT(0, "Not implemented"); Error_static(__FUNCTION__, "Not Implemented");}
#define NOIMPZERO {M_ASSERT(0, "Not implemented"); Error_static(__FUNCTION__, "Not Implemented"); return 0;}
#else
#define NOIMPVOID {}
#define NOIMPZERO {return 0;}
#endif

		virtual void operator= (const CRegistry& _Reg) NOIMPVOID;
		virtual void Clear() NOIMPVOID;
		virtual spCRegistry MakeThreadSafe() NOIMPZERO;
		virtual void SetChild(int _iChild, CRegistry*_pReg) NOIMPVOID;
		virtual void SetParent(CRegistry *) {}
		virtual void SetNumChildren(int _nChildren) NOIMPVOID;
		virtual void ConvertToType(int _Type, int _nDim) NOIMPVOID;
		// Sort register
		virtual void Sort(bint _bReversedOrder = false, bint _bSortByName = true) NOIMPVOID;
		virtual CRegistry* CreateDir(const char* _pPath) NOIMPZERO;
		virtual bint DeleteDir(const char* _pPath) NOIMPZERO;
		virtual void CopyDir(const CRegistry* _pReg, bint _bRecursive = true) NOIMPVOID;
		virtual void CopyValue(const CRegistry* _pReg) NOIMPVOID;
		virtual void AddReg(spCRegistry _spReg) NOIMPVOID;
		virtual CRegistry *CreateNewChild() NOIMPZERO;
		virtual void DeleteKey(int _iKey) NOIMPVOID;
		// Setting
		virtual void SetThisName(const char* _pName) NOIMPVOID;
		virtual void SetThisValue(const char* _pValue) NOIMPVOID;
		virtual void SetThisValueConvert(CStr _Value, int _nDim, int _Type) NOIMPVOID;
		virtual void SetThisValue(const wchar* _pValue) NOIMPVOID;
		virtual void SetThisValue(CStr _Value) NOIMPVOID;
		virtual void SetThisValuei(int32 _Value, int _StoreType = REGISTRY_TYPE_INT32) NOIMPVOID;
		virtual void SetThisValuef(fp4 _Value, int _StoreType = REGISTRY_TYPE_FP4) NOIMPVOID;
		virtual void SetThisValued(const uint8* _pValue, int _Size, bint _bQuick = true) NOIMPVOID;
		virtual void SetThisValued(TArray<uint8> _lValue, bint _bReference = true) NOIMPVOID;
		virtual void SetThisValuea(int _nDim, const CStr *_Value) NOIMPVOID;
		virtual void SetThisValuead(int _nDim, const TArray<uint8> *_lValue, bint _bReference = true) NOIMPVOID;
		virtual void SetThisValueai(int _nDim, const int32 *_Value, int _StoreType = REGISTRY_TYPE_INT32) NOIMPVOID;
		virtual void SetThisValueaf(int _nDim, const fp4 *_Value, int _StoreType = REGISTRY_TYPE_FP4) NOIMPVOID;
		// Anim
		virtual void Anim_ThisSetNumKF(int _iSeq, int _nKF) NOIMPVOID;
		virtual void Anim_ThisDeleteKF(int _iSeq, int _iKF) NOIMPVOID;
		virtual int Anim_ThisSetKFTime(int _iSeq, int _iKF, fp4 _Time)  NOIMPZERO;
		virtual void Anim_ThisSetAnimated(bint _bEnable) NOIMPVOID;
		virtual void Anim_ThisSetDisableAutoDemote(bint _bEnable) NOIMPVOID;
		virtual void Anim_ThisSetEnableTimed(bint _bEnable) NOIMPVOID;
		virtual void Anim_ThisSetFlags(uint32 _Flags) NOIMPVOID;
		virtual void Anim_ThisSetInterpolate(uint32 _InterpolateType, const fp4 *_pParams, int _nParams) NOIMPVOID;
		virtual int Anim_ThisAddSeq() NOIMPZERO;
		virtual int Anim_ThisInsertSeq(int _iSeqAfter) NOIMPZERO;
		virtual void Anim_ThisDeleteSeq(int _iSeq) NOIMPVOID;
		virtual void Anim_ThisSetNumSeq(int _nSeq) NOIMPVOID;
		virtual void Anim_ThisSetSeqLoopStart(int _iSeq, fp4 _Time) NOIMPVOID;
		virtual void Anim_ThisSetSeqLoopEnd(int _iSeq, fp4 _Time) NOIMPVOID;
		// Adds
		virtual int Anim_ThisAddKF(int _iSeq, CStr _Val, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFi(int _iSeq, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFf(int _iSeq, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFd(int _iSeq, const uint8* _pValue, int _Size, bint _bQuick = true, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFd(int _iSeq, TArray<uint8> _lValue, bint _bReference = true, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFa(int _iSeq, int _nDim, const CStr *_pVal, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFai(int _iSeq, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFaf(int _iSeq, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4, fp4 _Time = -1) NOIMPZERO;
		virtual int Anim_ThisAddKFad(int _iSeq, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true, fp4 _Time = -1) NOIMPZERO;
		// Set
		virtual void Anim_ThisSetKFValueConvert(int _iSeq, int _iKF, CStr _Val, int _nDim, int _StoreType, fp4 _Time) NOIMPVOID;
		virtual void Anim_ThisSetKFValue(int _iSeq, int _iKF, CStr _Val) NOIMPVOID;
		virtual void Anim_ThisSetKFValuei(int _iSeq, int _iKF, int32 _Val, int _StoreType = REGISTRY_TYPE_INT32) NOIMPVOID;
		virtual void Anim_ThisSetKFValuef(int _iSeq, int _iKF, fp4 _Val, int _StoreType = REGISTRY_TYPE_FP4) NOIMPVOID;
		virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, const uint8* _pValue, int _Size, bint _bQuick = true) NOIMPVOID;
		virtual void Anim_ThisSetKFValued(int _iSeq, int _iKF, TArray<uint8> _lValue, bint _bReference = true) NOIMPVOID;
		virtual void Anim_ThisSetKFValuea(int _iSeq, int _iKF, int _nDim, const CStr *_pVal) NOIMPVOID;
		virtual void Anim_ThisSetKFValueai(int _iSeq, int _iKF, int _nDim, const int32 *_pVal, int _StoreType = REGISTRY_TYPE_INT32) NOIMPVOID;
		virtual void Anim_ThisSetKFValueaf(int _iSeq, int _iKF, int _nDim, const fp4 *_pVal, int _StoreType = REGISTRY_TYPE_FP4) NOIMPVOID;
		virtual void Anim_ThisSetKFValuead(int _iSeq, int _iKF, int _nDim, const TArray<uint8> *_lValue, bint _bReference = true) NOIMPVOID;
		////////////////// IO
		// Get/Set user flags, user flags are 8-bit
		virtual void SetThisUserFlags(uint32 _Value) NOIMPVOID;
		virtual int XRG_Parse(char* _pOrgData, char* _pData, int _Size, CRegistry_ParseContext& _ParseContext) NOIMPZERO;
		virtual int XRG_Parse(wchar* _pOrgData, wchar* _pData, int _Size, CRegistry_ParseContext& _ParseContext) NOIMPZERO;
		virtual void XRG_Read(const CStr& _Filename, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) NOIMPVOID;	// Unicode detection is automatic.
		virtual void XRG_Read(CCFile *_pFile, CStr _ThisFileName, TArray<CStr> _lDefines, bint _bSlashIsEscape = true) NOIMPVOID;	// Unicode detection is automatic.
		virtual void XRG_Read(const CStr& _Filename) NOIMPVOID;								// XRG_Read(_FileName, TArray<CStr>::TArray<CStr>(), true)
		virtual void ReadSimple(CCFile* _pFile) NOIMPVOID;									// Unicode detection is automatic.
		virtual void ReadSimple(CStr _FileName) NOIMPVOID;									// Unicode detection is automatic.
		virtual void ReadRegistryDir(CStr _Dir, CCFile* _pFile) NOIMPVOID;
		virtual void ReadRegistryDir(CStr _Dir, CStr _Filename) NOIMPVOID;
		// -------------------------------------------------------------------
		// Binary IO
		virtual void Read(CCFile* _pFile, int _Version) NOIMPVOID;
		virtual void Write(CCFile* _pFile) const NOIMPVOID;
		// Read/Write from data-file entry.
		virtual bint Read(CDataFile* _pDFile, const char* _pEntryName) NOIMPZERO;
		virtual void Write(CDataFile* _pDFile, const char* _pEntryName) const NOIMPVOID;
		// Read/Write datafile.
		virtual void Read(const char* _pFileName) NOIMPVOID;
		virtual void Write(const char* _pFileName) const NOIMPVOID;



	};

	class CMemoryStreamData
	{
	public:
		uint8 m_Data[1 << 17];
	};

	class CMemoryStream
	{
	public:
		CMemoryStreamData *m_pData;
		TDA_Pool<CMemoryStreamData> *m_pPool;
		CMemoryStream(TDA_Pool<CMemoryStreamData> &_Pool)
		{
			m_pData = _Pool.New();
			m_pPool = &_Pool;
			m_Stream.Open(m_pData->m_Data, 0, 1<<17, CFILE_BINARY | CFILE_WRITE);
			m_File.Open(&m_Stream, CFILE_BINARY | CFILE_WRITE);
		}
		~CMemoryStream()
		{
			if (m_pData)
				m_pPool->Delete(m_pData);
		}

		CMemoryStream(void *_pStream)
		{
			m_pPool = NULL;
			m_pData = NULL;
			m_Stream.Open((uint8 *)_pStream, 1 << 17, 1 << 17, CFILE_BINARY | CFILE_READ);
			m_File.Open(&m_Stream, CFILE_BINARY | CFILE_READ);
		}
		CStream_Memory m_Stream;
		CCFile m_File;

		mint GetLen()
		{
			return m_File.Pos();
		}
		void *GetPtr()
		{
			return m_pData->m_Data;
		}

		template <int t_bLE, typename t_CValue>
		void Write(const t_CValue &_Value)
		{
			if (t_bLE)
				m_File.WriteLE(_Value);
			else
				m_File.WriteBE(_Value);
		}

		template <int t_bLE, typename t_CValue>
		void Read(t_CValue &_Value)
		{
			if (t_bLE)
				m_File.ReadLE(_Value);
			else
				m_File.ReadBE(_Value);
		}

	};


	class CCompiledData
	{
	public:
		TThinArray<uint32> m_lRegistryNodes;
		TArray<uint8> m_lHeapData;
		int m_iCompiledData;
#ifdef M_ENABLE_REGISTRYCOMPILEDVTABLEHACK
		TThinArray<void *> m_VirtualTable;
#endif

		CCompiledData &operator = (const CCompiledData &_Source)
		{
			m_lRegistryNodes = _Source.m_lRegistryNodes;
			m_lHeapData = _Source.m_lHeapData;
			m_iCompiledData = _Source.m_iCompiledData;
#ifdef M_ENABLE_REGISTRYCOMPILEDVTABLEHACK
			m_VirtualTable = _Source.m_lHeapData;
#endif
			return *this;
		}

		uint32 *GetNodePtr(int _Place)
		{
			return &(m_lRegistryNodes[_Place]);
		}

		const uint32 *GetNodePtr(int _iPlace) const
		{
			return &(m_lRegistryNodes[_iPlace]);
		}

		void *GetData(int _iData)
		{
			return &(m_lHeapData[_iData]);
		}

		const void *GetData(int _iData) const
		{
			return &(m_lHeapData[_iData]);
		}

		CStr GetString(int _iData) const
		{
			return CStr((CStr::CStrData *)(m_lHeapData.GetBasePtr() + _iData));
		}

		const ch8* GetStringPtr(int _iData) const
		{
			return (const ch8 *)((CStr::CStrData *)(&m_lHeapData[_iData]) + 1);
		}
	};


	typedef void (FRegistryCompiledTypeConvert)(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst);

	class CRegistry_RegistryCompiled_Helpers
	{
	public:

		static void RegistryConvertStrToStr(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc == 1 && _nDimDst > 1)
			{
				uint16 *pSource = (uint16 *)_pSource;
				CStr *pDest = (CStr *)_pDest;

				CStr Source = _pConstReg->Private_GetData()->GetString((*pSource) << 1);
				if (Source.IsAnsi())
				{
					const ch8 *pParse = Source.Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						pDest[i] = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
					}
				}
				else
				{
					const wchar *pParse = Source.StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						pDest[i] = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
					}
				}	
			}
			else
			{
				uint16 *pSource = (uint16 *)_pSource;
				CStr *pDest = (CStr *)_pDest;

				int nMin = Min(_nDimSrc, _nDimDst);
				for (int i = 0; i < nMin; ++i)
					pDest[i] = _pConstReg->Private_GetData()->GetString((pSource[i]) << 1);

				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = CStr();
			}
		}

		template <typename t_CType>
		static void RegistryConvertDefault(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			for(int i = 0; i < _nDimDst; ++i)
				((t_CType *)(_pDest))[i] = t_CType();
		}

		template <typename t_CType>
		static void RegistryConvertClear(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			for (int i = 0; i < _nDimDst; ++i)
				((t_CType *)(_pDest))[i] = 0;
		}

		static void RegistryConvertDataToStr(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			uint16 *pSource = (uint16 *)_pSource;
			CStr *pDest = (CStr *)_pDest;

			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				uint32 *pSourcePtr = ((uint32 *)_pConstReg->Private_GetData()->GetData((pSource[i]) << 2));

				pDest[i] = Base64EncodeData(pSourcePtr+1, *pSourcePtr);
			}
		}
		
		static void RegistryConvertStrToData(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			typedef TList_Vector<uint8> t_CType;
			if (_nDimSrc == 1 && _nDimDst > 1)
			{
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				CStr Source = _pConstReg->Private_GetData()->GetString((*pSource) << 1);
				Source = Source.Ansi();

				const ch8 *pParse = Source.Str();
				for (int i = 0; i < _nDimDst; ++i)
				{
					CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
					if (Dst != "")
						pDest[i] = Base64DecodeData(Dst);
					else
						pDest[i].Clear();
				}
			}
			else
			{
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				int nMin = Min(_nDimSrc, _nDimDst);
				for (int i = 0; i < nMin; ++i)
				{
					CStr Source = _pConstReg->Private_GetData()->GetString((pSource[i]) << 1);
					Source = Source.Ansi();

					if (Source != "")
					{
						pDest[i] = Base64DecodeData(Source);
					}
					else
					{
						pDest[i].Clear();
					}
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i].Clear();
			}
		}

		template <typename t_CType>
		static void RegistryConvertStrToInt(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc == 1 && _nDimDst > 1)
			{
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				CStr Source = _pConstReg->Private_GetData()->GetString((*pSource) << 1);

				if (Source.IsAnsi())
				{
					const ch8 *pParse = Source.Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						pDest[i] = NStr::StrToInt(Dst.Str(),(t_CType)0);
					}
				}
				else
				{
					const wchar *pParse = Source.StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						pDest[i] = NStr::StrToInt(Dst.StrW(),(t_CType)0);
					}
				}
			}
			else
			{
				int nMin = Min(_nDimSrc, _nDimDst);
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				for (int i = 0; i < nMin; ++i)
				{
					CStr Source = _pConstReg->Private_GetData()->GetString((pSource[i]) << 1);
					if (Source.IsAnsi())
						pDest[i] = NStr::StrToInt(Source.Str(), (t_CType)0);
					else	
						pDest[i] = NStr::StrToInt(Source.StrW(), (t_CType)0);
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = 0;

			}
		}

		template <typename t_CType, typename t_CConvertType>
		static void RegistryConvertStrToFloat(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc == 1 && _nDimDst > 1)
			{
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				CStr Source = _pConstReg->Private_GetData()->GetString((*pSource) << 1);

				if (Source.IsAnsi())
				{
					const ch8 *pParse = Source.Str();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						pDest[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Dst.Str(),(fp8)0));
					}
				}
				else
				{
					const wchar *pParse = Source.StrW();
					for (int i = 0; i < _nDimDst; ++i)
					{
						CStr Dst = ParseEsqSeqCompatible(pParse, NStr::StrLen(pParse), true, ",");
						pDest[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Dst.StrW(),(fp8)0));
					}
				}
			}
			else
			{
				int nMin = Min(_nDimSrc, _nDimDst);
				uint16 *pSource = (uint16 *)_pSource;
				t_CType *pDest = (t_CType *)_pDest;
				for (int i = 0; i < nMin; ++i)
				{
					CStr Source = _pConstReg->Private_GetData()->GetString((pSource[i]) << 1);
					if (Source.IsAnsi())
						pDest[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Source.Str(), (fp8)0));
					else
						pDest[i] = CMClosestAssign::Assign((t_CType)0, NStr::StrToFloat(Source.StrW(), (fp8)0));
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = 0;
			}
		}


		template <typename t_CType>
		static void RegistryConvertIntToStr(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc > 1 && _nDimDst == 1)
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;

				CStr Dest;
				for (int i = 0; i < _nDimSrc; ++i)
				{
					t_CType &Source = pSource[i];

					if (i == 0)
						Dest = CStrF("%d", (int)(Source));
					else
						Dest += CStrF(",%d", (int)(Source));
				}
				*pDest = Dest;
			}
			else
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;
				int nMin = Min(_nDimSrc, _nDimDst);
				for (int i = 0; i < nMin; ++i)
				{
					pDest[i] = CStrF("%d", (int)(pSource[i]));
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = CStr();
			}
		}

		template <typename t_CType>
		static void RegistryConvertIntToStrHex(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc > 1 && _nDimDst == 1)
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;

				CStr Dest;
				for (int i = 0; i < _nDimSrc; ++i)
				{
					t_CType &Source = pSource[i];

					if (i == 0)
						Dest = CStrF("0x%08x", (int)(Source));
					else
						Dest += CStrF(",0x%08x", (int)(Source));
				}
				*pDest = Dest;
			}
			else
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;
				int nMin = Min(_nDimSrc, _nDimDst);
				for (int i = 0; i < nMin; ++i)
				{
					pDest[i] = CStrF("0x%08x", (int)(pSource[i]));
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = CStr();
			}
		}

		template <typename t_CType>
		static void RegistryConvertFloatToStr(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			if (_nDimSrc > 1 && _nDimDst == 1)
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;

				CStr Dest;
				for (int i = 0; i < _nDimSrc; ++i)
				{
					t_CType &Source = pSource[i];

					if (i == 0)
						Dest = CStrF("%f", (fp4)(Source));
					else
						Dest += CStrF(",%f", (fp4)(Source));
				}
				*pDest = Dest;
			}
			else
			{
				t_CType *pSource = (t_CType *)_pSource;
				CStr *pDest = (CStr *)_pDest;
				int nMin = Min(_nDimSrc, _nDimDst);
				for (int i = 0; i < nMin; ++i)
				{
					pDest[i] = CStrF("%f", (fp4)(pSource[i]));
				}
				for (int i = nMin; i < _nDimDst; ++i)
					pDest[i] = CStr();
			}
		}

		template <typename t_CType0, typename t_CType1>
		static void RegistryConvertCompatible(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				t_CType0 &Source = ((t_CType0 *)_pSource)[i];
				t_CType1 &Dest = ((t_CType1 *)_pDest)[i];
				Dest = (t_CType1)Source;
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType1 *)(_pDest)))[i] = t_CType1();
		}

		template <typename t_CType0>
		static void RegistryConvertSame(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				t_CType0 &Source = ((t_CType0 *)_pSource)[i];
				t_CType0 &Dest = ((t_CType0 *)_pDest)[i];
				Dest = Source;
			}
			for (int i = nMin; i < _nDimDst; ++i)
				(((t_CType0 *)(_pDest)))[i] = t_CType0();
		}

		static void RegistryConvertCopyListVector(const CRegistry_Const *_pConstReg, const void *_pSource, int _nDimSrc, void *_pDest, int _nDimDst)
		{
			uint16 *pSource = (uint16 *)_pSource;

			int nMin = Min(_nDimSrc, _nDimDst);
			for (int i = 0; i < nMin; ++i)
			{
				uint32 *pSourcePtr = ((uint32 *)_pConstReg->Private_GetData()->GetData((pSource[i]) << 2));

				TList_Vector<uint8> &Dest = ((TList_Vector<uint8> *)_pDest)[i];

				mint Len = *pSourcePtr;
				Dest.SetLen(Len);
				memcpy(Dest.GetBasePtr(), pSourcePtr+1, Len);
			}
			for (int i = nMin; i < _nDimDst; ++i)
				((TList_Vector<uint8> *)_pDest)[i].Clear();
		}
	};

	static FRegistryCompiledTypeConvert *ms_lTypeConvert[REGISTRY_TYPE_MAX][REGISTRY_TYPE_MAX];
	TThinArray<CCompiledData> m_CompiledData;

	class CCompileContext
	{
	public:
		CCompileContext():
			m_MemoryStreamDataPool(512 * 1024)
		{
			m_iCurrentCompiledData = 0;
			m_bFastSearch = 0;
			m_bRoot = true;
		}
		~CCompileContext()
		{
			for (int i = 0; i < m_CompiledData.Len(); ++i)
			{
				if (m_CompiledData[i])
					delete m_CompiledData[i];
			}
		}

		TDA_Pool<CMemoryStreamData> m_MemoryStreamDataPool;

		int m_bRoot;
		int m_iCurrentCompiledData;
		int m_bFastSearch;
		class CCompiledDataCompile
		{
		public:
			CCompiledDataCompile()
			{
				m_lStrDataHeaderMask.SetLen(1 << 16);
				m_DataHeap.SetLen(1 << 17);
				m_IndexEntries.SetLen(1 << 16);
				m_NodeHeap.SetLen(1 << 16);
				memset(m_lStrDataHeaderMask.GetBasePtr(), 0, m_lStrDataHeaderMask.ListSize());
				memset(m_DataHeap.GetBasePtr(), 0, m_DataHeap.ListSize());
				memset(m_NodeHeap.GetBasePtr(), 0, m_NodeHeap.ListSize());

				m_UsedDataHeap = 0;
				m_UsedNodeHeap = 0;
				m_UsedDataHeapSaved = 0;
				m_UsedNodeHeapSaved = 0;
				m_ReusedDataHeap = 0;
				m_ReusedNodeHeap = 0;
				m_ReusedDataHeapSaved = 0;
				m_ReusedNodeHeapSaved = 0;
			}

			~CCompiledDataCompile()
			{
				while (m_Index.GetRoot())
				{
					CIndex *pIndex = m_Index.GetRoot();
					m_Index.f_Remove(pIndex);
					m_IndexPool.Delete(pIndex);
				}
			}
			TThinArray<uint8> m_lStrDataHeaderMask;
			TThinArray<uint8> m_DataHeap;
			TThinArray<uint32> m_NodeHeap;
			TThinArray<CMD5Digest> m_NodeChecksums;
			TThinArray<CMD5Digest> m_NodeChecksumsChildren;

			enum 
			{
                EIndexSize = 20
			};

			class CIndex
			{
			public:
				class CEntry
				{
				public:
					CEntry()
					{
						m_pData = NULL;
						m_DataLen = 0;
						m_pIndex = NULL;
					}
					CIndex *m_pIndex;
					void *m_pData;
					mint m_DataLen;
					DLinkD_Link(CEntry, m_Link);
				};

				DLinkD_List(CEntry, m_Link) m_Entries;

				class CCompare
				{
				public:
					static aint Compare(const CIndex *_pFirst, const CIndex *_pSecond, void *_pContext)
					{
						const CEntry *pFirst = _pFirst->m_Entries.GetFirstConst();
						const CEntry *pSecond = _pSecond->m_Entries.GetFirstConst();
						mint TestLen = Min(pFirst->m_DataLen, pSecond->m_DataLen);

						int iCmp = memcmp(pFirst->m_pData, pSecond->m_pData, TestLen);
						if (iCmp)
							return iCmp;

						if (pFirst->m_DataLen < pSecond->m_DataLen)
							return -1;
						else if (pFirst->m_DataLen > pSecond->m_DataLen)
							return 1;

						return 0;						
					}

					static aint Compare(const CIndex *_pTest, CIndex::CEntry const & _Key, void *_pContext)
					{
						const CEntry *pFirst = _pTest->m_Entries.GetFirstConst();
						const CEntry *pSecond = &_Key;
						mint TestLen = Min(pFirst->m_DataLen, pSecond->m_DataLen);

						int iCmp = memcmp(pFirst->m_pData, pSecond->m_pData, TestLen);
						if (iCmp)
							return iCmp;

						if (pFirst->m_DataLen < pSecond->m_DataLen)
							return -1;
						else if (pFirst->m_DataLen > pSecond->m_DataLen)
							return 1;

						return 0;						
					}
				};

				DAVLAligned_Link(CIndex, m_Link, CIndex::CEntry, CCompare);
			};

			TThinArray<CIndex::CEntry> m_IndexEntries;
			DAVLAligned_Tree(CIndex, m_Link, CIndex::CEntry, CIndex::CCompare) m_Index;
			typedef DIdsTreeAVLAligned_Iterator(CIndex, m_Link, CIndex::CEntry, CIndex::CCompare) CIndexIter;
			typedef DLinkD_Iter(CIndex::CEntry, m_Link) CIndexEntryIter;
			TDA_Pool<CIndex> m_IndexPool;

			void IndexAddedData(int _Pos, int _Len)
			{
				int iPos = _Pos >> 1; // Alignment of 2
				int iStartPos = Max(iPos - EIndexSize, 0);
				int iEndPos = (_Pos + _Len + 1) >> 1;
				int iChangePos = _Pos >> 1;

				TAP_RCD<CIndex::CEntry> Iter = m_IndexEntries;
				for (int i = iStartPos; i < iEndPos; ++i)
				{
					CIndex::CEntry *pEntry = &(Iter[i]);
					if (pEntry->m_Link.IsInList())
					{
						if (pEntry->m_Link.IsAloneInList())
						{
							m_Index.f_Remove(pEntry->m_pIndex);
							pEntry->m_Link.Unlink();
							m_IndexPool.Delete(pEntry->m_pIndex);
						}
						pEntry->m_Link.Unlink();
						pEntry->m_pIndex = NULL;
					}
				}

				uint8 *pDataStart = m_DataHeap.GetBasePtr();


				for (int i = iStartPos; i < iEndPos; ++i)
				{
					CIndex::CEntry *pEntry = &(Iter[i]);
					pEntry->m_pData = pDataStart + (i << 1);
					pEntry->m_DataLen = Min((iEndPos - i), int(EIndexSize)) * 2;
					CIndex *pIndex = m_Index.FindEqual(*pEntry);
					if (pIndex)
					{
						pIndex->m_Entries.Insert(pEntry);
						pEntry->m_pIndex = pIndex;
					}
					else
					{
						CIndex *pIndex = m_IndexPool.New();
						pIndex->m_Entries.Insert(pEntry);
						pEntry->m_pIndex = pIndex;
						m_Index.f_Insert(pIndex);
					}
				}

#if 0
				M_TRACEALWAYS("\n\nInserted\n\n");
				{
					CIndexIter Iter = m_Index;

					while (Iter)
					{
						const CIndex::CEntry *pEntry = Iter->m_Entries.GetFirstConst();
						M_TRACEALWAYS("%d %x\n", pEntry->m_DataLen, pEntry->m_pData);

						++Iter;
					}
				}
#endif
			}

			CRegistryCompiledInternal::CRegistry_Const *FindNode(const CMD5Digest &_Digest, int _iData)
			{
				if (!m_NodeChecksumsChildren.GetBasePtr())
					return NULL;

				int nNodes = m_UsedNodeHeapSaved / sizeof(uint32);
				TAP_RCD<CMD5Digest> TapChildren = m_NodeChecksumsChildren;
				for (int i = 0; i < nNodes; ++i)
				{
					if (TapChildren[i] == _Digest)
					{
						CRegistryCompiledInternal::CRegistry_Const *pNode = (CRegistryCompiledInternal::CRegistry_Const *)(&(m_NodeHeap[i]));
						if ((uint32 &)pNode == _iData)
						{
							M_ASSERT(TapChildren[i] != CMD5Digest(), "");
							return pNode;
						}
					}
				}
				return NULL;
			}

			void *GetData(int _Index)
			{
				return m_DataHeap.GetBasePtr() + _Index;
			}

			CStr::CStrData *GetString(int _iData) const
			{
				return (CStr::CStrData *)(m_DataHeap.GetBasePtr() + _iData);
			}

			mint m_UsedDataHeap;
			mint m_UsedNodeHeap;
			mint m_UsedDataHeapSaved;
			mint m_UsedNodeHeapSaved;

			mint m_ReusedDataHeap;
			mint m_ReusedNodeHeap;
			mint m_ReusedDataHeapSaved;
			mint m_ReusedNodeHeapSaved;

			void ResetUsage()
			{
				int iStartPos = m_UsedDataHeapSaved >> 1;
				int iEndPos = (m_UsedDataHeap + 1) >> 1;

				TAP_RCD<CIndex::CEntry> Iter = m_IndexEntries;
				for (int i = iStartPos; i < iEndPos; ++i)
				{
					CIndex::CEntry *pEntry = &(Iter[i]);
					if (pEntry->m_Link.IsInList())
					{
						if (pEntry->m_Link.IsAloneInList())
						{
							m_Index.f_Remove(pEntry->m_pIndex);
							pEntry->m_Link.Unlink();
							m_IndexPool.Delete(pEntry->m_pIndex);
						}
						pEntry->m_Link.Unlink();
						pEntry->m_pIndex = NULL;
						pEntry->m_pData = NULL;
						pEntry->m_DataLen = 0;
					}
				}

				IndexAddedData(m_UsedDataHeapSaved, 0); // Reset lengths
				memset(m_DataHeap.GetBasePtr() + m_UsedDataHeapSaved, 0, m_UsedDataHeap - m_UsedDataHeapSaved);

				m_UsedDataHeap = m_UsedDataHeapSaved;
				m_UsedNodeHeap = m_UsedNodeHeapSaved;
				m_ReusedDataHeap = m_ReusedDataHeapSaved;
				m_ReusedNodeHeap = m_ReusedNodeHeapSaved;
			}
			void CommitUsage()
			{
				m_UsedDataHeapSaved = m_UsedDataHeap;
				m_UsedNodeHeapSaved = m_UsedNodeHeap;
				m_ReusedDataHeapSaved = m_ReusedDataHeap;
				m_ReusedNodeHeapSaved = m_ReusedNodeHeap;
			}
		};

		CRegistryCompiledInternal::CRegistry_Const *FindNode(const CMD5Digest &_Digest, int _iData)
		{
			for (int i = 0; i < m_CompiledData.Len(); ++i)
			{
				CRegistryCompiledInternal::CRegistry_Const *pRet = m_CompiledData[i]->FindNode(_Digest, _iData);
				if (pRet)
					return pRet;

			}
			return NULL;
		}

		CCompiledDataCompile *CompiledDataFromNode(CRegistryCompiledInternal::CRegistry_Const &_Node, int &_iNode)
		{
			uint32 *pToFind = (uint32 *)&_Node;
			for (int i = 0; i < m_CompiledData.Len(); ++i)
			{
				if (pToFind >= m_CompiledData[i]->m_NodeHeap.GetBasePtr() && pToFind < (m_CompiledData[i]->m_NodeHeap.GetBasePtr() + m_CompiledData[i]->m_NodeHeap.Len()))
				{
					_iNode = pToFind - m_CompiledData[i]->m_NodeHeap.GetBasePtr();
					return m_CompiledData[i];
				}
			}
			return NULL;
		}

		TList_Vector<CCompiledDataCompile*> m_CompiledData;

		template <int _bLE>
		void ChecksumData(CRegistryCompiledInternal::CRegistry_Const &_Node, CMD5 &_MD5, int _Type, int _nDim, int _iData)
		{
			uint32 iData = (uint32 &)_Node;
			CCompiledDataCompile *pCompiledData = m_CompiledData[iData];
			int iRet = 0;
			int Type = _Type;
			int nDim = _nDim;

			switch (Type)
			{
			case REGISTRY_TYPE_VOID:
				break;
			case REGISTRY_TYPE_STR:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));

					for (int d = 0; d < nDim; ++d)
					{
						uint16 iStr;
						DataStream.Read<_bLE>(iStr);
						CStr::CStrData *pStr = pCompiledData->GetString(iStr << 1);
						void *pData = pStr->StrData();
						int Len = pStr->Len() * (pStr->IsUnicode() + 1);
						if (pData)
							_MD5.f_AddData(pData, Len);
					}
				}
				break;
			case REGISTRY_TYPE_DATA:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));

					for (int d = 0; d < nDim; ++d)
					{
						uint16 iData;
						DataStream.Read<_bLE>(iData);

						CMemoryStream DataDataStream(pCompiledData->GetData(iData << 2));

						uint32 Length;
						DataDataStream.Read<_bLE>(Length);
						TList_Vector<uint8> Temp;
						Temp.SetLen(Length);
						DataDataStream.m_File.Read(Temp.GetBasePtr(), Length);
						_MD5.f_AddData(Temp.GetBasePtr(), Length);
					}
				}
				break;
			case REGISTRY_TYPE_UINT8:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));
					for (int d = 0; d < nDim; ++d)
					{
						uint8 Data;
						DataStream.Read<_bLE>(Data);
						_MD5.f_AddData(&Data, sizeof(Data));
					}
				}
				break;
			case REGISTRY_TYPE_INT16:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));
					for (int d = 0; d < nDim; ++d)
					{
						int16 Data;
						DataStream.Read<_bLE>(Data);
						_MD5.f_AddData(&Data, sizeof(Data));
					}
				}
				break;
			case REGISTRY_TYPE_INT32:
			case REGISTRY_TYPE_UINT32:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));
					for (int d = 0; d < nDim; ++d)
					{
						uint32 Data;
						DataStream.Read<_bLE>(Data);
						_MD5.f_AddData(&Data, sizeof(Data));
					}
				}
				break;
			case REGISTRY_TYPE_FP4:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));
					for (int d = 0; d < nDim; ++d)
					{
						fp4 Data;
						DataStream.Read<_bLE>(Data);
						_MD5.f_AddData(&Data, sizeof(Data));
					}
				}
				break;
			case REGISTRY_TYPE_FP2:
				{
					CMemoryStream DataStream(pCompiledData->GetData(_iData << 1));
					for (int d = 0; d < nDim; ++d)
					{
						fp2 Data;
						DataStream.Read<_bLE>(Data.m_Half);
						_MD5.f_AddData(&Data.m_Half, sizeof(Data.m_Half));
					}
				}
				break;
			default:
				Error_static(M_FUNCTION, "Unsupported data type");
				break;
			}

		}

		template <int _bLE>
		void CalcNodeDataChecksum(CRegistryCompiledInternal::CRegistry_Const &_Node, CMD5 &_MD5)
		{
			uint32 iData = (uint32 &)_Node;
			CCompiledDataCompile *pCompiledData = m_CompiledData[iData];

			// Name
			int iNameStr = _Node.Private_Get_NameDataIndex();
			CStr::CStrData *pNameStr = pCompiledData->GetString(iNameStr);
			{
				void *pData = pNameStr->StrData();
				int Len = pNameStr->Len() * (pNameStr->IsUnicode() + 1);
				if (pData)
					_MD5.f_AddData(pData, Len);
			}

			int iValue = _Node.Private_Get_ValueIndex();
			CMemoryStream MainStream(pCompiledData->GetData(iValue));
			uint16 Header;
			MainStream.Read<_bLE>(Header);
			_MD5.f_AddData(&Header, sizeof(Header));

			bint bAnimated = Header & 1;
			uint32 Type = (Header & (DBitRange(0, 3) << 1)) >> 1;
			uint32 nDim = ((Header & (DBitRange(0,4) << 5)) >> 5) + 1;
			if (bAnimated)
			{
				uint16 AnimHeader0;
				uint16 AnimHeader1;
				MainStream.Read<_bLE>(AnimHeader0);
				MainStream.Read<_bLE>(AnimHeader1);
				_MD5.f_AddData(&AnimHeader0, sizeof(AnimHeader0));
				_MD5.f_AddData(&AnimHeader1, sizeof(AnimHeader1));

				bint bTimed = AnimHeader0 & 1;
				uint32 AnimFlags = (AnimHeader0 & (DBitRange(0,6) << 1)) >> 1;
				int nParams = (AnimHeader1 & (DBitRange(0,7) << 8)) >> 8;
				uint32 Interpolate = (AnimHeader1 & (DBitRange(0,7) << 0)) >> 0;
				int nSeq = (AnimHeader0 & (DBitRange(0,7) << 8)) >> 8;

				if (nParams)
				{
					uint16 IPIndex;
					MainStream.Read<_bLE>(IPIndex);

					CMemoryStream DataStream(pCompiledData->GetData(IPIndex << 2));
					for (int i = 0; i < nParams; ++i)
					{
						fp4 Data;
						DataStream.Read<_bLE>(Data);
						_MD5.f_AddData(&Data, sizeof(Data));
					}
				}

				// Build Sequences
				if (nSeq)
				{
					for (int i = 0; i < nSeq; ++i)
					{
						uint16 SeqIndex;
						MainStream.Read<_bLE>(SeqIndex);
						CMemoryStream SequenceStream(pCompiledData->GetData(SeqIndex << 1));

						{
							uint16 nKF;
							SequenceStream.Read<_bLE>(nKF);
							_MD5.f_AddData(&nKF, sizeof(nKF));

							uint16 LoopIndex;
							SequenceStream.Read<_bLE>(LoopIndex);

							fp4 *LoopData = (fp4 *)pCompiledData->GetData(LoopIndex << 2);
							_MD5.f_AddData(LoopData, sizeof(fp4) * 2);

							if (bTimed)
							{			
								uint16 TimeIndex;
								SequenceStream.Read<_bLE>(TimeIndex);
								CMemoryStream TimeStream(pCompiledData->GetData(TimeIndex << 2));

								for (int j = 0; j < nKF; ++j)
								{
									fp4 Data;
									TimeStream.Read<_bLE>(Data);
									_MD5.f_AddData(&Data, sizeof(Data));
								}

							}
							for (int j = 0; j < nKF; ++j)
							{
								uint16 iData;
								SequenceStream.Read<_bLE>(iData);
								ChecksumData<_bLE>(_Node, _MD5, Type, nDim, iData);
							}
						}

					}
				}
			}
			else
			{
				uint16 iData;
				MainStream.Read<_bLE>(iData);
				ChecksumData<_bLE>(_Node, _MD5, Type, nDim, iData);
			}

		}

		template <int _bLE>
		CMD5Digest CalcChecksum(CRegistryCompiledInternal::CRegistry_Const &_Node)
		{
			int iThisNode = 0;
			CCompiledDataCompile *pThisCompiledData = CompiledDataFromNode(_Node, iThisNode);
			if (pThisCompiledData)
			{
				pThisCompiledData->m_NodeChecksums.SetLen(1 << 16);
				pThisCompiledData->m_NodeChecksumsChildren.SetLen(1 << 16);

				if (pThisCompiledData->m_NodeChecksums[iThisNode] != CMD5Digest())
					return pThisCompiledData->m_NodeChecksums[iThisNode];			
			}


			uint32 iData = (uint32 &)_Node;
			CMD5 MD5;
			CMD5 MD5Children;

			CCompiledDataCompile *pCompiledData = m_CompiledData[iData];

			if (pThisCompiledData)
				CalcNodeDataChecksum<_bLE>(_Node, MD5);

			uint32 *pNodeData = &(pCompiledData->m_NodeHeap[_Node.Private_Get_ChildNodeStart()]);
			uint32 nChildren = _Node.Private_Get_NumChildren();
			uint32 *pNodes = pNodeData + ((nChildren + 1) >> 1);
			mint NodeSize = (_Node.Private_Get_ChildrenHasChildren() ? sizeof(uint32) : 0) + sizeof(CRegistry_Const);

			for (int i = 0; i < nChildren; ++i)
			{
				CRegistryCompiledInternal::CRegistry_Const &Node = *((CRegistryCompiledInternal::CRegistry_Const *)(((uint8 *)pNodes + NodeSize * i)));

				CMD5Digest Digest = CalcChecksum<_bLE>(Node);

				MD5.f_AddData(Digest.f_GetData(), 16);
				MD5Children.f_AddData(Digest.f_GetData(), 16);
			}

			if (pThisCompiledData)
			{
				pThisCompiledData->m_NodeChecksums[iThisNode] = MD5;
				pThisCompiledData->m_NodeChecksumsChildren[iThisNode] = MD5Children;
				return pThisCompiledData->m_NodeChecksums[iThisNode];
			}
			else
				return MD5;

		}

		void ResetUsage()
		{
            if (m_iCurrentCompiledData < m_CompiledData.Len())
				m_CompiledData[m_iCurrentCompiledData]->ResetUsage();
		}

		void StartSearch()
		{
			ResetUsage();
			if (m_bFastSearch)
			{
			}
//			else
//				m_iCurrentCompiledData = 0;

			if (m_CompiledData.Len() < 1)
			{
				m_CompiledData.Add(DNew(CCompiledDataCompile) CCompiledDataCompile);
			}
		}
		bint NextCompiledData()
		{
			ResetUsage();
			++m_iCurrentCompiledData;

			if (m_iCurrentCompiledData >= m_CompiledData.Len())
			{
				while (m_iCurrentCompiledData >= m_CompiledData.Len())
				{
					m_CompiledData.Add(DNew(CCompiledDataCompile) CCompiledDataCompile);
				}
				return true;
			}
			return false;
		}
		void CommitData()
		{
            if (m_iCurrentCompiledData < m_CompiledData.Len())
				m_CompiledData[m_iCurrentCompiledData]->CommitUsage();
		}

		template <int f_bLE>
		int AddHeapStr(CStr _String, int _Alignment = 2)
		{
			CStr Dupl = _String;
			Dupl.MakeUnique();
			CStr::CStrData *pStrData = Dupl.GetStrData();
			mint DataLen = _String.Len() + sizeof(uint16);
			
			if (!pStrData)
			{
				CStr::CStrData StrData;
				StrData.f_bIsAllocated(0);
				StrData.f_MRTC_ReferenceCount(2);
				StrData.f_bIsEmpty(1);
				StrData.f_bIsUnicode(0);
#ifdef CPU_LITTLEENDIAN
				if (!f_bLE)
					Swap_uint16(StrData.m_Data);
#else
				if (f_bLE)
					Swap_uint16(StrData.m_Data);
#endif
				int iRet = AddHeapData(&StrData, DataLen, true, _Alignment);
#ifdef CPU_LITTLEENDIAN
				if (!f_bLE)
					Swap_uint16(StrData.m_Data);
#else
				if (f_bLE)
					Swap_uint16(StrData.m_Data);
#endif
				return iRet;
			}
			else
			{
				++DataLen;
				bint bAlloc = pStrData->f_bIsAllocated();
				mint RefCount = pStrData->f_bIsAllocated();
				pStrData->f_bIsAllocated(0);
				pStrData->f_MRTC_ReferenceCount(2);
#ifdef CPU_LITTLEENDIAN
				if (!f_bLE)
					Swap_uint16(pStrData->m_Data);
#else
				if (f_bLE)
					Swap_uint16(pStrData->m_Data);
#endif
				int iRet = AddHeapData(pStrData, DataLen, true, _Alignment);
#ifdef CPU_LITTLEENDIAN
				if (!f_bLE)
					Swap_uint16(pStrData->m_Data);
#else
				if (f_bLE)
					Swap_uint16(pStrData->m_Data);
#endif
				pStrData->f_bIsAllocated(bAlloc);
				pStrData->f_MRTC_ReferenceCount(RefCount);
				return iRet;
			}
		}
#if 0
		int AddHeapDataOld(void *_pData, mint _DataSize, bint _bString, int _Alignment)
		{
			CCompiledDataCompile* pCompiledData = m_CompiledData[m_iCurrentCompiledData];
			uint8 *pData = pCompiledData->m_DataHeap.GetBasePtr();
			uint8 *pStrMask = pCompiledData->m_lStrDataHeaderMask.GetBasePtr();
			int iMax = pCompiledData->m_UsedDataHeap - _DataSize;
			for (int i = 0; i <= iMax; i += _Alignment)
			{
				if (_bString)
				{
					if (!pStrMask[i >> 1])
						continue;
				}
				else
				{
					int PreI = i;
					mint Max = (i + _DataSize + 1);
					if (Max > iMax)
						Max = iMax;
					for (int j = i >> 1; j < Max >> 1; ++j)
					{
						if (pStrMask[j])
						{
							i = (((((j+1) << 1) + _Alignment - 1) / _Alignment) * _Alignment);
							break;
						}
					}
					if (PreI != i)
					{
						i = i - _Alignment;
						continue;
					}
				}

				if (memcmp(pData + i, _pData, _DataSize) == 0)
				{
					pCompiledData->m_ReusedDataHeap += _DataSize;
					return i / _Alignment;
				}
			}
			return -1;
		}
#endif

		int AddHeapData(void *_pData, mint _DataSize, bint _bString, int _Alignment)
		{
#if 1
	//		int iOldData = AddHeapDataOld(_pData, _DataSize, _bString, _Alignment);

			CCompiledDataCompile* pCompiledData = m_CompiledData[m_iCurrentCompiledData];
			uint8 *pData = pCompiledData->m_DataHeap.GetBasePtr();
			uint8 *pStrMask = pCompiledData->m_lStrDataHeaderMask.GetBasePtr();
			CCompiledDataCompile::CIndex::CEntry *pEntries = pCompiledData->m_IndexEntries.GetBasePtr();

			int iMax = pCompiledData->m_UsedDataHeap - _DataSize;

			CCompiledDataCompile::CIndex::CEntry ToFind;
			ToFind.m_pData = _pData;
			ToFind.m_DataLen = Min(_DataSize, mint(CCompiledDataCompile::EIndexSize * 2));

			if (pCompiledData->m_Index.GetRoot())
			{
				CCompiledDataCompile::CIndexIter Iter;
				Iter.SetRoot(pCompiledData->m_Index);
				Iter.FindSmallestGreaterThanEqualForward(ToFind);

				if (Iter)
				{
					int ToCheck = Min(_DataSize, mint(CCompiledDataCompile::EIndexSize * 2));

					mint AlignmentCheck = mint(_Alignment) - 1;

					while (Iter)
					{
						CCompiledDataCompile::CIndex *pIndex = Iter;
						CCompiledDataCompile::CIndex::CEntry *pEntry = pIndex->m_Entries.GetFirst();
						if (pEntry->m_DataLen < ToCheck)
							break;

						if (memcmp(pEntry->m_pData, _pData, ToCheck))
							break; // Not found

						CCompiledDataCompile::CIndexEntryIter EntryIter = pIndex->m_Entries;

						while (EntryIter)
						{
							CCompiledDataCompile::CIndex::CEntry *pEntry = EntryIter;
							uint8 *pData = (uint8 *)pEntry->m_pData;

							int iIndex = pEntry - pEntries;
							if (pStrMask[iIndex] == _bString) // Must be same stringness
							{
								// Check alignment
								if (!((mint)pData & AlignmentCheck))
								{
									if (_DataSize > ToCheck)
									{
										// Must check following bytes
										mint MaxSize = (pCompiledData->m_UsedDataHeap) - (iIndex << 1);
										if (_DataSize <= MaxSize)
										{
											if (memcmp((uint8 *)_pData + ToCheck, pData + ToCheck, _DataSize - ToCheck) == 0)
											{
												pCompiledData->m_ReusedDataHeap += _DataSize;
												int iRet = iIndex / (_Alignment >> 1);
//												if (iRet != iOldData)
//													M_BREAKPOINT;
												return iRet;

											}
										}									
									}
									else
									{
										pCompiledData->m_ReusedDataHeap += _DataSize;
										int iRet = iIndex / (_Alignment >> 1);
//										if (iRet != iOldData)
//											M_BREAKPOINT;
										return iRet;
									}
								}
							}
							
							++EntryIter;
						}

						++Iter;
					}
				}
			}

	//		if (iOldData >= 0)
	//			M_BREAKPOINT;

			mint WritePos = ((pCompiledData->m_UsedDataHeap + _Alignment - 1) / _Alignment) * _Alignment;
			mint DataLeft = (1 << 17) - WritePos;
			if (DataLeft < _DataSize)
				return -1;

			if (_bString)
			{
				pStrMask[WritePos >> 1] = 1;
			}
			memcpy(pData + WritePos, _pData, _DataSize);
			pCompiledData->IndexAddedData(pCompiledData->m_UsedDataHeap, (WritePos + _DataSize) - pCompiledData->m_UsedDataHeap);
			pCompiledData->m_UsedDataHeap = WritePos + _DataSize;
			return WritePos / _Alignment;
#else
			CCompiledDataCompile* pCompiledData = m_CompiledData[m_iCurrentCompiledData];
			uint8 *pData = pCompiledData->m_DataHeap.GetBasePtr();
			uint8 *pStrMask = pCompiledData->m_lStrDataHeaderMask.GetBasePtr();
			int iMax = pCompiledData->m_UsedDataHeap - _DataSize;
			for (int i = 0; i <= iMax; i += _Alignment)
			{
				if (_bString)
				{
					if (!pStrMask[i >> 1])
						continue;
				}
				else
				{
					int PreI = i;
					mint Max = (i + _DataSize + 1);
					if (Max > iMax)
						Max = iMax;
					for (int j = i >> 1; j < Max >> 1; ++j)
					{
						if (pStrMask[j])
						{
							i = (((((j+1) << 1) + _Alignment - 1) / _Alignment) * _Alignment);
							break;
						}
					}
					if (PreI != i)
					{
						i = i - _Alignment;
						continue;
					}
				}

				if (memcmp(pData + i, _pData, _DataSize) == 0)
				{
					pCompiledData->m_ReusedDataHeap += _DataSize;
					return i / _Alignment;
				}
			}
			mint WritePos = ((pCompiledData->m_UsedDataHeap + _Alignment - 1) / _Alignment) * _Alignment;
			mint DataLeft = (1 << 17) - WritePos;
			if (DataLeft < _DataSize)
				return -1;

			if (_bString)
			{
				pStrMask[WritePos >> 1] = 1;
			}
			pCompiledData->m_UsedDataHeap = WritePos + _DataSize;
			memcpy(pData + WritePos, _pData, _DataSize);
			return WritePos / _Alignment;
#endif
		}

		uint32 *GetNodeDataPtr(int _Index)
		{
			CCompiledDataCompile* pCompiledData = m_CompiledData[m_iCurrentCompiledData];
			return &pCompiledData->m_NodeHeap[_Index];
		}
		int AllocNodeData(mint _DataSize)
		{
			CCompiledDataCompile* pCompiledData = m_CompiledData[m_iCurrentCompiledData];

			mint RealSize = ((_DataSize + 3) & ~3);
			aint MaxSize = (1 << 16)*4 - pCompiledData->m_UsedNodeHeap;
			if (aint(_DataSize) > MaxSize)
				return -1;

			int iRet = pCompiledData->m_UsedNodeHeap / 4;
			pCompiledData->m_UsedNodeHeap += RealSize;

			return iRet;
		}
	};


	CRegistryCompiledInternal()
	{
	}

	~CRegistryCompiledInternal()
	{
	}

	template <int _bLE>
	int StoreData(CRegistry* _pReg, CCompileContext &_Context, int _iSeq, int _iKF)
	{
		int iRet = 0;
		int Type = _pReg->GetType();
		int nDim = _pReg->GetDimensions();

		switch (Type)
		{
		case REGISTRY_TYPE_VOID:
			break;
		case REGISTRY_TYPE_STR:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				CStr Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValuea(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
				{
					int iStr = _Context.AddHeapStr<_bLE>(Data[d]);
					if (iStr < 0)
						return -1;
                    DataStream.Write<_bLE>((uint16)iStr);
				}
				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 2);
				if (iData0 < 0)
					return -1;
				iRet = iData0;
			}
			break;
		case REGISTRY_TYPE_DATA:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				TArray<uint8> Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValuead(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
				{
					CMemoryStream DataDataStream(_Context.m_MemoryStreamDataPool);
					uint32 Length = Data[d].Len();
					DataDataStream.Write<_bLE>(Length);
					DataDataStream.m_File.Write(Data[d].GetBasePtr(), Length);

					int iValue = _Context.AddHeapData(DataDataStream.GetPtr(), DataDataStream.GetLen(), false, 4);
					if (iValue < 0)
						return -1; 
                    DataStream.Write<_bLE>((uint16)iValue);
				}
				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 2);
				if (iData0 < 0)
					return -1;
				iRet = iData0;
			}
			break;
		case REGISTRY_TYPE_UINT8:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				int32 Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValueai(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
					DataStream.Write<_bLE>((uint8)Data[d]);

				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 2);
				if (iData0 < 0)
					return -1;
				iRet = iData0;
			}
			break;
		case REGISTRY_TYPE_INT16:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				int32 Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValueai(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
					DataStream.Write<_bLE>((int16)Data[d]);

				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 2);
				if (iData0 < 0)
					return -1;
				iRet = iData0;
			}
			break;
		case REGISTRY_TYPE_INT32:
		case REGISTRY_TYPE_UINT32:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				int32 Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValueai(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
					DataStream.Write<_bLE>(Data[d]);

				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 4);
				if (iData0 < 0)
					return -1;
				iRet = iData0 << 1;
			}
			break;
		case REGISTRY_TYPE_FP4:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				fp4 Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValueaf(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
					DataStream.Write<_bLE>(Data[d]);

				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 4);
				if (iData0 < 0)
					return -1;
				iRet = iData0 << 1;
			}
			break;
		case REGISTRY_TYPE_FP2:
			{
				CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);
				fp4 Data[REGISTRY_MAX_DIMENSIONS];
				_pReg->Anim_ThisGetKFValueaf(_iSeq, _iKF, nDim, Data);
				for (int d = 0; d < nDim; ++d)
					DataStream.Write<_bLE>(fp2(Data[d]).m_Half);

				int iData0 = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 2);
				if (iData0 < 0)
					return -1;
				iRet = iData0;
			}
			break;
		default:
			Error_static(M_FUNCTION, "Unsupported data type");
			break;
		}

		return iRet;
	}

	class CSortEntry
	{
	public:
		int m_iIndex;
		uint16 m_Hash;

		static int M_CDECL QSortCompare(const CSortEntry* _pFirst,const CSortEntry* _pSecond)
		{
#if 0
			if (_pFirst->m_iIndex > _pSecond->m_iIndex)
				return 1;
			else if (_pFirst->m_iIndex < _pSecond->m_iIndex)
				return -1;
			else
			{
				M_ASSERT(0, "Should not happen");
				return 0;
			}
#else
			if (_pFirst->m_Hash > _pSecond->m_Hash)
				return 1;
			else if (_pFirst->m_Hash < _pSecond->m_Hash)
				return -1;
			else
			{
				if (_pFirst->m_iIndex > _pSecond->m_iIndex)
					return 1;
				else if (_pFirst->m_iIndex < _pSecond->m_iIndex)
					return -1;
				else
				{
					M_ASSERT(0, "Should not happen");
					return 0;
				}
			}
#endif
		}
	};

	template <int _bLE>
	void Compile_r(CRegistry* _pReg, CCompileContext &_Context, CRegistryCompiledInternal::CRegistry_Const &_Node, CCompileContext *_pSecondContext, CRegistryCompiledInternal::CRegistry_Const *_pSecondReg)
	{
		bint bTriedNewData = false;
		_Context.StartSearch();
		goto NormalBranch;
RestartSearch:
		if (_Context.NextCompiledData())
		{
			if (bTriedNewData)
				Error_static(M_FUNCTION, "Key too large, didn't fit in one compiled data");
			bTriedNewData = true;
		}
NormalBranch:

		// Build data

		bint bChildrenHasChildren = false;
		bint bChildren = _Node.Private_Get_HasChildren();

		int nChildren = _pReg->GetNumChildren();
		if (bChildren)
		{
//			M_ASSERT(nChildren > 0, "");
			_Node.Private_Set_NumChildren(nChildren);
			_Node.Private_Set_ChildNodeStart(0);
		}
		else
		{
			M_ASSERT(nChildren == 0, "");
		}

		for (int i = 0; i < nChildren; ++i)
		{
			if (_pReg->GetChild(i)->GetNumChildren())
			{
				bChildrenHasChildren = true;
				break;
			}
		}
		
		mint ChildSize = nChildren * ((bChildrenHasChildren ? sizeof(uint32) : 0) + sizeof(CRegistry_Const)) + ((nChildren + 1) >> 1) * sizeof(uint32);

		if (bChildren)
		{
			_Node.Private_Set_ChildrenHasChildren(bChildrenHasChildren);
			_Node.Private_Set_ChildNodeStart(0);
		}

		if (!_Context.m_bRoot)
		{

			//// ToDo
			int iNameStr = _Context.AddHeapStr<_bLE>(_pReg->GetThisName(), 4);
			if (iNameStr < 0)
				goto RestartSearch;
			_Node.Private_Set_NameDataIndex(iNameStr);

			CMemoryStream MainStream(_Context.m_MemoryStreamDataPool);

			bint bAnimated = _pReg->Anim_ThisGetAnimated();
			uint32 Type = _pReg->GetType();
			uint32 nDim = _pReg->GetDimensions();
			uint16 Header = 0;
			if (bAnimated)
				Header |= 1;
			Header |= (Type & DBitRange(0,3)) << 1;
			Header |= ((nDim-1) & DBitRange(0,4)) << 5;

			MainStream.Write<_bLE>(Header);
			if (bAnimated)
			{
				//uint32 m_InternalAnimFlags:1; // Timed
				//uint32 m_AnimFlags:7;
				//uint32 m_nSeq:8;
				//
				//uint32 m_AnimIPType:8;
				//uint32 m_nData:8;
				bint bTimed = _pReg->Anim_ThisGetEnableTimed();
				uint32 AnimFlags = _pReg->Anim_ThisGetFlags();
				fp4 Params[256];
				int nParams = 256;
				uint32 Interpolate = _pReg->Anim_ThisGetInterpolate(Params, nParams);
				uint16 AnimHeader0 = 0;
				int nSeq = _pReg->Anim_ThisGetNumSeq();

				if (bTimed)
					AnimHeader0 |= 1;
				AnimHeader0 |= (AnimFlags & DBitRange(0,6)) << 1;
				AnimHeader0 |= (nSeq & DBitRange(0,7)) << 8;

				uint16 AnimHeader1 = 0;
				AnimHeader1 |= (Interpolate & DBitRange(0,7)) << 0;
				AnimHeader1 |= (nParams & DBitRange(0,7)) << 8;

				MainStream.Write<_bLE>(AnimHeader0);
				MainStream.Write<_bLE>(AnimHeader1);

				if (nParams)
				{
					CMemoryStream DataStream(_Context.m_MemoryStreamDataPool);

					for (int i = 0; i < nParams; ++i)
						DataStream.Write<_bLE>(Params[i]);

					int IPIndex = _Context.AddHeapData(DataStream.GetPtr(), DataStream.GetLen(), false, 4);
					if (IPIndex < 0)
						goto RestartSearch;
					MainStream.Write<_bLE>((uint16)IPIndex);
				}

				// Build Sequences
				if (nSeq)
				{
					for (int i = 0; i < nSeq; ++i)
					{
						CMemoryStream SequenceStream(_Context.m_MemoryStreamDataPool);

						{
							uint16 nKF = _pReg->Anim_ThisGetNumKF(i);
							SequenceStream.Write<_bLE>(nKF);
							fp4 LoopData[2];
							LoopData[0] = _pReg->Anim_ThisGetSeqLoopStart(i);
							LoopData[1] = _pReg->Anim_ThisGetSeqLoopEnd(i);

							int LoopIndex =_Context.AddHeapData(LoopData, sizeof(LoopData), false, 4);
							if (LoopIndex < 0)
								goto RestartSearch;

							SequenceStream.Write<_bLE>((uint16)LoopIndex);

							if (bTimed)
							{			
								CMemoryStream TimeStream(_Context.m_MemoryStreamDataPool);

								for (int j = 0; j < nKF; ++j)
									TimeStream.Write<_bLE>((fp4)_pReg->Anim_ThisGetKFTime(i, j));

								int Index =_Context.AddHeapData(TimeStream.GetPtr(), TimeStream.GetLen(), false, 4);
								if (Index < 0)
									goto RestartSearch;
								SequenceStream.Write<_bLE>((uint16)Index);
							}
							for (int j = 0; j < nKF; ++j)
							{
								int iData = StoreData<_bLE>(_pReg, _Context, i, j);
								if (iData < 0)
									goto RestartSearch;

								SequenceStream.Write<_bLE>((uint16)iData);
							}
						}

						int SeqIndex =_Context.AddHeapData(SequenceStream.GetPtr(), SequenceStream.GetLen(), false, 2);
						if (SeqIndex < 0)
							goto RestartSearch;
						MainStream.Write<_bLE>((uint16)SeqIndex);
					}
				}
			}
			else
			{
				int iData = StoreData<_bLE>(_pReg, _Context, 0, 0);
				if (iData < 0)
					goto RestartSearch;
				MainStream.Write<_bLE>((uint16)iData);
			}

			int iValue = _Context.AddHeapData(MainStream.GetPtr(), MainStream.GetLen(), false, 2);
			if (iValue < 0)
				goto RestartSearch;
			_Node.Private_Set_ValueIndex(iValue);
		}

		int iNodeChildren = 0;

		if (nChildren > 0)
		{
			M_ASSERT(bChildren, "");
			if (_pSecondContext && !_Context.m_bRoot && 0)
			{
				// Try to find already existing identical child nodes
				int iData = *((uint32 *)_pSecondReg);
				int iNode;
				CCompileContext::CCompiledDataCompile *pData = _pSecondContext->CompiledDataFromNode(*_pSecondReg, iNode);
				if (pData)
				{
					CMD5Digest &Digest = pData->m_NodeChecksumsChildren[iNode];

					CRegistryCompiledInternal::CRegistry_Const *pRegistry = _Context.FindNode(Digest, _Context.m_iCurrentCompiledData);
					if (pRegistry)
					{
						_Node.Private_Set_ChildNodeStart(pRegistry->Private_Get_ChildNodeStart());
						_Context.CommitData();
						((uint32 &)_Node) = _Context.m_iCurrentCompiledData;
						return;
					}
				}
			}
			iNodeChildren = _Context.AllocNodeData(ChildSize);
			if (iNodeChildren < 0)
				goto RestartSearch;

			_Node.Private_Set_ChildNodeStart(iNodeChildren);
		}

		_Context.m_bRoot = false;

		_Context.CommitData();
		((uint32 &)_Node) = _Context.m_iCurrentCompiledData;

		if (nChildren)
		{
			uint32 *pNodeData = NULL;
			pNodeData = _Context.GetNodeDataPtr(iNodeChildren);


			CSortEntry *pSortArray = DNew(CSortEntry) CSortEntry[nChildren];
			for (int i = 0; i < nChildren; ++i)
			{
				pSortArray[i].m_iIndex = i;
				pSortArray[i].m_Hash = StringToHash(_pReg->GetChild(i)->GetThisName().UpperCase()) & 0xffff;
			}
			qsort(pSortArray, nChildren, sizeof(CSortEntry), (int (M_CDECL *)(const void *, const void *))CSortEntry::QSortCompare);
			uint16 *pHashStart = (uint16 *)pNodeData;
			for (int i = 0; i < nChildren; ++i)
			{
				pHashStart[i] = pSortArray[i].m_Hash;
			}

			uint32 *pNodes = pNodeData + ((nChildren + 1) >> 1);
			mint NodeSize = (bChildrenHasChildren ? sizeof(uint32) : 0) + sizeof(CRegistry_Const);

			for (int i = 0; i < nChildren; ++i)
			{
				int iChild = pSortArray[i].m_iIndex;

				CRegistryCompiledInternal::CRegistry_Const &Node = *((CRegistryCompiledInternal::CRegistry_Const *)(((uint8 *)pNodes + NodeSize * i)));
				Node.Private_Set_HasChildren(bChildrenHasChildren);
				
				if (_pSecondContext)
				{
					// Try to find an equal node 
					int iData = *((uint32 *)_pSecondReg);
					uint32 *pNodeData2 = &(_pSecondContext->m_CompiledData[iData]->m_NodeHeap[_pSecondReg->Private_Get_ChildNodeStart()]);
					uint32 *pNodes2 = pNodeData2 + ((nChildren + 1) >> 1);
					CRegistryCompiledInternal::CRegistry_Const &Node2 = *((CRegistryCompiledInternal::CRegistry_Const *)(((uint8 *)pNodes2 + NodeSize * i)));

					Compile_r<_bLE>(_pReg->GetChild(iChild), _Context, Node, _pSecondContext, &Node2);
				}
				else
					Compile_r<_bLE>(_pReg->GetChild(iChild), _Context, Node, NULL, NULL);
			}
			if (_pSecondContext)
			{
				_Context.CalcChecksum<_bLE>(_Node); // Calculate checksum for this node
			}
			delete [] pSortArray;
		}
	}

	uint32 m_RootNodeChildren;
	CRegistry_Const *GetFromNode(int _iData, int _Node)
	{
		return (CRegistry_Const *)(&(m_CompiledData[_iData].m_lRegistryNodes[_Node]));
	}

	void Patchup_r(CRegistry_Const *_pThis)
	{
		if (*((uint32 *)_pThis) > 65536)
		{
			// This node is already patched
			return ;
		}
		int iData = *((uint32 *)_pThis);
		new(_pThis) CRegistry_Const();
#ifdef M_ENABLE_REGISTRYCOMPILEDVTABLEHACK
#else
		_pThis->m_pCompiledData = &m_CompiledData[iData];
#endif
		int nChildren = _pThis->Private_Get_NumChildren();
		int iChildBase = _pThis->Private_Get_ChildNodeStart();
		int iNodeStart = (nChildren + 1) >> 1;
		iChildBase += iNodeStart;

		int Stride = sizeof(CRegistry_Const) / sizeof(uint32) + (_pThis->Private_Get_ChildrenHasChildren() ? 1 : 0);
		for (int i = 0; i < nChildren; ++i)
		{
			CRegistry_Const *pThis = GetFromNode(iData, iChildBase + i * Stride);
			Patchup_r(pThis);
		}
	}

	void SetFromCompileContext(CCompileContext &_Context)
	{
		int Len = _Context.m_CompiledData.Len();
		m_CompiledData.SetLen(Len);
		for (int i = 0; i < Len; ++i)
		{
			CCompileContext::CCompiledDataCompile *pCompiledData = _Context.m_CompiledData[i];
			CCompiledData *pDest = &(m_CompiledData[i]);
			pDest->m_lHeapData.SetLen(pCompiledData->m_UsedDataHeap);
			pDest->m_iCompiledData = i;
			memcpy(pDest->m_lHeapData.GetBasePtr(), pCompiledData->m_DataHeap.GetBasePtr(), pCompiledData->m_UsedDataHeap);
			pDest->m_lRegistryNodes.SetLen(pCompiledData->m_UsedNodeHeap / sizeof(uint32));
			memcpy(pDest->m_lRegistryNodes.GetBasePtr(), pCompiledData->m_NodeHeap.GetBasePtr(), pCompiledData->m_UsedNodeHeap);
		}

		if (m_RootNodeChildren)
		{
			int iNodeStart = (m_RootNodeChildren + 1) >> 1;
			CRegistry_Const *pRootNode = GetFromNode(0, iNodeStart);
			int Stride = (sizeof(CRegistry_Const) + (pRootNode->Private_Get_HasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);
			for (int i = 0; i < m_RootNodeChildren; ++i)
			{
				CRegistry_Const *pNode = GetFromNode(0, iNodeStart + i * Stride);
				Patchup_r(pNode);
			}
		}
	}

	class CWriteHelper
	{
	public:
		CWriteHelper()
		{
			m_nNodesLarge = 0;
			m_nNodesSmall = 0;
			m_nHashEntries = 0;
			m_iCurrent = 0;
			m_Tags.SetLen(1 << 16);
			for (int i = 0; i < m_Tags.Len(); ++i)
				m_Tags[i] = -1;
		}

		TThinArray<int32> m_Tags;

		uint32 m_nNodesLarge;
		uint32 m_nNodesSmall;
		uint32 m_nHashEntries;
		uint32 m_iCurrent;
	};

	void ReadNode(CRegistry_Const *_pThis, CCFile *_pFile)
	{
		uint32 iCompiled; 
		_pFile->ReadLE(iCompiled);
		_pThis->m_pCompiledData = &(m_CompiledData[iCompiled]);
//		CCompiledData *pData = _pThis->Private_GetData();

		_pFile->ReadLE(_pThis->m_Data0);

		if (_pThis->Private_Get_HasChildren())
		{
			_pFile->ReadLE(*((uint32 *)(_pThis + 1)));
		}
	}	
	
	void ReadNode_r(CRegistry_Const *_pThis, CCFile *_pFile, CWriteHelper *_pHelper)
	{
		CCompiledData *pData = _pThis->Private_GetData();

		int nChildren = _pThis->Private_Get_NumChildren();
		int iData = pData->m_iCompiledData;
		if (nChildren)
		{
			int iOrgNode = _pThis->Private_Get_ChildNodeStart();
			if (_pHelper[iData].m_Tags[iOrgNode] >= 0)
			{
				_pThis->Private_Set_ChildNodeStart(_pHelper[iData].m_Tags[iOrgNode]);
				return;
			}

			_pThis->Private_Set_ChildNodeStart(_pHelper[iData].m_iCurrent);
			_pHelper[iData].m_Tags[iOrgNode] = _pHelper[iData].m_iCurrent;

			int Stride = (sizeof(CRegistry_Const) + (_pThis->Private_Get_ChildrenHasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);

			_pHelper[iData].m_iCurrent += Stride * nChildren + ((nChildren + 1) >> 1);

			const uint32 *pNodes = pData->GetNodePtr(_pThis->Private_Get_ChildNodeStart());
			_pFile->ReadLE((uint16 *)pNodes, (nChildren + 1) & (~1));

			for (int i = 0; i < nChildren; ++i)
			{
				CRegistry_Const *pThis = _pThis->Private_GetChild(i);
				new (pThis) CRegistry_Const;
				ReadNode(pThis, _pFile);
				ReadNode_r(pThis, _pFile, _pHelper);
			}
		}
	}

	void Read(CCFile *_pFile, int _Version)
	{
		uint32 nData;
		_pFile->ReadLE(nData);

		TArray<CWriteHelper> Helper;
		Helper.SetLen(nData);
		m_CompiledData.SetLen(nData);
		CWriteHelper *pHelper = Helper.GetBasePtr();

		for (int i = 0; i < nData; ++i)
		{
			m_CompiledData[i].m_iCompiledData = i;
			_pFile->ReadLE(pHelper[i].m_nNodesLarge);
			_pFile->ReadLE(pHelper[i].m_nNodesSmall);
			_pFile->ReadLE(pHelper[i].m_nHashEntries);

			int ConstSize = sizeof(CRegistry_Const);
			int NeededSize = pHelper[i].m_nNodesLarge * (ConstSize + sizeof(uint32));
			NeededSize += pHelper[i].m_nNodesSmall * ConstSize;
			NeededSize += pHelper[i].m_nHashEntries * sizeof(uint32);
			NeededSize /= sizeof(uint32);
			M_ASSERT(NeededSize <= 1 << 16, "Overflow");
			m_CompiledData[i].m_lRegistryNodes.SetLen(NeededSize);
		}

		uint32 RootNodeChildren;
		_pFile->ReadLE(RootNodeChildren);
		m_RootNodeChildren = RootNodeChildren;

		if (m_RootNodeChildren)
		{
			_pFile->ReadLE((uint16 *)m_CompiledData[0].m_lRegistryNodes.GetBasePtr(), (m_RootNodeChildren + 1) & (~1));
			pHelper[0].m_iCurrent += (m_RootNodeChildren + 1) >> 1;

			int iNodeStart = (m_RootNodeChildren + 1) >> 1;
			CRegistry_Const *pRootNode = GetFromNode(0, iNodeStart);
			new (pRootNode) CRegistry_Const;
			ReadNode(pRootNode, _pFile);
			int Stride = (sizeof(CRegistry_Const) + (pRootNode->Private_Get_HasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);
			pHelper[0].m_iCurrent += Stride * m_RootNodeChildren;
			ReadNode_r(pRootNode, _pFile, pHelper);

			for (int i = 1; i < m_RootNodeChildren; ++i)
			{
				CRegistry_Const *pNode = GetFromNode(0, iNodeStart + i * Stride);
				new (pNode) CRegistry_Const;
				ReadNode(pNode, _pFile);
				ReadNode_r(pNode, _pFile, pHelper);
			}
		}

		for (int i = 0; i < nData; ++i)
		{
			int NeededSize = pHelper[i].m_nNodesLarge * (sizeof(CRegistry_Const) + sizeof(uint32));
			NeededSize += pHelper[i].m_nNodesSmall * sizeof(CRegistry_Const);
			NeededSize += pHelper[i].m_nHashEntries * sizeof(uint32);
			NeededSize /= sizeof(uint32);
			M_ASSERT(NeededSize == pHelper[i].m_iCurrent, "Error in allocation");
		}

		for (int i = 0; i < nData; ++i)
		{
			uint32 Len;
			_pFile->ReadLE(Len);
			m_CompiledData[i].m_lHeapData.SetLen(Len);
			_pFile->Read(m_CompiledData[i].m_lHeapData.GetBasePtr(), Len);
		}
	}

	void GetWriteData_r(CRegistry_Const *_pThis, CWriteHelper *_pWriteData)
	{
		CCompiledData *pData = _pThis->Private_GetData();
		CWriteHelper &WriteHelper = _pWriteData[pData->m_iCompiledData];

		int nChildren = _pThis->Private_Get_NumChildren();
		if (_pThis->Private_Get_HasChildren() && nChildren)
		{
			int iOrgNode = _pThis->Private_Get_ChildNodeStart();
			if (WriteHelper.m_Tags[iOrgNode] == 1)
			{
				return;
			}
			WriteHelper.m_Tags[iOrgNode] = 1;

			int nChildren = _pThis->Private_Get_NumChildren();
			if (_pThis->Private_Get_ChildrenHasChildren())
				WriteHelper.m_nNodesLarge += nChildren;
			else
				WriteHelper.m_nNodesSmall += nChildren;
			WriteHelper.m_nHashEntries += (nChildren + 1) >> 1;
			for (int i = 0; i < nChildren; ++i)
			{
				CRegistry_Const *pThis = _pThis->Private_GetChild(i);
				GetWriteData_r(pThis, _pWriteData);
			}
		}
	}

	void Write_r(CRegistry_Const *_pThis, CCFile *_pFile, CWriteHelper *_pWriteData)
	{
		CCompiledData *pData = _pThis->Private_GetData();
		CWriteHelper &WriteHelper = _pWriteData[pData->m_iCompiledData];

		_pFile->WriteLE((uint32)pData->m_iCompiledData);
		_pFile->WriteLE(_pThis->m_Data0);
		int nChildren = _pThis->Private_Get_NumChildren();
		if (_pThis->Private_Get_HasChildren())
		{
			_pFile->WriteLE(*((uint32 *)(_pThis + 1)));

			if (nChildren)
			{
				int iNode = _pThis->Private_Get_ChildNodeStart();
				if (WriteHelper.m_Tags[iNode] == 2)
					return;
				WriteHelper.m_Tags[iNode] = 2;

				const uint32 *pNodes = pData->GetNodePtr(iNode);
				_pFile->WriteLE((uint16 *)pNodes, (nChildren + 1) & (~1));

				for (int i = 0; i < nChildren; ++i)
				{
					CRegistry_Const *pThis = _pThis->Private_GetChild(i);
					Write_r(pThis, _pFile, _pWriteData);
				}
			}
		}
	}

	void Write(CCFile *_pFile)
	{
		TArray<CWriteHelper> Helper;
		Helper.SetLen(m_CompiledData.Len());

		if (m_RootNodeChildren)
		{
			int iNodeStart = (m_RootNodeChildren + 1) >> 1;
			CRegistry_Const *pRootNode = GetFromNode(0, iNodeStart);
			int Stride = (sizeof(CRegistry_Const) + (pRootNode->Private_Get_HasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);

			int nChildren = m_RootNodeChildren;
			if (pRootNode->Private_Get_HasChildren())
				Helper[0].m_nNodesLarge += nChildren;
			else
				Helper[0].m_nNodesSmall += nChildren;
			Helper[0].m_nHashEntries += (nChildren + 1) >> 1;

			for (int i = 0; i < m_RootNodeChildren; ++i)
			{
				CRegistry_Const *pNode = GetFromNode(0, iNodeStart + i * Stride);
				GetWriteData_r(pNode, Helper.GetBasePtr());
			}
		}

		uint32 nData = m_CompiledData.Len();
		_pFile->WriteLE((uint32)nData);
		for (int i = 0; i < nData; ++i)
		{
//			LogFile(CStr("Len %d = %d", i, m_CompiledData[i].m_lRegistryNodes.Len()));
			_pFile->WriteLE(Helper[i].m_nNodesLarge);
			_pFile->WriteLE(Helper[i].m_nNodesSmall);
			_pFile->WriteLE(Helper[i].m_nHashEntries);
		}

		_pFile->WriteLE((uint32)m_RootNodeChildren);
		if (m_RootNodeChildren)
		{
			_pFile->WriteLE((uint16 *)m_CompiledData[0].m_lRegistryNodes.GetBasePtr(), (m_RootNodeChildren + 1) & (~1));

			int iNodeStart = (m_RootNodeChildren + 1) >> 1;
			CRegistry_Const *pRootNode = GetFromNode(0, iNodeStart);

			int Stride = (sizeof(CRegistry_Const) + (pRootNode->Private_Get_HasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);

			for (int i = 0; i < m_RootNodeChildren; ++i)
			{
				CRegistry_Const *pNode = GetFromNode(0, iNodeStart + i * Stride);
				Write_r(pNode, _pFile, Helper.GetBasePtr());
			}
		}

		for (int i = 0; i < nData; ++i)
		{
			uint32 Len = m_CompiledData[i].m_lHeapData.Len();
			_pFile->WriteLE(Len);
			_pFile->Write(m_CompiledData[i].m_lHeapData.GetBasePtr(), Len);
		}
	}

	void Compile(CRegistry* _pReg, bint _bFastSearch)
	{
		m_CompiledData.Destroy();
		CCompileContext Context;
		Context.m_bFastSearch = _bFastSearch;

		uint32 NodeData[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
		NodeData[0] = 0xFFffFFff;
		CRegistryCompiledInternal::CRegistry_Const &Node = *((CRegistry_Const *)NodeData);
		Node.Private_Set_HasChildren(true);
		Node.Private_Set_ChildrenHasChildren(true);
#ifdef CPU_LITTLEENDIAN
		Compile_r<1>(_pReg, Context, Node, NULL, NULL);
#else
		Compile_r<0>(_pReg, Context, Node, NULL, NULL);
#endif

		if (_bFastSearch)
		{
			m_RootNodeChildren = Node.Private_Get_NumChildren();
			M_ASSERT(Node.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData[0] == 0, "Must be so");
			SetFromCompileContext(Context);
		}
		else
		{
			// Second pass
			CCompileContext Context2;
			Context2.m_bFastSearch = _bFastSearch;

#ifdef CPU_LITTLEENDIAN
			Context.CalcChecksum<1>(Node);
#else
			Context.CalcChecksum<0>(Node);
#endif

			uint32 NodeData2[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
			NodeData2[0] = 0xFFffFFff;
			CRegistryCompiledInternal::CRegistry_Const &Node2 = *((CRegistry_Const *)NodeData2);
			Node2.Private_Set_HasChildren(true);
			Node2.Private_Set_ChildrenHasChildren(true);
	#ifdef CPU_LITTLEENDIAN
			Compile_r<1>(_pReg, Context2, Node2, &Context, &Node);
	#else
			Compile_r<0>(_pReg, Context2, Node2, &Context, &Node);
	#endif

			m_RootNodeChildren = Node2.Private_Get_NumChildren();
			M_ASSERT(Node2.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData2[0] == 0, "Must be so");
			SetFromCompileContext(Context2);
		}
	}

	void CompileLE(CRegistry* _pReg, bint _bFastSearch)
	{
		m_CompiledData.Destroy();
		CCompileContext Context;
		Context.m_bFastSearch = _bFastSearch;
		uint32 NodeData[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
		NodeData[0] = 0xFFffFFff;
		CRegistryCompiledInternal::CRegistry_Const &Node = *((CRegistry_Const *)NodeData);
		Node.Private_Set_HasChildren(true);
		Node.Private_Set_ChildrenHasChildren(true);
		Compile_r<1>(_pReg, Context, Node, NULL, NULL);
		if (_bFastSearch)
		{
			m_RootNodeChildren = Node.Private_Get_NumChildren();
			M_ASSERT(Node.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData[0] == 0, "Must be so");
			SetFromCompileContext(Context);
		}
		else
		{
			// Second pass
			CCompileContext Context2;
			Context2.m_bFastSearch = _bFastSearch;

			Context.CalcChecksum<1>(Node);

			uint32 NodeData2[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
			NodeData2[0] = 0xFFffFFff;
			CRegistryCompiledInternal::CRegistry_Const &Node2 = *((CRegistry_Const *)NodeData2);
			Node2.Private_Set_HasChildren(true);
			Node2.Private_Set_ChildrenHasChildren(true);
			Compile_r<1>(_pReg, Context2, Node2, &Context, &Node);

			m_RootNodeChildren = Node2.Private_Get_NumChildren();
			M_ASSERT(Node2.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData2[0] == 0, "Must be so");
			SetFromCompileContext(Context2);
		}
	}

	void CompileBE(CRegistry* _pReg, bint _bFastSearch)
	{
		m_CompiledData.Destroy();
		CCompileContext Context;
		Context.m_bFastSearch = _bFastSearch;
		uint32 NodeData[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
		NodeData[0] = 0xFFffFFff;
		CRegistryCompiledInternal::CRegistry_Const &Node = *((CRegistry_Const *)NodeData);
		Node.Private_Set_HasChildren(true);
		Node.Private_Set_ChildrenHasChildren(true);
		Compile_r<0>(_pReg, Context, Node, NULL, NULL);
		if (_bFastSearch)
		{
			m_RootNodeChildren = Node.Private_Get_NumChildren();
			M_ASSERT(Node.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData[0] == 0, "Must be so");
			SetFromCompileContext(Context);
		}
		else
		{
			// Second pass
			CCompileContext Context2;
			Context2.m_bFastSearch = _bFastSearch;

			Context.CalcChecksum<0>(Node);

			uint32 NodeData2[((sizeof(CRegistry_Const)+sizeof(uint32)-1) / sizeof(uint32)) + 1];
			NodeData2[0] = 0xFFffFFff;
			CRegistryCompiledInternal::CRegistry_Const &Node2 = *((CRegistry_Const *)NodeData2);
			Node2.Private_Set_HasChildren(true);
			Node2.Private_Set_ChildrenHasChildren(true);
			Compile_r<0>(_pReg, Context2, Node2, &Context, &Node);

			m_RootNodeChildren = Node2.Private_Get_NumChildren();
			M_ASSERT(Node2.Private_Get_ChildNodeStart() == 0, "Must be so");
			M_ASSERT(NodeData2[0] == 0, "Must be so");
			SetFromCompileContext(Context2);
		}
	}
};


CRegistryCompiled::CRegistryCompiled()
{
	m_pInternal = DNew(CRegistryCompiledInternal) CRegistryCompiledInternal();
}

CRegistryCompiled::~CRegistryCompiled()
{
	if (m_pInternal)
		delete m_pInternal;
}

#define REGISTRY_COMPILED_VERSION 0x0202

#ifndef PLATFORM_CONSOLE
void CRegistryCompiled::Write(CDataFile* _pDFile)
{
	MAUTOSTRIP(CRegistryCompiled_Write, MAUTOSTRIP_VOID);
	CCFile* pFile = _pDFile->GetFile();

#ifdef CPU_LITTLEENDIAN
	// Write native format
	{
		_pDFile->BeginEntry("XCR_LE");
		m_pInternal->Write(pFile);
		_pDFile->EndEntry(REGISTRY_COMPILED_VERSION);
	}
	{
		CRegistryCompiledInternal Temp;
		Temp.CompileBE(GetRoot(), false);
		_pDFile->BeginEntry("XCR_BE");
		Temp.Write(pFile);
		_pDFile->EndEntry(REGISTRY_COMPILED_VERSION);
	}
#else
	// Write native format
	{
		_pDFile->BeginEntry("XCR_BE");
		m_pInternal->Write(pFile);
		_pDFile->EndEntry(REGISTRY_COMPILED_VERSION);
	}
	{
		CRegistryCompiledInternal Temp;
		Temp.CompileLE(GetRoot(), false);
		_pDFile->BeginEntry("XCR_LE");
		Temp.Write(pFile);
		_pDFile->EndEntry(REGISTRY_COMPILED_VERSION);
	}
#endif
}

void CRegistryCompiled::Write_XCR(const char* _pFileName)
{
	MAUTOSTRIP(CRegistryCompiled_Write_XCR, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Create(_pFileName);
	Write(&DFile);
	DFile.Close();
}
#endif


void CRegistryCompiled::Compile(CRegistry* _pReg, bint _FastSearch)
{
	m_pInternal->Compile(_pReg, _FastSearch);
}

spCRegistry CRegistryCompiled::GetRoot()
{
	spCRegistry spRoot = REGISTRY_CREATE;

	if (m_pInternal->m_RootNodeChildren)
	{
		spRoot->SetNumChildren(m_pInternal->m_RootNodeChildren);

		int iNodeStart = (m_pInternal->m_RootNodeChildren + 1) >> 1;
		CRegistryCompiledInternal::CRegistry_Const *pRootNode = m_pInternal->GetFromNode(0, iNodeStart);
		int Stride = (sizeof(CRegistryCompiledInternal::CRegistry_Const) + (pRootNode->Private_Get_HasChildren() ? sizeof(uint32) : 0)) / sizeof(uint32);
		for (int i = 0; i < m_pInternal->m_RootNodeChildren; ++i)
		{
			CRegistryCompiledInternal::CRegistry_Const *pNode = m_pInternal->GetFromNode(0, iNodeStart + i * Stride);
			spRoot->SetChild(i, pNode);
		}
	}

	return spRoot;
}

void CRegistryCompiled::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CRegistryCompiled_Write, MAUTOSTRIP_VOID);
	CCFile* pFile = _pDFile->GetFile();

#ifdef PLATFORM_WIN_PC
	int Platform = D_MPLATFORM;
	int bXDF = D_MXDFCREATE;

	if (bXDF && Platform == 4)	// Add other platforms here
	{
		// Read Big endian
		_pDFile->PushPosition();
		if (!_pDFile->GetNext("XCR_BE"))
			Error("Read", "XCR_BE not found.");
		int Version = _pDFile->GetUserData();
		if (Version != 0x0202)
			M_BREAKPOINT;
		m_pInternal->Read(pFile, Version);
		delete m_pInternal;
		m_pInternal = DNew(CRegistryCompiledInternal) CRegistryCompiledInternal();

		{
			_pDFile->PopPosition();
			// Read correct endian
			D_NOXDF;
		#ifdef CPU_LITTLEENDIAN
			// Write native format
			if (!_pDFile->GetNext("XCR_LE"))
				Error("Read", "XCR_LE not found.");
			int Version = _pDFile->GetUserData();
			if (Version != 0x0202)
				M_BREAKPOINT;
			m_pInternal->Read(pFile, Version);
		#else
			// Write native format
			if (!_pDFile->GetNext("XCR_BE"))
				Error("Read", "XCR_BE not found.");
			int Version = _pDFile->GetUserData();
			if (Version != 0x0202)
				M_BREAKPOINT;
			m_pInternal->Read(pFile, Version);
		#endif
		}
	}
	else
#endif
	{
	#ifdef CPU_LITTLEENDIAN
		// Write native format
		if (!_pDFile->GetNext("XCR_LE"))
			Error("Read", "XCR_LE not found.");
		int Version = _pDFile->GetUserData();
		if (Version != 0x0202)
			M_BREAKPOINT;
		m_pInternal->Read(pFile, Version);
	#else
		// Write native format
		if (!_pDFile->GetNext("XCR_BE"))
			Error("Read", "XCR_BE not found.");
		int Version = _pDFile->GetUserData();
		if (Version != 0x0202)
			M_BREAKPOINT;
		m_pInternal->Read(pFile, Version);
	#endif
	}

}



void CRegistryCompiled::Read_XCR(const char* _pFileName)
{
	MSCOPE(CRegistryCompiled::Read_XCR, RES_REGISTRY);
	MAUTOSTRIP(CRegistryCompiled_Read_XCR, MAUTOSTRIP_VOID);
	CDataFile DFile;
	DFile.Open(_pFileName);
	Read(&DFile);
	DFile.Close();

	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv && pEnv->GetValuei("REGISTRYDEBUG"))
	{
		CStr FileName;
		FileName.Capture(_pFileName);
		GetRoot()->XRG_Write(FileName + ".read.txt");
	}
}


#endif

CRegistryCompiledInternal::FRegistryCompiledTypeConvert *CRegistryCompiledInternal::ms_lTypeConvert[REGISTRY_TYPE_MAX][REGISTRY_TYPE_MAX] = 
{
	// Void
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<CStr>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<uint32>
	}
	,
	// Str
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToStr,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToData,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToInt<uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToInt<int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToInt<int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToFloat<fp4, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToFloat<fp2, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertStrToInt<uint32>
	}
	,
	// Data
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDataToStr,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCopyListVector,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertClear<uint32>
	}
	,
	// uint8
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertIntToStr<uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint8, int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint8, int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint8, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint8, fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint8, uint32>
	}
	,
	// int16
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertIntToStr<int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int16, uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int16, int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int16, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int16, fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int16, uint32>
	}
	,
	// int32
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertIntToStr<int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int32, uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int32, int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int32, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int32, fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<int32, uint32>
	}
	,
	// fp4
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertFloatToStr<fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp4, uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp4, int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp4, int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp4, fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp4, uint32>,
	}
	,
	// fp2
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertFloatToStr<fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp2, uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp2, int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp2, int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp2, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<fp2, int32>,
	}
	,
	// uint32
	{
		NULL,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertIntToStrHex<uint32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertDefault<TList_Vector<uint8> >,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint32, uint8>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint32, int16>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint32, int32>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint32, fp4>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertCompatible<uint32, fp2>,
		CRegistryCompiledInternal::CRegistry_RegistryCompiled_Helpers::RegistryConvertSame<uint32>,
	}
};
