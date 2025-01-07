/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Follower.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWObject_Follower

	History:		
		060304:		Created File
		060614:		Use listener system instead of OnRefresh(), + some cleanup
\*____________________________________________________________________________________________*/
#include "PCH.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			Game object that follows another object,
					with some options.

	Comments:		
\*____________________________________________________________________*/
class CWObject_Follower : public CWObject_Model
{
	typedef CWObject_Model parent;
	MRTC_DECLARE_SERIAL_WOBJECT;

	enum
	{
		FLAGS_POS_LOCKX =		M_Bit(0),
		FLAGS_POS_LOCKY =		M_Bit(1),
		FLAGS_POS_LOCKZ =		M_Bit(2),

		FLAGS_ROT_LOCKX =		M_Bit(3),
		FLAGS_ROT_LOCKY =		M_Bit(4),
		FLAGS_ROT_LOCKZ =		M_Bit(5),
		FLAGS_ROT_LOCKMASK =	DBitRange(3,5),

		FLAGS_ROTATION =		M_Bit(6),		// animated rotation
		FLAGS_ROTATION_CLIENT =	M_Bit(7),		// animated rotation is client-only

		// m_Data[] index enums
		DATA_FLAGS =				0,	//	Data(0)  - Flags
		DATA_TARGET =				1,	//	Data(1)  - Target namehash / Target object index
		DATA_POSOFFSET =			2,	//	Data(2)  - Position offset
		DATA_MODELFLAGS =			3,	//	Data(3)  - Model flags
		DATA_ROT_SPEED_INTERVAL =	4,	//	Data(4)  - Rotation speed, interval [           speed  (16)              | active_time (8) | inactive_time (8) ]
		DATA_ROT_AXIS =				5,	//	Data(5)  - Rotation axis (-1..1)    [           x (10)       |         y (10)         |         z (10)         ]
	};

public:
	virtual void OnCreate()
	{
		m_Data[DATA_FLAGS] = 0;
		m_Data[DATA_ROT_SPEED_INTERVAL] = uint(0.25f * 256.0f) | (uint(1.0f * 16.0f) << 16);	// speed = 0.5, active = 1.0, inactive = 0.0
		m_Data[DATA_ROT_AXIS] =           uint(1.0f * 511.0f) << 20;							// rotaxis = [0, 0, 1]
	}

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
	{
		static const char* s_lFlags[] = 
		{ 
			"pos_lockx", "pos_locky", "pos_lockz", 
			"rot_lockx", "rot_locky", "rot_lockz", 
			NULL 
		};

		switch (_KeyHash)
		{
		case MHASH2('FLAG','S'):  // "FLAGS"
			Data(DATA_FLAGS) |= _pKey->GetThisValue().TranslateFlags(s_lFlags);
			break;

		case MHASH2('ROT','MODE'): // "ROTMODE"
			Data(DATA_FLAGS) |= _pKey->GetThisValuei() << 6;
			break;

		case MHASH2('TARG','ET'): // "TARGET"
			Data(DATA_TARGET) = StringToHash( _pKey->GetThisValue() );
			break;

		case MHASH2('ROTS','PEED'):
			{
				fp32 x = _pKey->GetThisValuef();
				int Speed = Clamp(RoundToInt(x * 256.0f), -32768, 32767);
				Data(DATA_ROT_SPEED_INTERVAL) = (m_Data[DATA_ROT_SPEED_INTERVAL] & 0xffff0000) | (Speed & 0xffff);
				break;
			}

		case MHASH2('INTE','RVAL'): // "INTERVAL"
			{
				fp32 aDurations[2]; // active time, inactive time
				_pKey->GetThisValueaf(2, aDurations);
				uint ActiveTime = Clamp(TruncToInt(16.0f * aDurations[0]), 0, 255);
				uint InactiveTime = Clamp(TruncToInt(16.0f * aDurations[1]), 0, 255);
				Data(DATA_ROT_SPEED_INTERVAL) = (m_Data[DATA_ROT_SPEED_INTERVAL] & 0x0000ffff) | (ActiveTime << 16) | (InactiveTime << 24);
				break;
			}

		case MHASH2('ROT','AXIS'): // "ROTAXIS"
			{
				CVec3Dfp32 tmp;
				_pKey->GetThisValueaf(3, tmp.k);
				const fp32 Scale = 511.0f;
				const uint Mask = 1023;
				int rx = RoundToInt(Clamp(tmp.k[0], -1.0f, 1.0f) * Scale);
				int ry = RoundToInt(Clamp(tmp.k[1], -1.0f, 1.0f) * Scale);
				int rz = RoundToInt(Clamp(tmp.k[2], -1.0f, 1.0f) * Scale);
				Data(DATA_ROT_AXIS) =  (rx & Mask) | ((ry & Mask) << 10) | ((rz & Mask) << 20);
				break;
			}

		default:
			parent::OnEvalKey(_KeyHash, _pKey); 
		}
	}

