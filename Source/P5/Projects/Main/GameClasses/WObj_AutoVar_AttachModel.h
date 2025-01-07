#ifndef __WOBJ_AUTOVAR_ATTACHMODEL_H
#define __WOBJ_AUTOVAR_ATTACHMODEL_H

#include "MRTC.h"
#include "MMath.h"

#define ATTACHMODEL_NUMMODELS 4
//#define ATTACHMODEL_NUMEXTRAMODELS 1

//AR-CHANGE: Added forward declarations..
class CMapData;
class CWorld_Client;
class CWObject_Client;
class CXR_Anim_SequenceData;
class CXR_AnimState;
class CXR_Engine;
class CXR_Model;
class CXR_Skeleton;
class CXR_SkeletonInstance;
class CXR_ModelInstance;

#define MACRO_ADJUSTEDANGLE(p) (p < 0.0f ? p + 1.0f + M_Floor(-p) : (p > 1.0f ? p - M_Floor(p) : p))
#ifndef __MACROINLINEACCESS
#define __MACROINLINEACCESS
#define MACRO_INLINEACCESS_RW(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; } \
	M_INLINE type& Get##name() { return m_##name; }

#define MACRO_INLINEACCESS_R(name, type) \
	M_INLINE const type& Get##name() const { return m_##name; }

#define MACRO_INLINEACCESS_RWEXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; } \
	M_INLINE type& Get##name() { return variable; }

#define MACRO_INLINEACCESS_REXT(name, variable, type) \
	M_INLINE const type& Get##name() const { return variable; }
#endif

#define ATTACHMODEL_SKELANIM_BLENDTIME (0.1f)

class CAttach_ExtraModels
{
public:
	struct sItemEffect
	{
		uint16 m_iModel;
		int16 m_iAttachPoint;
	};
	// Extra effect, nummodels client only... (...)
	TThinArray<sItemEffect> m_lEffects;
	TThinArray<CMTime> m_lExtraModelStartTime;
	TThinArray<fp32> m_lExtraModelDuration;
	TThinArray< TPtr<CXR_ModelInstance> > m_lspExtraModelInstances;
	TThinArray<uint16> m_liExtraModel;
	TThinArray<int16> m_lExtraModelSeed;
	TThinArray<uint8> m_liExtraModelAttachPoint;

	~CAttach_ExtraModels()
	{
		Clear();
	}

	void Clear()
	{
		m_lEffects.Clear();
		m_lExtraModelStartTime.Clear();
		m_lExtraModelSeed.Clear();
		m_lExtraModelDuration.Clear();
		m_liExtraModel.Clear();
		m_liExtraModelAttachPoint.Clear();
		m_lspExtraModelInstances.Clear();
	}

	void SetLen(int _Len)
	{
		if(_Len <= 0)
		{
			Clear();
			return;
		}

		if(m_lEffects.Len() != _Len)
		{
			m_lEffects.SetLen(_Len);
			FillChar(&m_lEffects[0], 0, sizeof(CAttach_ExtraModels::sItemEffect) * _Len);
		}

		if(m_lExtraModelStartTime.Len() != _Len)
		{
			m_lExtraModelStartTime.SetLen(_Len);
			FillChar(&m_lExtraModelStartTime[0], 0, sizeof(CMTime) * _Len);
		}

		if (m_lExtraModelSeed.Len() != _Len)
		{
			m_lExtraModelSeed.SetLen(_Len);
			FillChar(&m_lExtraModelSeed[0], 0, sizeof(int16) * _Len);
		}

		if(m_lExtraModelDuration.Len() != _Len)
		{
			m_lExtraModelDuration.SetLen(_Len);
			FillChar(&m_lExtraModelDuration[0], 0, sizeof(fp32) * _Len);
		}

		if(m_liExtraModel.Len() != _Len)
		{
			m_liExtraModel.SetLen(_Len);
			FillChar(&m_liExtraModel[0], 0, sizeof(uint16) * _Len);
		}

		if(m_liExtraModelAttachPoint.Len() != _Len)
		{
			m_liExtraModelAttachPoint.SetLen(_Len);
			FillChar(&m_liExtraModelAttachPoint[0], 0, sizeof(uint8) * _Len);
		}

		if(m_lspExtraModelInstances.Len() != _Len)
		{
			m_lspExtraModelInstances.SetLen(_Len);
			for(int i = 0; i < _Len; i++)
				m_lspExtraModelInstances[i] = NULL;
		}
	}

