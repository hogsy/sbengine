#include "pch.h"

#include "WDynamicsConstraint.h"

/*
	TODO: Ta bort gammalt skit...
 */

static fp32 GetPlanarRotation(const CVec4Dfp32& _From, const CVec4Dfp32& _To, const CVec4Dfp32& _Axis)
{
	CVec3Dfp32 F = CWD_RigidBodyState::To3D(_From);
	CVec3Dfp32 T = CWD_RigidBodyState::To3D(_To);
	CVec3Dfp32 A = CWD_RigidBodyState::To3D(_Axis);

	CVec3Dfp32 Ref;
	A.CrossProd(F, Ref);
	fp32 u = T * F;
	fp32 v = T * Ref;
	fp32 Angle = atan2f(v, u);
	return Angle;
}

static fp32 Angle_(const CVec4Dfp32& _v1, const CVec4Dfp32& _v2)
{
	return M_ACos((_v1 * _v2) / (_v1.Length() * _v2.Length()));
}

class CWD_LegacyConstraint
{
public:

	static void SolveHingeJoint(CWD_RigidBodyState &_RBState1, CWD_RigidBodyState& _RBState2)
	{

	}

	static void SolveBallJointWorld(CWD_RigidBodyState &_RBState1,vec128 _WorldRef,vec128 _RA, vec128 _dt)
		//const CVec3Dfp32& _WorldRef, const CVec3Dfp32& _RA, fp32 _dt)
	{
		// CVec3Dfp32 tmp,tmp2;
		// CVec3Dfp32 wdiff,p1;

		vec128 v_p1 = _RA;
		vec128 v_linv = _RBState1.m_Velocity.v;
		vec128 v_angv = _RBState1.m_AngularVelocity.v;
		vec128 v_pos = _RBState1.m_Position.v;
		vec128 v_dt = _dt;
		vec128 v_worldref = _WorldRef;

		{
			//const CMat4Dfp32 &T1 = _RBState1.GetTransform();

			//CMat4Dfp32 R1 = _RBState1.GetTransform();//T1;
			vec128 Quat = _RBState1.m_Orientation;
			vec128 negmsk = M_VConstMsk(1,1,1,0);
			vec128 QuatConj = M_VNegMsk(Quat,negmsk);

			//Test quat mul
			v_p1 = M_VQuatMul( M_VQuatMul(Quat,v_p1),QuatConj );
		}

		vec128 v_n,v_rlen;

		{
			vec128 v_zero = M_VScalar(0);
			vec128 v_nearzero = M_VScalar(0.00001f);
			vec128 v_epsilon = M_VScalar(0.005f);

			vec128 v_bias = v_zero;
			//		if (wdiff.Length() > len)
			{
				//CVec3Dfp32 Ps = _RBState1.GetPosition();
				vec128 v_diff = M_VAdd( v_p1, v_pos );//M_VLd_V3_Slow(&Ps) );
					//M_VLd(_RBState1.m_Position.k[0],_RBState1.m_Position.k[1],_RBState1.m_Position.k[2],0));
		
				v_diff = M_VSub(v_diff,v_worldref);

				vec128 v_difflen = M_VLen3(v_diff);
				vec128 v_abs = M_VSub(v_difflen,v_epsilon);

				if ( M_VCmpAllGT(M_VAbs(v_abs),v_nearzero) )
				{
					vec128 v_nrm;
					if( M_VCmpAllEq(v_difflen,v_zero) ) v_nrm = v_zero;
					else v_nrm = M_VNrm3( v_diff );
					v_bias = M_VMul(v_nrm, v_abs);
					v_bias = M_VMul(v_bias, M_VRcp_Est(v_dt));
				}
			}

			{
				vec128 v_vp1 = M_VXpd(v_angv,v_p1);
				v_vp1 = M_VAdd(v_vp1, v_linv );
				v_n = M_VAdd(v_vp1,v_bias);
				if( !M_VCmpAnyGT(M_VAbs(v_n),v_nearzero) ) 
				{
					v_rlen = v_zero;
					v_n = v_zero;
				}
				else 
				{
					v_rlen = M_VLen3(v_n);
					v_n = M_VNrm3( v_n );
				}
			}
		}


		{
			// Create matrix
			vec128 r0 = _RBState1.m_WorldInertiaTensorInvert.r[0];
			vec128 r1 = _RBState1.m_WorldInertiaTensorInvert.r[1];
			vec128 r2 = _RBState1.m_WorldInertiaTensorInvert.r[2];
			vec128 r3 = _RBState1.m_WorldInertiaTensorInvert.r[3];
			M_VTranspose4x4(r0, r1, r2, r3);

			vec128 v_tmp,v_tmp2;
			
			v_tmp = M_VXpd(v_p1,v_n);
			v_tmp2 = M_VDp4x4(v_tmp, r0, v_tmp, r1, v_tmp, r2, v_tmp, r3);
			v_tmp = M_VXpd(v_tmp2,v_p1);

			vec128 v_invmass = M_VRcp_Est(_RBState1.m_Mass);
			vec128 v_denom = M_VRcp_Est( M_VAdd(v_invmass,M_VDp3(v_n,v_tmp)) );

			vec128 v_impulse = M_VMul( v_n, M_VNeg(M_VMul(v_rlen, v_denom)) );

			{
				_RBState1.m_Velocity.v = M_VAdd(M_VMul(v_impulse,v_invmass),v_linv);
				
				v_tmp = M_VXpd(v_p1,v_impulse);
				v_tmp2 = M_VDp4x4(v_tmp, r0, v_tmp, r1, v_tmp, r2, v_tmp, r3);

				_RBState1.m_AngularVelocity.v = M_VAdd(v_tmp2,v_angv);
			}
		}
	}

#if 0			
	static void SolveBallJoint(CWD_RigidBodyState &_RBState1, CWD_RigidBodyState& _RBState2, 
							   const CVec3Dfp32& _RA, const CVec3Dfp32& _RB, fp32 _dt)
	{
		CVec4Dfp32 Mov1[2],Mov2[2];
		CVec4Dfp32 Ang1[2],Ang2[2];

		CVec4Dfp32 *lpVec[8] = {Mov1,Mov1+1,Mov2,Mov2+1,Ang1,Ang1+1,Ang2,Ang2+1};

		{
			CVec3Dfp32 tmp,tmp2;

			const CMat4Dfp32 &T1 = _RBState1.GetTransform();
			const CMat4Dfp32 &T2 = _RBState2.GetTransform();

			CMat4Dfp32 R1 = T1;
			CMat4Dfp32 R2 = T2;

			R1.UnitNot3x3();
			R2.UnitNot3x3();

			CVec3Dfp32 p1 = _RA;
			CVec3Dfp32 p2 = _RB;
			p1 *= R1;
			p2 *= R2;

			CVec3Dfp32 wp1 = p1 + _RBState1.GetPosition();
			CVec3Dfp32 wp2 = p2 + _RBState2.GetPosition();
			CVec3Dfp32 wdiff = wp1 - wp2;

			const fp32 len = 0.005f;
			CVec3Dfp32 vbias(0);
			//		if (wdiff.Length() > len)
			if (M_Fabs(wdiff.Length() - len) > 0.00001f )
			{
				CVec3Dfp32 wdiffn = wdiff;
				wdiffn.Normalize();
				vbias = wdiffn * ((wdiff.Length() - len) / _dt);
			}

			CVec3Dfp32 vp1, vp2;
			_RBState1.GetAngularVelocity().CrossProd(p1, tmp);
			vp1 = _RBState1.GetVelocity() + tmp;

			_RBState2.GetAngularVelocity().CrossProd(p2, tmp);
			vp2 = _RBState2.GetVelocity() + tmp;

			CVec3Dfp32 vr = vp1 - vp2 + vbias;

			CVec3Dfp32 n = vr;
			n.Normalize();

			fp32 B1 = 1.0f / _RBState1.GetMass();
			fp32 B2 = 1.0f / _RBState2.GetMass();

			p1.CrossProd(n,tmp);
			tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
			tmp2.CrossProd(p1,tmp);
			fp32 B3 = n*tmp;

			p2.CrossProd(n,tmp);
			tmp.MultiplyMatrix(_RBState2.m_WorldInertiaTensorInvert,tmp2);
			tmp2.CrossProd(p2,tmp);
			fp32 B4 = n*tmp;

			fp32 denom =  B1 + B2 + B3 + B4;

			CVec3Dfp32 impulse = n * (-(vr.Length()) / denom);

			fp32 rb1massinv = 1.0f / _RBState1.GetMass();
			fp32 rb2massinv = 1.0f / _RBState2.GetMass();

	//		if (rb1state->m_active)
			{
				Mov1[0] = _RBState1.m_Velocity + CWD_RigidBodyState::To4D_LastZero(impulse * (rb1massinv));
				p1.CrossProd(impulse,tmp);
				tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
				Ang1[0] = _RBState1.m_AngularVelocity + CWD_RigidBodyState::To4D_LastZero(tmp2);
			}

			impulse *= -1.0f;

	//		if (rb2state->m_active)
			{
				Mov2[0] = _RBState2.m_Velocity + CWD_RigidBodyState::To4D_LastZero(impulse * (rb2massinv));
				p2.CrossProd(impulse,tmp);
				tmp.MultiplyMatrix(_RBState2.m_WorldInertiaTensorInvert, tmp2);
				Ang2[0] = _RBState2.m_AngularVelocity + CWD_RigidBodyState::To4D_LastZero(tmp2);
			}
		}
#else

	static void SolveBallJoint(CWD_RigidBodyState &_RBState1, CWD_RigidBodyState& _RBState2, 
		vec128 _RA, vec128 _RB, vec128 _dt)
	{
		vec128	v_p1 = _RA;
		vec128	v_p2 = _RB;
		vec128 v_dt = _dt;
		vec128 v_wdiff;
		
		{
			//Quat mul
			vec128 negmsk = M_VConstMsk(1,1,1,0);
			vec128 Quat,QuatConj;
			
			Quat = _RBState1.m_Orientation;
			QuatConj = M_VNegMsk(Quat,negmsk);
			v_p1 = M_VQuatMul( M_VQuatMul(Quat,v_p1),QuatConj );

			Quat = _RBState2.m_Orientation;
			QuatConj = M_VNegMsk(Quat,negmsk);
			v_p2 = M_VQuatMul( M_VQuatMul(Quat,v_p2),QuatConj );
		
			//Worldpos, diff
			vec128 v_wp1 = M_VAdd(v_p1,_RBState1.m_Position);
			vec128 v_wp2 = M_VAdd(v_p2,_RBState2.m_Position);
			v_wdiff = M_VSub(v_wp1,v_wp2);
		}

		vec128 v_linv1 = _RBState1.m_Velocity.v;
		vec128 v_angv1 = _RBState1.m_AngularVelocity.v;
		vec128 v_linv2 = _RBState2.m_Velocity.v;
		vec128 v_angv2 = _RBState2.m_AngularVelocity.v;

		//bias
		vec128 v_zero = M_VScalar(0);
		vec128 v_nearzero = M_VScalar(0.00001f);
		vec128 v_bias = v_zero;
		{
			vec128 v_epsilon = M_VScalar(0.005f);

			vec128 v_diff = M_VSub( M_VLen3(v_wdiff), v_epsilon );
			if( M_VCmpAllGT(M_VAbs(v_diff),v_nearzero) )
			{
				vec128	v_nrm;
				if( M_VCmpAllEq(v_diff,v_zero) ) v_nrm = v_zero;
				else v_nrm = M_VNrm3(v_wdiff);
				v_bias = M_VMul(v_nrm,v_diff);
				v_bias = M_VMul(v_bias, M_VRcp_Est(v_dt));
			}
		}

		vec128 v_n,v_rlen;
		{
			vec128 v_vp1 = M_VXpd(v_angv1,v_p1);
			v_vp1 = M_VAdd(v_vp1,v_linv1);

			vec128 v_vp2 = M_VXpd(v_angv2,v_p2);
			v_vp2 = M_VAdd(v_vp2,v_linv2);

			vec128 v_vr = M_VAdd(M_VSub(v_vp1,v_vp2),v_bias);

			//v128 ops have problems with 0
			if( !M_VCmpAnyGT(M_VAbs(v_vr),v_nearzero) )
			{
				v_n = v_zero;
				v_rlen = v_zero;
			}
			else
			{
				v_n = M_VNrm3(v_vr);
				v_rlen = M_VLen3(v_vr);
			}
		}


		{
			//Get inverted inertia tensors
			vec128 r10 = _RBState1.m_WorldInertiaTensorInvert.r[0];
			vec128 r11 = _RBState1.m_WorldInertiaTensorInvert.r[1];
			vec128 r12 = _RBState1.m_WorldInertiaTensorInvert.r[2];
			vec128 r13 = _RBState1.m_WorldInertiaTensorInvert.r[3];
			M_VTranspose4x4(r10,r11,r12,r13);

			vec128 r20 = _RBState2.m_WorldInertiaTensorInvert.r[0];
			vec128 r21 = _RBState2.m_WorldInertiaTensorInvert.r[1];
			vec128 r22 = _RBState2.m_WorldInertiaTensorInvert.r[2];
			vec128 r23 = _RBState2.m_WorldInertiaTensorInvert.r[3];
			M_VTranspose4x4(r20,r21,r22,r23);

			vec128 v_invmass1 = M_VRcp_Est(_RBState1.m_Mass);
			vec128 v_invmass2 = M_VRcp_Est(_RBState2.m_Mass);
			vec128 v_denom = M_VAdd(v_invmass1,v_invmass2);

			vec128 v_tmp,v_tmp2;
			v_tmp = M_VXpd(v_p1,v_n);
			v_tmp2 = M_VDp4x4(v_tmp, r10, v_tmp, r11, v_tmp, r12, v_tmp, r13);
			v_tmp = M_VXpd(v_tmp2,v_p1);
			v_denom = M_VAdd(v_denom,M_VDp3(v_tmp,v_n));

			v_tmp = M_VXpd(v_p2,v_n);
			v_tmp2 = M_VDp4x4(v_tmp, r20, v_tmp, r21, v_tmp, r22, v_tmp, r23);
			v_tmp = M_VXpd(v_tmp2,v_p2);
			v_denom = M_VRcp_Est( M_VAdd(v_denom,M_VDp3(v_tmp,v_n)) );

			vec128 v_impulse = M_VMul( v_n, M_VNeg(M_VMul(v_rlen,v_denom)) );

			{
				_RBState1.m_Velocity.v = M_VAdd(M_VMul(v_impulse,v_invmass1),v_linv1);
				v_tmp = M_VXpd(v_p1,v_impulse);
				v_tmp2 = M_VDp4x4(v_tmp, r10, v_tmp, r11, v_tmp, r12, v_tmp, r13);
				_RBState1.m_AngularVelocity.v = M_VAdd(v_tmp2,v_angv1);
			}

			v_impulse = M_VNeg(v_impulse);

			{
				_RBState2.m_Velocity.v = M_VAdd(M_VMul(v_impulse,v_invmass2),v_linv2);
				v_tmp = M_VXpd(v_p2,v_impulse);
				v_tmp2 = M_VDp4x4(v_tmp, r20, v_tmp, r21, v_tmp, r22, v_tmp, r23);
				_RBState2.m_AngularVelocity.v = M_VAdd(v_tmp2,v_angv2);
			}
		}
	}
#endif


	static void SolveMinDistanceWorld(CWD_RigidBodyState &_RBState1, const CVec3Dfp32& _WorldReferencePoint,
									  const CVec3Dfp32& _RA, fp32 _MinDistance, fp32 _dt)
	{
		CVec3Dfp32 p1;
		CVec3Dfp32 RelativePosition, N;
		{
			const CMat4Dfp32& T1 = _RBState1.GetTransform();

			_RA.MultiplyMatrix3x3(T1,p1);
			//CVec3Dfp32 p1 = _RA;
			//p1.MultiplyMatrix3x3(T1);

			CVec3Dfp32 wp1 = _RA * T1;
			CVec3Dfp32 wp2 = _WorldReferencePoint;
			RelativePosition = wp1 - wp2;
		}

		// CVec3Dfp32 tmp, tmp2;
		{
			CVec3Dfp32 wv1, wv2;
			_RBState1.GetAngularVelocity().CrossProd(p1, wv1);
			wv1 += _RBState1.GetVelocity();

//			CVec3Dfp32 RelativeVelocity = wv1;
//			CVec3Dfp32 ExtrapolatedRelativePosition = RelativePosition + wv1 * _dt;


			CVec3Dfp32 NewRelativePosition = RelativePosition + wv1*_dt;

			fp32 Distance = (RelativePosition + wv1*_dt).Length();
			if (Distance < _MinDistance)
			{
				//CVec3Dfp32 tmp = RelativePosition + wv1*_dt;
				//tmp.Normalize();
				//			NewRelativePosition -= tmp * (Distance - MinDistance);
				NewRelativePosition *= _MinDistance / Distance; //ExtrapolatedRelativePosition.Length();
			}
			else return;


			CVec3Dfp32 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0f / _dt);
			N = NewRelativeVelocity - wv1;
		}

		fp32 Vel = N.Length();
		N.Normalize();
/*
		{
			fp32 massinv = 1.0f / _RBState1.GetMass();
			fp32 num = -Vel;

			fp32 denom = massinv;
			fp32 Impulse = num / denom;

			CVec3Dfp32 force = N * Impulse;

			force *= -1.0f;
		}
*/
		// fp32 B1 = 1.0 / _RBState1.GetMass();

		{
			CVec3Dfp32 tmp,tmp2;
			p1.CrossProd(N,tmp);
			tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
			tmp2.CrossProd(p1,tmp);
			// fp32 B3 = N*tmp;

			fp32 denom2 =  (1.0f / _RBState1.GetMass()) + (N*tmp);

			CVec3Dfp32 impulse = N * ((Vel) / denom2);

			fp32 rb1massinv = 1.0f / _RBState1.GetMass();

	//		if (pState1->m_active)
			{
				_RBState1.m_Velocity += CWD_RigidBodyState::To4D_LastZero(impulse * (rb1massinv));
				p1.CrossProd(impulse,tmp);
				tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
				_RBState1.m_AngularVelocity += CWD_RigidBodyState::To4D_LastZero(tmp2);
			}
		}

	}
#if 0
	static void SolveMinDistance(CWD_RigidBodyState &_RBState1, CWD_RigidBodyState& _RBState2, 
								 const CVec3Dfp32& _RA, const CVec3Dfp32& _RB, fp32 _MinDistance, fp32 _dt)
	{
		CVec4Dfp32 Mov1[2],Mov2[2];
		CVec4Dfp32 Ang1[2],Ang2[2];

		CVec4Dfp32 *lpVec[8] = {Mov1,Mov1+1,Mov2,Mov2+1,Ang1,Ang1+1,Ang2,Ang2+1};

		{
			const CMat4Dfp32& T1 = _RBState1.GetTransform();
			const CMat4Dfp32& T2 = _RBState2.GetTransform();

			CVec3Dfp32 p1 = _RA;
			CVec3Dfp32 p2 = _RB;

			p1.MultiplyMatrix3x3(T1);
			p2.MultiplyMatrix3x3(T2);

			CVec3Dfp32 wp1 = _RA * T1;
			CVec3Dfp32 wp2 = _RB * T2;

			CVec3Dfp32 RelativePosition = wp1 - wp2;

			CVec3Dfp32 tmp, tmp2;

			CVec3Dfp32 wv1, wv2;
			_RBState1.GetAngularVelocity().CrossProd(p1, tmp);
			wv1 = _RBState1.GetVelocity() + tmp;

			_RBState2.GetAngularVelocity().CrossProd(p2, tmp);
			wv2 = _RBState2.GetVelocity() + tmp;

			CVec3Dfp32 RelativeVelocity = wv1 - wv2;
			CVec3Dfp32 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * _dt;

			fp32 Distance = ExtrapolatedRelativePosition.Length();
			fp32 MinDistance = _MinDistance;

			CVec3Dfp32 NewVel(0);
			CVec3Dfp32 NewRelativePosition = ExtrapolatedRelativePosition;

			if (Distance < MinDistance)
			{
				CVec3Dfp32 tmp = ExtrapolatedRelativePosition;
				tmp.Normalize();
				//			NewRelativePosition -= tmp * (Distance - MinDistance);
				NewRelativePosition *= MinDistance / ExtrapolatedRelativePosition.Length();
			}
			else return;

			CVec3Dfp32 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (1.0f / _dt);

			CVec3Dfp32 N = NewRelativeVelocity - RelativeVelocity;

			fp32 Vel = N.Length();
			N.Normalize();

			fp32 massinv = 1.0f / _RBState1.GetMass() + 1.0f / _RBState2.GetMass();
			fp32 num = -Vel;

			fp32 denom = massinv;
			fp32 Impulse = num / denom;

			CVec3Dfp32 force = N * Impulse;

			force *= -1.0f;

			fp32 B1 = 1.0 / _RBState1.GetMass();
			fp32 B2 = 1.0 / _RBState2.GetMass();

			p1.CrossProd(N,tmp);
			tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
			tmp2.CrossProd(p1,tmp);
			fp32 B3 = N*tmp;

			p2.CrossProd(N,tmp);
			tmp.MultiplyMatrix(_RBState2.m_WorldInertiaTensorInvert,tmp2);
			tmp2.CrossProd(p2,tmp);
			fp32 B4 = N*tmp;

			fp32 denom2 =  B1 + B2 + B3 + B4;

			CVec3Dfp32 impulse = N * ((Vel) / denom2);

			fp32 rb1massinv = 1.0f / _RBState1.GetMass();
			fp32 rb2massinv = 1.0f / _RBState2.GetMass();

	//		impulse *= 0.25f;

	//		if (pState1->m_active)
			{
				Mov1[0] = _RBState1.m_Velocity + CWD_RigidBodyState::To4D_LastZero(impulse * (rb1massinv));
				p1.CrossProd(impulse,tmp);
				tmp.MultiplyMatrix(_RBState1.m_WorldInertiaTensorInvert,tmp2);
				Ang1[0] = _RBState1.m_AngularVelocity + CWD_RigidBodyState::To4D_LastZero(tmp2);
			}

			impulse *= -1.0f;

	//		if (pState2->m_active)
			{
				Mov2[0] = _RBState2.m_Velocity + CWD_RigidBodyState::To4D_LastZero(impulse * (rb2massinv));
				p2.CrossProd(impulse,tmp);
				tmp.MultiplyMatrix(_RBState2.m_WorldInertiaTensorInvert,tmp2);
				Ang2[0] = _RBState2.m_AngularVelocity + CWD_RigidBodyState::To4D_LastZero(tmp2);
			}
		}

#else
	static void SolveMinDistance(CWD_RigidBodyState &_RBState1, CWD_RigidBodyState& _RBState2, 
							 vec128 _RA, vec128 _RB, const vec128 &_MinDistance, const vec128 &_dt)
	{
		vec128	v_p1 = _RA;
		vec128	v_p2 = _RB;
		vec128 v_dt = _dt;
		vec128 v_wdiff;

		vec128 v_linv1 = _RBState1.m_Velocity.v;
		vec128 v_angv1 = _RBState1.m_AngularVelocity.v;
		vec128 v_linv2 = _RBState2.m_Velocity.v;
		vec128 v_angv2 = _RBState2.m_AngularVelocity.v;

		{
			//Quat mul
			vec128 negmsk = M_VConstMsk(1,1,1,0);
			vec128 Quat,QuatConj;

			Quat = _RBState1.m_Orientation;
			QuatConj = M_VNegMsk(Quat,negmsk);
			v_p1 = M_VQuatMul( M_VQuatMul(Quat,v_p1),QuatConj );

			Quat = _RBState2.m_Orientation;
			QuatConj = M_VNegMsk(Quat,negmsk);
			v_p2 = M_VQuatMul( M_VQuatMul(Quat,v_p2),QuatConj );

			//Worldpos, diff
			vec128 v_wp1 = M_VAdd(v_p1,_RBState1.m_Position);
			vec128 v_wp2 = M_VAdd(v_p2,_RBState2.m_Position);
			v_wdiff = M_VSub(v_wp1,v_wp2);
		}

		vec128 v_n;
		vec128 v_vel;
		{
			vec128 v_tmp;
			vec128 v_wv1,v_wv2;
			vec128 v_epsilon = M_VScalar(0.0001f);

			v_tmp = M_VXpd(v_angv1,v_p1);
			v_wv1 = M_VAdd(v_linv1,v_tmp);
			v_tmp = M_VXpd(v_angv2,v_p2);
			v_wv2 = M_VAdd(v_linv2,v_tmp);

			vec128 v_wrel,v_wposep;
			v_wrel = M_VSub(v_wv1,v_wv2);
			v_wposep = M_VMAdd(v_wrel,v_dt,v_wdiff);

			vec128 v_mindist = _MinDistance;
			vec128 v_dist;
			if( M_VCmpAnyGT(M_VAbs(v_wposep),v_epsilon) )
			{
				v_dist = M_VLen3(v_wposep);
				if( M_VCmpAnyGE(v_dist,v_mindist) ) return;

				v_wposep = M_VMul( v_wposep, M_VMul( v_mindist, M_VRcp(v_dist) ) );
			}

			vec128 v_wrelnew = M_VMul(M_VSub(v_wposep,v_wdiff),M_VRcp(v_dt));

			v_n = M_VSub(v_wrelnew,v_wrel);

			if( M_VCmpAnyGT(M_VAbs(v_n),v_epsilon) )
			{
				v_vel = M_VLen3(v_n);
				v_n = M_VNrm3(v_n);
			}
			else
			{
				v_vel = M_VZero();
				v_n = v_vel;
			}
		}

		{
			vec128 v_invmass1 = M_VRcp_Est(_RBState1.m_Mass);
			vec128 v_invmass2 = M_VRcp_Est(_RBState2.m_Mass);
			vec128 v_massinv = M_VAdd(v_invmass1,v_invmass2);
			vec128 v_num = M_VNeg(v_vel);
			vec128 v_impulse = M_VMul(v_num,M_VRcp(v_massinv));

			vec128 v_force = M_VNeg(M_VMul(v_n,v_impulse));

			//Get inverted inertia tensors
			vec128 r10 = _RBState1.m_WorldInertiaTensorInvert.r[0];
			vec128 r11 = _RBState1.m_WorldInertiaTensorInvert.r[1];
			vec128 r12 = _RBState1.m_WorldInertiaTensorInvert.r[2];
			vec128 r13 = _RBState1.m_WorldInertiaTensorInvert.r[3];
			M_VTranspose4x4(r10,r11,r12,r13);

			vec128 r20 = _RBState2.m_WorldInertiaTensorInvert.r[0];
			vec128 r21 = _RBState2.m_WorldInertiaTensorInvert.r[1];
			vec128 r22 = _RBState2.m_WorldInertiaTensorInvert.r[2];
			vec128 r23 = _RBState2.m_WorldInertiaTensorInvert.r[3];
			M_VTranspose4x4(r20,r21,r22,r23);

			vec128 v_tmp,v_tmp2;
			v_tmp = M_VXpd(v_p1,v_n);
			v_tmp2 = M_VDp4x4(v_tmp, r10, v_tmp, r11, v_tmp, r12, v_tmp, r13);
			v_tmp = M_VXpd(v_tmp2,v_p1);
			v_massinv = M_VAdd(v_massinv,M_VDp3(v_tmp,v_n));

			v_tmp = M_VXpd(v_p2,v_n);
			v_tmp2 = M_VDp4x4(v_tmp, r20, v_tmp, r21, v_tmp, r22, v_tmp, r23);
			v_tmp = M_VXpd(v_tmp2,v_p2);
			v_massinv = M_VRcp_Est( M_VAdd(v_massinv,M_VDp3(v_tmp,v_n)) );

			v_impulse = M_VMul( v_n, M_VMul( v_vel, v_massinv ) );

			{
				_RBState1.m_Velocity.v = M_VAdd(M_VMul(v_impulse,v_invmass1),v_linv1);
				v_tmp = M_VXpd(v_p1,v_impulse);
				v_tmp2 = M_VDp4x4(v_tmp, r10, v_tmp, r11, v_tmp, r12, v_tmp, r13);
				_RBState1.m_AngularVelocity.v = M_VAdd(v_tmp2,v_angv1);
			}

			v_impulse = M_VNeg(v_impulse);

			{
				_RBState2.m_Velocity.v = M_VAdd(M_VMul(v_impulse,v_invmass2),v_linv2);
				v_tmp = M_VXpd(v_p2,v_impulse);
				v_tmp2 = M_VDp4x4(v_tmp, r20, v_tmp, r21, v_tmp, r22, v_tmp, r23);
				_RBState2.m_AngularVelocity.v = M_VAdd(v_tmp2,v_angv2);
			}
		}
#endif
	}
};

class CWD_ConstraintUtil
{
public:

