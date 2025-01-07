#ifndef WAG2_ClientData_h
#define WAG2_ClientData_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph2/AnimGraph2.h"
#include "WAG2I.h"
//#include "../../../XR/Classes/GameWorld/WAnimGraphInstance/WAG2I_Context.h"

//--------------------------------------------------------------------------------

//#define AG2_RECORDPROPERTYCHANGES
#ifdef AG2_RECORDPROPERTYCHANGES
class CXRAG2_PropertyRecorder
{
public:
	enum
	{
		AG2_PROPERTYRECORDER_NONE = 0,
		AG2_PROPERTYRECORDER_RECORDING,
		AG2_PROPERTYRECORDER_PLAYBACK,
	};
	class CPropertyChanges
	{
	public:
		class CPropertyChange
		{
		public:
			fp32 m_Time; // Relative time from 
			union
			{
				fp32 m_FloatVal;
				int m_IntVal;
				bool m_BoolVal;
			};
			CPropertyChange(){}
			CPropertyChange(fp32 _Time, fp32 _FloatVal)
			{
				m_Time = _Time;
				m_FloatVal = _FloatVal;
			}
			CPropertyChange(fp32 _Time, int _IntVal)
			{
				m_Time = _Time;
				m_IntVal = _IntVal;
			}
			CPropertyChange(fp32 _Time, bool _BoolVal)
			{
				m_Time = _Time;
				m_BoolVal = _BoolVal;
			}
		};
		TArray<CPropertyChange> m_lChanges;

		void GetValue(fp32 _Time, int& _Value);
		void GetValue(fp32 _Time, fp32& _Value);
		void GetValue(fp32 _Time, bool& _Value);
		void GetValueLerp(fp32 _Time, fp32& _Value);

		void AddChange(fp32 _Time, fp32 _FloatVal)
		{
			m_lChanges.Add(CPropertyChange(_Time, _FloatVal));
		}

		void AddChange(fp32 _Time, int _IntVal)
		{
			m_lChanges.Add(CPropertyChange(_Time, _IntVal));
		}

		void AddChange(fp32 _Time, bool _BoolVal)
		{
			m_lChanges.Add(CPropertyChange(_Time, _BoolVal));
		}
		M_INLINE int GetLatestInt() { return m_lChanges[m_lChanges.Len() - 1].m_IntVal; }
		M_INLINE float GetLatestFloat() { return m_lChanges[m_lChanges.Len() - 1].m_FloatVal; }
		M_INLINE bool GetLatestBool() { return m_lChanges[m_lChanges.Len() - 1].m_BoolVal; }

		void WriteInt(CCFile& _File);
		void WriteFloat(CCFile& _File);
		void WriteBool(CCFile& _File);
		void ReadInt(CCFile& _File);
		void ReadFloat(CCFile& _File);
		void ReadBool(CCFile& _File);
	};
	class CImpulseChanges
	{
	public:
		class CImpulseChange
		{
		public:
			fp32 m_Time;
			int16 m_ImpulseType;
			int16 m_ImpulseValue;
			int8 m_iToken;
			CImpulseChange(){}
			CImpulseChange(fp32 _Time, CXRAG2_Impulse _Impulse, int8 _iToken)
			{
				m_Time = _Time;
				m_ImpulseType = _Impulse.m_ImpulseType;
				m_ImpulseValue = _Impulse.m_ImpulseValue;
				m_iToken = _iToken;
			}
		};
		TArray<CImpulseChange> m_lChanges;
		void AddChange(fp32 _Time, CXRAG2_Impulse _Impulse, int8 _iToken);
		bool IsChange(CXRAG2_Impulse& _Impulse, int8& _iToken, fp32 CurTime, fp32 _LastTime, int32& _iImpulse);
		void Write(CCFile& _File);
		void Read(CCFile& _File);
	};
	class CPositionChanges
	{
	public:
		// First change is position when recording started
		CMat4Dfp32 m_InvStartPosition;
		class CPositionChange
		{
		public:
			CMat4Dfp32 m_Position;
			fp32 m_Time;
			CPositionChange(){}
			CPositionChange(fp32 _Time, const CMat4Dfp32& _Val)
			{
				m_Time = _Time;
				m_Position = _Val;
			}
		};
		TArray<CPositionChange> m_lChanges;
		void AddChange(fp32 _Time, const CMat4Dfp32& _Position);
		M_INLINE const CMat4Dfp32& GetLatest() { return m_lChanges[m_lChanges.Len() - 1].m_Position; }
		void Write(CCFile& _File);
		void Read(CCFile& _File);
		CMat4Dfp32 GetPosition(fp32 _Time, bool _bFromOrg = true);
	};
	class CTokenInfo
	{
	public:
		fp32 m_StateTime;
		int16 m_iGraphBlockMoveToken;
		int16 m_iEnterMoveToken;
		int8 m_iToken;
	};
	// Int changes, first one is the base
	TArray<CTokenInfo> m_lTokens;
	TArray<CPropertyChanges> m_lIntChanges;
	TArray<CPropertyChanges> m_lFloatChanges;
	TArray<CPropertyChanges> m_lBoolChanges;
	TArray<CPropertyChanges> m_lFunctionChanges;
	CPropertyChanges m_LowerBodyOffsetChanges;
	CImpulseChanges m_ImpulseChanges;
	CPositionChanges m_PositionChanges;
	CMTime m_StartTime;
	CWAG2I* m_pWAG2I;
	int m_State;


