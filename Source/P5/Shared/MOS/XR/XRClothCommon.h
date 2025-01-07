#ifndef _INC_XRClothCommon
#define _INC_XRClothCommon

//#pragma optimize("", off)
//#pragma inline_depth(0)

#define CLOTH_LENGTHADJUST_FACTOR 1.0f
#define CLOTH_COLLISIONADJUST_FACTOR 1.0f
//#define CLOTH_SOLID_TRANSLATE_DISTANCE (0.03f*32)
#define CLOTH_SOLID_TRANSLATE_DISTANCE (0.00f*32)
#define CLOTH_LOOSE_FIX_POINTS
#define CLOTH_ENABLE_CAPSULES

#ifndef M_RTM
//#define CLOTH_VERBOSE      -- enable locally only
#endif

//#define CLOTH_DISABLE_COLLISIONS
//#define CLOTH_ENABLE_SOLIDS 
//#define CLOTH_CACHE_COLLISIONS
//#define CLOTH_STORE_SKINNED_POSITION
#define CLOTH_PRETRANSFORM

// TODO: Fixa detta HACK!!! (5 är hårdkodat!)
#define CLOTH_IMPULSE_FACTOR (1.0f/5.0f)
//#define CLOTH_IMPULSE_FACTOR (0.0f)
//#define CLOTH_IMPULSE_ADJUST

#define CLOTH_ADJUST_FLOOR 
#define CLOTH_CHECK_BAD_INPUT 1			// Is this is turned on, some IsValidMat()-checks is performed to dodge bad data

#if 0 && (!defined(M_RTM) || defined(M_Profile))
# define CLOTH_PARANOIA					// If this is turned on, code will assert on valid data
#endif


#define IsValidFloat(f) ((f) == (f))
#define IsValidVec3(v) (IsValidFloat((v).k[0]) && IsValidFloat((v).k[1]) && IsValidFloat((v).k[2]))
#define IsValidVec4(v) (IsValidVec3(v) && IsValidFloat((v).k[3]))
#define IsValidMat(m) (IsValidFloat((m).k[0][0]) && IsValidFloat((m).k[0][1]) && IsValidFloat((m).k[0][2]) &&\
					   IsValidFloat((m).k[1][0]) && IsValidFloat((m).k[1][1]) && IsValidFloat((m).k[1][2]) &&\
					   IsValidFloat((m).k[2][0]) && IsValidFloat((m).k[2][1]) && IsValidFloat((m).k[2][2]) &&\
					   IsValidFloat((m).k[3][0]) && IsValidFloat((m).k[3][1]) && IsValidFloat((m).k[3][2]))
#define IsFirstElementValidInMat(m) (IsValidFloat(m.k[0][0]))

//#define CLOTH_PARANOIA
#ifdef CLOTH_PARANOIA
# define CHECK_VEC3(v) if (!IsValidVec3(v)) M_BREAKPOINT
# define CHECK_MAT(m) if (!IsValidMat(m)) M_BREAKPOINT
# define CHECK_VEC128(v128) { CVec4Dfp32 tmp;tmp.v = v128; if (!IsValidVec4(tmp)) M_BREAKPOINT; }
#else // !CLOTH_PARANOIA
# define CHECK_VEC3(v)
# define CHECK_MAT(m)
# define CHECK_VEC128(v)
#endif


const static CVec4Dfp32 g_Epsilon(0.0001f, 0.0001f, 0.0001f, _FP32_MAX);
const static CVec4Dfp32 g_One(1.0f, 1.0f, 1.0f, 1.0f);
const static CVec4Dfp32 g_Zero(0.0f, 0.0f, 0.0f, 0.0f);


M_FORCEINLINE vec128 M_VSafeRcp_Est(vec128 _v) 
{
	_v = M_VSelMsk(M_VCmpLEMsk(_v, g_Epsilon.v), g_Epsilon.v, _v);
	return M_VRcp_Est(_v); 
}


M_FORCEINLINE static vec128 M_VSafeNrm4(vec128 _v)
{
	vec128 Len = M_VLen4(_v);
	vec128 Epsilon = M_VConst(0.0001f, 0.0001f, 0.0001f, 0.0001f);
	Len = M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), M_VOne(), Len);

	return M_VMul(M_VRcp(Len), _v);
}