	M_INLINE static bool GenerateFixAngleConstraintContact(int _iRB1, 
														   const CVec4Dfp32& _Axis1,	
														   const CWD_RigidBodyState& _RBState1,
														   const CVec4Dfp32& _RA, 
														   const CVec4Dfp32& _RW,
														   const CVec4Dfp32& _WorldAxis,
														   fp32 _MinAngle, fp32 _MaxAngle,
														   fp32 _RelativeAngle,															
														   CWD_ContactInfo& _CI1)
	{

		const CMat4Dfp32& T1 = _RBState1.GetTransform();

		vec128 WorldAnchorPoint1 = M_VMulMat(_RA, T1);

		vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);

		// TODO: Hmmm, här förutsätter man att det är 1:an som är axeln....
		vec128 Axis1 = M_VNrm3(_Axis1);
		Axis1 = M_VMulMat(Axis1, T1);

		vec128 Axis2 = M_VNrm3(_WorldAxis);

		vec128 Axis = M_VMul(M_VAdd(Axis1, Axis2), M_VScalar(0.5f));

		// TODO: !!!! __LocalRAAngle SKA VARA EN VEKTOR. SKULLE DET VARA DET SKULLE M_VSub ge fel resultat!!!!!!!!
		vec128 WorldRA = M_VSub(M_VMulMat(_RA, T1), T1.r[3]);
		//vec128 WorldRB = M_VSub(M_VMulMat(_RB, T2), T2.r[3]);