	CXRAG2_PropertyRecorder();

	void StartRecording(const CWAG2I_Context* _pContext, CWAG2I* _pWAG2I, fp32 _LowerBodyOffset);
	void Update(const CWAG2I_Context* _pContext, fp32 _LowerBodyOffset);
	void AddImpulse(const CWAG2I_Context* _pContext, CXRAG2_Impulse _Impulse, int8 _Token);
	void StopOperation();
	void Write(CCFile& _File);
	void Read(CCFile& _File);

	void StartUpdating(CWAG2I* _pWAG2I);
	void UpdateToAnimGraph(CWAG2I_Context* _pContext, fp32 _CurTime, fp32 _LastTime);
};
#endif
class CWAG2I_Token;
class CWAG2I_Context;
class CWAG2I_StateInstance;
class CWO_ClientData_AnimGraph2Interface;

class CAG2Val
{
	enum
	{
		AG2VAL_TYPE_FLOAT = 1,
		AG2VAL_TYPE_TIME = 2,
		AG2VAL_TYPE_INT = 4,
		AG2VAL_TYPE_BOOL = 8,
	};
	uint8 m_Data[sizeof(fp32) > sizeof(CMTime) ? sizeof(fp32) : sizeof(CMTime)];
public:
	int m_Type;

	M_INLINE CAG2Val()
	{
		m_Type = AG2VAL_TYPE_FLOAT;
		*((fp32 *)m_Data) = 0.0f;
	}

	M_INLINE int IsTime() const 
	{
		return (m_Type & AG2VAL_TYPE_TIME);
	}

	M_INLINE int IsFloat() const 
	{
		return (m_Type & AG2VAL_TYPE_FLOAT);
	}

	M_INLINE int IsInt() const 
	{
		return (m_Type & AG2VAL_TYPE_INT);
	}

	M_INLINE static CAG2Val From(fp32 _Fp)
	{
		CAG2Val Ret;
		Ret.SetFp(_Fp);
		return Ret;
	}

	M_INLINE static CAG2Val From(int _Val)
	{
		CAG2Val Ret;
		Ret.SetInt(_Val);
		return Ret;
	}

	M_INLINE static CAG2Val From(bool _Val)
	{
		CAG2Val Ret;
		Ret.SetBool(_Val);
		return Ret;
	}

	M_INLINE static CAG2Val From(CMTime &_Time)
	{
		CAG2Val Ret;
		Ret.SetTime(_Time);
		return Ret;
	}

	M_INLINE CAG2Val Scale(fp32 _Scale) const
	{
		CAG2Val Ret;
		if (m_Type & AG2VAL_TYPE_TIME)
			Ret.SetTime(GetTime().Scale(_Scale));
		else
			Ret.SetFp(GetFp() * _Scale);
		return Ret;
	}

