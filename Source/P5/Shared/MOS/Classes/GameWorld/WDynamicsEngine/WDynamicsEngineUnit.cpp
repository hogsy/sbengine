#include "pch.h"
#include "WDynamicsEngine2.h"
#include "WDynamicsEngineUnit.h"
#include "WDynamicsEngineLCP.h"

/*
	Unit testing
*/

static CWD_RigidBodyState *CreateBlock(fp32 _Mass, const CVec3Dfp32 &_Dim)
{
	CVec3Dfp32 Tensor = CWD_Inertia::Block(_Mass, _Dim);

	CWD_RigidBodyState *pState = MNew(CWD_RigidBodyState);
	pState->Clear();
	CQuatfp32 q;
	q.Unit();
	pState->Create(CVec3Dfp32(0.0f), q, _Mass, Tensor);
	pState->UpdateWorldInertiaTensorInvert();

	return pState;
}

void CWD_DynamicsWorldUnitTester::Test1()
{
	CWD_DynamicsWorld *pWorld = MNew(CWD_DynamicsWorld);

	CWD_RigidBody2 *pRB1 = MNew(CWD_RigidBody2);
	CWD_RigidBodyState *pState1 = CreateBlock(12.0f, CVec3Dfp32(2.0f, 3.0f, 4.0f));

	CWD_RigidBody2 *pRB2 = MNew(CWD_RigidBody2);
	CWD_RigidBodyState *pState2 = CreateBlock(16.0f, CVec3Dfp32(6.0f, 2.0f, 1.0f));

	int iRB1 = pWorld->AddRigidBody(pRB1, *pState1);
	int iRB2 = pWorld->AddRigidBody(pRB2, *pState2);

	CVec3Dfp32 N(6.0f, 7.0f, 2.0f);
	N.Normalize();

	CWD_ContactInfo CI;
	CI.m_iRB1 = iRB1;
	CI.m_iRB2 = iRB2;
	CI.m_Normal = CVec4Dfp32(N[0], N[1], N[2], 1.0f);
	CI.m_PointOfCollision = CVec4Dfp32(1.0f, 5.0f, 2.0f, 1.0f);

	TList_Vector<CWD_ContactInfo> lContactInfo;
	lContactInfo.Add(CI);
	TAP_RCD<CWD_ContactInfo> pContactInfo = lContactInfo;

	pWorld->PreCalculateImpulseDenominator(pContactInfo);

	fp32 Denom = lContactInfo[0].m_CollisionImpulseDenominator;
	M_ASSERT(M_Fabs(Denom - 0.82761) < 0.001, "Invalid collision impulse denominator!");
}

void CWD_DynamicsWorldUnitTester::Test2()
{
	CWD_DynamicsWorld *pWorld = MNew(CWD_DynamicsWorld);

	CWD_RigidBody2 *pRB1 = MNew(CWD_RigidBody2);
	CWD_RigidBodyState *pState1 = CreateBlock(1.0f, CVec3Dfp32(4.0f, 4.0f, 4.0f));
	pState1->SetPosition(CVec3Dfp32(0.0f, 0.0f, 0.0f));

	CWD_RigidBody2 *pRB2 = MNew(CWD_RigidBody2);
	CWD_RigidBodyState *pState2 = CreateBlock(1.0f, CVec3Dfp32(4.0f, 4.0f, 4.0f));
	//	pState2->SetPosition(CVec3Dfp32(-4.0f, -4.0f, 0.0f));
	pState2->SetPosition(CVec3Dfp32(-4.0f, 0.0f, 0.0f));
	pState2->SetVelocity(CVec3Dfp32(10.0f, 0.0f, 0.0f));

	int iRB1 = pWorld->AddRigidBody(pRB1, *pState1);
	int iRB2 = pWorld->AddRigidBody(pRB2, *pState2);

	CVec3Dfp32 N(-1.0f, 0.0f, 0.0f);
	N.Normalize();

	CWD_ContactInfo CI;
	CI.m_iRB1 = iRB1;
	CI.m_iRB2 = iRB2;
	CI.m_Normal = CVec4Dfp32(N[0], N[1], N[2], 1.0f);
	//	CI.m_PointOfCollision = CVec4Dfp32(-2.0f, -2.0f, 0.0f, 1.0f);
	CI.m_PointOfCollision = CVec4Dfp32(-2.0f, 0.0f, 0.0f, 1.0f);

	TList_Vector<CWD_ContactInfo> lContactInfo;
	lContactInfo.Add(CI);
	TAP_RCD<CWD_ContactInfo> pContactInfo = lContactInfo;

	pWorld->UpdateOrientatioMatrixAndTensor();
	pWorld->PreCalculateImpulseDenominator(pContactInfo);
	pWorld->ProcessCollisions(pContactInfo, 0.0f, 1.0f / 60.0f);

	//fp32 Denom = lContactInfo[0].m_CollisionImpulseDenominator;
	//M_ASSERT(M_Fabs(Denom - 0.82761) < 0.001, "Invalid collision impulse denominator!");
}