	int GetNumExtraModels() const
	{
		return m_lEffects.Len();
	}

	CMTime GetTime(const CMTime& _Time, int32 _iSlot)
	{
		if (_Time.Compare(m_lExtraModelStartTime[_iSlot]) < 0)
			return CMTime::CreateFromSeconds(0.0f);

		return _Time - m_lExtraModelStartTime[_iSlot];
	}

	void CopyFrom(const CAttach_ExtraModels& _From)
	{
		int32 Len = _From.GetNumExtraModels();
		SetLen(Len);
		const sItemEffect* pFrom = _From.m_lEffects.GetBasePtr();
		sItemEffect* pEffects = m_lEffects.GetBasePtr();

		for (int32 i = 0; i < Len; i++)
		{
			pEffects[i] = pFrom[i];

			m_lExtraModelStartTime[i] = _From.m_lExtraModelStartTime[i];
			m_lExtraModelSeed[i] = _From.m_lExtraModelSeed[i];
			m_lExtraModelDuration[i] = _From.m_lExtraModelDuration[i];
			m_liExtraModel[i] = _From.m_liExtraModel[i];
			m_liExtraModelAttachPoint[i] = _From.m_liExtraModelAttachPoint[i];

			// Hmm, how safe is this (copied from attachmodel...)
			if (_From.m_lspExtraModelInstances[i])
			{
				if (!m_lspExtraModelInstances[i] || strcmp(m_lspExtraModelInstances[i]->MRTC_ClassName(), _From.m_lspExtraModelInstances[i]->MRTC_ClassName()))
					m_lspExtraModelInstances[i] = _From.m_lspExtraModelInstances[i]->Duplicate();
				else
					*m_lspExtraModelInstances[i] = *_From.m_lspExtraModelInstances[i];
			}
			else 
				m_lspExtraModelInstances[i] = NULL;

			//TPtr<CXR_ModelInstance> m_lspExtraModelInstances[ATTACHMODEL_NUMEXTRAMODELS];
		}
	}
	void Pack(uint8*& _pD, uint8& _Mask, int& _CurBit) const
	{
		uint8 EffectsLen = (uint8)GetNumExtraModels();
		if (EffectsLen)
		{
			_Mask |= _CurBit;
			PTR_PUTUINT8(_pD,EffectsLen);
			const sItemEffect* pEffects = m_lEffects.GetBasePtr();
			for (int8 i = 0; i < EffectsLen; i++)
			{
				PTR_PUTUINT16(_pD,pEffects[i].m_iModel);
				PTR_PUTINT16(_pD,pEffects[i].m_iAttachPoint);
			}
		}
		_CurBit <<= 1;
		/*for (int32 i = 0; i < ATTACHMODEL_NUMEXTRAMODELS; i++)
		{
			if (m_iExtraModel[i] != 0)
			{
				_Mask |= _CurBit;
				PTR_PUTCMTIME(_pD,m_ExtraModelStartTime[i]);
				PTR_PUTFP32(_pD,m_ExtraModelDuration[i]);
				PTR_PUTUINT16(_pD,m_iExtraModel[i]);
				PTR_PUTUINT8(_pD,m_iExtraModelAttachPoint[i]);
				// Hrm, model instances, should be created on clients only...
			}
			_CurBit <<= 1;
			//TPtr<CXR_ModelInstance> m_lspExtraModelInstances[ATTACHMODEL_NUMEXTRAMODELS];
		}*/
	}

