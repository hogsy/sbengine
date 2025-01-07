#include "pch.h"
#include "WDynamicsEngine2.h"
#include "WDynamicsEngineLCP.h"

/*
	LCP-solver
 */

void CWD_LCPSolver::Solve(const CWD_Island& _Island)
{
	TAP_RCD<CWD_RigidBodyState> pBodyStates = _Island.GetBodyStates();
	int nBodies = pBodyStates.Len();
	int nConstraints = 0;

	/*
		if (nConstraints == 0) ...
		CreateConstraints(...)
	 */

	m_lFext.QuickSetLen(nBodies);
	m_lMassInv.QuickSetLen(nBodies);
	m_lInertiaTensorInv.QuickSetLen(nBodies);
	m_lJacobian.QuickSetLen(2 * nConstraints);
	m_lJacobianIndices.QuickSetLen(2 * nConstraints);

	m_lMiJt_t.QuickSetLen(2 * nConstraints);
	m_lMiJt_t_Indices.QuickSetLen(2 * nConstraints);

	for (int i = 0; i < nBodies; i++)
	{
		const CWD_RigidBodyState &State = pBodyStates[i];

		// TODO: Tänk över om m_MassInverted ska lagras som vec128 eller inte?
		CVec4Dfp32 Tmp = State.m_MassInverted;
		m_lMassInv[i] = Tmp[0];
		m_lInertiaTensorInv[i] = CLCPMath::PackInertiaTensor(State.m_TensorInverted);

		m_lFext[i] = CVector4Pair(CVec4Dfp32(0.0f, -9.81f, 0.0f, 0.0f), CVec4Dfp32(0.0f, 0.0f, 0.0f, 0.0f));
	}

	CLCPMath::Mi_Times_V(nConstraints, 
						 TVectorIterator<fp32>(m_lMassInv), 
						 TVectorIterator<CVector4Pair>(m_lInertiaTensorInv), 
						 TVectorIterator<CVector4Pair>(m_lFext), 
						 TVectorIterator<CVector4Pair>(m_lMi_Fext));

	CLCPMath::Mi_Times_Jt_t(nConstraints, 
							TVectorIterator<fp32>(m_lMassInv), 
							TVectorIterator<CVector4Pair>(m_lInertiaTensorInv),
							TSparseMatrixIterator<CVector4Pair>(m_lJacobian, m_lJacobianIndices), 
							TSparseMatrixIterator<CVector4Pair>(m_lMiJt_t, m_lMiJt_t_Indices));


	/*
	TAP_RCD<CWD_RigidBodyState> pBodyStates = _Island.GetBodyStates();
	int nBodies = pBodyStates.Len();

	TThinArray<fp32> Minv;
	CInertiaTensorVector Iinv(nBodies);
	TThinArray<vec128> Velocity;
	Velocity.SetLen(2 * nBodies);
	TThinArray<vec128> ExternalForces;
	ExternalForces.SetLen(2 * nBodies);

	for (int i = 0; i < nBodies; i++)
	{
	const CWD_RigidBodyState State& = pBodyStates[i];
	Minv[i] = State.m_MassInverted[0];
	Iinv.Set(i, State.m_TensorInverted);

	Velocity[2*i + 0] = State.m_Velocity;
	Velocity[2*i + 1] = State.m_AngularVelocity;
	ExternalForces[2 *i + 0] = CVec4Dfp32(0.0f, -9.81f * State.m_Mass[0], 0.0f);
	ExternalForces[2 *i + 1] = CVec4Dfp32(0.0f, 0.0f, 0.0f);
	}
	*/

}