		// TODO: Denna operationen är nog onödig, borde funka med -= RelativeAngle nedan
		// Då ska M_VMulMat också bort. Problemt är att vinklarna kan gå utanför [-PI, PI] efter -=...
		// Titta även på den andra Generate...
		CAxisRotfp32 AxisRot((CVec3Dfp32& )Axis, 1.0f *_RelativeAngle * (1.0f / (2.0f * _PI)));
		CMat4Dfp32 RotMat;
		AxisRot.CreateMatrix(RotMat);		


		fp32 PlanarAngle = GetPlanarRotation(M_VMulMat(M_VNrm3(WorldRA), RotMat), M_VNrm3(_RW), Axis);
//		fp32 PlanarAngle = GetPlanarRotation(M_VNrm3(WorldRA), M_VNrm3(_RW), Axis);
//		PlanarAngle -= _RelativeAngle;

		vec128 NN1 = M_VNrm3(M_VXpd(Axis, WorldRA));
		vec128 NN2 = M_VNrm3(M_VXpd(Axis, _RW));

		if (PlanarAngle >= 0.0f)
		{
			fp32 A = _PI - PlanarAngle;

			if (A > _MaxAngle)
			{
				fp32 dw = M_Fabs(A - _MaxAngle);
				fp32 Dist1 = dw * _RA.Length() * 0.5f;

				_CI1.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState1.m_Position);
				_CI1.m_iRB1 = _iRB1;
				_CI1.m_iRB2 = 0;
				_CI1.m_Normal = NN1;
				_CI1.m_BiasVelocity = M_VZero();
				_CI1.m_Distance = Dist1;

				return true;

			}			
		}
		else
		{
			fp32 A = _PI - M_Fabs(PlanarAngle);

			if (A > _MinAngle)
			{
				fp32 dw = M_Fabs(A - _MinAngle);
				fp32 Dist1 = dw * _RA.Length() * 0.5f;

				_CI1.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState1.m_Position);
				_CI1.m_iRB1 = _iRB1;
				_CI1.m_iRB2 = 0;
				_CI1.m_Normal = M_VNeg(NN1);
				_CI1.m_BiasVelocity = M_VZero();
				_CI1.m_Distance = Dist1;

				return true;
			}			
		}

		return false;
	}



	M_INLINE static bool GenerateAngleConstraintContact(int _iRB1, int _iRB2, 
														const CVec4Dfp32& _Axis1,
														const CVec4Dfp32& _Axis2,
														const CWD_RigidBodyState& _RBState1, const CWD_RigidBodyState& _RBState2,
														const CVec4Dfp32& _RA, const CVec4Dfp32& _RB, 
														fp32 _MinAngle, fp32 _MaxAngle,
														fp32 _RelativeAngle,
														CWD_ContactInfo& _CI1, CWD_ContactInfo& _CI2)
	{

		const CMat4Dfp32& T1 = _RBState1.GetTransform();
		const CMat4Dfp32& T2 = _RBState2.GetTransform();

		vec128 WorldAnchorPoint1 = M_VMulMat(_RA, T1);
		vec128 WorldAnchorPoint2 = M_VMulMat(_RB, T2);

		vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);
		vec128 LocalAnchorPoint2 = M_VSub(WorldAnchorPoint2, T2.r[3]);

		// TODO: Hmmm, här förutsätter man att det är 1:an som är axeln....
		vec128 Axis1 = M_VNrm3(_Axis1);
		Axis1 = M_VMulMat(Axis1, T1);

		vec128 Axis2 = M_VNrm3(_Axis2);
		Axis2 = M_VMulMat(Axis2, T2);

		vec128 Axis = M_VMul(M_VAdd(Axis1, Axis2), M_VScalar(0.5f));

		// TODO: !!!! __LocalRAAngle SKA VARA EN VEKTOR. SKULLE DET VARA DET SKULLE M_VSub ge fel resultat!!!!!!!!
		vec128 WorldRA = M_VSub(M_VMulMat(_RA, T1), T1.r[3]);
		vec128 WorldRB = M_VSub(M_VMulMat(_RB, T2), T2.r[3]);

		CAxisRotfp32 AxisRot((CVec3Dfp32& )Axis, 1.0f *_RelativeAngle * (1.0f / (2.0f * _PI)));
		CMat4Dfp32 RotMat;
		AxisRot.CreateMatrix(RotMat);		

		fp32 PlanarAngle = GetPlanarRotation(M_VMulMat(M_VNrm3(WorldRA), RotMat), M_VNrm3(WorldRB), Axis);
