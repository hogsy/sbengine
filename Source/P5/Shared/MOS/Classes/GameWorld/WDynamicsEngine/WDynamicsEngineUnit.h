#ifndef WDYNAMICSENGINEUNIT_H
#define WDYNAMICSENGINEUNIT_H

class CWD_DynamicsWorldUnitTester
{
public:
	void Test1();
	void Test2();
	void TestMisc();
	void TestJacobianMathLib();
	void Test_TransposeJ_Multiply_Transpose(TThinArray<CVector4Pair>& _MJT_T, TThinArray<uint16>& _MJT_Tindex);
	void Test_J_Mrecip_JT();
	void Test_CInertiaTensorVector();
	void Test_Solver();

	void Test_CreateTestData(TThinArray<fp32>& _Mrecip, TThinArray<CVector4Pair>& _Irecip, TThinArray<CVector4Pair>& _J, TThinArray<uint16>& _Jindex);

	static void CreateFloatArray(TThinArray<fp32>& _V, fp32 x1, ...);
	static void CreateInertiaArray(TThinArray<CVector4Pair>& _V, fp32 _x1, ...);

};

#endif