	M_INLINE int Compare(const CAG2Val &_Other) const 
	{
		if (IsTime() || _Other.IsTime())
			return CompareTime(_Other);
		else if (IsFloat())
			return CompareFp(_Other);
		else if (IsInt())
			return CompareInt(_Other);
		else
			return CompareBool(_Other);
	}

	M_INLINE int CompareFp(const CAG2Val &_Other) const
	{
		if (GetFp() > _Other.GetFp())
			return 1;
		else if (GetFp() < _Other.GetFp())
			return -1;
		else
			return 0;
	}

	M_INLINE int CompareInt(const CAG2Val &_Other) const
	{
		if (GetInt() > _Other.GetInt())
			return 1;
		else if (GetInt() < _Other.GetInt())
			return -1;
		else
			return 0;
	}

	M_INLINE int CompareBool(const CAG2Val &_Other) const
	{
		if (GetBool() > _Other.GetBool())
			return 1;
		else if (GetBool() < _Other.GetBool())
			return -1;
		else
			return 0;
	}

	M_INLINE int CompareTime(const CAG2Val &_Other) const 
	{
		return GetTime().Compare(_Other.GetTime());
	}

	M_INLINE void SetFp(const fp32 &_Fp)
	{
		m_Type = AG2VAL_TYPE_FLOAT;
		*((fp32 *)m_Data) = _Fp;
	}

	M_INLINE void SetInt(const int &_Val)
	{
		m_Type = AG2VAL_TYPE_INT;
		*((int *)m_Data) = _Val;
	}

	M_INLINE void SetBool(const bool &_Val)
	{
		m_Type = AG2VAL_TYPE_BOOL;
		*((bool *)m_Data) = _Val;
	}

	M_INLINE void SetTime(const CMTime &_Time)
	{
		m_Type = AG2VAL_TYPE_TIME;
		*((CMTime *)m_Data) = _Time;
	}

	M_INLINE const fp32 &GetFp() const
	{
		M_ASSERT(m_Type & AG2VAL_TYPE_FLOAT, "You should not convert a non float value");
		return *((fp32 *)m_Data);
	}

	M_INLINE const int &GetInt() const
	{
		M_ASSERT(m_Type & AG2VAL_TYPE_INT, "You should not convert a non int value");
		return *((int *)m_Data);
	}

	M_INLINE const bool &GetBool() const
	{
		M_ASSERT(m_Type & AG2VAL_TYPE_BOOL, "You should not convert a non bool value");
		return *((bool *)m_Data);
	}

	M_INLINE const CMTime GetTime() const
	{
		switch (m_Type)
		{
		case AG2VAL_TYPE_TIME:
			return *((CMTime *)m_Data);
		case AG2VAL_TYPE_FLOAT:
			return CMTime::CreateFromSeconds(GetFp());
		default:
			return CMTime();
		}
	}
};