	virtual void OnSpawnWorld()
	{
		parent::OnSpawnWorld();

		// convert namehash -> object index
		uint32 TargetNameHash = m_Data[DATA_TARGET];
		int iTargetObj = m_pWServer->Selection_GetSingleTarget(TargetNameHash);
		Data(DATA_TARGET) = iTargetObj;

		if (iTargetObj)
		{ // we want to know if the target dies or moves
			m_pWServer->Object_AddListener(iTargetObj, m_iObject, CWO_LISTENER_EVENT_MOVED);
		}

		CVec3Dfp32 Offset = GetPosition();
		Offset -= m_pWServer->Object_GetPosition( iTargetObj );
		Data(DATA_POSOFFSET) = Offset.Pack32(1023.0f);

		//ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;	-- shouldn't be needed anymore

		CWObject* pParent = m_pWServer->Object_Get( GetParent() );
		if (pParent)
		{
			// make sure the parent gets smooth interpolation
			pParent->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
		}
	}

	static void GetMatrix(CWorld_PhysState* _pWPhysState, const CWObject_CoreData* _pObj, fp32 _IPTime, bool _bClient, CMat4Dfp32& _Result)
	{
		CalcPositionMatrix(_pWPhysState, _pObj, _IPTime, _Result);

		uint Flags = _pObj->m_Data[DATA_FLAGS];
		if ((Flags & FLAGS_ROTATION) && (_bClient || !(Flags & FLAGS_ROTATION_CLIENT)))
		{
			CMat4Dfp32 RotMat;
			CMTime t = CMTime::CreateFromTicks(_pObj->GetAnimTick(_pWPhysState), _pWPhysState->GetGameTickTime(), _IPTime);
			CalcRotation(_pObj, t, RotMat);
			M_VMatMul(RotMat, _Result, _Result);
		}
	}

	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
	{
		aint bDoRender = 0;
		CXR_Model* lpModels[CWO_NUMMODELINDICES];
		for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
			lpModels[i] = pModel;
			bDoRender |= aint(pModel);
		}