void CWD_DynamicsWorldUnitTester::TestMisc()
{
	CWD_RigidBodyState State;

	fp32 m = 123.0f;
	CVec3Dfp32 Tensor = CWD_Inertia::Block(m, 10.0f, 20.0f, 30.0f);

	CQuatfp32 UnitQ;
	UnitQ.Unit();

	State.Create(CVec3Dfp32(0.0f, 0.0f, 0.0f), UnitQ, m, Tensor);
	State.m_Velocity = CVec4Dfp32(10.0f, 0.0f, 0.0f, 0.0f);
	State.m_AngularVelocity = CVec4Dfp32(1.0f, 2.0f, 3.0f, 0.0f);

	CVec4Dfp32 KE = CWD_DynamicsWorld::GetKineticEnergySIMD(State);
	M_ASSERT(M_Fabs(KE[0] - 56375.0f) < 0.00001f, "!");

	fp32 KE2 = CWD_DynamicsWorld::GetKineticEnergy(State);
	M_ASSERT(M_Fabs(KE2 - 56375.0f) < 0.00001f, "!");

}

void CWD_DynamicsWorldUnitTester::Test_CInertiaTensorVector()
{
	CInertiaTensorVector IT(2);

	CMat4Dfp32 T; T.Unit();
	int x = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			T.k[i][j] = x++;
		}
	}

	IT.Set(0, CVec3Dfp32(1,2,3));
	IT.Set(1, T);
	IT.Print();  // T ska blir symmetrisk
}

void CWD_DynamicsWorldUnitTester::CreateFloatArray(TThinArray<fp32>& _V, fp32 x1, ...)
{
	int count = 0;
	fp32 x = x1;
	va_list valist;

	va_start(valist, x1); 
	while( x >= 0.0f)
	{
		count++;
		x = va_arg(valist, fp64);
	}
	va_end(valist);

	_V.SetLen(count);
	x = x1;
	int i = 0;
	va_start(valist, x1); 
	while( x >= 0.0f)
	{
		_V[i++] = x;
		x = va_arg(valist, fp64);
	}
	va_end(valist);
}

void CWD_DynamicsWorldUnitTester::CreateInertiaArray(TThinArray<CVector4Pair>& _V, fp32 _x1, ...)
{
	int count = 0;
	fp32 x = _x1;
	va_list valist;

	va_start(valist, _x1); 
	while( x >= 0.0f)
	{
		count++;
		x = va_arg(valist, fp64);
	}
	va_end(valist);

	M_ASSERT(count % 3 == 0, "!");
	count /= 3;

	_V.SetLen(count);

	x = _x1;
	int i = 0;
	va_start(valist, _x1); 

	fp32 x1 = _x1;
	fp32 x2 = va_arg(valist, fp64);
	fp32 x3 = va_arg(valist, fp64);

	while(_x1 >= 0.0f)
	{
		CVec4Dfp32 Tmp1, Tmp2;
		CLCPMath::PackInertiaTensor(CVec3Dfp32(x1,x2,x3), Tmp1, Tmp2);
		_V[i++] = CVector4Pair(Tmp1, Tmp2);

		x1 = va_arg(valist, fp64);
		if (x1 < 0.0f)
			break;
		x2 = va_arg(valist, fp64);
		x3 = va_arg(valist, fp64);
	}
	va_end(valist);
}

