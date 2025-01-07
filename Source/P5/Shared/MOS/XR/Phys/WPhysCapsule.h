/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    File:              PhysCapsule
					
    Author:            Anders Ekermo
					
    Copyright:         Copyright Starbreeze Studios AB 2006
					
    Contents:		   Phys capsule class (Line- SSV)
					
    Comments:          <comments>
					
    History:
        061016: anek, Created file
\*____________________________________________________________________*/

#ifndef _INC_WPhysCapsule
#define _INC_WPhysCapsule

//#include "../../MOS.h"

class CPhysCapsule
{

public:
	CMat4Dfp32	m_Pos;		//Position (X-axis is extents)
	CVec3Dfp32	m_E;		//Extents (Half-length, radius)

//Constructors
	CPhysCapsule() { };

	CPhysCapsule(const CPhysCapsule &_PhC)
	{
		*this = _PhC;
	}

//Operators
	void operator=(const CPhysCapsule &_PhC)
	{
		m_Pos = _PhC.m_Pos;
		m_E = _PhC.m_E;
	}

//Set
	void SetDimensions(const CVec3Dfp32 &_Extents)
	{
		m_E = _Extents;
	}

	void SetPosition(const CMat4Dfp32 &_Mat)
	{
		m_Pos = _Mat;
	}

	CVec3Dfp32 GetDimensions() const
	{
		return m_E;
	}

	void GetPosition(CMat4Dfp32 &_Mat) const
	{
		_Mat = m_Pos;
	}

//Helpers
	void GetBoundBox(CBox3Dfp32& _Box) const
	{
		CVec3Dfp32 Dim = m_Pos.GetRow(0);
		fp32 Rad = m_E.k[1];
		fp32 Ln = m_E.k[0];
		Dim.k[0] = Abs(Dim.k[0])*Ln + Rad;
		Dim.k[1] = Abs(Dim.k[1])*Ln + Rad;
		Dim.k[2] = Abs(Dim.k[2])*Ln + Rad;
		_Box.m_Min = m_Pos.GetRow(3) - Dim;
		_Box.m_Max = m_Pos.GetRow(3) + Dim;
	}


	void Transform(const CMat4Dfp32 &_Mat,CPhysCapsule &_Dest) const
	{
		_Dest.m_E = m_E;
		m_Pos.Multiply(_Mat,_Dest.m_Pos);
	}


	void GetPoints(CVec3Dfp32 &_a,CVec3Dfp32 &_b) const
	{
		CVec3Dfp32 Diff = m_Pos.GetRow(0) * m_E.k[0];
		_a = m_Pos.GetRow(3) + Diff;
		_b = m_Pos.GetRow(3) - Diff;
	}


};

int Phys_Collide_CapsuleFace(const CPhysCapsule &_Cps,const CVec3Dfp32 *_pV,const uint32 *_piV,uint _nV,const CPlane3Dfp32 &_Plane,
							 CVec3Dfp32 * _pPoints,CVec3Dfp32 * _pNormals,fp32 * _pDepths);

int Phys_Collide_CapsuleFace(const vec128 &_Cen,const vec128 &_Dir,const vec128 &_Dim,const vec128 &_Plane,const vec128 * M_RESTRICT _lVtx,uint32 _nVtx,
							 vec128 * _pPoints,vec128 &_Normal,vec128 & _Depth);


#endif