	void UnPack(const uint8*& _pD, CMapData* _pMapData, const int8 _Mask, int& _CurBit)
	{
		if (_Mask & _CurBit)
		{
			uint8 EffectsLen;
			PTR_GETUINT8(_pD,EffectsLen);

			SetLen(EffectsLen);
			sItemEffect* pEffects = m_lEffects.GetBasePtr();
			for (int8 i = 0; i < EffectsLen; i++)
			{
				PTR_GETUINT16(_pD,pEffects[i].m_iModel);
				PTR_GETINT16(_pD,pEffects[i].m_iAttachPoint);

//				m_lExtraModelStartTime[i] = CMTime::CreateFromSeconds(0.0f);
//				m_lExtraModelDuration[i] = 0;
//				m_liExtraModel[i] = 0;
//				m_liExtraModelAttachPoint[i] = 0;
//				m_lspExtraModelInstances[i] = NULL;
			}
		}
		else
		{
			Clear();
		}
		_CurBit <<= 1;
		/*for (int32 i = 0; i < ATTACHMODEL_NUMEXTRAMODELS; i++)
		{
			if (_Mask & _CurBit)
			{
				uint16 iOldModel = m_iExtraModel[i];
				PTR_GETCMTIME(_pD,m_ExtraModelStartTime[i]);
				PTR_GETFP32(_pD,m_ExtraModelDuration[i]);
				PTR_GETUINT16(_pD,m_iExtraModel[i]);
				PTR_GETUINT8(_pD,m_iExtraModelAttachPoint[i]);
				//ConOut(CStrF("Unpacking Modelinstance: %d:%f",m_iExtraModel[i],m_ExtraModelStartTime[i].GetTime()));
				// Hrm, model instances, should be created on clients only...
				// Create instance directly, or when rendering...?
				if (iOldModel != m_iExtraModel[i])
				{
					CXR_Model *pModel = _pMapData->GetResource_Model(m_iExtraModel[i]);
					if (pModel)
					{
						m_lspExtraModelInstances[i] = pModel->CreateModelInstance();
						if (m_lspExtraModelInstances[i] != NULL)
							m_lspExtraModelInstances[i]->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
					}
					else
						m_lspExtraModelInstances[i] = NULL;
				}
			}
			_CurBit <<= 1;
			//TPtr<CXR_ModelInstance> m_lspExtraModelInstances[ATTACHMODEL_NUMEXTRAMODELS];
		}*/
	}


	void SetExtraModel(CMapData* _pMapData, CMTime _EffectTime, fp32 _Duration, uint _iEffectSlot, int16 _Seed = 0)
	{
		if (_iEffectSlot < GetNumExtraModels())
		{
			// Don't care about overwriting
			int iOldModel = m_liExtraModel[_iEffectSlot];
			m_lExtraModelStartTime[_iEffectSlot] = _EffectTime;
			m_lExtraModelDuration[_iEffectSlot] = _Duration;
			int iNewModel = m_lEffects[_iEffectSlot].m_iModel;
			m_liExtraModel[_iEffectSlot] = iNewModel;
			m_liExtraModelAttachPoint[_iEffectSlot] = m_lEffects[_iEffectSlot].m_iAttachPoint;
			m_lExtraModelSeed[_iEffectSlot] = _Seed;
			// Create model instance if not created yet
			if (iOldModel != iNewModel)
			{
				CXR_Model *pModel = _pMapData->GetResource_Model(iNewModel);
				if (pModel)
				{
					m_lspExtraModelInstances[_iEffectSlot] = pModel->CreateModelInstance();
					if (m_lspExtraModelInstances[_iEffectSlot] != NULL)
						m_lspExtraModelInstances[_iEffectSlot]->Create(pModel, CXR_ModelInstanceContext(NULL, NULL));
				}
				else
					m_lspExtraModelInstances[_iEffectSlot] = NULL;
			}
		}
	}

	void RenderExtraModels(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_Model* _pBaseModel,
		CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject);

	void ResetSlot(int32 _iSlot)
	{
		M_ASSERT(_iSlot >= 0 && _iSlot < GetNumExtraModels(),"ResetSlot: iSlot out of range");
		m_lExtraModelStartTime[_iSlot].Reset();
		m_lExtraModelDuration[_iSlot] = 0.0f;
		m_liExtraModel[_iSlot] = 0;
		m_liExtraModelAttachPoint[_iSlot] = 0;
		m_lspExtraModelInstances[_iSlot] = NULL;
	}

	void Refresh(CMTime _GameTime)
	{
		// Check if any effects has expired
		int Len = GetNumExtraModels();
		for (int32 i = 0; i < Len; i++)
		{
			if ((_GameTime - m_lExtraModelStartTime[i]).GetTime() > m_lExtraModelDuration[i])
				ResetSlot(i);
		}
	}
};