//		fp32 PlanarAngle = GetPlanarRotation(M_VNrm3(WorldRA), M_VNrm3(WorldRB), Axis);
//		PlanarAngle += _RelativeAngle;

		vec128 NN1 = M_VNrm3(M_VXpd(Axis, WorldRA));
		vec128 NN2 = M_VNrm3(M_VXpd(Axis, WorldRB));

		if (PlanarAngle >= 0.0f)
		{
			fp32 A = _PI - PlanarAngle;

			if (A > _MaxAngle)
			{
				fp32 dw = M_Fabs(A - _MaxAngle);
				fp32 Dist1 = dw * _RA.Length() * 0.5f;
				fp32 Dist2 = dw * _RB.Length() * 0.5f;

				_CI1.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState1.m_Position);
				_CI1.m_iRB1 = _iRB1;
				_CI1.m_iRB2 = _iRB2;
				_CI1.m_Normal = NN1;
				_CI1.m_BiasVelocity = M_VZero();
				_CI1.m_Distance = Dist1;

				_CI2.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState2.m_Position);
				_CI2.m_iRB1 = _iRB1;
				_CI2.m_iRB2 = _iRB2;
				_CI2.m_Normal = NN2;
				_CI2.m_BiasVelocity = M_VZero();
				_CI2.m_Distance = Dist2;

				return true;

			}			
		}
		else
		{
			fp32 A = _PI - M_Fabs(PlanarAngle);

			if (A > _MinAngle)
			{
				fp32 dw = M_Fabs(A - _MinAngle);
				fp32 Dist1 = dw * _RA.Length() * 0.5f;
				fp32 Dist2 = dw * _RB.Length() * 0.5f;

				_CI1.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState1.m_Position);
				_CI1.m_iRB1 = _iRB1;
				_CI1.m_iRB2 = _iRB2;
				_CI1.m_Normal = M_VNeg(NN1);
				_CI1.m_BiasVelocity = M_VZero();
				_CI1.m_Distance = Dist1;

				_CI2.m_PointOfCollision = M_VSelComp(3, M_VZero(), _RBState2.m_Position);
				_CI2.m_iRB1 = _iRB1;
				_CI2.m_iRB2 = _iRB2;
				_CI2.m_Normal = M_VNeg(NN2);
				_CI2.m_BiasVelocity = M_VZero();
				_CI2.m_Distance = Dist2;

				return true;

			}			

		}

		return false;
	}


#if 1
	static void GenerateContact(int _iRB1,
								const CWD_RigidBodyState& _RBState1,
								const CVec4Dfp32& _RA,
								const CVec4Dfp32& _ReferencePoint,
								const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI)
	{		
		const CMat4Dfp32& T1 = _RBState1.GetTransform();

		vec128 WorldAnchorPoint1 = M_VMulMat(_RA, T1);
		vec128 WorldAnchorPoint2 = _ReferencePoint;

		vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);

		vec128 JointError = M_VSub(WorldAnchorPoint1, WorldAnchorPoint2);
 		vec128 JointErrorLen = M_VLen3(JointError);
		vec128 JointErrorDir = CWD_DynamicsUtilFunctions::M_VSafeNrm3(JointError);

		vec128 Slack = M_VLd(0.005f, 0.005f, 0.005f, 0.0f); 
		vec128 Tmp = M_VSub(JointErrorLen, Slack);
		vec128 Bias = M_VMul(JointErrorDir, M_VMul(Tmp, _dtinv));
		Bias = M_VSel(M_VCmpGEMsk(M_VAbs(Tmp), M_VZero()), M_VZero(), Bias);

		Bias = M_VMul(M_VLdScalar(0.1f), Bias);

		vec128 AnchorVelocity1 = M_VAdd(M_VXpd(_RBState1.m_AngularVelocity, LocalAnchorPoint1), _RBState1.m_Velocity);

		vec128 RelativeVelocity = M_VAdd(AnchorVelocity1, Bias);
		vec128 VelocityDirection = CWD_DynamicsUtilFunctions::M_VSafeNrm3_2(RelativeVelocity);

		vec128 PointOfCollision = M_VMul(M_VAdd(WorldAnchorPoint1, WorldAnchorPoint2), M_VHalf());
//		vec128 PointOfCollision = WorldAnchorPoint2;

		_CI.m_PointOfCollision = M_VSelComp(3, M_VZero(), PointOfCollision);
		// TODO: AVBRYT OLIKA NEGERINGAR!!!
//		_CI.m_Normal = M_VNeg(VelocityDirection);
		_CI.m_Normal = VelocityDirection;
		_CI.m_BiasVelocity = Bias;
//		_CI.m_Distance = M_Fabs(foo[0]);
		_CI.m_Distance = 0.0f;
		_CI.m_iRB1 = _iRB1;
		_CI.m_iRB2 = 0;
	}

#endif

	static void GenerateFixContact(int _iRB1,
								   const CWD_RigidBodyState& _RBState1,
								   const CVec4Dfp32& _RA,
								   const CVec4Dfp32& _ReferencePoint,
								   const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI)
	{		
		const CMat4Dfp32& T1 = _RBState1.GetTransform();
//		const CMat4Dfp32& T2 = _RBState2.GetTransform();

		vec128 WorldAnchorPoint1 = M_VMulMat(_RA, T1);
		//vec128 WorldAnchorPoint2 = M_VMulMat(_RB, T2);

		vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);
		//vec128 LocalAnchorPoint2 = M_VSub(WorldAnchorPoint2, T2.r[3]);

		vec128 JointError = M_VSub(WorldAnchorPoint1, _ReferencePoint);
 		vec128 JointErrorLen = M_VLen3(JointError);
		vec128 JointErrorDir = CWD_DynamicsUtilFunctions::M_VSafeNrm3(JointError);

		vec128 Slack = M_VLd(0.005f, 0.005f, 0.005f, 0.0f); 
		vec128 Tmp = M_VSub(JointErrorLen, Slack);

		vec128 Bias = M_VZero();

		vec128 AnchorVelocity1 = M_VAdd(M_VXpd(_RBState1.m_AngularVelocity, LocalAnchorPoint1), _RBState1.m_Velocity);
		//vec128 AnchorVelocity2 = M_VAdd(M_VXpd(_RBState2.m_AngularVelocity, LocalAnchorPoint2), _RBState2.m_Velocity);

//		vec128 RelativeVelocity = M_VAdd(M_VSub(AnchorVelocity1, AnchorVelocity2), Bias);
		vec128 RelativeVelocity = M_VAdd(AnchorVelocity1, Bias);
		vec128 VelocityDirection = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelativeVelocity);

//		vec128 PointOfCollision = M_VMul(M_VAdd(WorldAnchorPoint1, _ReferencePoint), M_VHalf());
		vec128 PointOfCollision = _ReferencePoint;

		_CI.m_PointOfCollision = M_VSelComp(3, M_VZero(), PointOfCollision);
		// TODO: AVBRYT OLIKA NEGERINGAR!!!
		//_CI.m_Normal = M_VNeg(VelocityDirection);
		_CI.m_Normal = M_VNeg(JointErrorDir);
//		_CI.m_Normal = JointErrorDir;
		_CI.m_BiasVelocity = Bias;

		CVec4Dfp32 BLA = JointErrorLen;
 
		_CI.m_Distance = M_Fabs(BLA[0] * 0.5f);
		//_CI.m_Distance = 0.0f;
		_CI.m_iRB1 = _iRB1;
		_CI.m_iRB2 = 0;
	}

	static void GenerateContact(int _iRB1, int _iRB2,  
								const CWD_RigidBodyState& _RBState1, const CWD_RigidBodyState& _RBState2,
								const CVec4Dfp32& _RA, const CVec4Dfp32& _RB, 
								const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI)
	{		
		const CMat4Dfp32& T1 = _RBState1.GetTransform();
		const CMat4Dfp32& T2 = _RBState2.GetTransform();

		vec128 WorldAnchorPoint1 = M_VMulMat(_RA, T1);
		vec128 WorldAnchorPoint2 = M_VMulMat(_RB, T2);

		vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);
		vec128 LocalAnchorPoint2 = M_VSub(WorldAnchorPoint2, T2.r[3]);

		vec128 JointError = M_VSub(WorldAnchorPoint1, WorldAnchorPoint2);
 		vec128 JointErrorLen = M_VLen3(JointError);
		vec128 JointErrorDir = CWD_DynamicsUtilFunctions::M_VSafeNrm3(JointError);

		CVec4Dfp32 fooo = JointError;
		if (fooo.Length() < 0.0001f)
		{
			int breakme = 0;
		}

#if 1
		vec128 Slack = M_VLd(0.005f, 0.005f, 0.005f, 0.0f); 
		vec128 Tmp = M_VSub(JointErrorLen, Slack);
		vec128 Bias = M_VMul(JointErrorDir, M_VMul(Tmp, _dtinv));
		Bias = M_VSel(M_VCmpGEMsk(M_VAbs(Tmp), M_VZero()), M_VZero(), Bias);
#else
//		vec128 Slack = M_VLd(0.005f, 0.005f, 0.005f, 0.005f);
		vec128 Slack = M_VLd(0.00f, 0.00f, 0.00f, 0.00f);
		vec128 Tmp = M_VSub(JointErrorLen, Slack);
		vec128 Bias = M_VMul(JointErrorDir, M_VMul(Tmp, _dtinv));
		//Bias = M_VSel(M_VCmpGEMsk(M_VAbs(Tmp), M_VZero()), M_VZero(), Bias);
