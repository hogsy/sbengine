#ifndef __WMODEL_MULTI_H
#define __WMODEL_MULTI_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Multimodel used to display XW files in Ogier

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CXR_Model_Multi
\*____________________________________________________________________________________________*/

#include "../../XR/XR.h"

class CXR_Model_Multi : public CXR_Model
{
public:
	enum
	{
		FLAGS_XRMODE_BIT0 = 1,
		FLAGS_XRMODE_BIT1 = 2,
		FLAGS_XRMODE_BIT2 = 4,
		FLAGS_XRMODE_BIT3 = 8,
		FLAGS_MAKEPHYSSURFACEINVISIBLE = 16,

		MASK_XRMODE = 15,
	};

	MRTC_DECLARE;

	DECLARE_OPERATOR_NEW


protected:
	TArray<spCXR_Model> m_lspModels;
	TArray<CMat4Dfp32> m_lPositions;

public:
	CXR_Model_Multi();
	int GetNumModels();
	CXR_Model *GetModel(int _iModel);
	CMat4Dfp32 GetPosition(int _iModel);

	bool CreateFromXW(const char *_pFileName, int _Flags);

	// Overrides
	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual CXR_PhysicsModel* Phys_GetInterface();

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	virtual void OnRefreshSurfaces();
	
	void AddToEngine(CXR_Engine *_pEngine, const CMat4Dfp32 &_Mat, CXR_AnimState &_AnimState, int _OnRenderFlags = 0, bool _bSkipFirst = false);
	CXR_Model *GetViewClipModel(CMat4Dfp32 &_Mat);
};

#endif