template <int Stride>
static void PrintJacobian(int _n, TSparseMatrixIterator<vec128, Stride> iJ)
{
	for (int i = 0; i < 2; i++)
	{
		CVector4Pair p = *iJ;
		M_TRACEALWAYS("%d:\n", iJ.Index());
		PrintFixed(p.a, 6);
		M_TRACEALWAYS(" ");
		PrintFixed(p.b, 6);
		M_TRACEALWAYS("\n");

		iJ++;

		p = *iJ;
		M_TRACEALWAYS("%d:\n", iJ.Index());
		PrintFixed(p.a, 6);
		M_TRACEALWAYS(" ");
		PrintFixed(p.b, 6);
		M_TRACEALWAYS("\n");

		iJ++;
	}
}

template <typename T, int Stride>
static void PrintVector(int _n, TVectorIterator<T, Stride> _V, int _ColumnWidth)
{
	for (int i = 0; i < _n; i++)
	{
		const T& t = *_V;
		PrintFixed(t, _ColumnWidth);
		_V++;
	}
}

void CWD_DynamicsWorldUnitTester::Test_Solver()
{
	TThinArray<fp32> Mrecip;
	TThinArray<CVector4Pair> Irecip;
	TThinArray<CVector4Pair> J;
	TThinArray<uint16> Jindex;
	Test_CreateTestData(Mrecip, Irecip, J, Jindex);

	TThinArray<CVector4Pair> MJT_T;
	TThinArray<uint16> MJT_Tindex;

	MJT_T.SetLen(4);
	MJT_Tindex.SetLen(4);
}

void CWD_DynamicsWorldUnitTester::Test_J_Mrecip_JT()
{
	TThinArray<fp32> Mrecip;
	TThinArray<CVector4Pair> Irecip;
	TThinArray<CVector4Pair> J;
	TThinArray<uint16> Jindex;
	Test_CreateTestData(Mrecip, Irecip, J, Jindex);

	TThinArray<CVector4Pair> MJT_T;
	TThinArray<uint16> MJT_Tindex;
	MJT_T.SetLen(4);
	MJT_Tindex.SetLen(4);

	CLCPMath::Mi_Times_Jt_t(2, 
		TVectorIterator<fp32>(Mrecip), 
		TVectorIterator<CVector4Pair>(Irecip), 
		TSparseMatrixIterator<CVector4Pair>(J, Jindex), 
		TSparseMatrixIterator<CVector4Pair>(MJT_T, MJT_Tindex));

	TThinArray<fp32> D;
	D.SetLen(2);

	CLCPMath::Sparse_Times_Sparse_Diagonal(2, 
		TSparseMatrixIterator<CVector4Pair>(J, Jindex), 
		TSparseMatrixIterator<CVector4Pair>(MJT_T, MJT_Tindex),
		TVectorIterator<fp32>(D));

	TThinArray<CVector4Pair> R;
	R.SetLen(2);
	memset(R.GetBasePtr(), 0, R.Len() * sizeof(CVector4Pair));

	TThinArray<fp32> Lambda;
	CreateFloatArray(Lambda, 123.0f, 456.0f, -1.0f);

	CLCPMath::Sparse_Times_Vector(2, TSparseMatrixIterator<CVector4Pair>(MJT_T, MJT_Tindex), TVectorIterator<fp32>(Lambda), TVectorIterator<CVector4Pair>(R));
	PrintVector(2, TVectorIterator<CVector4Pair>(R), 4);

	TThinArray<fp32> b;
	b.SetLen(2);

	CLCPMath::Divide(2, TSparseMatrixIterator<CVector4Pair>(J, Jindex), TVectorIterator<fp32>(b), TVectorIterator<fp32>(D));

}

