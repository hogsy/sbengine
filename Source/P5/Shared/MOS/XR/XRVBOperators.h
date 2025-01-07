
#ifndef __INC_XRVBOPERATORS
#define __INC_XRVBOPERATORS

#include "XR.h"
#include "XRSurfaceContext.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Util
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Util : public CXR_VBOperator
{
public:
	fp32 GetParam(const class CXW_LayerOperation& _Oper, int iParam, fp32 _Default) const;

	// UVW helpers
	virtual bool UVW_AddTransform(CXR_VBOperatorContext& _Context, class CXR_VertexBuffer* _pVB, const CMat4Dfp32& _Mat);
	virtual bool UVW_FlattenTransform(CXR_VBOperatorContext& _Context, class CXR_VertexBuffer* _pVB);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_TextureAnim
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_TextureAnim : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Texture animation"; };
	virtual int GetNumParams() { return 2; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual int OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const;
	virtual int OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Offset
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Offset : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "UVW Coordinate offset."; };
	virtual int GetNumParams() { return 3; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Scroll
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Scroll : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "UVW Coordinate Scroll."; };
	virtual int GetNumParams() { return 6; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Scale
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Scale : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "UVW Coordinate scaling."; };
	virtual int GetNumParams() { return 3; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Rotate
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Rotate : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "UVW Coordinate rotation."; };
	virtual int GetNumParams() { return 8; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenReflection
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_GenReflection : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Environment-mapping generation for cube maps using view-space pos/normal."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenEnv
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_GenEnv : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Simple and fast environment-mapping generation using view-space normals."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenEnv2
|__________________________________________________________________________________________________
\*************************************************************************************************/
#if 1

class CXR_VBOperator_GenEnv2 : public CXR_VBOperator_GenEnv
{
	MRTC_DECLARE;
};

#else

class CXR_VBOperator_GenEnv2 : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Environment-mapping suitable for world geometry."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};
#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Wave
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Wave : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	enum
	{
		VBOP_WAVE_AMP = 0,
		VBOP_WAVE_FREQUENCY,
		VBOP_WAVE_PERIODX,
		VBOP_WAVE_PERIODY,
		VBOP_WAVE_PERIODZ,
		VBOP_WAVE_NUMPARAMS,
	};

	virtual CStr GetDesc() { return "Sine-wave vertex pertubation with normal."; };
	virtual int GetNumParams() { return VBOP_WAVE_NUMPARAMS; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);
	virtual bool OnOperateFinish(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_GenColorViewN
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_GenColorViewN : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "View normal color-component generator."; };
	virtual int GetNumParams() { return 2; }
	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Particles
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Particles : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Convert mesh to particles."; };
	virtual int GetNumParams() { return 1; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Debug_Normals
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Debug_Normals : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Draw normals."; };
	virtual int GetNumParams() { return 1; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_Debug_Tangents
|__________________________________________________________________________________________________
\*************************************************************************************************/

class CXR_VBOperator_Debug_Tangents : public CXR_VBOperator
{
	MRTC_DECLARE;

public:
	virtual CStr GetDesc() { return "Draw tangents."; };
	virtual int GetNumParams() { return 1; }

	static CXR_VBOperatorParamDesc ms_lParamDesc[];
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_MulDecalBlend
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_MulDecalBlend : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	uint16 m_lTextureID[2];

	CXR_VBOperator_MulDecalBlend();

	virtual CStr GetDesc() { return "Texture blending for multiply-decals."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);

	virtual int OnGetTextureCount(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper) const;
	virtual int OnEnumTextureID(const class CXW_Surface* _pSurf, const class CXW_SurfaceLayer* _pSurfLayer, const class CXW_LayerOperation& _Oper, int _iEnum) const;
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_DarklingWallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/

// Note:
//
// This shouldn't be done from a VB operator, the support should be in the wallmark code itself!
//

class CXR_VBOperator_UseShader : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	CXR_VBOperator_UseShader();

	virtual CStr GetDesc() { return "Darkling wallmark surface rendering through a specific shader"; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; }//ms_lParamDesc; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_FP20_Water
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_FP20_Water : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

public:
	CXR_VBOperator_FP20_Water();

	virtual CStr GetDesc() { return "FP20 water renderer."; };
	virtual int GetNumParams() { return 0; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return NULL; };

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper, class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurf, const class CXW_LayerOperation& _Oper);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_VBOperator_FP20_Distort
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_VBOperator_FP20_Distort : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

	static CXR_VBOperatorParamDesc ms_lParamDesc[];

public:
	CXR_VBOperator_FP20_Distort();

	virtual CStr GetDesc() { return "FP20 Distort renderer."; }
	virtual const CXR_VBOperatorParamDesc* GetParamDesc() { return ms_lParamDesc; }
	virtual int GetNumParams() { return 3; }

	virtual bool OnOperate(CXR_VBOperatorContext& _Context, const class CXW_LayerOperation& _Oper,class CXR_VertexBuffer* _pVB);

	virtual bool OnTestHWAccelerated(CXR_VBOperatorContext& _Context, class CRC_Attributes* _pAttrib, const class CXW_LayerOperation& _Oper);

	virtual void OnInitSurface(class CXW_Surface* _pSurface, const class CXW_LayerOperation& _Oper);
};


#endif // __INC_XRVBOPERATORS