M_FORCEINLINE static vec128 M_VSafeSqrt_Est(vec128 _v)
{
	vec128 Epsilon = M_VConst(0.0001f, 0.0001f, 0.0001f, 0.0001f);
	_v = M_VSelMsk(M_VCmpLEMsk(_v, Epsilon), Epsilon, _v);

	return M_VSqrt_Est(_v);
}


#define ClosestPointToLine2(P, PA, PB, InvSqDistance, Res) \
{\
	vec128 ab = M_VSub(PB, PA);\
	\
	vec128 t = M_VDp3(M_VSub(P, PA), ab);\
	vec128 tmp = M_VMul(t, InvSqDistance);\
	tmp = M_VSelMsk(M_VConstMsk(0,0,0,1), g_Zero.v, tmp);\
	t = M_VClamp(tmp, g_Zero.v, g_One.v);\
	\
	Res = M_VMAdd(ab, t, PA);\
}\


#define CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(n) \
	int id1##n = pConstraint##n->m_id1;\
	int id2##n = pConstraint##n->m_id2;\
	\
	vec128 p1_##n = pJointPosition[id1##n].v; \
	vec128 p2_##n = pJointPosition[id2##n].v; \
	\
	vec128 Delta##n = M_VSub(p2_##n, p1_##n);\
	vec128 Len##n = M_VSafeSqrt_Est(M_VDp4(Delta##n, Delta##n));\
	Delta##n = M_VSafeNrm4(Delta##n);\
	\
	vec128 Misc##n = pConstraint##n->m_bFixed1_bFixed2_m_RestLength;\
	vec128 RestLength##n = M_VSplatZ(Misc##n);\
	vec128 Stretchlength##n = M_VMul(RestLength##n, StretchFactorInv);\
	vec128 Complength##n = M_VMul(RestLength##n, CompressionFactor);\
	\
	vec128 LengthDiff##n = M_VMax(M_VSub(Len##n, Stretchlength##n), M_VZero());\
	LengthDiff##n = M_VMin(M_VSub(Len##n, Complength##n), LengthDiff##n);\

