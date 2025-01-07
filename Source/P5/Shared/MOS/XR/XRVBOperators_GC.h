#ifndef _XRVBOPERATORS_GC_H
#define _XRVBOPERATORS_GC_H

#include  "XR.h"
#include  "XRSurfaceContext.h"
#include  "XRVBOperators.h"

/*
  CXR_VBOperator_GC_Bump.

*/
class CXR_VBOperator_GC_Bump : public CXR_VBOperator_Util
{
	MRTC_DECLARE;

  public:
          virtual CStr GetDesc()  { return "GameCube-only perturbed reflection vector cube mapping."; };

          virtual int GetNumParams()  { return( 0 );  }
          virtual const CXR_VBOperatorParamDesc *GetParamDesc() { return( NULL ); }

          virtual bool OnOperate( CXR_VBOperatorContext &_Context, const class CXW_LayerOperation &_Oper, class CXR_VertexBuffer *_pVB );

          virtual bool OnTestHWAccelerated( CXR_VBOperatorContext &_Context, class CRC_Attributes *_pAttrib, const class CXW_LayerOperation &_Oper );
          virtual void OnInitSurface( class CXW_Surface *_pSurf, const class CXW_LayerOperation &_Oper );
};

#endif
