
#ifndef WDYNAMICSCONSTRAINT_H
#define WDYNAMICSCONSTRAINT_H

#define WDYNAMICS_LEGACYCONSTRAINTS


#include "pch.h"
#include "WDynamicsEngine2.h"


class CWD_BallJointSolver
{
public:
	static vec128 Solve(CWD_DynamicsWorld& _World, vec128 _dt, int _iRB1, int _iRB2, vec128 _Point1, vec128 _Point2);
};


class M_ALIGN(16) CWD_MaxDistanceConstraint : public CWD_Constraint
{
public:
	CWD_MaxDistanceConstraint(int _iRB1, const CVec4Dfp32& _WorldReferencePoint, fp32 _MaxDistance);

	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1;
	CVec4Dfp32 M_ALIGN(16) m_WorldReferencePoint;
	fp32 m_MaxDistance;
};

class M_ALIGN(16) CWD_BallJoint : public CWD_Constraint
{
public:
	CWD_BallJoint(int _iRB1, int _iRB2, const CVec4Dfp32& _RA, const CVec4Dfp32& _RB, const CVec4Dfp32& _RA_Angle, const CVec4Dfp32& _RB_Angle);

	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1, m_iRB2;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_RB;
	CVec4Dfp32 M_ALIGN(16) m_RA_Angle;
	CVec4Dfp32 M_ALIGN(16) m_RB_Angle;
};

class M_ALIGN(16) CWD_BallJointWorld : public CWD_Constraint
{
public:
	CWD_BallJointWorld(int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _RA_Angle, const CVec4Dfp32& _WorldReferencePoint);

	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_RA_Angle;
	CVec4Dfp32 M_ALIGN(16) m_WorldReferencePoint;
};

class M_ALIGN(16) CWD_HingeJoint : public CWD_Constraint
{
public:
	CWD_HingeJoint(CWD_DynamicsWorld *pWorld, int _iRB1, int _iRB2, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _AngleAxisA, const CVec4Dfp32& _RB, const CVec4Dfp32& _AxisB, const CVec4Dfp32& _AngleAxisB, fp32 _RelativeAngle, fp32 _MaxAngle);
	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1, m_iRB2;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_RB;

	CVec4Dfp32 M_ALIGN(16) m_AxisA;
	CVec4Dfp32 M_ALIGN(16) m_AxisB;
	CVec4Dfp32 M_ALIGN(16) m_AngleAxisA;
	CVec4Dfp32 M_ALIGN(16) m_AngleAxisB;

	CMat4Dfp32 m_RelativeMatrix;
	CVec4Dfp32 M_ALIGN(16) m_LocalPos2;

	fp32 m_RelativeAngle;
	fp32 m_MaxAngle;

#ifdef WDYNAMICS_LEGACYCONSTRAINTS
	fp32 m_MinDistance;
#endif

};

class M_ALIGN(16) CWD_HingeJoint2 : public CWD_Constraint
{
public:
	CWD_HingeJoint2(CWD_DynamicsWorld *pWorld, int _iRB1, int _iRB2, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _RB, const CVec4Dfp32& _AxisB, fp32 _RelativeAngle,  fp32 _MinAngle, fp32 _MaxAngle);
	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1, m_iRB2;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_RB;

	CVec4Dfp32 M_ALIGN(16) m_AxisA;
	CVec4Dfp32 M_ALIGN(16) m_AxisB;

	fp32 m_RelativeAngle;
	fp32 m_MinAngle;
	fp32 m_MaxAngle;
};

class M_ALIGN(16) CWD_HingeJointWorld : public CWD_Constraint
{
public:
	CWD_HingeJointWorld(CWD_DynamicsWorld *_pWorld, int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _WorldReferencePoint, const CVec4Dfp32& _WorldAxis, const CVec4Dfp32& _AngleAxis, fp32 _MaxAngle);
	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_AxisA;
	CVec4Dfp32 M_ALIGN(16) m_AngleAxis;
	CVec4Dfp32 M_ALIGN(16) m_WorldReferencePoint;
	CVec4Dfp32 M_ALIGN(16) m_WorldAxis;

	fp32 m_MaxAngle;

#ifdef WDYNAMICS_LEGACYCONSTRAINTS
	fp32 m_MinDistance;
#endif

};

class M_ALIGN(16) CWD_HingeJointWorld2 : public CWD_Constraint
{
public:
	CWD_HingeJointWorld2(CWD_DynamicsWorld *_pWorld, int _iRB1, const CVec4Dfp32& _RA, const CVec4Dfp32& _AxisA, const CVec4Dfp32& _WorldReferencePoint, const CVec4Dfp32& _RW, const CVec4Dfp32& _WorldAxis, fp32 _RelativeAngle, fp32 _MinAngle, fp32 _MaxAngle);
	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt);
	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const;

protected:
	int m_iRB1;
	CVec4Dfp32 M_ALIGN(16) m_RA;
	CVec4Dfp32 M_ALIGN(16) m_AxisA;
	CVec4Dfp32 M_ALIGN(16) m_WorldReferencePoint;
	CVec4Dfp32 M_ALIGN(16) m_WorldAxis;
	CVec4Dfp32 M_ALIGN(16) m_RW;

	fp32 m_RelativeAngle;
	fp32 m_MinAngle;
	fp32 m_MaxAngle;

};


#endif