typedef bool (CWO_ClientData_AnimGraph2Interface::*PFN_ANIMGRAPH2_CONDITION)(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
typedef CAG2Val (CWO_ClientData_AnimGraph2Interface::*PFN_ANIMGRAPH2_PROPERTY)(const CWAG2I_Context* _pContext);
typedef bool (CWO_ClientData_AnimGraph2Interface::*PFN_ANIMGRAPH2_OPERATOR)(const CWAG2I_Context* _pContext, const CAG2Val&, const CAG2Val&);
typedef void (CWO_ClientData_AnimGraph2Interface::*PFN_ANIMGRAPH2_EFFECT)(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

#define MAX_ANIMGRAPH2_DEFCONDITIONS	10
#define MAX_ANIMGRAPH2_DEFPROPERTIES	20
#define MAX_ANIMGRAPH2_DEFOPERATORS	9
#define MAX_ANIMGRAPH2_DEFEFFECTS	25
#define NUM_DIALOGUE_PROPERTIES 6


enum
{
	PROPERTY_INT_PREVIOUSGRAPHBLOCKTYPE = 0,
	PROPERTY_INT_PREVIOUSGRAPHBLOCKVALUE = 1,
	PROPERTY_INT_DIALOG_BASE = 2,
	PROPERTY_INT_DIALOG_GESTURE = 3,
	PROPERTY_INT_DIALOG_FACIAL = 4,

	AG2_TOKEN_DIALOG_BASE = 6,
	AG2_TOKEN_DIALOG_GESTURE = 7,
	AG2_TOKEN_DIALOG_FACIAL = 8,
	AG2_TOKEN_DIALOG_FACIALGESTURE = 9,
	AG2_TOKEN_FACIAL = 10,

	AG2_IMPULSEVALUE_DIALOGCONTROL_TERMINATE = 0,
	AG2_IMPULSEVALUE_DIALOGCONTROL_CHANGETYPE = 1,
	AG2_IMPULSEVALUE_DIALOG_BASE		= 100,
	AG2_IMPULSEVALUE_DIALOG_GESTURE		= 101,
	AG2_IMPULSEVALUE_DIALOG_FACIAL		= 102,	
	AG2_IMPULSEVALUE_DIALOG_FACIALGESTURE	= 103,	

	AG2_CLD_IMPULSETYPE_DIALOG = 6,
	AG2_CLD_IMPULSETYPE_DIALOGTYPE = 22,

	AG2_PACKEDANIMPROPERTY_TYPESHIFT = 30,
	AG2_PACKEDANIMPROPERTY_TYPE_SETPROPERTY = 0,
	AG2_PACKEDANIMPROPERTY_TYPE_SENDIMPULSE = 1,
	AG2_PACKEDANIMPROPERTY_TYPE_MOVETOKEN = 2,
	AG2_PACKEDANIMPROPERTY_TYPE_DIALOGUE = 3,

	AG2_PACKEDANIMPROPERTY_SETPROPERTY_TYPESHIFT = 16,
	AG2_PACKEDANIMPROPERTY_SETPROPERTY_PROPERTYMASK = 0xffff,

	AG2_PACKEDANIMPROPERTY_TOKENSHIFT = 26,
	AG2_PACKEDANIMPROPERTY_TOKENMASK = 0xf,

	AG2_PACKEDANIMPROPERTY_MOVETOKEN_GRAPHBLOCKSHIFT = 13,
	AG2_PACKEDANIMPROPERTY_MOVETOKEN_GRAPHBLOCKMASK = 0x1fff,
	AG2_PACKEDANIMPROPERTY_MOVETOKEN_ACTIONMASK = 0x1fff,

	AG2_PACKEDANIMPROPERTY_SENDIMPULSE_IMPULSETYPEMASK = 0xffff,
};

class CWO_ClientData_AnimGraph2Interface
{
	protected:
		friend class CXRAG2_PropertyRecorder;
		class CAnimEvent
		{
		public:

			CMTime			m_GameTime;
			uint16			m_Value;
		};

		// Test...
		/*class CCachedProperty
		{
		public:
			// Value from property
			CAG2Val m_Val;
			// Time it was cached
			CMTime	m_GameTime;

			void SetVal(const CMTime& _GameTime, const CAG2Val& _Val)
			{
				m_Val = _Val;
				m_GameTime = _GameTime;
			}

			bool IsCached(const CMTime& _GameTime)
			{
				CMTime Time(_GameTime - m_GameTime);
				return Time.GetTime() < SERVER_TIMEPERFRAME;
			}
		};*/

		PFN_ANIMGRAPH2_CONDITION*	m_lpfnConditions;
		PFN_ANIMGRAPH2_PROPERTY*	m_lpfnProperties;
		PFN_ANIMGRAPH2_OPERATOR*		m_lpfnOperators;
		PFN_ANIMGRAPH2_EFFECT*		m_lpfnEffects;

		static PFN_ANIMGRAPH2_CONDITION	ms_lpfnDefConditions[MAX_ANIMGRAPH2_DEFCONDITIONS];
		static PFN_ANIMGRAPH2_PROPERTY	ms_lpfnDefProperties[MAX_ANIMGRAPH2_DEFPROPERTIES];
		static PFN_ANIMGRAPH2_OPERATOR ms_lpfnDefOperators[MAX_ANIMGRAPH2_DEFOPERATORS];
		static PFN_ANIMGRAPH2_EFFECT ms_lpfnDefEffects[MAX_ANIMGRAPH2_DEFEFFECTS];

		TThinArray<fp32>				m_lPropertiesFloat;
		TThinArray<int>				m_lPropertiesInt;
		TThinArray<bool>			m_lPropertiesBool;
		// Destination in world space :/
		CMat4Dfp32					m_MDestination;
		CMat4Dfp32					m_MStartPosition;
		fp32							m_MDestinationSpeed;

		int							m_nConditions;
		int							m_nProperties;
		int							m_nOperators;
		int							m_nEffects;

		CMTime						m_EnterStateTime;
		//int16						m_iEnterAction;
		int16						m_iEnterMoveToken;
		int16						m_iRandseed;
		fp32							m_AdjustmentOffset;
		fp32							m_AdjustmentCutOff;
		fp32							m_AnimLoopDuration;
		fp32							m_AnimMoveScale;
		fp32							m_EnterAnimTimeOffset;
		// Sync scale from walk to run
		fp32							m_SyncAnimScale;
		fp32							m_AdaptiveTimeScale;

		fp32							m_WallCollision; // Remove this (should be be handled as post anim effect)

		CVec3Dfp32					m_UpVector;		// 0,0,1 for normal characters

		// Important idle ticker
		uint8					m_ImportantAGEvent;
		// Register handled responses here...
		uint8					m_HandledResponse;
		uint8					m_ModeFlags;
		CWAG2I*					m_pAG2I;

public:
	enum
	{

		AG2_MODEFLAG_LOCKSETDEST = M_Bit(0),
	};
	#ifdef AG2_RECORDPROPERTYCHANGES
		CXRAG2_PropertyRecorder		m_PropertyRecorder;
	#endif

		CWO_ClientData_AnimGraph2Interface()
		{
			m_lpfnConditions = NULL;
			m_lpfnProperties = NULL;
			m_lpfnOperators = NULL;
			m_lpfnEffects = NULL;
			m_pAG2I = NULL;
			m_MDestination.Unit();
			m_MStartPosition.Unit();
			m_MDestination.GetRow(0) = _FP32_MAX;
			m_MDestination.GetRow(3) = _FP32_MAX;
			m_MDestinationSpeed = 0.0f;

			m_ImportantAGEvent = 0;
			m_ModeFlags = 0;
			m_UpVector = CVec3Dfp32(0,0,1);
			//AG2_RegisterCallbacks(NULL);
		}

		virtual void SetInitialProperties(const CWAG2I_Context* _pContext) pure;

		virtual void AG2_RegisterCallbacks(CWorld_PhysState* _pWPhysState)
		{
			AG2_RegisterCallbacks2(ms_lpfnDefConditions, MAX_ANIMGRAPH2_DEFCONDITIONS,
								ms_lpfnDefProperties, MAX_ANIMGRAPH2_DEFPROPERTIES,
								ms_lpfnDefOperators, MAX_ANIMGRAPH2_DEFOPERATORS,
								ms_lpfnDefEffects, MAX_ANIMGRAPH2_DEFEFFECTS);
		}

		void AG2_RegisterCallbacks2(PFN_ANIMGRAPH2_CONDITION* _lpfnConditions, int _nConditions,
			PFN_ANIMGRAPH2_PROPERTY* _lpfnProperties, int _nProperties,
			PFN_ANIMGRAPH2_OPERATOR* _lpfnOperators, int _nOperators, 
			PFN_ANIMGRAPH2_EFFECT* _lpfnEffects, int _nEffects)
		{
			m_lpfnConditions = _lpfnConditions;
			m_lpfnProperties = _lpfnProperties;
			m_lpfnOperators = _lpfnOperators;
			m_lpfnEffects = _lpfnEffects;
			m_nConditions = _nConditions;
			m_nProperties = _nProperties;
			m_nOperators = _nOperators;
			m_nEffects = _nEffects;
		}

		virtual void SetAG2I(CWAG2I* _pAG2I) { m_pAG2I = _pAG2I; }
		virtual void AG2_RefreshGlobalProperties(const CWAG2I_Context* _pContext);
		virtual void AG2_RefreshStateInstanceProperties(const CWAG2I_Context* _pContext, const CWAG2I_StateInstance* _pStateInstance);
		virtual void AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2MoveTokenIndex _iEnterMoveToken) {}

		void SetNumProperties(int32 _NumPropertiesFloat, int32 _NumPropertiesInt, int32 _NumPropertiesBool);
		void SetNumPropertiesFloat(int32 _NumPropertiesFloat);
		void SetNumPropertiesInt(int32 _NumPropertiesInt);
		void SetNumPropertiesBool(int32 _NumPropertiesBool);

		const CVec3Dfp32& GetUpVector() const { return m_UpVector; }
		void SetUpVector(const CVec3Dfp32& _Vec) { m_UpVector = _Vec; }

		M_INLINE fp32 GetSyncAnimTimeScale() const { return m_SyncAnimScale; }
		M_INLINE void SetSyncAnimTimeScale(fp32 _Scale) { m_SyncAnimScale = _Scale; }
		M_INLINE fp32 GetAdaptiveTimeScale() const { return m_AdaptiveTimeScale; }
		M_INLINE void SetAdaptiveTimeScale(fp32 _Scale) { m_AdaptiveTimeScale = _Scale; }

		// Impulse monitor, monitors gameplay situation and sends impulses accordingly
		virtual void UpdateImpulseState(const CWAG2I_Context* _pContext) pure;

		bool SetPackedAnimProperty(const CWAG2I_Context *_pContext, uint32 _iProperty, fp32 _Value);

		void PrecacheDialog(const CWAG2I_Context* _pContext, int16 _DialogType, const TArray<CXRAG2_Impulse>& _lTypes);
		void EnumerateDialogTypes(const CWAG2I_Context* _pContext, int16 _DialogType, TArray<int16>& _lTypes, TArray<CStr>* _lpDesc);
		

		M_AGINLINE void SetDestination(const CMat4Dfp32& _MDest,fp32 _Speed = 0.0f) { if (!(m_ModeFlags & AG2_MODEFLAG_LOCKSETDEST)) { m_MDestination = _MDest; m_MDestinationSpeed = _Speed; } }
		M_AGINLINE void SetDestinationLock(const CMat4Dfp32& _MDest,fp32 _Speed = 0.0f) { m_MDestination = _MDest; m_MDestinationSpeed = _Speed; m_ModeFlags |= AG2_MODEFLAG_LOCKSETDEST; }
		M_AGINLINE void ClearDestinationLock() { m_ModeFlags &= ~AG2_MODEFLAG_LOCKSETDEST; }
		M_AGINLINE bool GetDestinationLock() { return((m_ModeFlags & AG2_MODEFLAG_LOCKSETDEST) ? true : false);}
		M_AGINLINE const CMat4Dfp32& GetDestination() const { return m_MDestination;	}
		M_AGINLINE const CMat4Dfp32& GetStartPosition() const { return m_MStartPosition;	}
		M_AGINLINE const fp32& GetDestinationSpeed() const { return m_MDestinationSpeed;	}
		M_AGINLINE const fp32& GetAdjustmentOffset() const { return m_AdjustmentOffset;	}
		M_AGINLINE const fp32& GetAdjustmentCutOff() const { return m_AdjustmentCutOff;	}
		M_AGINLINE const fp32& GetAnimMoveScale() const { return m_AnimMoveScale; }
 
		M_INLINE void SetPropertyFloat(int32 _iProperty, fp32 _Value)
		{
			M_ASSERT(m_lPropertiesFloat.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			m_lPropertiesFloat[_iProperty] = _Value;
		}

		M_INLINE void SetPropertyInt(int32 _iProperty, int32 _Value)
		{
			M_ASSERT(m_lPropertiesInt.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			m_lPropertiesInt[_iProperty] = _Value;
		}

		M_INLINE void SetPropertyBool(int32 _iProperty, bool _Value)
		{
			M_ASSERT(m_lPropertiesBool.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			m_lPropertiesBool[_iProperty] = _Value;
		}

		M_INLINE fp32 GetPropertyFloat(int32 _iProperty)
		{
			M_ASSERT(m_lPropertiesFloat.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			return m_lPropertiesFloat[_iProperty];
		}

		M_INLINE int32 GetPropertyInt(int32 _iProperty)
		{
			M_ASSERT(m_lPropertiesInt.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			return m_lPropertiesInt[_iProperty];
		}

		M_INLINE bool GetPropertyBool(int32 _iProperty)
		{
			M_ASSERT(m_lPropertiesBool.Len() > _iProperty, "PROPERTY INDEX TOO GREAT");
			return m_lPropertiesBool[_iProperty];
		}

		M_INLINE void RegisterImportantAGEvent() { m_ImportantAGEvent = 0; }
		M_INLINE void IncrementLastAGEvent() { m_ImportantAGEvent++; }
		M_INLINE uint8 GetImportantAGEvent() { return m_ImportantAGEvent; }
		//M_INLINE void SetWallCollision(fp32 _Collision) { m_WallCollision = _Collision; }
		//M_INLINE fp32 GetWallCollision() { return m_WallCollision; }

		CAG2Val AG2_EvaluateProperty(const CWAG2I_Context* _pContext, int _iProperty, int _PropertyType)
		{
			switch (_PropertyType)
			{
			case AG2_PROPERTYTYPE_FLOAT:
				{
					//M_ASSERT(m_lPropertiesFloat.ValidPos(_iProperty), "PROPERTYINDEX OUT OF RANGE");
					M_ASSERTHANDLER(m_lPropertiesFloat.ValidPos(_iProperty),"PROPERTYINDEX OUT OF RANGE", return CAG2Val::From((fp32)0.0f));
					return CAG2Val::From(m_lPropertiesFloat[_iProperty]);
				}
			case AG2_PROPERTYTYPE_INT:
				{
					//M_ASSERT(m_lPropertiesInt.ValidPos(_iProperty), "PROPERTYINDEX OUT OF RANGE");
					M_ASSERTHANDLER(m_lPropertiesInt.ValidPos(_iProperty),"PROPERTYINDEX OUT OF RANGE", return CAG2Val::From((int)0));
					return CAG2Val::From(m_lPropertiesInt[_iProperty]);
				}
			case AG2_PROPERTYTYPE_BOOL:
				{
					//M_ASSERT(m_lPropertiesBool.ValidPos(_iProperty), "PROPERTYINDEX OUT OF RANGE");
					M_ASSERTHANDLER(m_lPropertiesBool.ValidPos(_iProperty),"PROPERTYINDEX OUT OF RANGE", return CAG2Val::From(false));
					return CAG2Val::From(m_lPropertiesBool[_iProperty]);
				}
			case AG2_PROPERTYTYPE_FUNCTION:
				{
					M_ASSERTHANDLER(_iProperty < m_nProperties && m_lpfnProperties[_iProperty],"PROPERTYINDEX OUT OF RANGE", return CAG2Val::From((int)0));
					//M_ASSERT(_iProperty < m_nProperties && m_lpfnProperties[_iProperty], "PROPERTYINDEX OUT OF RANGE");
					return (this->*(m_lpfnProperties[_iProperty]))(_pContext);
				}
			default:
				M_ASSERT(false,"UNDEFINED PROPERTY TYPE");
				return CAG2Val::From(0);
			}
/*



#ifndef	M_RTM
			if (m_lpfnProperties == NULL)
				return CAG2Val();

			if ((_iProperty < 0) || (_iProperty >= m_nProperties))
				return CAG2Val();
#endif	// M_RTM

			if(m_lpfnProperties[_iProperty] != NULL)
				return (this->*(m_lpfnProperties[_iProperty]))(_pContext);

			return CAG2Val();*/
		}
		
		bool AG2_EvaluateOperator(const CWAG2I_Context* _pContext, int _iOperator, const CAG2Val &_OperandA, const CAG2Val &_OperandB)
		{
#ifndef	M_RTM
			if (m_lpfnOperators == NULL)
				return false;

			if ((_iOperator < 0) || (_iOperator >= m_nOperators))
				return false;
#endif	// M_RTM

			return (this->*(m_lpfnOperators[_iOperator]))(_pContext, _OperandA, _OperandB);
		}
		
	
		bool AG2_EvaluateCondition(const CWAG2I_Context* _pContext, int _iProperty, int _iPropertyType, int _iOperator, const CAG2Val& _Constant, fp32& _TimeFraction)
		{
			if(m_lpfnConditions[_iProperty] != NULL)
			{
				return (this->*(m_lpfnConditions[_iProperty]))(_pContext, _iOperator, _Constant, _TimeFraction);
			}
			return false;
		}

		bool AG2_DoProperty(const CWAG2I_Context* _pContext, int _iProperty, int _PropertyType, int _iOperator, const CAG2Val& _Constant, fp32& _TimeFraction)
		{
			_TimeFraction = 0.0f;
			 return (this->*(m_lpfnOperators[_iOperator]))(_pContext, AG2_EvaluateProperty(_pContext, _iProperty, _PropertyType), _Constant);
		}
		
		void AG2_InvokeEffect(const CWAG2I_Context* _pContext, uint8 _iEffect, const CXRAG2_ICallbackParams* _pParams)
		{
			M_ASSERTHANDLER(m_lpfnEffects && _iEffect < m_nEffects,"AG2_InvokeEffect error", return);

			if (m_lpfnEffects[_iEffect] == NULL)
				return;

			(this->*(m_lpfnEffects[_iEffect]))(_pContext, _pParams);
		}

		M_INLINE CMTime GetEnterStateTime() { return m_EnterStateTime; }

		bool Condition_StateTime(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
		bool Condition_AnimExitPoint(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
		bool Condition_AnimLoopCount(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
		bool Condition_AnimLoopCountOffset(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
		bool Condition_StateTimeOffset(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);
		// 0<->1
		bool Condition_LoopedAnimCount(const CWAG2I_Context* _pContext, int iOperator, const CAG2Val &_Constant, fp32& _TimeFraction);

		CAG2Val Property_NULL(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);

		/*CAG2Val Property_StateTime(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { CMTime TmpTime = _pContext->m_GameTime - m_EnterStateTime; return CAG2Val::From(TmpTime); };
		CAG2Val Property_Dialogue0(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[0]); }
		CAG2Val Property_Dialogue1(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[1]); }
		CAG2Val Property_Dialogue2(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[2]); }
		CAG2Val Property_Dialogue3(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[3]); }
		CAG2Val Property_Dialogue4(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[4]); }
		CAG2Val Property_Dialogue5(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From(m_Dialogue_Cur[5]); }
		CAG2Val Property_StateTimeOffset(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);
		CAG2Val Property_Rand1(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);
		CAG2Val Property_Rand255(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams) { return CAG2Val::From((Property_Rand1(_pContext,_pParams).GetFp() * 255.0f)); }
		CAG2Val Property_LoopedAnimTime(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);
		CAG2Val Property_HandledResponse(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);*/

		bool OperatorEQ(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) == 0); }
		bool OperatorGT(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) > 0); }
		bool OperatorLT(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) < 0); }
		bool OperatorGE(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) >= 0); }
		bool OperatorLE(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) <= 0); }
		bool OperatorNE(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.Compare(_OperandB) != 0); }
		bool OperatorMOD(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB);

		// Should be int only
		bool OperatorAND(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.GetInt() & _OperandB.GetInt()) != 0; } ;
		bool OperatorNOTAND(const CWAG2I_Context* _pContext, const CAG2Val &_OperandA, const CAG2Val &_OperandB) { return (_OperandA.GetInt() & _OperandB.GetInt()) == 0; } ;

		void Effect_RegisterHandledResponse(const CWAG2I_Context* _pContext, const CXRAG2_ICallbackParams* _pParams);

		virtual void Clear();
		void Copy(const CWO_ClientData_AnimGraph2Interface& _CD);
		bool ConditionHelper(const CWAG2I_Context* _pContext, fp32 TimeOffset, fp32& _TimeFraction, bool _bOpEqual, bool _bOpGreater, bool _bOpLess);
		virtual void Write(CCFile* _pFile) {};
		virtual void Read(CCFile* _pFile) {};
};

//--------------------------------------------------------------------------------

#endif /* WAG2_ClientData_h */