#endif
//		CVec4Dfp32 foo = JointErrorLen;

		Bias = M_VMul(M_VLdScalar(0.5f), Bias);
		Bias = M_VZero();

		CVec4Dfp32 Bar = Bias;
		if (Bar.Length() > 100.0f)
		{
			int breakme = 0;

		}

		vec128 AnchorVelocity1 = M_VAdd(M_VXpd(_RBState1.m_AngularVelocity, LocalAnchorPoint1), _RBState1.m_Velocity);
		vec128 AnchorVelocity2 = M_VAdd(M_VXpd(_RBState2.m_AngularVelocity, LocalAnchorPoint2), _RBState2.m_Velocity);

		vec128 RelativeVelocity = M_VAdd(M_VSub(AnchorVelocity1, AnchorVelocity2), Bias);
		vec128 VelocityDirection = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelativeVelocity);

		vec128 PointOfCollision = M_VMul(M_VAdd(WorldAnchorPoint1, WorldAnchorPoint2), M_VHalf());

		_CI.m_PointOfCollision = M_VSelComp(3, M_VZero(), PointOfCollision);
		// TODO: AVBRYT OLIKA NEGERINGAR!!!
		//_CI.m_Normal = M_VNeg(VelocityDirection);
		_CI.m_Normal = M_VNeg(JointErrorDir);
		_CI.m_BiasVelocity = Bias;

		CVec4Dfp32 BLA = JointErrorLen;
 
		_CI.m_Distance = M_Fabs(BLA[0] * 0.5f);
		//_CI.m_Distance = 0.0f;
		_CI.m_iRB1 = _iRB1;
		_CI.m_iRB2 = _iRB2;
	}

	static bool GenerateMinDistanceContact(int _iRB1, int _iRB2, 
										   const CWD_RigidBodyState& _RBState1, const CWD_RigidBodyState& _RBState2,
										   const CVec4Dfp32& _RA, const CVec4Dfp32& _RB, 
										   fp32 _MinDistance,
										   const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI1, CWD_ContactInfo& _CI2)
	{

		const CMat4Dfp32 &T1 = _RBState1.GetTransform();
		const CMat4Dfp32 &T2 = _RBState2.GetTransform();

		CVec3Dfp32 p1 = CVec3Dfp32(_RA[0], _RA[1], _RA[2]);
		CVec3Dfp32 p2 = CVec3Dfp32(_RB[0], _RB[1], _RB[2]);
		p1.MultiplyMatrix3x3(T1);
		p2.MultiplyMatrix3x3(T2);

		CVec3Dfp32 wp1 = CVec3Dfp32(_RA[0], _RA[1], _RA[2]) * T1;
		CVec3Dfp32 wp2 = CVec3Dfp32(_RB[0], _RB[1], _RB[2]) * T2;

		CVec3Dfp32 RelativePosition = wp1 - wp2;

		CVec3Dfp32 tmp, tmp2;

		CVec3Dfp32 wv1, wv2;
		_RBState1.GetAngularVelocity().CrossProd(p1, tmp);
		wv1 = _RBState1.GetVelocity() + tmp;

		_RBState2.GetAngularVelocity().CrossProd(p2, tmp);
		wv2 = _RBState2.GetVelocity() + tmp;

		CVec3Dfp32 RelativeVelocity = wv1 - wv2;
		CVec3Dfp32 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * (1.0f / _dtinv[0]);

		fp32 Distance = ExtrapolatedRelativePosition.Length();
		fp32 MinDistance = _MinDistance;

		CVec3Dfp32 NewVel(0);
		CVec3Dfp32 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MinDistance)
		{
			CVec3Dfp32 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			//			NewRelativePosition -= tmp * (Distance - MinDistance);
			NewRelativePosition *= MinDistance / ExtrapolatedRelativePosition.Length();
		}
		else return false;

		CVec3Dfp32 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (_dtinv[0]);

		CVec3Dfp32 RelVel = NewRelativeVelocity - RelativeVelocity;
		//		CVec3Dfp64 N = m_pRigidBody->GetVelocity() - NewVel;
		fp32 Vel = RelVel.Length();

		CVec3Dfp32 N = RelVel;
		N.Normalize();

		CVec3Dfp32 P = (wp1 + wp2) * 0.5f;

		_CI1.m_PointOfCollision = CVec4Dfp32(wp1[0], wp1[1], wp1[2], 0.0f);
//		_CI1.m_PointOfCollision = _RBState1.m_Position;
		_CI1.m_Normal = -CVec4Dfp32(N[0], N[1], N[2], 0.0f);
		_CI1.m_BiasVelocity = CVec4Dfp32(0.0f);
		//_CI1.m_BiasVelocity = CWD_RigidBodyState::To4D(RelVel * 0.05f);
		//_CI1.m_Distance = (NewRelativePosition - RelativePosition).Length();
		_CI1.m_Distance = 0.0f;
		_CI1.m_iRB1 = _iRB1;
		_CI1.m_iRB2 = 0;

		
		_CI2.m_PointOfCollision = CVec4Dfp32(wp2[0], wp2[1], wp2[2], 0.0f);
//		_CI2.m_PointOfCollision = _RBState2.m_Position;
		_CI2.m_Normal = CVec4Dfp32(N[0], N[1], N[2], 0.0f);
		_CI2.m_BiasVelocity = CVec4Dfp32(0.0f);
		_CI2.m_BiasVelocity = CWD_RigidBodyState::To4D(RelVel * -0.05f);
		//_CI1.m_Distance = (NewRelativePosition - RelativePosition).Length();
		_CI2.m_Distance = 0.0f;
		_CI2.m_iRB1 = _iRB2;
		_CI2.m_iRB2 = 0;

		return true;

	}


	static bool GenerateMaxDistanceContact(int _iRB1,
										   const CWD_RigidBodyState& _RBState1, 
										   fp32 _MaxDistance,
										   const CVec4Dfp32& _WorldReferencePoint,
										   const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI)
	{
		const CMat4Dfp32 &T1 = _RBState1.GetTransform();

		CVec3Dfp32 wp1 = CWD_RigidBodyState::To3D(_WorldReferencePoint);
		CVec3Dfp32 wp2 = _RBState1.GetPosition();

		CVec3Dfp32 RelativePosition = wp1 - wp2;
		CVec3Dfp32 RelativeVelocity = -_RBState1.GetVelocity();
		CVec3Dfp32 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * (1.0f / _dtinv[0]);

		fp32 Distance = ExtrapolatedRelativePosition.Length();
		fp32 MaxDistance = _MaxDistance;

		CVec3Dfp32 NewVel(0.0f);
		CVec3Dfp32 NewRelativePosition = ExtrapolatedRelativePosition;


		if (Distance > MaxDistance)
		{
			CVec3Dfp32 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return false;

		CVec3Dfp32 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (_dtinv[0]);

		CVec3Dfp32 NormalVel = NewRelativeVelocity - RelativeVelocity;
		fp32 Vel = NormalVel.Length();

		if (Vel < 0.001f)
		{
			return false;
		}

		CVec3Dfp32 N = NormalVel;
		N.Normalize();

		_CI.m_PointOfCollision = _RBState1.m_Position;
		_CI.m_Normal = -CVec4Dfp32(N[0], N[1], N[2], 0.0f);
		_CI.m_BiasVelocity = CWD_RigidBodyState::To4D_LastZero(NormalVel * 1.0f);
		//_CI.m_BiasVelocity = CVec4Dfp32(0.0f);
		//_CI1.m_Distance = (NewRelativePosition - RelativePosition).Length();
//		_CI.m_Distance = RelativePosition.Length();
		_CI.m_Distance = 0.0f;
		_CI.m_iRB1 = _iRB1;
		_CI.m_iRB2 = 0;

		return true;

	}


	static bool GenerateMinDistanceContact3(int _iRB1, int _iRB2,
										   const CWD_RigidBodyState& _RBState1, const CWD_RigidBodyState& _RBState2,
										   fp32 _MaxDistance,
										   const CVec4Dfp32& _dtinv, CWD_ContactInfo& _CI1, CWD_ContactInfo& _CI2)
	{
		const CMat4Dfp32 &T1 = _RBState1.GetTransform();
		const CMat4Dfp32 &T2 = _RBState2.GetTransform();

		CVec3Dfp32 wp1 = _RBState1.GetPosition();
		CVec3Dfp32 wp2 = _RBState2.GetPosition();

		CVec3Dfp32 RelativePosition = wp1 - wp2;
		CVec3Dfp32 RelativeVelocity = _RBState2.GetVelocity() - _RBState1.GetVelocity();
		CVec3Dfp32 ExtrapolatedRelativePosition = RelativePosition + RelativeVelocity * (1.0f / _dtinv[0]);

		fp32 Distance = ExtrapolatedRelativePosition.Length();
		fp32 MaxDistance = _MaxDistance;

		CVec3Dfp32 NewVel(0.0f);
		CVec3Dfp32 NewRelativePosition = ExtrapolatedRelativePosition;

		if (Distance < MaxDistance)
		{
			CVec3Dfp32 tmp = ExtrapolatedRelativePosition;
			tmp.Normalize();
			NewRelativePosition *= MaxDistance / ExtrapolatedRelativePosition.Length();
		}
		else return false;

		CVec3Dfp32 NewRelativeVelocity = (NewRelativePosition - RelativePosition) * (_dtinv[0]);

		CVec3Dfp32 NormalVel = NewRelativeVelocity - RelativeVelocity;
		fp32 Vel = NormalVel.Length();

		if (Vel < 0.001f)
		{
			return false;
		}

		CVec3Dfp32 N = NormalVel;
		N.Normalize();

		_CI1.m_PointOfCollision = _RBState1.m_Position;
		_CI1.m_Normal = CVec4Dfp32(N[0], N[1], N[2], 0.0f);
		//_CI1.m_BiasVelocity = CWD_RigidBodyState::To4D_LastZero(NormalVel * 0.2f);
		_CI1.m_BiasVelocity = CVec4Dfp32(0.0f);
		_CI1.m_Distance = 0.0f;
		_CI1.m_iRB1 = _iRB1;
		_CI1.m_iRB2 = 0;

		_CI2.m_PointOfCollision = _RBState2.m_Position;
		_CI2.m_Normal = -CVec4Dfp32(N[0], N[1], N[2], 0.0f);
		//_CI2.m_BiasVelocity = CWD_RigidBodyState::To4D_LastZero(NormalVel * 0.2f);
		_CI2.m_BiasVelocity = CVec4Dfp32(0.0f);
		_CI2.m_Distance = 0.0f;
		_CI2.m_iRB1 = _iRB2;
		_CI2.m_iRB2 = 0;

		return true;

	}

};

/*
	Joints/Constraints
 */

 CWD_MaxDistanceConstraint::CWD_MaxDistanceConstraint(int _iRB1, const CVec4Dfp32& _WorldReferencePoint, fp32 _MaxDistance)
 {
	 m_iRB1 = _iRB1;
	 m_WorldReferencePoint = _WorldReferencePoint;
	 m_MaxDistance = _MaxDistance;
 }

 fp32 CWD_MaxDistanceConstraint::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
 {
	 M_ASSERT(false, "!");

	 return -1.0f;
 }

 int CWD_MaxDistanceConstraint::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
 {
	 if (pContactInfo.Len() < 1) return 0;

	 vec128 dtinv = M_VRcp(_dt);

	 const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

	 CWD_ContactInfo& CI = pContactInfo[0];

	 if (CWD_ConstraintUtil::GenerateMaxDistanceContact(m_iRB1, RBState1, m_MaxDistance, m_WorldReferencePoint, dtinv, CI))
	 {
		 return 1;
	 }

	 return 0;
 }

 /*
	CWD_BallJoint
 */