#define CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(n)\
	vec128 ScaleFactor##n = M_VConst(0.45f, 0.45f, 0.45f, 0.45f);\
	vec128 NotFixed1##n = M_VSub(M_VOne(), M_VSplatX(Misc##n));\
	vec128 NotFixed2##n = M_VSub(M_VOne(), M_VSplatY(Misc##n));\
	\
	vec128 D1##n = M_VMul(M_VMul(M_VMul(Delta##n, LengthDiff##n),  ScaleFactor##n), NotFixed1##n);\
	vec128 D2##n = M_VMul(M_VMul(M_VMul(Delta##n, LengthDiff##n),  ScaleFactor##n), NotFixed2##n);\
	pJointPosition[id1##n] = M_VAdd(p1_##n, D1##n);\
	pJointPosition[id2##n] = M_VSub(p2_##n, D2##n);\
	CHECK_VEC3(pJointPosition[id1##n]); \
	CHECK_VEC3(pJointPosition[id2##n]); 

M_INLINE static vec128 CapsuleDelta(const TCapsule<fp32>& _Capsule, vec128 _P)
{
	vec128 InvSqDistance = M_VSplatY(_Capsule.m_Misc);
	vec128 Closest;
	ClosestPointToLine2(_P, _Capsule.m_Pointv1, _Capsule.m_Pointv2, InvSqDistance, Closest);
	vec128 CollisionNormal = M_VSub(Closest, _P);
	vec128 DistanceSqr = M_VDp4(CollisionNormal, CollisionNormal);
	vec128 Radius = M_VLdScalar(_Capsule.m_Radius);
	vec128 RadiusSqr = M_VMul(Radius, Radius);
	vec128 Tmp = M_VMul(Radius, M_VRsq(DistanceSqr));
	Tmp = M_VSub(Tmp, M_VOne());
	CollisionNormal= M_VMul(CollisionNormal, Tmp);
	CollisionNormal = M_VSelMsk(M_VCmpGEMsk(DistanceSqr, RadiusSqr), M_VZero(), CollisionNormal);
	return CollisionNormal;
}


M_INLINE static vec128 InvertedCapsuleDelta(const TCapsule<fp32>& _Capsule, vec128 _P)
{
	vec128 InvSqDistance = M_VSplatY(_Capsule.m_Misc);
	vec128 Closest;
	ClosestPointToLine2(_P, _Capsule.m_Pointv1, _Capsule.m_Pointv2, InvSqDistance, Closest);
	vec128 CollisionNormal = M_VSub(Closest, _P);
	vec128 DistanceSqr = M_VDp4(CollisionNormal, CollisionNormal);
	vec128 Radius = M_VLdScalar(_Capsule.m_Radius);
	vec128 RadiusSqr = M_VMul(Radius, Radius);
	vec128 Tmp = M_VMul(Radius, M_VRsq(DistanceSqr));
	Tmp = M_VSub(Tmp, M_VOne());
	CollisionNormal= M_VMul(CollisionNormal, Tmp);
	CollisionNormal = M_VSelMsk(M_VCmpGEMsk(DistanceSqr, RadiusSqr), M_VNeg(CollisionNormal),M_VZero());
	return CollisionNormal;
}



void CClothWrapper::AdjustFloorConstraint(fp32 _FloorOffset)
{
	for (uint32 i=0; i<m_nIds; i++)
	{
		int id = m_pIds[i];
		if (!m_pFixedJoints->Get(id)) 
		{
			CVec3Dfp32 &p = (CVec3Dfp32 &) m_pJointPosition[id];
			if (p.k[2] < _FloorOffset) 
			{
				p.k[2]= _FloorOffset;
			}
		}
	}
}


void CClothWrapper::AdjustLength4(int _ConstraintType)
{
	uint32 nClothConstraints;
	CXR_ClothConstraintData *pClothConstraints;
	fp32 SFI = 1.0f / m_pParams[CLOTHPARAM_STRETCHFACTOR];
	fp32 CF = m_pParams[CLOTHPARAM_COMPRESSIONFACTOR];
	if (_ConstraintType == 1)
	{
		nClothConstraints=m_nShearConstraints;
		pClothConstraints=m_pShearConstraints;
		SFI = 1.0f / m_pParams[CLOTHPARAM_SHEARSTRETCHFACTOR];
		CF = m_pParams[CLOTHPARAM_SHEARCOMPRESSIONFACTOR];
	}

	vec128 StretchFactorInv = M_VLdScalar(SFI);
	vec128 CompressionFactor = M_VLdScalar(CF);

	nClothConstraints=m_nSkeletonConstraints;
	pClothConstraints=m_pSkeletonConstraints;
	CVec4Dfp32* pJointPosition=m_pJointPosition;
	for (uint32 i = 0; i < (nClothConstraints/4); i++)
	{
		CXR_ClothConstraintData *pConstraint1 = &pClothConstraints[4*i];
		CXR_ClothConstraintData *pConstraint2 = &pClothConstraints[4*i+1];
		CXR_ClothConstraintData *pConstraint3 = &pClothConstraints[4*i+2];
		CXR_ClothConstraintData *pConstraint4 = &pClothConstraints[4*i+3];

		CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(1)			
		CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(1)

		CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(2)
		CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(2)

		CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(3)
		CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(3)

		CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(4)
		CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(4)
	}

	for (uint32 i = (nClothConstraints/4)*4; i < nClothConstraints; i++)
	{
		CXR_ClothConstraintData *pConstraint1 = &pClothConstraints[i];
		M_ASSERT(i < nClothConstraints, "");
		CLOTH_ADJUST_LENGTH4_CALC_DIFF_SIMD(1)
		CLOTH_ADJUST_LENGTH4_ADJUST_SIMD(1)
	}
}



void CClothWrapper::AdjustVertexCapsuleConstraintTransformed(int _inode)
{
	vec128 P = m_pJointPosition[_inode].v;
	CHECK_VEC128(P);

	for (uint32 i = 0; i < m_nNormalCapsules / 4; i += 4) 
	{
		const int index = i * 4;
		const TCapsule<fp32>& Capsule1 = m_pNormalCapsulesTransformed[index + 0];
		const TCapsule<fp32>& Capsule2 = m_pNormalCapsulesTransformed[index + 1];
		const TCapsule<fp32>& Capsule3 = m_pNormalCapsulesTransformed[index + 2];
		const TCapsule<fp32>& Capsule4 = m_pNormalCapsulesTransformed[index + 3];

		vec128 D1 = CapsuleDelta(Capsule1, P);
		P = M_VSub(P, D1);
		vec128 D2 = CapsuleDelta(Capsule2, P);
		P = M_VSub(P, D2);
		vec128 D3 = CapsuleDelta(Capsule3, P);
		P = M_VSub(P, D3);
		vec128 D4 = CapsuleDelta(Capsule4, P);
		P = M_VSub(P, D4);
	}
	for (uint32 i = (m_nNormalCapsules / 4)*4; i < m_nNormalCapsules; i++)
	{
		const TCapsule<fp32>& Capsule = m_pNormalCapsulesTransformed[i + 0];
		vec128 D = CapsuleDelta(Capsule, P);
		P = M_VSub(P, D);
	}

	if (m_pInsideCapsuleJointMasks[_inode])
	{
		uint64 mask=1;
		for (uint i=0;i<m_nInvertedCapsules;i++)
		{
			if (m_pInsideCapsuleJointMasks[_inode] & mask)
			{
				const TCapsule<fp32>& Capsule = m_pInvertedCapsulesTransformed[i];
				vec128 D = InvertedCapsuleDelta(Capsule, P);
				P = M_VAdd(P, D);
			}
			mask=mask<<1;
		}
	}
	CHECK_VEC128(P);
	m_pJointPosition[_inode] = P;
}


void CClothWrapper::AdjustVertexCapsuleConstraintTransformed2(int _inode1, int _inode2)
{
	CHECK_VEC3(m_pJointPosition[_inode1]);
	CHECK_VEC3(m_pJointPosition[_inode2]);

	vec128 P1 = m_pJointPosition[_inode1].v;
	vec128 P2 = m_pJointPosition[_inode2].v;


	// TODO: Debugtest
//	TCapsule<fp32>* pCapsulesDebug = m_lCapsulesTransformed.GetBasePtr();

	for (uint32 i = 0; i < m_nNormalCapsules/4; i++) 
	{
		int index = i * 4;

		const TCapsule<fp32>& Capsule1 = m_pNormalCapsulesTransformed[index + 0];
		const TCapsule<fp32>& Capsule2 = m_pNormalCapsulesTransformed[index + 1];
		const TCapsule<fp32>& Capsule3 = m_pNormalCapsulesTransformed[index + 2];
		const TCapsule<fp32>& Capsule4 = m_pNormalCapsulesTransformed[index + 3];

		vec128 P = M_VMul(M_VAdd(P1, P2), M_VHalf());
		CHECK_VEC128(P);

		vec128 D1 = CapsuleDelta(Capsule1, P);
		CHECK_VEC128(D1);
		P1 = M_VSub(P1, D1);
		P2 = M_VSub(P2, D1);
		CHECK_VEC128(P1);
		CHECK_VEC128(P2);
		P = M_VMul(M_VAdd(P1, P2), M_VHalf());

		vec128 D2 = CapsuleDelta(Capsule2, P);
		CHECK_VEC128(D2);
		P1 = M_VSub(P1, D2);
		P2 = M_VSub(P2, D2);
		CHECK_VEC128(P1);
		CHECK_VEC128(P2);
		P = M_VMul(M_VAdd(P1, P2), M_VHalf());

		vec128 D3 = CapsuleDelta(Capsule3, P);
		CHECK_VEC128(D3);
		P1 = M_VSub(P1, D3);
		P2 = M_VSub(P2, D3);
		CHECK_VEC128(P1);
		CHECK_VEC128(P2);
		P = M_VMul(M_VAdd(P1, P2), M_VHalf());

		vec128 D4 = CapsuleDelta(Capsule4, P);
		CHECK_VEC128(D4);
		P1 = M_VSub(P1, D4);
		P2 = M_VSub(P2, D4);
		CHECK_VEC128(P1);
		CHECK_VEC128(P2);
	}

	for (uint32 i = (m_nNormalCapsules / 4)*4; i < m_nNormalCapsules; i++)
	{
		const TCapsule<fp32>& Capsule = m_pNormalCapsulesTransformed[i];
		vec128 P = M_VMul(M_VAdd(P1, P2), M_VHalf());

		vec128 D = CapsuleDelta(Capsule, P);
		P1 = M_VSub(P1, D);
		P2 = M_VSub(P2, D);

		CHECK_VEC128(P1);
		CHECK_VEC128(P2);
	}
	if (m_pInsideCapsuleJointMasks[_inode1] && m_pInsideCapsuleJointMasks[_inode2])
	{
		uint64 mask=1;
		for (uint32 i = 0; i < m_nInvertedCapsules; i++) 
		{
			if (m_pInsideCapsuleJointMasks[_inode1] & mask &&
				m_pInsideCapsuleJointMasks[_inode2] & mask)
			{
				const TCapsule<fp32>& Capsule = m_pInvertedCapsulesTransformed[i];

				vec128 P = M_VMul(M_VAdd(P1, P2), M_VHalf());
				CHECK_VEC128(P);

				vec128 D = InvertedCapsuleDelta(Capsule, P);
				CHECK_VEC128(D);
				P1 = M_VAdd(P1, D);
				P2 = M_VAdd(P2, D);
				CHECK_VEC128(P1);
				CHECK_VEC128(P2);
			}
			mask=mask<<1;
		}
	}
	m_pJointPosition[_inode1].v = P1;
	m_pJointPosition[_inode2].v = P2;
}

M_FORCEINLINE void Cloth_Interpolate2(const CMat4Dfp32& _Pos, const CMat4Dfp32& _Pos2, CMat4Dfp32& _Dest, const fp32& _Time)		// fp32& because we want the option to avoid LHS.
{
	vec128 p0x = _Pos.r[0];
	vec128 p0y = _Pos.r[1];
	vec128 p0w = _Pos.r[3];
	vec128 p1x = _Pos2.r[0];
	vec128 p1y = _Pos2.r[1];
	vec128 p1w = _Pos2.r[3];
	vec128 t = M_VLdScalar(_Time);
	vec128 z = M_VZero();
	vec128 tpos = M_VClamp(t, z, M_VScalar(2.5f));
	vec128 trot = M_VClamp(t, z, M_VScalar(1.5f));

	vec128 dstw = M_VLrp(p0w, p1w, tpos);
	vec128 dstx = M_VLrp(p0x, p1x, trot);
	vec128 dstytmp = M_VLrp(p0y, p1y, trot);
	vec128 dstz = M_VXpd(dstx, dstytmp);
	vec128 dsty = M_VXpd(dstz, dstx);
	M_VNrm3x4(dstx, dsty, dstz, z);

	_Dest.r[0] = dstx;
	_Dest.r[1] = dsty;
	_Dest.r[2] = dstz;
	_Dest.r[3] = dstw;
}



void CClothWrapper::InterpolateSkeleton(fp32 _Fraction)
{

	int n = Min(m_nJointPosition, m_nBoneTransform);
	for (int i = 0; i < n; i++)
	{
		if (m_pCollisionBones->Get(i))
		{
			CMat4Dfp32 M;
			const CMat4Dfp32& Tmp = m_pSkelBoneTransform[i];
			const CMat4Dfp32& Last = m_pLastBoneTransform[i];

#if CLOTH_CHECK_BAD_INPUT
			if (!IsFirstElementValidInMat(Last))
			{
				if (IsFirstElementValidInMat(Tmp))
					M = Tmp;									// Curr valid, Last invalid
			}
			else if (!IsFirstElementValidInMat(Tmp))
				M = Last;										// Curr invalid, Last valid
			else
#endif
				Cloth_Interpolate2(Last, Tmp, M, _Fraction);	// Curr & Last valid

			m_pBoneTransform[i] = M;
		}
		else
		{
			m_pBoneTransform[i]=m_pSkelBoneTransform[i];
		}
	}	
}


void CClothWrapper::TransformCapsules()
{
	{
		const TCapsule<fp32>* pSrc = m_pNormalCapsules;
		TCapsule<fp32>* pDst = m_pNormalCapsulesTransformed;

		for (uint i = 0; i < m_nNormalCapsules; i++)
		{
			uint iNode = pSrc[i].m_UserValue;
			M_ASSERTHANDLER(iNode < m_nBoneTransform, "invalid node index in cloth data!", continue);
			pDst[i].m_Misc=pSrc[i].m_Misc;
			pDst[i].m_Pointv1=M_VMulMat4x3(M_VSetW1(pSrc[i].m_Pointv1),m_pBoneTransform[iNode]);
			pDst[i].m_Pointv2=M_VMulMat4x3(M_VSetW1(pSrc[i].m_Pointv2),m_pBoneTransform[iNode]);
		}
	}
	{
		const TCapsule<fp32>* pSrc = m_pInvertedCapsules;
		TCapsule<fp32>* pDst = m_pInvertedCapsulesTransformed;

		for (uint i = 0; i < m_nInvertedCapsules; i++)
		{
			uint iNode = pSrc[i].m_UserValue;
			M_ASSERTHANDLER(iNode < m_nBoneTransform, "invalid node index in cloth data!", continue);
			pDst[i].m_Misc=pSrc[i].m_Misc;
			pDst[i].m_Pointv1=M_VMulMat4x3(M_VSetW1(pSrc[i].m_Pointv1),m_pBoneTransform[iNode]);
			pDst[i].m_Pointv2=M_VMulMat4x3(M_VSetW1(pSrc[i].m_Pointv2),m_pBoneTransform[iNode]);
		}
	}
}


void CClothWrapper::Integrate (fp32 _Damp,
							   fp64 _dt,
							   fp32 _Fraction,
							   int _iStep,
							   fp64 *_MeanSqError)
{	

	InterpolateSkeleton(_Fraction);

#ifdef CLOTH_PRETRANSFORM
#ifdef CLOTH_ENABLE_CAPSULES
	TransformCapsules();
#endif
#endif

	/*
	Reset position to previous state
	and apply gravity
	*/
	for (uint32 i = 0; i < m_nIds; i++)
	{
		int id = m_pIds[i];
		if (m_pFixedJoints->Get(id))
		{
			vec128 pp=M_VMulMat4x3(m_pSkelNodeDatas[id].m_LocalCenterv,m_pBoneTransform[id]);
			CHECK_VEC128(pp);
			m_pJointPosition[id] = pp;
		}
	}

#ifdef CLOTH_ADJUST_FLOOR
	if (m_bAdjustFloor)
	{
		AdjustFloorConstraint(m_FloorOffset);
	}
#endif

#ifndef CLOTH_DISABLE_COLLISIONS
	for (uint32 i = 0; i < m_nIds; i++)
	{
		int id = m_pIds[i];
		if (!m_pFixedJoints->Get(id))
		{
			AdjustVertexCapsuleConstraintTransformed(id);
		}
	}
	for (uint32 i=0; i<m_nSkeletonConstraints; i++)
	{
		const CXR_ClothConstraintData* pClothConstraint = &m_pSkeletonConstraints[i];
		if (!(pClothConstraint->m_bFixed1 == 1.0f || pClothConstraint->m_bFixed2 == 1.0f))
			AdjustVertexCapsuleConstraintTransformed2(pClothConstraint->m_id1, pClothConstraint->m_id2);
	}

#endif

	const CVec4Dfp32 force(0,0,_dt*15*4*5*3*2*2*2*2*1.5f * 1.0f * m_pParams[CLOTHPARAM_CLOTHDENSITY] * 2.0f,0.0f);
	for (uint32 i = 0; i < m_nIds; i++)
	{
		int id = m_pIds[i];
		if (_iStep == 0)
			if (!m_pFixedJoints->Get(id))
			{
				vec128 pos=M_VMul(m_pJointPosition[id].v,M_VLdScalar(2.0f-_Damp));
				pos=M_VSub(pos,M_VMul(m_pPrevJointPosition[id].v,M_VLdScalar(1.0f-_Damp)));
				pos=M_VSub(pos,M_VMul(force.v,M_VLdScalar(_dt*_dt)));
				m_pPrevJointPosition[id].v = m_pJointPosition[id].v;
				CHECK_VEC128(pos);
				m_pJointPosition[id].v = pos;
			}
	}

	uint nIterationSteps = (int)m_pParams[CLOTHPARAM_NITERATIONSTEPS];
	for (uint j = 0; j < nIterationSteps; j++) 
	{
		AdjustLength4(0);
		AdjustLength4(1);
	}

	fp64 MeanSqError = 0.0;

#ifndef CLOTH_DISABLE_COLLISIONS
	for (uint32 i = 0; i < m_nIds; i++)
	{
		int id = m_pIds[i];
		if (!m_pFixedJoints->Get(id))
		{
			AdjustVertexCapsuleConstraintTransformed(id);
		}
	}

	for (uint32 i = 0; i < m_nSkeletonConstraints; i++)
	{
		const CXR_ClothConstraintData* pClothConstraint = &m_pSkeletonConstraints[i];

		if (!(pClothConstraint->m_bFixed1 == 1.0f || pClothConstraint->m_bFixed2 == 1.0f))
		{
			AdjustVertexCapsuleConstraintTransformed2(pClothConstraint->m_id1,pClothConstraint->m_id2);
		}
	}

#endif

	if (_MeanSqError != NULL)
		*_MeanSqError = MeanSqError;
}

vec128 CClothWrapper::GetWorldPosition(int _ijoint)
{
	return M_VMulMat4x3(m_pSkelNodeDatas[_ijoint].m_LocalCenterv,m_pBoneTransform[_ijoint]);
}


void CClothWrapper::PostIntegrate() 
{
	//
	// Store last bone transform
	//
	{
		const CMat4Dfp32* pSrc = m_pSkelBoneTransform;
		CMat4Dfp32* pDst = m_pLastBoneTransform;
		for (uint i = 0; i < m_nSkelBoneTransform; i++)
			pDst[i]=pSrc[i];
	}

	for (uint32 i = 0; i < m_nIds; i++)
	{
		const int id = m_pIds[i];

		const bool NoBoneRotation = false;
		if (!m_pFixedJoints->Get(id))
		{
			if (NoBoneRotation)
			{			
				CMat4Dfp32 m=m_pSkelBoneTransform[id];
				vec128 P = M_VMulMat4x3(m_pSkelNodeDatas[id].m_LocalCenterv,m_pSkelBoneTransform[id]);
				vec128 Diff = M_VSub(P , m_pJointPosition[id].v);
				m_pSkelBoneTransform[id].r[3]=M_VSub(m_pSkelBoneTransform[id].r[3],Diff);
			}
			else
			{
				if (id >= int(m_nSkelNodeDatas))
					continue;

				const CJointVertsIndicies& riJoints = m_piJointVerticies[id];

				/*
				TODO: Uträkning av relativ rotation är i nöd av konstgjord andning...
				*/

				vec128 snormal=M_VConst(1,0,0,0);
				vec128 su = M_VNrm3(M_VSub(GetWorldPosition(riJoints.m_iJointVerts[0]) , GetWorldPosition(id)));
				vec128 sv= M_VNrm3(M_VXpd(su,snormal));
				su=M_VNrm3(M_VXpd(snormal,sv));

				vec128 cu = M_VNrm3(M_VSub(m_pJointPosition[riJoints.m_iJointVerts[0]].v,m_pJointPosition[id].v));
				vec128 cv= M_VNrm3(M_VXpd(cu,snormal));
				cu=M_VNrm3(M_VXpd(snormal,cv));

				CMat4Dfp32 sm;
				sm.r[0]= snormal;
				sm.r[1]= su;
				sm.r[2]= sv;
				sm.r[3]= M_VConst(0,0,0,1.0f);

				CMat4Dfp32 cm;
				cm.r[0]= snormal;
				cm.r[1]= cu;
				cm.r[2]= cv;
				cm.r[3]= M_VConst(0,0,0,1.0f);

				M_VTranspose4x3(sm.r[0],sm.r[1],sm.r[2],sm.r[3]);
				sm.r[3]=M_VConst(0,0,0,1.0f);
				CMat4Dfp32 ism;
				M_VMatMul(sm,cm,ism);

				const vec128 LocalCenter = m_pSkelNodeDatas[id].m_LocalCenterv;
				const vec128 JointPos = m_pJointPosition[id].v;

				CMat4Dfp32 m = m_pBoneTransform[id];
				const vec128 p = M_VMulMat4x3(LocalCenter,m);
				m.r[3]=M_VSetW1(M_VSub(m.r[3],M_VSub(p,JointPos)));

				CMat4Dfp32 Tback;
				Tback.r[0] = M_VConst(1.0f,0,0,0);
				Tback.r[1] = M_VConst(0,1.0f,0,0);
				Tback.r[2] = M_VConst(0,0,1.0f,0);
				Tback.r[3] = M_VSetW1(M_VNeg(JointPos));

				CMat4Dfp32 T;
				T.r[0] = M_VConst(1.0f,0,0,0);
				T.r[1] = M_VConst(0,1.0f,0,0);
				T.r[2] = M_VConst(0,0,1.0f,0);
				T.r[3] = M_VSetW1(JointPos);

				CMat4Dfp32 tmp,tmp2;
				M_VMatMul(Tback,ism,tmp);
				M_VMatMul(tmp,T,tmp2);
				CMat4Dfp32 mprim;
				M_VMatMul(m,tmp2,mprim);

				CHECK_MAT(mprim);
				m_pSkelBoneTransform[id]=mprim;
			}
		}
	}
}

void CClothWrapper::SkinClothBones(fp32 _Blend, bool _bUsefalloff)
{
	fp32 BlendFalloff = m_pParams[CLOTHPARAM_BLENDFALLOFFACTOR];
	fp32 MinBlend = _bUsefalloff ? Max(0.0f, m_pParams[CLOTHPARAM_MINBLENDFACTOR]) : 0.0f;
	fp32 FalloffFactor = _bUsefalloff ? (-_Blend / BlendFalloff) : 0.0f;

	const CXR_ClothBoneWeightsData* M_RESTRICT pClothBoneWeights = m_pClothBoneWeights;
	const CMat4Dfp32* M_RESTRICT pSkelBoneTransform = m_pSkelBoneTransform;
	const fp32* M_RESTRICT pClosestFixedJointDistance = m_pClosestFixedJointDistance;
	const int* M_RESTRICT pIds = m_pIds;
	const CXR_SkeletonNodeData* M_RESTRICT pSkelNodeDatas = m_pSkelNodeDatas;
	CVec4Dfp32* M_RESTRICT pJointPosition = m_pJointPosition;

	const uint nSkelNodeDatas = m_nSkelNodeDatas;
	const ClothBlendMode BlendMode = m_BlendMode;
	const uint nIds = m_nIds;

	for (uint i = 0; i < nIds; i++)
	{
		const uint id = pIds[i];
		const CXR_ClothBoneWeightsData& BoneWeights = pClothBoneWeights[id];
		const uint nWeights = BoneWeights.GetBoneCount();
		if (id >= nSkelNodeDatas)
			continue;

		const vec128 origpos = pSkelNodeDatas[id].m_LocalCenterv;
		vec128 newpos = M_VConst(0.0f, 0.0f, 0.0f, 1.0f);
		if (BlendMode == CLOTH_SKINBLEND)
		{
			for (uint j = 0; j < nWeights; j++)
			{
				uint iBone = BoneWeights.GetBoneIndex(j);
				fp32 w = BoneWeights.GetBoneWeight(j);
				const CMat4Dfp32& m = pSkelBoneTransform[iBone];
				newpos = M_VAdd(newpos, M_VMul(M_VMulMat4x3(origpos, m), M_VLdScalar(w)));
			}
		}
		else if (BlendMode == CLOTH_ANIMATIONBLEND)
		{
			newpos = M_VMulMat4x3(origpos, pSkelBoneTransform[id]);
		}

		if (nWeights > 0 || BlendMode == CLOTH_ANIMATIONBLEND)
		{
			fp32 B = _Blend;
			B += FalloffFactor * pClosestFixedJointDistance[id];
			B = Max(MinBlend, B);

			pJointPosition[id] = M_VSetW1(M_VLrp(pJointPosition[id].v, newpos, M_VLdScalar(B)));
		}
	}
}


void CClothWrapper::Step()
{
	fp32 Blend = m_pParams[CLOTHPARAM_BLENDFACTOR];
	if (m_Blend >= 0.0f)
		Blend = m_Blend;

#ifdef CLOTH_USE_FULLBLEND
	Blend = 1.0f;
#endif

	bool bUseFalloff = (m_Blend <= 0.0f);
	if (m_dt > 0.0f && (Blend < 1.0f || bUseFalloff))
	{
		fp32 SimFreq = m_pParams[CLOTHPARAM_SIMULATIONFREQUENCY];
		if (m_SimFreq >= 0)
			SimFreq = m_SimFreq;

		//fp32 Step = 1.0f / SimFreq;
		uint nSteps = Max(1, int(m_dt * SimFreq));
		fp32 damp = m_pParams[CLOTHPARAM_DAMPFACTOR];
		fp32 dt = m_dt / nSteps;

		for (uint k = 0; k < nSteps; k++) 
		{
			fp32 frac = (nSteps == 1) ? 1.0f : (k / fp32(nSteps-1));
			Integrate(damp, dt, frac, k, NULL);
		}
	}

	SkinClothBones(Blend, bUseFalloff);
	PostIntegrate();
}



#endif