/////////////////////////////////////////////////////////////////
// AutoVar_AttachModel
class CAutoVar_AttachModel
{
public:
	enum
	{
		FLAGS_SERVERANIMTIME = 1,
		ATTACHMODEL_MAXANIMLAYERS = 4,
	};

	// Current weapon animgraph instance 
	class CWAG2I* m_pAG2I;

	// Animation control
	CMTime m_AC_Cur;
	CMTime m_AC_Target;
	CMTime m_AC_Timestamp;
	fp32 m_AC_Speed;

	uint16 m_AC_RandSeed;
	uint16 m_SurfaceOcclusionMask;											// Occlusion masks
	uint16 m_SurfaceShadowOcclusionMask;

	uint16 m_iModel[ATTACHMODEL_NUMMODELS];									// Replicated
	uint8 m_iAttachPoint[ATTACHMODEL_NUMMODELS];							// Replicated
	int8 m_ModelFlags[ATTACHMODEL_NUMMODELS];

	uint8 m_Flags;															// Replicated
	uint8 m_iAttachRotTrack;												// Replicated
	// Which token this weapon runs on
	uint8 m_iWeaponToken;													// Replicated

	// Client only information
	TPtr<CXR_ModelInstance> m_lspModelInstances[ATTACHMODEL_NUMMODELS];

	// Test...
	CAttach_ExtraModels m_ExtraModels;
	// Sets an extramodel ^^
	//void SetExtraModel(uint16 _iExtraModel, uint8 _iAttach, int32 _iSlot);

//	fp32 m_AC_LastKeyframe;
//	int m_AC_LastUpdate;
//	uint32 m_AC_iSequence;

	TPtr<CXR_SkeletonInstance> m_spSkeletonInstance;
	
	CAutoVar_AttachModel();
	void Clear();
	
	bool IsValid();

	void UpdateModel(const CAutoVar_AttachModel &_Model);

	void SetBaseModel(int _iModel, int _iAttachPoint, int _iAttachRotTrack);
	void SetModel(int _Index, int _iModel, int _iAttachPoint);
	void SetFlags(int _Index, int _Flags);


	void AC_SetTarget(const CMTime& _Target, fp32 _Speed, const CMTime& _Timestamp);
	CMTime AC_Get(const CMTime& _Time);

	
	// New anim system
	void SetAG2I(CWAG2I* pAnimGraph);

	// Test function for skeleton animating a model
	void CreateClientSkelInstance();
	bool GetAnimLayers(CXR_Skeleton* _pSkel, CXR_AnimState* _pAnimState, CMat4Dfp32* _pMatrix, CWorld_PhysState* _pWPhys, const CMTime& _Time);
	//bool DoSkelAnim(CXR_AnimState* _pAnimState, CMat4Dfp32* _pMatrix, CWorld_PhysState* _pWPhys, const CMTime& _Time);

	virtual bool GetModel0_RenderMatrix(CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, CXR_Model *_pModel,
										int _iRotTrack, int _iAttachPoint, CMat4Dfp32 &_Mat);

	virtual bool GetModel0_RenderInfo(CMapData *_pMapData, CXR_Engine* _pEngine, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, const CMTime& _Time,
									  CXR_AnimState &_RetAnim, CMat4Dfp32 &_RetPos, CXR_Model *&_pRetModel, CWorld_PhysState* _pWPhys);

	virtual void RenderExtraModels(CMapData *_pMapData, CXR_Engine* _pEngine,
								   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel,
								   const CMat4Dfp32 &_Mat, const CMTime& _Time, uint16 _iObject);

	virtual void RenderAll(CMapData *_pMapData, CXR_Engine* _pEngine,
						   CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel,
						   const CMat4Dfp32 &_Mat, const CMTime& _Time, CWorld_PhysState* _pWPhys, uint16 _iObject);
	
	virtual void RefreshModelInstances(CWObject_Client *_pObj, CWorld_Client *_pWClient, int _GameTick);
	virtual void Refresh(CWObject_Client *_pObj, CWorld_Client *_pWClient, int _GameTick);
	
//	virtual void EvalAnimKey(int _iController, const CXR_Anim_DataKey* _pKey, int _Tick);

	void CopyFrom(const CAutoVar_AttachModel& _From);
	void Pack(uint8 *&_pD, CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, CMapData* _pMapData);
};

#endif