		if (bDoRender)
		{
			// set up animstate & render matrix
			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_Data[3] = ~(_pObj->m_Data[DATA_MODELFLAGS]);

			CMat4Dfp32 RenderMat;
			GetMatrix(_pWClient, _pObj, _pWClient->GetRenderTickFrac(), true, RenderMat);

			// render models
			for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
				_pEngine->Render_AddModel(lpModels[i], RenderMat, AnimState);
		}
	}

	virtual aint OnMessage(CWObject_Message& _Msg)
	{
		switch (_Msg.m_Msg)
		{
		case OBJSYSMSG_LISTENER_EVENT:
			{
				if (_Msg.m_Param0 == CWO_LISTENER_EVENT_MOVED)
				{
					// target moved - update position!
					M_ASSERT(m_Data[DATA_TARGET] == _Msg.m_iSender, "[CWObject_Follower] got event from other than target!");

					CMat4Dfp32 NewMat;
					GetMatrix(m_pWServer, this, 1.0f, false, NewMat);
					m_pWServer->Object_SetPosition_World(m_iObject, NewMat);
				}
				return 1;
			}
		}
		return parent::OnMessage(_Msg);
	}

	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
	{
		switch (_Msg.m_Msg)
		{
		case OBJMSG_HOOK_GETCURRENTMATRIX:
			{
				CMat4Dfp32& ResultMat = *(CMat4Dfp32*)_Msg.m_Param0;
				fp32 IPTime = _pWClient->GetRenderTickFrac();
				GetMatrix(_pWClient, _pObj, IPTime, true, ResultMat);
		//		_pWClient->Debug_RenderMatrix(ResultMat, 0.0f, false);
				return 1;
			}
		}

		return parent::OnClientMessage(_pObj, _pWClient, _Msg);
	}

	static void CalcPositionMatrix(CWorld_PhysState* _pWPhysState, const CWObject_CoreData* _pObj, fp32 _IPTime, CMat4Dfp32& _Result)
	{
		_Result = _pObj->GetPositionMatrix(); // fallback
		CWObject_CoreData* pTarget = _pWPhysState->Object_GetCD( _pObj->m_Data[DATA_TARGET] );
		if (!pTarget)
			return;

		CVec3Dfp32 Offset;
		Offset.Unpack32(_pObj->m_Data[DATA_POSOFFSET], 1023.0f);
		uint32 Flags = _pObj->m_Data[DATA_FLAGS];

		CVec3Dfp32 TargetPos;
		if ((Flags & FLAGS_ROT_LOCKMASK) != FLAGS_ROT_LOCKMASK)
		{
			// Some rotaxis isn't locked (must interpolate matrix)
			CMat4Dfp32 TargetMat;
			Interpolate2(pTarget->GetLastPositionMatrix(), pTarget->GetPositionMatrix(), TargetMat, _IPTime);
			TargetPos = TargetMat.GetRow(3);

			if (Flags & FLAGS_ROT_LOCKMASK)
			{
				CVec3Dfp32 CurrPos, CurrAngles, TargetAngles;
				CurrAngles.CreateAnglesFromMatrix(0, _Result);
				TargetAngles.CreateAnglesFromMatrix(0, TargetMat);

				if (!(Flags & FLAGS_ROT_LOCKX)) CurrAngles.k[0] = TargetAngles.k[0];
				if (!(Flags & FLAGS_ROT_LOCKY)) CurrAngles.k[1] = TargetAngles.k[1];
				if (!(Flags & FLAGS_ROT_LOCKZ)) CurrAngles.k[2] = TargetAngles.k[2];

				CurrPos = _Result.GetRow(3);
				CurrAngles.CreateMatrixFromAngles(0, _Result);
				_Result.GetRow(3) = CurrPos;
			}
			else
			{
				// No rotaxis is locked
				_Result.GetRow(0) = TargetMat.GetRow(0);
				_Result.GetRow(1) = TargetMat.GetRow(1);
				_Result.GetRow(2) = TargetMat.GetRow(2);
			}
		}
		else
		{
			// All rotaxis are locked (can interpolate only position)
			pTarget->GetLastPosition().Lerp(pTarget->GetPosition(), _IPTime, TargetPos);
		}

		if (!(Flags & FLAGS_POS_LOCKX)) _Result.GetRow(3).k[0] = TargetPos.k[0] + Offset.k[0];
		if (!(Flags & FLAGS_POS_LOCKY)) _Result.GetRow(3).k[1] = TargetPos.k[1] + Offset.k[1];
		if (!(Flags & FLAGS_POS_LOCKZ)) _Result.GetRow(3).k[2] = TargetPos.k[2] + Offset.k[2];
	}

	static void CalcRotation(const CWObject_CoreData* _pObj, CMTime _Time, CMat4Dfp32& _Result)
	{
		fp32 Speed =        ((_pObj->m_Data[DATA_ROT_SPEED_INTERVAL] << 16) >> 16) * (1.0f / 256.0f);
		fp32 TimeActive =   ((_pObj->m_Data[DATA_ROT_SPEED_INTERVAL] << 8) >> 24) * (1.0f / 16.0f);
		fp32 TimeInactive = ((_pObj->m_Data[DATA_ROT_SPEED_INTERVAL] << 0) >> 24) * (1.0f / 16.0f);
		fp32 IntervalTime = TimeActive + TimeInactive;

		CVec3Dfp32 Axis;
		const fp32 Scale = 1.0f / 511.0f;
		Axis.k[0] = int((_pObj->m_Data[DATA_ROT_AXIS] << 22) >> 22) * Scale;
		Axis.k[1] = int((_pObj->m_Data[DATA_ROT_AXIS] << 12) >> 22) * Scale;
		Axis.k[2] = int((_pObj->m_Data[DATA_ROT_AXIS] <<  2) >> 22) * Scale;

		CMTime Time;
		int nInterval = _Time.GetNumModulus(IntervalTime);
		fp32 CurInterval = _Time.GetTimeModulus(IntervalTime);
		if (CurInterval > TimeActive)
			Time = CMTime::CreateFromTicks(nInterval + 1, TimeActive);
		else
			Time = CMTime::CreateFromTicks(nInterval, TimeActive) + CMTime::CreateFromSeconds(CurInterval);

		CAxisRotfp32 AxisRot(Axis, Time.GetTimeModulusScaled(Speed, 1.0f));
		AxisRot.CreateMatrix(_Result);
	}
};


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Follower, parent, 0x0100);
