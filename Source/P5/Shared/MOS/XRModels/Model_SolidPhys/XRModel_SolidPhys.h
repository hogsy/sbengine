
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Physics solid model

	Author:			Magnus Högdahl, Christian Murray

	Copyright:		Starbreeze AB 2005
					
	Contents:		
					
	History:		
		050914:		Created File. 



\*____________________________________________________________________________________________*/

#ifndef __INC_XRModel_SolidPhys
#define __INC_XRModel_SolidPhys

#include "../../XR/XR.h"

#include "../../XR/Solids/XRIndexedSolid.h"

class CXR_Model_SolidPhys : public CXR_PhysicsModel
{
public:
	CXR_IndexedSolidContainer m_Solids;


	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const class CXR_SFCreateInfo& _CreateInfo);
	virtual int GetModelClass() { return CXR_MODEL_CLASS_SOLIDPHYS; };

	// CXR_PhysicsModel overrides
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox);

	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
  	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags ) {};
};

typedef TPtr<CXR_Model_SolidPhys> spCXR_Model_SolidPhys;

class CXR_SFCreateInfo
{
public:
	spCXR_Model_SolidPhys m_spMaster;
};

#endif // __INC_XRModel_SolidPhys
