
#include "PCH.h"
#include "MRender.h"
#include "MRCCore.h"

#pragma warning(disable:4005)
#define CollectNameI(NPreName,NVertex,NNormal,NColor,NSpecular,NFog,NNumTexture,NonStandard) NPreName##NonStandard##NNumTexture##_##NFog##NSpecular##NColor##NNormal##NVertex
#define CollectName(NPreName,NVertex,NNormal,NColor,NSpecular,NFog,NNumTexture,NonStandard) CollectNameI(NPreName,NVertex,NNormal,NColor,NSpecular,NFog,NNumTexture,NonStandard)

#define CN_NonStandard 0

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_NonStandard
#define CN_NonStandard 1

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 0
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 1
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 2
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 3
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 0
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 0
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 0
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 0
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

#undef CN_Vertex
#define CN_Vertex 1
#undef CN_Normal
#define CN_Normal 1
#undef CN_Color
#define CN_Color 1
#undef CN_Specular
#define CN_Specular 1
#undef CN_Fog
#define CN_Fog 1
#undef CN_NumTexture
#define CN_NumTexture 4
#include"MRCCollectFunction.cpp"

FCollect CRC_Core::m_lCollectFunctionsAll[160] =
{
	// Standard no texture
	 CollectAll_00_00001 // 1
	,CollectAll_00_00011 // 1
	,CollectAll_00_00101 // 3
	,CollectAll_00_00111 // 5
	,CollectAll_00_01001 // 7
	,CollectAll_00_01011 // 9
	,CollectAll_00_01101 // 11
	,CollectAll_00_01111 // 13
	,CollectAll_00_10001 // 15
	,CollectAll_00_10011 // 3
	,CollectAll_00_10101 // 5
	,CollectAll_00_10111 // 7
	,CollectAll_00_11001 // 9
	,CollectAll_00_11011 // 11
	,CollectAll_00_11101 // 13
	,CollectAll_00_11111 // 15

	// Standard 1 texture
	,CollectAll_01_00001 // 1
	,CollectAll_01_00011 // 1
	,CollectAll_01_00101 // 3
	,CollectAll_01_00111 // 5
	,CollectAll_01_01001 // 7
	,CollectAll_01_01011 // 9
	,CollectAll_01_01101 // 11
	,CollectAll_01_01111 // 13
	,CollectAll_01_10001 // 15
	,CollectAll_01_10011 // 3
	,CollectAll_01_10101 // 5
	,CollectAll_01_10111 // 7
	,CollectAll_01_11001 // 9
	,CollectAll_01_11011 // 11
	,CollectAll_01_11101 // 13
	,CollectAll_01_11111 // 15

	// Standard 2 texture
	,CollectAll_02_00001 // 1
	,CollectAll_02_00011 // 1
	,CollectAll_02_00101 // 3
	,CollectAll_02_00111 // 5
	,CollectAll_02_01001 // 7
	,CollectAll_02_01011 // 9
	,CollectAll_02_01101 // 11
	,CollectAll_02_01111 // 13
	,CollectAll_02_10001 // 15
	,CollectAll_02_10011 // 3
	,CollectAll_02_10101 // 5
	,CollectAll_02_10111 // 7
	,CollectAll_02_11001 // 9
	,CollectAll_02_11011 // 11
	,CollectAll_02_11101 // 13
	,CollectAll_02_11111 // 15

	// Standard 3 texture
	,CollectAll_03_00001 // 1
	,CollectAll_03_00011 // 1
	,CollectAll_03_00101 // 3
	,CollectAll_03_00111 // 5
	,CollectAll_03_01001 // 7
	,CollectAll_03_01011 // 9
	,CollectAll_03_01101 // 11
	,CollectAll_03_01111 // 13
	,CollectAll_03_10001 // 15
	,CollectAll_03_10011 // 3
	,CollectAll_03_10101 // 5
	,CollectAll_03_10111 // 7
	,CollectAll_03_11001 // 9
	,CollectAll_03_11011 // 11
	,CollectAll_03_11101 // 13
	,CollectAll_03_11111 // 15

	// Standard 4 texture
	,CollectAll_04_00001 // 1
	,CollectAll_04_00011 // 1
	,CollectAll_04_00101 // 3
	,CollectAll_04_00111 // 5
	,CollectAll_04_01001 // 7
	,CollectAll_04_01011 // 9
	,CollectAll_04_01101 // 11
	,CollectAll_04_01111 // 13
	,CollectAll_04_10001 // 15
	,CollectAll_04_10011 // 3
	,CollectAll_04_10101 // 5
	,CollectAll_04_10111 // 7
	,CollectAll_04_11001 // 9
	,CollectAll_04_11011 // 11
	,CollectAll_04_11101 // 13
	,CollectAll_04_11111 // 15


	// Non Standard 0 texture
	,CollectAll_10_00001 // 1
	,CollectAll_10_00011 // 1
	,CollectAll_10_00101 // 3
	,CollectAll_10_00111 // 5
	,CollectAll_10_01001 // 7
	,CollectAll_10_01011 // 9
	,CollectAll_10_01101 // 11
	,CollectAll_10_01111 // 13
	,CollectAll_10_10001 // 15
	,CollectAll_10_10011 // 3
	,CollectAll_10_10101 // 5
	,CollectAll_10_10111 // 7
	,CollectAll_10_11001 // 9
	,CollectAll_10_11011 // 11
	,CollectAll_10_11101 // 13
	,CollectAll_10_11111 // 15

	// Non Standard 1 texture
	,CollectAll_11_00001 // 1
	,CollectAll_11_00011 // 1
	,CollectAll_11_00101 // 3
	,CollectAll_11_00111 // 5
	,CollectAll_11_01001 // 7
	,CollectAll_11_01011 // 9
	,CollectAll_11_01101 // 11
	,CollectAll_11_01111 // 13
	,CollectAll_11_10001 // 15
	,CollectAll_11_10011 // 3
	,CollectAll_11_10101 // 5
	,CollectAll_11_10111 // 7
	,CollectAll_11_11001 // 9
	,CollectAll_11_11011 // 11
	,CollectAll_11_11101 // 13
	,CollectAll_11_11111 // 15

	// Non Standard 2 texture
	,CollectAll_12_00001 // 1
	,CollectAll_12_00011 // 1
	,CollectAll_12_00101 // 3
	,CollectAll_12_00111 // 5
	,CollectAll_12_01001 // 7
	,CollectAll_12_01011 // 9
	,CollectAll_12_01101 // 11
	,CollectAll_12_01111 // 13
	,CollectAll_12_10001 // 15
	,CollectAll_12_10011 // 3
	,CollectAll_12_10101 // 5
	,CollectAll_12_10111 // 7
	,CollectAll_12_11001 // 9
	,CollectAll_12_11011 // 11
	,CollectAll_12_11101 // 13
	,CollectAll_12_11111 // 15

	// Non Standard 3 texture
	,CollectAll_13_00001 // 1
	,CollectAll_13_00011 // 1
	,CollectAll_13_00101 // 3
	,CollectAll_13_00111 // 5
	,CollectAll_13_01001 // 7
	,CollectAll_13_01011 // 9
	,CollectAll_13_01101 // 11
	,CollectAll_13_01111 // 13
	,CollectAll_13_10001 // 15
	,CollectAll_13_10011 // 3
	,CollectAll_13_10101 // 5
	,CollectAll_13_10111 // 7
	,CollectAll_13_11001 // 9
	,CollectAll_13_11011 // 11
	,CollectAll_13_11101 // 13
	,CollectAll_13_11111 // 15

	// Non Standard 4 texture
	,CollectAll_14_00001 // 1
	,CollectAll_14_00011 // 1
	,CollectAll_14_00101 // 3
	,CollectAll_14_00111 // 5
	,CollectAll_14_01001 // 7
	,CollectAll_14_01011 // 9
	,CollectAll_14_01101 // 11
	,CollectAll_14_01111 // 13
	,CollectAll_14_10001 // 15
	,CollectAll_14_10011 // 3
	,CollectAll_14_10101 // 5
	,CollectAll_14_10111 // 7
	,CollectAll_14_11001 // 9
	,CollectAll_14_11011 // 11
	,CollectAll_14_11101 // 13
	,CollectAll_14_11111 // 15
};

