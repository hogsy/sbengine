#include "PCH.h"
#include "WObj_Physical.h"

// -------------------------------------------------------------------
//  PHYSICAL
// -------------------------------------------------------------------
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Physical, CWObject_Model, 0x0100);

CWObject_Physical::CWObject_Physical()
{
	MAUTOSTRIP(CWObject_Physical_ctor, MAUTOSTRIP_VOID);
	m_PhysFlags = PHYSICAL_FLAGS_MEDIUMACCEL | PHYSICAL_FLAGS_MEDIUMROTATE;
	m_MediumResist = 1.0f;
	m_Mass = 1.0f;
	m_Density = 1.0f;
}

void CWObject_Physical::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MAUTOSTRIP(CWObject_Physical_OnEvalKey, MAUTOSTRIP_VOID);
	const CStr KeyName = _pKey->GetThisName();
	CStr _Value = _pKey->GetThisValue();
	const fp32 Valuef = _Value.Val_fp64();
	switch (_KeyHash)
	{
	case MHASH5('PHYS','_MED','IUMR','ESIS','T'): // "PHYS_MEDIUMRESIST"
		{
			m_MediumResist = Valuef;
			break;
		}
	case MHASH3('PHYS','_DEN','SITY'): // "PHYS_DENSITY"
		{
			m_Density = Valuef;
			break;
		}
	case MHASH3('PHYS','_MAS','S'): // "PHYS_MASS"
		{
			m_Mass = Valuef;
			break;
		}
	default:
		{
			CWObject_Model::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void RenormalizeMatrix(CMat4Dfp32& _Dest);
void RenormalizeMatrix(CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(RenormalizeMatrix, MAUTOSTRIP_VOID);
	CVec3Dfp32::GetMatrixRow(_Dest, 0).Normalize();
	CVec3Dfp32::GetMatrixRow(_Dest, 2) = -(CVec3Dfp32::GetMatrixRow(_Dest, 1) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
	CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
}

aint CWObject_Physical::OnMessage(const CWObject_Message& _Msg)
{
	MAUTOSTRIP(CWObject_Physical_OnMessage, 0);
/*	switch(_Msg.m_Msg)
	{
	case OBJMSG_PHYSICS_RADIALSHOCKWAVE :
		{
			CVec3Dfp32 v = GetPosition() - _Msg.m_VecParam0;
			fp32 d = v.Length();
			if (d > _Msg.m_Param0) return 1;
			fp32 i = (1.0f - (d / fp32(_Msg.m_Param0))) * fp32(_Msg.m_Param1);
//		LogFile(CStrF("%f, %d, %d, %f", d, _Msg.m_Param0, _Msg.m_Param1, i));
			m_pWServer->Object_AddVelocity(m_iObject, v * (i  / d));
			return 1;
		}
	default :*/
		return CWObject_Model::OnMessage(_Msg);
//	}
}

/*
		const int MaxSamples = 128;
		CXR_MediumDesc Mediums[MaxSamples];
		CVec3Dfp32 MediumSamples[MaxSamples];

		int nMediumSmp = Phys_GetMediumSamplingPoints(MediumSamples, MaxSamples);
		m_pWServer->Phys_GetMediums(MediumSamples, nMediumSmp, Mediums);
*/

void CWObject_Physical::OnRefresh()
{
	MAUTOSTRIP(CWObject_Physical_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_Model::OnRefresh();
}

void PhysUtil_Move(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, const CPhysUtilParams& _Params, fp32 _dTime, const CVec3Dfp32& _UserVel, const CVec3Dfp32* _pMediumV, const CXR_MediumDesc* _pMediums, int _nMediums)
{
	MAUTOSTRIP(PhysUtil_Move, MAUTOSTRIP_VOID);
//	CWObject_Model::OnRefresh();

//	if (!(m_PhysFlags & PHYSICAL_FLAGS_DISABLE))
	{

		// FIXME:  Gravity, generalize!
		// m_pWServer->Object_AddVelocity(m_iObject, CVec3Dfp32(0,0,-15.17f/8.0f));
	//	CVec3Dfp32 Gravity(0, 0, -15.17f/8.0f);
		CVec3Dfp32 Gravity(0, 0, -16.0f/8.0f);

		int Flags = _Params.m_Flags;

//		_pPhysState->Object_AddVelocity(_pObj->m_iObject, _UserVel /** _dTime*/);

		CVec3Dfp32 MediumVel = 0;
		fp32 nMediumInv = 1.0f / fp32(_nMediums);
		CMat4Dfp32 Rot;
		Rot.Unit();
		int nRot = 0;

		fp32 AvgThickness = 0.0f;

		// Since we nowdays have acceleration, this should be translated to acc and used when
		// responding to CWO_PHYSEVENT_GETACCELERATION.
		if (_Params.m_PhysFlags & PHYSICAL_FLAGS_MEDIUMACCEL)
		{
			for(int v = 0; v < _nMediums; v++)
			{
				CVec3Dfp32 objv = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
				fp32 dmlen = (_pMediums[v].m_Velocity - objv).Length();	// Speed difference between the medium and the object.

				_pPhysState->Object_AccelerateTo(_pObj->m_iObject, _pMediums[v].m_Velocity, CVec3Dfp32(_pMediums[v].m_Thickness) * _Params.m_MediumResist * dmlen * nMediumInv);
				AvgThickness += _pMediums[v].m_Thickness;
				CVec3Dfp32 Vel(Gravity * (_Params.m_Density - _pMediums[v].m_Density) * nMediumInv);
				MediumVel += Vel;


				CVec3Dfp32 vTP;
				CVec3Dfp32 Axis;
				_pObj->GetPosition().Sub(_pMediumV[v], vTP);
		//if (nMediumSmp)
		//	ConOut(CStrF("        Medium %d, ", Mediums[v].m_MediumFlags) + vTP.GetString());
				Vel.CrossProd(vTP, Axis);
				fp32 MomentSqr = Axis.LengthSqr();
				if (MomentSqr > 0.001f)
				{
					fp32 Moment = M_Sqrt(MomentSqr);
					Axis *= 1.0f / Moment;

					CMat4Dfp32 dRot, NewRot;
				Moment *= 0.005f;
					Axis.CreateAxisRotateMatrix(-Moment, dRot);
					for(int i = 0; i < 3; i++)
						for(int j = 0; j < 3; j++)
							Rot.k[i][j] += dRot.k[i][j];
					nRot++;

		//ConOut(CStrF("(CWObject_Physical::OnRefresh) Moment %f, ", Moment) + Axis.GetString() );
				}
			}

			if (Flags & 1) 
				if (MediumVel.k[2] < 0.0f) MediumVel.k[2] = 0;

/*ConOutL(CStrF("Gravity (%f) %s, Vel %s", _dTime, (char*)MediumVel.GetString(), (char*)_pObj->GetMoveVelocity().GetString() ));*/

			_pPhysState->Object_AddVelocity(_pObj->m_iObject, MediumVel * _dTime);
			AvgThickness *= nMediumInv;
		}

#ifdef NEVER
		if ((m_PhysFlags & PHYSICAL_FLAGS_MEDIUMROTATE) && nRot)
		{
			fp32 nRotInv = 1.0f / fp32(nRot);
			for(int i = 0; i < 3; i++)
				for(int j = 0; j < 3; j++)
					Rot.k[i][j] *= nRotInv;
			RenormalizeMatrix(Rot);
			CQuatfp32 QRot;
			QRot.Create(Rot);

			CAxisRotfp32 Rot = GetRotVelocity();
			Rot.m_Angle *= 0.9f;

			Rot.Multiply(QRot);
			_pPhysState->Object_SetRotVelocity(_pObj->m_iObject, Rot);
		}
#endif

		_pPhysState->Object_MovePhysical(&_Selection, _pObj->m_iObject, _dTime);
	}
//ConOutL(_pObj->GetMoveVelocity().GetString());
}

int CWObject_Physical::Phys_GetMediumSamplingPoints(CVec3Dfp32* _pRetV, int _MaxV)
{
	MAUTOSTRIP(CWObject_Physical_Phys_GetMediumSamplingPoints, 0);
	_pRetV[0] = GetPosition();
	return 1;
}

CVec3Dfp32 CWObject_Physical::Phys_GetUserVelocity(const CXR_MediumDesc& _MediumDesc, int& _Flags)
{
	MAUTOSTRIP(CWObject_Physical_Phys_GetUserVelocity, CVec3Dfp32());
	return 0;
}

void CWObject_Physical::OnLoad(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Physical_OnLoad, MAUTOSTRIP_VOID);
	CWObject_Model::OnLoad(_pFile);
	_pFile->ReadLE(m_PhysFlags);
	_pFile->ReadLE(m_MediumResist);
	_pFile->ReadLE(m_Density);
	_pFile->ReadLE(m_Mass);
}

void CWObject_Physical::OnSave(CCFile* _pFile)
{
	MAUTOSTRIP(CWObject_Physical_OnSave, MAUTOSTRIP_VOID);
	CWObject_Model::OnSave(_pFile);
	_pFile->WriteLE(m_PhysFlags);
	_pFile->WriteLE(m_MediumResist);
	_pFile->WriteLE(m_Density);
	_pFile->WriteLE(m_Mass);
}

void CWObject_Physical::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Physical_OnClientRefresh, MAUTOSTRIP_VOID);
	CWObject_Model::OnClientRefresh(_pObj, _pWClient);
}

