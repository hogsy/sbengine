#if 0 //LEGACY
#ifndef __WModel_Tentacles_h__
#define __WModel_Tentacles_h__

#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "../../../../Shared/MOS/XRModels/Model_TriMesh/WTriMesh.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentaclesData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTentaclesData : public CXR_ModelInstance
{
public:
	CTentaclesData();

	// pure virtual overrides
	virtual void Create(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	virtual void OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat43fp32* _pMat = NULL, int _nMat = 0, int _Flags = 0);
	virtual bool NeedRefresh(class CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context);
	virtual TPtr<CXR_ModelInstance> Duplicate() const;
	virtual void operator= (const CXR_ModelInstance &_Instance);
	void operator= (const CTentaclesData &_Instance);

	struct SegTemp
	{
		struct Param
		{
			CVec3Dfp32 Pos;
			CQuatfp32 Rot;
			fp32 Size;
			fp32 Wiggle;
		};
		Param m_Start;
		Param m_End;
		bool m_bFree;
		int8 m_iParent; // used to override start-pos&rot
		uint8 m_ID;

		///
		struct SplinePoint
		{
			fp32 t;			// [0..1]
			CVec3Dfp32 pos;	// calculated position
			fp32 len;		// length to previous point
		};
		uint m_nBones;
	};

	SegTemp m_Segs[100];
	uint m_nSegs;
	bool m_bNeedRefresh;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_Tentacles
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_Tentacles : public CXR_Model_Custom
{
	MRTC_DECLARE;

	CBox3Dfp32 m_BoundBox;
	CStr m_Keys;

	// Tentacles template pieces:
	enum { MaxTemplates = 10 };
	spCXR_Model_TriangleMesh m_lspTemplates[MaxTemplates];	// mesh data

	CMat43fp32 m_Inv1; // end position of straight segment (inverted -> start)

public:
	void SetTemplateData(uint8 _Index, spCXR_Model_TriangleMesh _spMesh);
	void SnapMeshData(CXR_Model_TriangleMesh& _Mesh);

	virtual void OnPrecache(CXR_Engine* _pEngine);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pReg);
	virtual void OnCreate(const char* _keys);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, 
	                      CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, 
	                      spCXR_WorldLightState _spWLS, 
	                      const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, 
						  const CMat43fp32& _VMat, int _Flags);

	virtual TPtr<CXR_ModelInstance> CreateModelInstance();
	virtual aint GetParam(int _Param);

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState*);
};


#endif // __WModel_Tentacles_h__
#endif//LEGACY