void CWD_DynamicsWorldUnitTester::Test_CreateTestData(TThinArray<fp32>& _Mrecip, TThinArray<CVector4Pair>& _Irecip, TThinArray<CVector4Pair>& _J, TThinArray<uint16>& _Jindex)
{
	CreateFloatArray(_Mrecip, 10.0f, 20.0f, -1.0f);
	CreateInertiaArray(_Irecip, 1.0f, 2.0f, 3.0f, 10.0f, 20.0f, 30.0f, -1.0f);

	_J.SetLen(4);
	_J[0] = CVector4Pair(CVec4Dfp32(1,2,3,0), CVec4Dfp32(4,5,6,0));
	_J[1] = CVector4Pair(CVec4Dfp32(7,8,9,0), CVec4Dfp32(10,11,12,0));
	_J[2] = CVector4Pair(CVec4Dfp32(11,21,31,0), CVec4Dfp32(41,51,61,0));
	_J[3] = CVector4Pair(CVec4Dfp32(71,81,91,0), CVec4Dfp32(101,111,121,0));

	_Jindex.SetLen(4);
	for (int i = 0; i < 4; i++)
	{
		_Jindex[i] = i % 2 == 0 ? 0 : 1;
	}
}

void CWD_DynamicsWorldUnitTester::Test_TransposeJ_Multiply_Transpose(TThinArray<CVector4Pair>& _MJT_T, TThinArray<uint16>& _MJT_Tindex)
{
	TThinArray<fp32> Mrecip;
	CreateFloatArray(Mrecip, 10.0f, 20.0f, -1.0f);

	TThinArray<CVector4Pair> Irecip;
	CreateInertiaArray(Irecip, 1.0f, 2.0f, 3.0f, 10.0f, 20.0f, 30.0f, -1.0f);

	TThinArray<uint16> Jindex; //, MJT_Tindex;
	TThinArray<CVector4Pair> J; //, MJT_T;

	J.SetLen(4);
	J[0] = CVector4Pair(CVec4Dfp32(1,2,3,0), CVec4Dfp32(4,5,6,0));
	J[1] = CVector4Pair(CVec4Dfp32(7,8,9,0), CVec4Dfp32(10,11,12,0));
	J[2] = CVector4Pair(CVec4Dfp32(11,21,31,0), CVec4Dfp32(41,51,61,0));
	J[3] = CVector4Pair(CVec4Dfp32(71,81,91,0), CVec4Dfp32(101,111,121,0));

	Jindex.SetLen(4);
	_MJT_T.SetLen(4);
	_MJT_Tindex.SetLen(4);
	for (int i = 0; i < 4; i++)
	{
		Jindex[i] = i % 2 == 0 ? 0 : 1;
		_MJT_Tindex[i] = i % 2 == 0 ? 0 : 1;
	}

	TSparseMatrixIterator<CVector4Pair> iJ(J, Jindex);
	TSparseMatrixIterator<CVector4Pair> iMJT_T(_MJT_T, _MJT_Tindex);

	CLCPMath::Mi_Times_Jt_t(2, TVectorIterator<fp32>(Mrecip), TVectorIterator<CVector4Pair>(Irecip), iJ, iMJT_T);

	for (int i = 0; i < 2; i++)
	{
		CVector4Pair p = *iMJT_T;
		M_TRACEALWAYS("%d:\n", iMJT_T.Index());
		PrintFixed(p.a, 6);
		M_TRACEALWAYS(" ");
		PrintFixed(p.b, 6);
		M_TRACEALWAYS("\n");

		iMJT_T++;

		p = *iMJT_T;
		M_TRACEALWAYS("%d:\n", iMJT_T.Index());
		PrintFixed(p.a, 6);
		M_TRACEALWAYS(" ");
		PrintFixed(p.b, 6);
		M_TRACEALWAYS("\n");

		iMJT_T++;
	}
}

