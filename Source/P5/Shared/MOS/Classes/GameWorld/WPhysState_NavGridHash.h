#ifndef _INC_WPHYSSTATE_NAVGRIDHASH
#define _INC_WPHYSSTATE_NAVGRIDHASH

#include "WPhysState_Hash.h"
#include "WMapData.h"

class CSpaceEnumSpecialization_NavGrid
{
public:
	static CMapData *ms_pWorldData;

	class CHashSpecialization
	{
	public:
		class CElementMixin
		{
		public:
			CMat4Dfp32 m_Position;
			CBox3Dfp32 m_Bounding;
			inline void Assign(CWObject_CoreData *_pObj)
			{
				m_Position = _pObj->GetPositionMatrix();

				const CWO_PhysicsState& PhysState = _pObj->GetPhysState();
				//m_Bounding.m_Min = CVec3Dfp32(0,0,0);
				//m_Bounding.m_Max = CVec3Dfp32(0,0,0);

				//_pObj->Get
				//M_ASSERT(PhysState.m_nPrim, "This should never happen");
				//_pObj->GetVisBoundBox(&m_Bounding);

				// fetch the physics model
				int iPhysModel = PhysState.m_Prim[0].m_iPhysModel;
				CXR_Model *pModel = ms_pWorldData->GetResource_Model(iPhysModel); // oh crap
				M_ASSERT(pModel, "This should never happen");
				CXR_PhysicsModel *pPhysModel = pModel->Phys_GetInterface();
				M_ASSERT(pPhysModel, "This should never happen");
				pPhysModel->GetBound_Box(m_Bounding, PhysState.m_Prim[0].m_PhysModelMask);
			}

			inline void Release() { }
			inline const CBox3Dfp32 *GetBox() const { static CBox3Dfp32 box, temp; temp = m_Bounding; temp.Transform(m_Position, box); return &box; }

			inline const CElementMixin &GetReturnValue(int ID) const { return *this; }

			inline bool BoxEnumCheck(const CWO_EnumParams_Box& _Params, const CWO_Hash_Settings &_Settings) const { return true; }
		};

		// types
		typedef CElementMixin ElementMixinType;
		typedef CElementMixin EnumerateReturnType;

		class CStorage
		{
		public:
			// expose the arrays so we can send them off to the VPU
			TThinArrayAlign<CWO_HashLink,16> m_lLinks;
			TThinArrayAlign<TWO_HashElement<ElementMixinType>,16> m_lElements;
			TThinArrayAlign<int16,16> m_lHash;

			inline void Init(int _nBoxes, int _MaxIDs)
			{
				M_TRACEALWAYS("[NAVHASH] Init(%d,%d)\r\n", _nBoxes, _MaxIDs);
				m_lLinks.SetLen(_MaxIDs*4);
				m_lElements.SetLen(_MaxIDs);
				m_lHash.SetLen(_nBoxes*_nBoxes+2);

				FillChar(m_lLinks.GetBasePtr(), m_lLinks.ListSize(), 0);
				FillChar(m_lElements.GetBasePtr(), m_lElements.ListSize(), 0);
				FillChar(m_lHash.GetBasePtr(), m_lHash.ListSize(), 0);
			}

			inline CWO_HashLink &GetLink(int index) { return m_lLinks[index]; }
			inline TWO_HashElement<ElementMixinType> &GetElement(int index) { return m_lElements[index]; }
			inline int16 &GetHash(int index) { return m_lHash[index]; }

			// these are not needed
			inline void TagElement(int _ID) { }
			inline bool IsElementTagged(int _ID) { return false; }
			inline void FinalizeEnum(EnumerateReturnType *_pRetValues, int _NumValues) {}
		};

		typedef CStorage StorageType;
	};

	class TWO_Hash_RW_Nude : public TWO_Hash_RW<CHashSpecialization>
	{
	public:
		// expose internal data
		int NumLinks() const { return m_Data.m_lLinks.Len(); }
		int NumElements() const { return m_Data.m_lElements.Len(); }
		int NumHash() const { return m_Data.m_lHash.Len(); }

		const CWO_HashLink *GetLinksPtr() const { return m_Data.m_lLinks.GetBasePtr(); }
		const TWO_HashElement<CSpaceEnumSpecialization_NavGrid::CHashSpecialization::ElementMixinType> *GetElementsPtr() const { return m_Data.m_lElements.GetBasePtr(); }
		const int16 *GetHashPtr() const { return m_Data.m_lHash.GetBasePtr(); }
	};

	typedef CHashSpecialization::ElementMixinType EnumerateReturnType;
	typedef TWO_Hash_RW_Nude HashType1;
	typedef TWO_Hash_RW_Nude HashType2;
};

// A new class just to expose the internal data storage so we can feed it to the VPUs
class CWO_SpaceEnum_RW_NavGrid : public TWO_SpaceEnum_RW<CSpaceEnumSpecialization_NavGrid>
{
public:
	CSpaceEnumSpecialization_NavGrid::TWO_Hash_RW_Nude &GetHash(int index)
	{
		if(index)
			return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_NavGrid>::m_Hash2;
		return TWO_SpaceEnum_RO<CSpaceEnumSpecialization_NavGrid>::m_Hash1;
	}
};

//typedef TWO_SpaceEnum_RW<CSpaceEnumSpecialization_NavGrid> CWO_SpaceEnum_RW_NavGrid;
//extern CWO_SpaceEnum_RW_NavGrid m_NavGridSpace;


#endif