CWD_BallJoint::CWD_BallJoint(int _iRB1, int _iRB2, const CVec4Dfp32& _RA, const CVec4Dfp32& _RB, const CVec4Dfp32& _RA_Angle, const CVec4Dfp32& _RB_Angle)
{
	m_iRB1 = _iRB1;
	m_iRB2 = _iRB2;
	m_RA = _RA;
	m_RB = _RB;
	m_RA_Angle = _RA_Angle;
	m_RB_Angle = _RB_Angle;

	//m_bUseSolve = true;
}

fp32 CWD_BallJoint::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
{
	//CVec4Dfp32 E = CWD_BallJointSolver::Solve(_World, _dt, m_iRB1, m_iRB2, m_RA, m_RB);
	//return E[0];

	CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

	/*
	CVec4Dfp32 tmp = _dt;
	fp32 dt = tmp[0];
	*/
	CWD_LegacyConstraint::SolveBallJoint(RBState1, RBState2, m_RA, m_RB, _dt);

/*	RBState1.m_Velocity = M_VSub(RBState1.m_Velocity, M_VMul(RBState1.m_Velocity, M_VLdScalar(0.005f)));
	RBState1.m_AngularVelocity = M_VSub(RBState1.m_AngularVelocity, M_VMul(RBState1.m_AngularVelocity, M_VLdScalar(0.005f)));

	RBState2.m_Velocity = M_VSub(RBState2.m_Velocity, M_VMul(RBState2.m_Velocity, M_VLdScalar(0.005f)));
	RBState2.m_AngularVelocity = M_VSub(RBState2.m_AngularVelocity, M_VMul(RBState2.m_AngularVelocity, M_VLdScalar(0.005f)));
*/
	return 0.0f;

}

#if 0
int CWD_BallJoint::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 3) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CMat4Dfp32& T1 = _World.GetTransform(m_iRB1);
	const CMat4Dfp32& T2 = _World.GetTransform(m_iRB2);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	const CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

	vec128 WorldAnchorPoint1 = M_VMulMat(m_RA, T1);
	vec128 WorldAnchorPoint2 = M_VMulMat(m_RB, T2);

	vec128 LocalAnchorPoint1 = M_VSub(WorldAnchorPoint1, T1.r[3]);
	vec128 LocalAnchorPoint2 = M_VSub(WorldAnchorPoint2, T2.r[3]);

	vec128 LocalRAAngle = M_VSub(M_VMulMat(m_RA_Angle, T1), T1.r[3]);
	vec128 LocalRBAngle = M_VSub(M_VMulMat(m_RB_Angle, T2), T2.r[3]);

	vec128 JointError = M_VSub(WorldAnchorPoint1, WorldAnchorPoint2);
	vec128 JointErrorLen = M_VLen3(JointError);
	vec128 JointErrorDir = CWD_DynamicsUtilFunctions::M_VSafeNrm3(JointError);

	vec128 Slack = M_VLd(0.005f, 0.005f, 0.005f, 0.0f);
	vec128 Tmp = M_VSub(JointErrorLen, Slack);
	vec128 Bias = M_VMul(JointErrorDir, M_VMul(Tmp, dtinv));
	Bias = M_VSel(M_VCmpGEMsk(M_VAbs(Tmp), M_VZero()), M_VZero(), Bias);

	Bias = M_VMul(M_VLdScalar(0.001f), Bias);
	Bias = M_VZero();

	vec128 AnchorVelocity1 = M_VAdd(M_VXpd(RBState1.m_AngularVelocity, LocalAnchorPoint1), RBState1.m_Velocity);
	vec128 AnchorVelocity2 = M_VAdd(M_VXpd(RBState2.m_AngularVelocity, LocalAnchorPoint2), RBState2.m_Velocity);
 
	vec128 RelativeVelocity = M_VAdd(M_VSub(AnchorVelocity1, AnchorVelocity2), Bias);
	vec128 VelocityDirection = CWD_DynamicsUtilFunctions::M_VSafeNrm3_2(RelativeVelocity);

	vec128 PointOfCollision = M_VMul(M_VAdd(WorldAnchorPoint1, WorldAnchorPoint2), M_VHalf());

	int nContacts = 0;

	vec128 DaAxis = CWD_DynamicsUtilFunctions::M_VSafeNrm3_2(M_VXpd(LocalRAAngle, LocalRBAngle));
	fp32 Angle = GetPlanarRotation(LocalRAAngle, LocalRBAngle, DaAxis);
	fp32 Angle2 = GetPlanarRotation(LocalAnchorPoint1, LocalAnchorPoint2, DaAxis) + _PI / 2.0f;
	Angle2 = fmodf(Angle2 + 4 * _PI, _PI);
	fp32 AngleDiff = Angle - Angle2;


	fp32 Kalle = fmodf(-4.0f, 3.0f);

	CVec4Dfp32 tmp4 = M_VDp3(M_VNrm3(LocalRAAngle), M_VNrm3(LocalRBAngle));
	fp32 Angle__ = M_ACos(tmp4[0]);

	CVec4Dfp32 tmp3= M_VDp3(M_VNrm3(LocalAnchorPoint1), M_VNrm3(LocalAnchorPoint2));
	fp32 Angle3 = M_ACos(tmp3[0]) + _PI / 2.0f;
	Angle3 = fmodf(Angle3 + 4 * _PI, _PI);	
	fp32 AngleDiff2 = Angle__ - Angle3;

//	fp32 AngleDiff = fmodf(Angle - Angle2, 3.14159265);

	//fp32 MinAngle = _PI / 2.0f; 
	fp32 MinAngle = _PI * 0.98f;
	//fp32 MinAngle = _PI * 0.75f;
	//fp32 MinAngle = _PI * 0.99f;

#if 0
	if (Angle < MinAngle)
	{
		vec128 N1 = M_VNrm3(M_VXpd(DaAxis, LocalAnchorPoint1));
		vec128 N2 = M_VNrm3(M_VXpd(DaAxis, LocalAnchorPoint2));

		// Arclength approximation of error
		fp32 dw = M_Fabs(MinAngle - Angle);
		fp32 Dist1 = dw * m_RA.Length();
		fp32 Dist2 = dw * m_RB.Length();

		CWD_ContactInfo& CI1 = pContactInfo[nContacts];
		CI1.m_PointOfCollision = M_VSelComp(3, M_VZero(), RBState1.m_Position);
		CI1.m_iRB1 = m_iRB1;
		CI1.m_iRB2 = m_iRB2;
		CI1.m_Normal = N1;
		CI1.m_BiasVelocity = M_VZero();
		CI1.m_Distance = Dist1;
		nContacts++;

		CWD_ContactInfo& CI2 = pContactInfo[nContacts];
		CI2.m_PointOfCollision = M_VSelComp(3, M_VZero(), RBState2.m_Position);
		CI2.m_iRB1 = m_iRB1;
		CI2.m_iRB2 = m_iRB2;
		CI2.m_Normal = N2;
		CI2.m_BiasVelocity = M_VZero();
		CI2.m_Distance = Dist2;
		nContacts++;
	}
#endif

	CWD_ContactInfo& CI = pContactInfo[nContacts];
	CI.m_PointOfCollision = M_VSelComp(3, M_VZero(), PointOfCollision);
	CI.m_iRB1 = m_iRB1;
	CI.m_iRB2 = m_iRB2;
	// TODO: AVBRYT OLIKA NEGERINGAR!!!
	CI.m_Normal = M_VNeg(VelocityDirection);
	CI.m_BiasVelocity = Bias;
	CI.m_Distance = 0.0f;

	CVec4Dfp32 Foo = JointError;
	if (Foo[0] > 0.0f)
	{
		//CI.m_Distance = M_Fabs(Foo[0] * 0.1f);


	}

	nContacts++;

	return nContacts;
}
#else

int CWD_BallJoint::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 3) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	const CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

	int nContacts = 1;

	CWD_ContactInfo& CI1 = pContactInfo[0];

	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA, m_RB, dtinv, CI1);

	CWD_ContactInfo& CI2 = pContactInfo[1];
	CWD_ContactInfo& CI3 = pContactInfo[2];

	/*
	if (CWD_ConstraintUtil::GenerateAngleConstraintContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA, m_RB, m_RA_Angle, m_RB_Angle, CI2, CI3))
	{
		//nContacts += 2;
	}*/

	fp32 MinDist = 2.0f * (CWD_RigidBodyState::To3D(m_RA).Length() + CWD_RigidBodyState::To3D(m_RB).Length()) * 0.99;

	//fp32 RelativeAngle = 0.0f;
	fp32 RelativeAngle = 0.25f;

	CMat4Dfp32 T = RBState1.GetTransform();
	CMat4Dfp32 Tinv;
	T.InverseOrthogonal(Tinv);

	CMat4Dfp32 T2 = RBState2.GetTransform();

	CVec3Dfp32 HingeAxis = (CWD_RigidBodyState::To3D(m_RA) * T) / (RBState1.GetPosition() - RBState2.GetPosition());
	if (HingeAxis.Length() > 0.001f)
	{
		HingeAxis.Normalize();

		HingeAxis = CVec3Dfp32(0,0,1);


//		CAxisRotfp32 AxisRot(HingeAxis, RelativeAngle / (2.0 * 3.14159265));
		CAxisRotfp32 AxisRot(HingeAxis, RelativeAngle );
		CMat4Dfp32 AxisRotMatrix;
		AxisRot.CreateMatrix(AxisRotMatrix);


		CVec3Dfp32 HingeMid = CWD_RigidBodyState::To3D(m_RA) * T;

		CVec3Dfp32 LocalPos2 = -CWD_RigidBodyState::To3D(m_RA);
		LocalPos2 *= T;
		LocalPos2 -= HingeMid;
		LocalPos2 *= AxisRotMatrix;
		LocalPos2 += HingeMid;
		LocalPos2 *= Tinv;


/*		if (CWD_ConstraintUtil::GenerateMinDistanceContact(m_iRB1, m_iRB2, RBState1, RBState2, CWD_RigidBodyState::To4D(LocalPos2), -m_RB, MinDist, dtinv, CI2))
		{
			nContacts += 1;
		}*/

	}


	return nContacts;
}

#endif

 /*
	CWD_BallJointWorld
 */

CWD_BallJointWorld::CWD_BallJointWorld(int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _RA_Angle, const CVec4Dfp32& _WorldReferencePoint)
{
	m_iRB1 = _iRB1;
	m_RA = _RA;
	m_RA_Angle = _RA_Angle;
	m_WorldReferencePoint = _WorldReferencePoint;

	m_bUseSolve = true;
}

fp32 CWD_BallJointWorld::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
{
	CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

//	vec128 tmp = _dt;
	vec128 dt = _dt;
	CWD_LegacyConstraint::SolveBallJointWorld(RBState1, m_WorldReferencePoint.v, m_RA.v, dt);

	RBState1.m_Velocity = M_VSub(RBState1.m_Velocity, M_VMul(RBState1.m_Velocity, M_VLdScalar(0.005f)));
	RBState1.m_AngularVelocity = M_VSub(RBState1.m_AngularVelocity, M_VMul(RBState1.m_AngularVelocity, M_VLdScalar(0.005f)));

	return 0.0f;
}

int CWD_BallJointWorld::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 3) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

	int nContacts = 1;

	CWD_ContactInfo& CI1 = pContactInfo[0];