FCollect CRC_Core::m_lCollectFunctionsIndexed[160] = 
{
	 CollectIndexed_00_00001 // 1
	,CollectIndexed_00_00011 // 1
	,CollectIndexed_00_00101 // 3
	,CollectIndexed_00_00111 // 5
	,CollectIndexed_00_01001 // 7
	,CollectIndexed_00_01011 // 9
	,CollectIndexed_00_01101 // 11
	,CollectIndexed_00_01111 // 13
	,CollectIndexed_00_10001 // 15
	,CollectIndexed_00_10011 // 3
	,CollectIndexed_00_10101 // 5
	,CollectIndexed_00_10111 // 7
	,CollectIndexed_00_11001 // 9
	,CollectIndexed_00_11011 // 11
	,CollectIndexed_00_11101 // 13
	,CollectIndexed_00_11111 // 15

	,CollectIndexed_01_00001 // 1
	,CollectIndexed_01_00011 // 1
	,CollectIndexed_01_00101 // 3
	,CollectIndexed_01_00111 // 5
	,CollectIndexed_01_01001 // 7
	,CollectIndexed_01_01011 // 9
	,CollectIndexed_01_01101 // 11
	,CollectIndexed_01_01111 // 13
	,CollectIndexed_01_10001 // 15
	,CollectIndexed_01_10011 // 3
	,CollectIndexed_01_10101 // 5
	,CollectIndexed_01_10111 // 7
	,CollectIndexed_01_11001 // 9
	,CollectIndexed_01_11011 // 11
	,CollectIndexed_01_11101 // 13
	,CollectIndexed_01_11111 // 15

	,CollectIndexed_02_00001 // 1
	,CollectIndexed_02_00011 // 1
	,CollectIndexed_02_00101 // 3
	,CollectIndexed_02_00111 // 5
	,CollectIndexed_02_01001 // 7
	,CollectIndexed_02_01011 // 9
	,CollectIndexed_02_01101 // 11
	,CollectIndexed_02_01111 // 13
	,CollectIndexed_02_10001 // 15
	,CollectIndexed_02_10011 // 3
	,CollectIndexed_02_10101 // 5
	,CollectIndexed_02_10111 // 7
	,CollectIndexed_02_11001 // 9
	,CollectIndexed_02_11011 // 11
	,CollectIndexed_02_11101 // 13
	,CollectIndexed_02_11111 // 15

	,CollectIndexed_03_00001 // 1
	,CollectIndexed_03_00011 // 1
	,CollectIndexed_03_00101 // 3
	,CollectIndexed_03_00111 // 5
	,CollectIndexed_03_01001 // 7
	,CollectIndexed_03_01011 // 9
	,CollectIndexed_03_01101 // 11
	,CollectIndexed_03_01111 // 13
	,CollectIndexed_03_10001 // 15
	,CollectIndexed_03_10011 // 3
	,CollectIndexed_03_10101 // 5
	,CollectIndexed_03_10111 // 7
	,CollectIndexed_03_11001 // 9
	,CollectIndexed_03_11011 // 11
	,CollectIndexed_03_11101 // 13
	,CollectIndexed_03_11111 // 15

	,CollectIndexed_04_00001 // 1
	,CollectIndexed_04_00011 // 1
	,CollectIndexed_04_00101 // 3
	,CollectIndexed_04_00111 // 5
	,CollectIndexed_04_01001 // 7
	,CollectIndexed_04_01011 // 9
	,CollectIndexed_04_01101 // 11
	,CollectIndexed_04_01111 // 13
	,CollectIndexed_04_10001 // 15
	,CollectIndexed_04_10011 // 3
	,CollectIndexed_04_10101 // 5
	,CollectIndexed_04_10111 // 7
	,CollectIndexed_04_11001 // 9
	,CollectIndexed_04_11011 // 11
	,CollectIndexed_04_11101 // 13
	,CollectIndexed_04_11111 // 15

	,CollectIndexed_10_00001 // 1
	,CollectIndexed_10_00011 // 1
	,CollectIndexed_10_00101 // 3
	,CollectIndexed_10_00111 // 5
	,CollectIndexed_10_01001 // 7
	,CollectIndexed_10_01011 // 9
	,CollectIndexed_10_01101 // 11
	,CollectIndexed_10_01111 // 13
	,CollectIndexed_10_10001 // 15
	,CollectIndexed_10_10011 // 3
	,CollectIndexed_10_10101 // 5
	,CollectIndexed_10_10111 // 7
	,CollectIndexed_10_11001 // 9
	,CollectIndexed_10_11011 // 11
	,CollectIndexed_10_11101 // 13
	,CollectIndexed_10_11111 // 15

	,CollectIndexed_11_00001 // 1
	,CollectIndexed_11_00011 // 1
	,CollectIndexed_11_00101 // 3
	,CollectIndexed_11_00111 // 5
	,CollectIndexed_11_01001 // 7
	,CollectIndexed_11_01011 // 9
	,CollectIndexed_11_01101 // 11
	,CollectIndexed_11_01111 // 13
	,CollectIndexed_11_10001 // 15
	,CollectIndexed_11_10011 // 3
	,CollectIndexed_11_10101 // 5
	,CollectIndexed_11_10111 // 7
	,CollectIndexed_11_11001 // 9
	,CollectIndexed_11_11011 // 11
	,CollectIndexed_11_11101 // 13
	,CollectIndexed_11_11111 // 15

	,CollectIndexed_12_00001 // 1
	,CollectIndexed_12_00011 // 1
	,CollectIndexed_12_00101 // 3
	,CollectIndexed_12_00111 // 5
	,CollectIndexed_12_01001 // 7
	,CollectIndexed_12_01011 // 9
	,CollectIndexed_12_01101 // 11
	,CollectIndexed_12_01111 // 13
	,CollectIndexed_12_10001 // 15
	,CollectIndexed_12_10011 // 3
	,CollectIndexed_12_10101 // 5
	,CollectIndexed_12_10111 // 7
	,CollectIndexed_12_11001 // 9
	,CollectIndexed_12_11011 // 11
	,CollectIndexed_12_11101 // 13
	,CollectIndexed_12_11111 // 15

	,CollectIndexed_13_00001 // 1
	,CollectIndexed_13_00011 // 1
	,CollectIndexed_13_00101 // 3
	,CollectIndexed_13_00111 // 5
	,CollectIndexed_13_01001 // 7
	,CollectIndexed_13_01011 // 9
	,CollectIndexed_13_01101 // 11
	,CollectIndexed_13_01111 // 13
	,CollectIndexed_13_10001 // 15
	,CollectIndexed_13_10011 // 3
	,CollectIndexed_13_10101 // 5
	,CollectIndexed_13_10111 // 7
	,CollectIndexed_13_11001 // 9
	,CollectIndexed_13_11011 // 11
	,CollectIndexed_13_11101 // 13
	,CollectIndexed_13_11111 // 15

	,CollectIndexed_14_00001 // 1
	,CollectIndexed_14_00011 // 1
	,CollectIndexed_14_00101 // 3
	,CollectIndexed_14_00111 // 5
	,CollectIndexed_14_01001 // 7
	,CollectIndexed_14_01011 // 9
	,CollectIndexed_14_01101 // 11
	,CollectIndexed_14_01111 // 13
	,CollectIndexed_14_10001 // 15
	,CollectIndexed_14_10011 // 3
	,CollectIndexed_14_10101 // 5
	,CollectIndexed_14_10111 // 7
	,CollectIndexed_14_11001 // 9
	,CollectIndexed_14_11011 // 11
	,CollectIndexed_14_11101 // 13
	,CollectIndexed_14_11111 // 15
};

FCollect CRC_Core::Collect_GetFunctionAll(int _iFunction)
{
	return CRC_Core::m_lCollectFunctionsAll[_iFunction];
}

FCollect CRC_Core::Collect_GetFunctionIndexed(int _iFunction)
{
	return CRC_Core::m_lCollectFunctionsIndexed[_iFunction];
}