void CWD_DynamicsWorldUnitTester::TestJacobianMathLib()
{
	/*	CInertiaTensorVector::PrintFixed(123.45454f);
	M_TRACEALWAYS("\n");
	CInertiaTensorVector::PrintFixed(1.45454f);
	M_TRACEALWAYS("\n");
	*/

	fp32 _m[2]= {10, 20};
	vec128 M_ALIGN(16) _I[4] = { M_VConst(1,2,3,0), M_VConst(4,5,6,0), M_VConst(11,12,13,0), M_VConst(14,15,16,0) };

	CMassVector m(2);
	m.Set(0, 10);
	m.Set(1, 20);

	CInertiaTensorVector I(2);
	I.Set(0, CVec3Dfp32(1,2,3));
	I.Set(1, CVec3Dfp32(10,20,30));

	CSparseJacobianMatrix MJT_T(2, 2);


	CSparseJacobianMatrix J(2, 2);
	CSparseJacobianMatrix::Iterator iJ = J.Begin();

	CInertiaTensorVector IT(2);
	//	IT.Set(0, CVec3Dfp32(1,2,3));
	//	IT.Set(1, CVec3Dfp32(4,5,6));
	CMat4Dfp32 T1; T1.Unit();
	CMat4Dfp32 T2; T2.Unit();
	T1.k[0][0] = 1; T1.k[1][1] = 2; T1.k[2][2] = 3; 
	T2.k[0][0] = 11; T2.k[1][1] = 12; T2.k[2][2] = 13; 
	IT.Set(0, T1);
	IT.Set(1, T2);
	IT.Print();


	iJ.JacA() = M_VConst(1,2,3,0);
	iJ.JacB() = M_VConst(4,5,6,0);
	iJ.Index() = 0;
	iJ++;

	iJ.JacA() = M_VConst(7,8,9,0);
	iJ.JacB() = M_VConst(10,11,12,0);
	iJ.Index() = 1;
	iJ++;

	iJ.JacA() = M_VConst(11,21,31,0);
	iJ.JacB() = M_VConst(41,51,61,0);
	iJ.Index() = 0;
	iJ++;

	iJ.JacA() = M_VConst(71,81,91,0);
	iJ.JacB() = M_VConst(101,111,121,0);
	iJ.Index() = 1;
	iJ++;

	M_TRACEALWAYS("\n\n\n");
	J.Print();

	CLCPMath::Mi_Times_Jt_t(m, I, J, MJT_T);
	MJT_T.Print();


	TVectorIterator<fp32, 1> foo1(NULL);
	TVectorIterator<vec128, 1> foo2(NULL);

	TThinArray<fp32> v;
	v.SetLen(2*6);
	for (int i = 0; i < v.Len(); i++)
	{
		v[i] = i;
	}

	TThinArray<fp32> r;
	r.SetLen(2*6);

	TThinArray<fp32> mm;
	mm.SetLen(2);
	mm[0] = 10.0f;
	mm[1] = 20.0f;

	TThinArray<vec128> II;
	II.SetLen(2*2);
	CVec4Dfp32 Tmp1, Tmp2;
	CLCPMath::PackInertiaTensor(CVec3Dfp32(1,2,3), Tmp1, Tmp2);
	II[0] = Tmp1;
	II[1] = Tmp2;
	CLCPMath::PackInertiaTensor(CVec3Dfp32(10,20,30), Tmp1, Tmp2);
	II[2] = Tmp1;
	II[3] = Tmp2;

	TVectorIterator<fp32> fooo(mm);
	for (int i = 0; i <  2; i++)
	{
		*fooo = 123;
		fooo++;
	}


	CLCPMath::Mi_Times_V(2, TVectorIterator<fp32>(mm), TVectorIterator<vec128>(II), TVectorIterator<fp32>(v), TVectorIterator<fp32>(r));


	/*
	const int N = 4;

	static M_ALIGN(16) vec128 J[N];
	static M_ALIGN(16) vec128 V[N];
	static M_ALIGN(16) vec128 VResult[N];
	uint16 Map[1*2];

	vec128 Tmp = M_VZero();

	for (int i = 0; i < N; i++)
	{
	J[i] = Tmp;
	V[i] = Tmp;
	VResult[i] = M_VZero();

	Tmp = M_VAdd(Tmp, M_VOne());
	}

	int Index = 0;
	for (int i = 0; i < 1; i++)
	{
	for (int j = 0; j < 2; j++)
	{
	Map[i * 2 + j] = Index++;
	}
	}

	CSparseJacobianMatrix JMat(J, Map, 1, 2, 1, 2);
	CVector Vec((fp32 *) V, 12);
	CVector VecResult((fp32 *) VResult, 1);

	JMat.Multiply(Vec, VecResult);
	*/

}