//	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA, m_RB, dtinv, CI1);
	CWD_ConstraintUtil::GenerateContact(m_iRB1, RBState1, m_RA, m_WorldReferencePoint, dtinv, CI1);

	CWD_ContactInfo& CI2 = pContactInfo[1];
	CWD_ContactInfo& CI3 = pContactInfo[2];
/*
	if (CWD_ConstraintUtil::GenerateAngleConstraintContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA, m_RB, m_RA_Angle, m_RB_Angle, CI2, CI3))
	{
//		nContacts += 2;
	}
*/
	return nContacts;
}


/*
	Solvers
 */

vec128 CWD_BallJointSolver::Solve(CWD_DynamicsWorld& _World, vec128 _dt, int _iRB1, int _iRB2, vec128 _Point1, vec128 _Point2)
{
	M_ASSERT(false, "!");
	return M_VZero();
}

/*
  CWD_HingeJoint
 */

CWD_HingeJoint::CWD_HingeJoint(CWD_DynamicsWorld *_pWorld, int _iRB1, int _iRB2, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _AngleAxisA, const CVec4Dfp32& _RB, const CVec4Dfp32& _AxisB, const CVec4Dfp32& _AngleAxisB, fp32 _RelativeAngle, fp32 _MaxAngle)
{
	m_iRB1 = _iRB1;
	m_iRB2 = _iRB2;
	m_RA = _RA;
	m_AxisA = _AxisA * 0.5f;
	m_RB = _RB;
	m_AxisB = _AxisB * 0.5f;

	m_AngleAxisA = _AngleAxisA;
	m_AngleAxisB = _AngleAxisB;

	m_AxisA[3] = 0.0f;
	m_AxisB[3] = 0.0f;

	m_RelativeAngle = -_RelativeAngle;
//	m_RelativeAngle = 3.1415f * 0.54f;
	m_MaxAngle = _MaxAngle;

#ifdef WDYNAMICS_LEGACYCONSTRAINTS
	m_bUseSolve = true;	

	const CMat4Dfp32& T1 = _pWorld->GetTransform(m_iRB1);
	const CMat4Dfp32& T2 = _pWorld->GetTransform(m_iRB2);

	CVec3Dfp32 Ref1 = -((const CVec3Dfp32 &) m_RA);
	CVec3Dfp32 Ref2 = -((const CVec3Dfp32 &) m_RB);

	CVec3Dfp32 Kalle1 = ((const CVec3Dfp32 &) m_RA) * T1;
	CVec3Dfp32 Kalle2 = ((const CVec3Dfp32 &) m_RB) * T2;

	fp32 fooo = Ref1.Length() + Ref2.Length();
	fp32 baaar = Kalle1.Distance(Kalle2);

	Ref1.Normalize();
	Ref2.Normalize();

	Ref1 *= 0.01f;
	Ref2 *= 0.01f;

	CVec3Dfp32 Ref1World = Ref1 * T1;
	CVec3Dfp32 Ref2World = Ref2 * T2;
	CVec3Dfp32 Anchor = ((const CVec3Dfp32 &) m_RA) * T1;

	fp32 b = Ref1World.Distance(Anchor);
	fp32 c = Ref2World.Distance(Anchor);

	fp32 Arne1 = Kalle1.Distance(Anchor);
	fp32 Arne2 = Kalle2.Distance(Anchor);

	fp32 MinDistance = b * b + c * c - 2.0f * b * c * M_Cos(_PI - m_MaxAngle * 0.5f);
	m_MinDistance = M_Sqrt(MinDistance);
	//m_MinDistance = 1.1f;

/*
	CVec3Dfp32 RefA = CWD_RigidBodyState::To3D(-m_RA);
	CVec3Dfp32 RefB = CWD_RigidBodyState::To3D(-m_RB);
	RefA.Normalize();
	RefB.Normalize();

	RefA *= 0.01f;
	RefB *= 0.01f;
*/
	CVec3Dfp32 HingeAxis = CWD_RigidBodyState::To3D(m_AxisB);
	HingeAxis.Normalize();
	HingeAxis.MultiplyMatrix3x3(T2);

	CAxisRotfp32 AxisRot(HingeAxis, m_RelativeAngle / (2.0f * 3.14159265f));
	AxisRot.CreateMatrix(m_RelativeMatrix);

	//CVec3Dfp32 HingeMid = CWD_RigidBodyState::To3D(m_RB) * T2;

	CMat4Dfp32 T2Inv;
	T2.InverseOrthogonal(T2Inv);

	vec128 v_small = M_VConst(-0.01f,-0.01f,-0.01f,1.0f);
	vec128 v_mask = M_VConst_u32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0);
	vec128 v_one = M_VOne();
	vec128 LocalPos2 = M_VSelMsk(v_mask,M_VMul(v_small,M_VNrm3(m_RB)),v_one);

	//Get inverted inertia tensors
	vec128 r0,r1,r2,r3;

	r0 = T2.r[0]; r1 = T2.r[1];
	r2 = T2.r[2]; r3 = T2.r[3];
	M_VTranspose4x4(r0,r1,r2,r3);

	vec128 v_hingemid = M_VDp4x4(m_RB, r0, m_RB, r1, m_RB, r2, m_RB, r3);
	LocalPos2 = M_VDp4x4(LocalPos2, r0, LocalPos2, r1, LocalPos2, r2, LocalPos2, r3);
	LocalPos2 = M_VSub(LocalPos2,v_hingemid);

	r0 = m_RelativeMatrix.r[0]; r1 = m_RelativeMatrix.r[1];
	r2 = m_RelativeMatrix.r[2]; r3 = m_RelativeMatrix.r[3];
	M_VTranspose4x4(r0,r1,r2,r3);
	LocalPos2 = M_VDp4x4(LocalPos2, r0, LocalPos2, r1, LocalPos2, r2, LocalPos2, r3);

	LocalPos2 = M_VAdd(LocalPos2,v_hingemid);

	r0 = T2Inv.r[0]; r1 = T2Inv.r[1];
	r2 = T2Inv.r[2]; r3 = T2Inv.r[3];
	M_VTranspose4x4(r0,r1,r2,r3);
	LocalPos2 = M_VDp4x4(LocalPos2, r0, LocalPos2, r1, LocalPos2, r2, LocalPos2, r3);

	m_LocalPos2 = LocalPos2; 

#endif
}

fp32 CWD_HingeJoint::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
{

	CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

//	CVec4Dfp32 tmp = _dt;
//	fp32 dt = tmp[0];

	CWD_LegacyConstraint::SolveBallJoint(RBState1, RBState2, M_VAdd(m_RA,m_AxisA), M_VAdd(m_RB,m_AxisB), _dt);
	CWD_LegacyConstraint::SolveBallJoint(RBState1, RBState2, M_VSub(m_RA,m_AxisA), M_VSub(m_RB,m_AxisB), _dt);

/*	fp64 b = Ref1World.Distance(Anchor);
	fp64 c = Ref2World.Distance(Anchor);

	fp64 MinDistance = b * b + c * c - 2 * b * c * cos(_PI - _MaxAngle);
	MinDistance = M_Sqrt(MinDistance);
	Constraint.m_ScalarParams[CHingeConstraintSolver::MINDISTANCE] = MinDistance;
*/
//	fp32 MinDistance = 2.0f * (CWD_RigidBodyState::To3D(m_RA).Length() + CWD_RigidBodyState::To3D(m_RB).Length()) * 0.7f;



//	fp32 RelativeAngle = 0.6f * 2.0f * 3.141592f;
//	fp32 RelativeAngle = m_RelativeAngle;
	//fp32 RelativeAngle = 0.0f;

//	const CMat4Dfp32 &T = RBState2.GetTransform();

//	CMat4Dfp32 Tinv;
//	T.InverseOrthogonal(Tinv);
/*
	CVec3Dfp32 Foo = CWD_RigidBodyState::To3D(-m_RA) * RBState1.GetTransform();
*/
	CVec3Dfp32 RefA = CWD_RigidBodyState::To3D(-m_RA);
//	CVec3Dfp32 RefB = CWD_RigidBodyState::To3D(-m_RB);
	RefA.Normalize();
//	RefB.Normalize();

	RefA *= 0.01f;
//	RefB *= 0.01f;

/*	CVec3Dfp32 HingeAxis = CWD_RigidBodyState::To3D(m_AxisB);
	HingeAxis.Normalize();
	HingeAxis.MultiplyMatrix3x3(T);

	CAxisRotfp32 AxisRot(HingeAxis, RelativeAngle / (2.0 * 3.14159265));
	CMat4Dfp32 AxisRotMatrix;
	AxisRot.CreateMatrix(AxisRotMatrix);
*/
	/*
	CVec3Dfp32 HingeMid = CWD_RigidBodyState::To3D(m_RB) * T;

	CVec3Dfp32 LocalPos2 = RefB;
	LocalPos2 *= T;
	LocalPos2 -= HingeMid;
	LocalPos2 *= m_RelativeMatrix;
	LocalPos2 += HingeMid;
	LocalPos2 *= Tinv;
*/
//	CWD_LegacyConstraint::SolveMinDistance(RBState1, RBState2, CWD_RigidBodyState::To3D(-m_RA), CWD_RigidBodyState::To3D(-m_RB), MinDistance, dt);
//	CWD_LegacyConstraint::SolveMinDistance(RBState1, RBState2, CWD_RigidBodyState::To3D(-m_RA), LocalPos2, m_MinDistance, dt);

	vec128 v_epsilon = M_VScalar(-0.01f);
	CWD_LegacyConstraint::SolveMinDistance(RBState1, RBState2, M_VMul(M_VNrm3(m_RA),v_epsilon),m_LocalPos2, M_VLdScalar(m_MinDistance), _dt);

	return 0.0f;
}

int CWD_HingeJoint::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 4) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	const CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

	int nContacts = 0;

	CWD_ContactInfo& CI1 = pContactInfo[0];
	CWD_ContactInfo& CI2 = pContactInfo[1];

	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA + m_AxisA, m_RB + m_AxisB, dtinv, CI1);
	nContacts++;
	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA - m_AxisA, m_RB - m_AxisB, dtinv, CI2);
	nContacts++;

	CWD_ContactInfo& CI3 = pContactInfo[2];
	CWD_ContactInfo& CI4 = pContactInfo[3];
	
	fp32 MinDist = 2.0f * (CWD_RigidBodyState::To3D(m_RA).Length() + CWD_RigidBodyState::To3D(m_RB).Length()) * 0.99;

	CVec4Dfp32 Axis = m_AxisA;
	Axis.Normalize();
	CAxisRotfp32 Rot(CVec3Dfp32(Axis[0], Axis[1], Axis[2]) , 0.125f / 1.0f);
