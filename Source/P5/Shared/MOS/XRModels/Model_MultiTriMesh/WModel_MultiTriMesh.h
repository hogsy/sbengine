#ifndef __WMODEL_MULTITRIMESH_H
#define __WMODEL_MULTITRIMESH_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_MultiTriMesh.h

Author:			Anders Ekermo

Copyright:		Copyright Starbreeze Studios AB 2005

Contents:		Multimodel to display several instances of the same WTriMesh model

Comments:		<comments>

History:		
051207: ae, created file
\*____________________________________________________________________*/

#include "../Model_TriMesh/WTriMesh.h"

class CXR_Model_MultiTriMesh;
typedef TPtr<CXR_Model_MultiTriMesh> spCXR_Model_MultiTriMesh;

enum
{

CTM_MULTITRIMESH_MAXINSTANCES = 255

};

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Class:			MultiTriMesh model class

Comments:		Use with GPU or CPU, define MTMESH_USE_CPU in cpp-file
\*____________________________________________________________________*/
class CXR_Model_MultiTriMesh : 
	public CXR_Model,
	public CXR_VBContainer
{

public:
	CXR_Model_MultiTriMesh();

MRTC_DECLARE

DECLARE_OPERATOR_NEW

// -------------------------------------------------------------------
//Derived from CXR_Model

	virtual void Create(const char * _pParam);
	virtual void Create(const char * _pParam, CDataFile * _pDFile,CCFile * _pFile);

	virtual int GetRenderPass(const CXR_AnimState * _pAnimState = NULL);
	virtual int GetVariationIndex(const char* _pName);
	virtual int GetNumVariations();

#ifndef M_RTMCONSOLE
	virtual CStr GetVariationName(int _iVariation);
#endif

	virtual CXR_Model * GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, 
							   int *_piLod = NULL);
	virtual CXR_Skeleton * GetSkeleton();
	virtual CXR_Skeleton * GetPhysSkeleton();

	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param,aint _Value);
	virtual int GetParamfv(int _Param, fp32 * _pRetValues);
	virtual void SetParamfv(int _Param, const fp32 * _pValues);

	virtual fp32 GetBound_Sphere(const CXR_AnimState * _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, int _Mask, CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState = NULL);

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, 
		CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, const CXR_AnimState* _pAnimState, 
		const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);

	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	virtual void OnPostPrecache(CXR_Engine* _pEngine);
	virtual void OnResourceRefresh(int _Flags = 0);

	virtual CXR_WallmarkInterface * Wallmark_GetInterface();
	virtual CXR_PhysicsModel * Phys_GetInterface();


// -------------------------------------------------------------------
//Derived from CXR_VBContainer

	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual CFStr GetName(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual void Release(int _iLocal);

protected:

	// TThinArray<uint8>	m_lLOD;

	spCXR_Model_TriangleMesh m_spTriangleMesh;
	CBox3Dfp32	m_BoundingBox;

	// uint16	m_nMaxInstances,m_nCurrentInstances;

	// Don't store these locally
	// TThinArray<CMat4Dfp32>	m_lMatrices;
	// TThinArray<CMat4Dfp32>	m_lRenderMatrices;

	//Generate instances, skeleton-wise
	void GenerateInstances(CXR_Model_TriangleMesh * _pMesh);

	//Generate LOD clusters, after primitive
	// Not needed- LOD's are generated directly into the VB
	// void GenerateLODClusters();

	//Generate Vertex Buffer
	bool GenerateVB(class CTriMesh_RenderInstanceParamters * _pRenderParams,CXR_Engine * _pEngine,
					CXR_Model_TriangleMesh * _pMesh,CTM_VertexBuffer * _pC,int _iC,CMat4Dfp32 * _pMatrices,
					uint16 _nMatrices,uint8 * _piLOD);

// -------------------------------------------------------------------
// MultiTriMesh- exclusive!

	void	SetInstances(CMat4Dfp32 * _pPos,uint8 _nPos);


};

#endif
