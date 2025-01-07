
#ifndef __INC_XRVBOPERATORS_NVIDIA
#define __INC_XRVBOPERATORS_NVIDIA

#include "XR.h"
#include "XRSurfaceContext.h"
#include "XRVBOperators.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_GenEnv
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_NV20_GenEnv : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "NV20-only perturbed reflection vector cube mapping."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_RMBM2D
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_NV20_RMBM2D : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "NV20-only Reflection mapped bump mapping."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_Transform
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if 0
class CXR_VBOperator_NV20_Transform : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Dummy vertex program to fix z-fighting"; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_MultiTxt
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_MultiTxt : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Single pass multitexturing blending setup."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_Fresnel
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_NV20_Fresnel : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	int m_TextureID_Normalize;

	CXR_VBOperator_NV20_Fresnel();

	virtual CStr GetDesc() { return "NV20-only perturbed reflection vector cube mapping."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);

	virtual int OnGetTextureCount(const class CXW_Surface* _pSurf, class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const;
	virtual int OnEnumTextureID(const class CXW_Surface* _pSurf, class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_NV20_YUV2RGB
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_NV20_YUV2RGB : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "NV20-only perturbed reflection vector cube mapping."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};



#endif // __INC_XRVBOPERATORS_NVIDIA