//	CAxisRotfp32 Rot(CVec3Dfp32(Axis[0], Axis[1], Axis[2]) , -0.0f / 1.0f);
	CMat4Dfp32 T;
	Rot.CreateMatrix(T);

	//CVec4Dfp32 Foo = m_AngleAxisA * T;
	CVec4Dfp32 Foo = m_AngleAxisA;
	//CVec4Dfp32 Bar = CVec4Dfp32(Foo[0], Foo[1], Foo[2], 1.0f;);

	CMat4Dfp32 T2 = RBState2.GetTransform();
	CMat4Dfp32 T2Inv;
	T2.InverseOrthogonal(T2Inv);

	fp32 RelativeAngle = 3.1415f / 2.0f;
	//fp32 RelativeAngle = 0.0f / 2.0f;
	CVec3Dfp32 HingeAxis = CVec3Dfp32(m_AxisB[0], m_AxisB[1], m_AxisB[2]);
	HingeAxis.MultiplyMatrix3x3(T2);

	CAxisRotfp32 AxisRot(HingeAxis, RelativeAngle / (2.0f * 3.14159265f));
	CMat4Dfp32 AxisRotMatrix;
	AxisRot.CreateMatrix(AxisRotMatrix);

	CVec3Dfp32 HingeMid = CVec3Dfp32(m_RB[0], m_RB[1], m_RB[2]) * T2;

	CVec3Dfp32 LocalPos2 = -CVec3Dfp32(m_AngleAxisB[0], m_AngleAxisB[1], m_AngleAxisB[2]);
	LocalPos2 *= T2;
	LocalPos2 -= HingeMid;
	LocalPos2 *= AxisRotMatrix;
	LocalPos2 += HingeMid;
	LocalPos2 *= T2Inv;

	/*
	if (CWD_ConstraintUtil::GenerateMinDistanceContact(m_iRB1, m_iRB2, RBState1, RBState2, -m_RA, CWD_RigidBodyState::To4D(LocalPos2), MinDist, dtinv, CI3, CI4))
//	if (CWD_ConstraintUtil::GenerateMinDistanceContact(m_iRB1, m_iRB2, RBState1, RBState2, -m_RA, -m_RB, MinDist, dtinv, CI3))
	{
		nContacts += 2;
	}*/

	/*
	if (CWD_ConstraintUtil::GenerateMínDistanceContact3(m_iRB1, m_iRB2, RBState1, RBState2, 1.9f, dtinv, CI3, CI4))
	{
		nContacts += 2;
	}*/

/*	if (CWD_ConstraintUtil::GenerateAngleConstraintContact(m_iRB1, m_iRB2, m_AxisA, m_AxisB, RBState1, RBState2, m_RA, m_RB, Foo, m_AngleAxisB, 0.0f, CI3, CI4))
	{
		nContacts += 2;
	}
*/
	return nContacts;
}

/*
	CWD_HingeJoint2
*/

CWD_HingeJoint2::CWD_HingeJoint2(CWD_DynamicsWorld *_pWorld, int _iRB1, int _iRB2, 
								 const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, 
								 const CVec4Dfp32& _RB, const CVec4Dfp32& _AxisB, 
								 fp32 _RelativeAngle,  fp32 _MinAngle, fp32 _MaxAngle)
{
	m_iRB1 = _iRB1;
	m_iRB2 = _iRB2;
	m_RA = _RA;
	m_AxisA = _AxisA * 0.5f;
	m_RB = _RB;
	m_AxisB = _AxisB * 0.5f;

	m_AxisA[3] = 0.0f;
	m_AxisB[3] = 0.0f;

	m_RelativeAngle = -_RelativeAngle;

	m_MinAngle = _MinAngle;
	m_MaxAngle = _MaxAngle;
}

fp32 CWD_HingeJoint2::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
{
	M_ASSERT(false, "NOT SUPPORTED!");
	return 0.0f;
}

int CWD_HingeJoint2::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 4) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);
	const CWD_RigidBodyState& RBState2 = _World.GetRigidBodyState(m_iRB2);

	int nContacts = 0;

	CWD_ContactInfo& CI1 = pContactInfo[0];
	CWD_ContactInfo& CI2 = pContactInfo[1];

	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA + m_AxisA, m_RB + m_AxisB, dtinv, CI1);
	nContacts++;
	CWD_ConstraintUtil::GenerateContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA - m_AxisA, m_RB - m_AxisB, dtinv, CI2);
	nContacts++;

	WDYNAMICS_CHECK_VEC128(CI1.m_Normal);
	WDYNAMICS_CHECK_VEC128(CI2.m_Normal);

	CWD_ContactInfo& CI3 = pContactInfo[2];
	CWD_ContactInfo& CI4 = pContactInfo[3];

	if (CWD_ConstraintUtil::GenerateAngleConstraintContact(m_iRB1, m_iRB2, m_AxisA, m_AxisB, RBState1, RBState2, m_RA, m_RB, m_MinAngle, m_MaxAngle, m_RelativeAngle, CI3, CI4))
	{
		nContacts += 2;
	}

	return nContacts;
}


/*
  CWD_HingeJointWorld
 */


CWD_HingeJointWorld::CWD_HingeJointWorld(CWD_DynamicsWorld *_pWorld, int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _WorldReferencePoint, const CVec4Dfp32& _WorldAxis, const CVec4Dfp32& _AngleAxis, fp32 _MaxAngle)
{
	m_iRB1 = _iRB1;
	m_RA = _RA;
	m_AxisA = _AxisA * 0.5f;
	m_AxisA[3] = 0.0f;
	m_AngleAxis = _AngleAxis;
	m_AngleAxis[3] = 0.0f;

	m_WorldReferencePoint = _WorldReferencePoint;
	m_WorldAxis = _WorldAxis * 0.5f;
	m_WorldAxis[3] = 0.0f;

	m_MaxAngle = _MaxAngle;

#ifdef WDYNAMICS_LEGACYCONSTRAINTS
	m_bUseSolve = true;	

	const CMat4Dfp32& T1 = _pWorld->GetTransform(m_iRB1);
	
	CVec3Dfp32 Ref1 = -((const CVec3Dfp32 &) m_RA);

	CVec3Dfp32 Ref1World = Ref1 * T1;
	CVec3Dfp32 Ref2World = ((const CVec3Dfp32 &) m_WorldReferencePoint) - ((const CVec3Dfp32 &) m_AngleAxis);

	CVec3Dfp32 Anchor = ((const CVec3Dfp32 &) m_RA) * T1;

	fp32 b = Ref1World.Distance(Anchor);
	fp32 c = Ref2World.Distance(Anchor);

	fp32 MinDistance = b * b + c * c - 2.0f * b * c * M_Cos(_PI - m_MaxAngle * 0.5f);
	m_MinDistance = M_Sqrt(MinDistance);

#endif
}

fp32 CWD_HingeJointWorld::Solve(CWD_DynamicsWorld& _World, vec128 _dt)
{
	CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

	CVec4Dfp32 tmp = _dt;
	vec128 dt = _dt;

	CWD_LegacyConstraint::SolveBallJointWorld(RBState1, M_VAdd(m_WorldReferencePoint.v,m_WorldAxis), M_VAdd(m_RA,m_AxisA), dt);
	CWD_LegacyConstraint::SolveBallJointWorld(RBState1, M_VSub(m_WorldReferencePoint.v,m_WorldAxis), M_VSub(m_RA,m_AxisA), dt);

	const CMat4Dfp32 &T = RBState1.GetTransform();
	CMat4Dfp32 Tinv;
	T.InverseOrthogonal(Tinv);

	CVec3Dfp32 RefA = CWD_RigidBodyState::To3D(-m_RA);
	RefA.Normalize();

	RefA *= 0.01f;

	CVec3Dfp32 WorldRef = ((const CVec3Dfp32&) m_WorldReferencePoint) - ((const CVec3Dfp32&) m_AngleAxis);
	CWD_LegacyConstraint::SolveMinDistanceWorld(RBState1, WorldRef,- ((const CVec3Dfp32& )m_RA), m_MinDistance, tmp.k[0]);

	return 0.0f;
}

int CWD_HingeJointWorld::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 4) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

	int nContacts = 2;

	CWD_ContactInfo& CI1 = pContactInfo[0];
	CWD_ContactInfo& CI2 = pContactInfo[1];

	CWD_ConstraintUtil::GenerateContact(m_iRB1, RBState1, m_RA + m_AxisA, m_WorldReferencePoint + m_WorldAxis, dtinv, CI1);
	CWD_ConstraintUtil::GenerateContact(m_iRB1, RBState1, m_RA - m_AxisA, m_WorldReferencePoint - m_WorldAxis, dtinv, CI2);

//	CWD_ConstraintUtil::GenerateContact(m_iRB1, RBState1, m_RA + m_AxisA, m_WorldReferencePoint + m_WorldAxis, dtinv, CI1);

	CWD_ContactInfo& CI3 = pContactInfo[2];
	CWD_ContactInfo& CI4 = pContactInfo[3];

	
/*	if (CWD_ConstraintUtil::GenerateAngleConstraintContact(m_iRB1, m_iRB2, RBState1, RBState2, m_RA, m_RB, m_RA, m_RB, CI3, CI4))
	{
		nContacts += 2;
	}
*/
	return nContacts;
}


CWD_HingeJointWorld2::CWD_HingeJointWorld2(CWD_DynamicsWorld *_pWorld, 
										   int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, 
										   const CVec4Dfp32& _WorldReferencePoint, const CVec4Dfp32& _RW, const CVec4Dfp32& _WorldAxis, 
										   fp32 _RelativeAngle, fp32 _MinAngle, fp32 _MaxAngle)
{
	m_iRB1 = _iRB1;
	m_RA = _RA;
	m_AxisA = _AxisA * 0.5f;
	m_AxisA[3] = 0.0f;
	
	m_WorldReferencePoint = _WorldReferencePoint;
	m_RW = _RW;
	m_RW[3] = 0.0f;

	m_WorldAxis = _WorldAxis * 0.5f;
	m_WorldAxis[3] = 0.0f;

	m_RelativeAngle = _RelativeAngle;
	m_MinAngle = _MinAngle;
	m_MaxAngle = _MaxAngle;
}

fp32 CWD_HingeJointWorld2::Solve(CWD_DynamicsWorld& _World, vec128 _dt) 
{
	M_ASSERT("NOT SUPPORTED!", false);
	return -1.0f;
}

int CWD_HingeJointWorld2::GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const
{
	if (pContactInfo.Len() < 4) return 0;

	vec128 dtinv = M_VRcp(_dt);

	const CWD_RigidBodyState& RBState1 = _World.GetRigidBodyState(m_iRB1);

	int nContacts = 0;

	CWD_ContactInfo& CI1 = pContactInfo[0];
	CWD_ContactInfo& CI2 = pContactInfo[1];

	CWD_ConstraintUtil::GenerateFixContact(m_iRB1, RBState1, m_RA + m_AxisA, m_WorldReferencePoint + m_WorldAxis, dtinv, CI1);
	nContacts++;

	CWD_ConstraintUtil::GenerateFixContact(m_iRB1, RBState1, m_RA - m_AxisA, m_WorldReferencePoint - m_WorldAxis, dtinv, CI2);
	nContacts++;

	WDYNAMICS_CHECK_VEC128(CI1.m_Normal);
	WDYNAMICS_CHECK_VEC128(CI2.m_Normal);
	
	CWD_ContactInfo& CI3 = pContactInfo[2];

	if (CWD_ConstraintUtil::GenerateFixAngleConstraintContact(m_iRB1, m_AxisA, RBState1, m_RA, m_RW, m_WorldAxis, m_MinAngle, m_MaxAngle, m_RelativeAngle, CI3))
	{
		nContacts += 1;
		WDYNAMICS_CHECK_VEC128(CI3.m_Normal);
	}

	return nContacts;
}
