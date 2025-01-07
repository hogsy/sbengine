
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Cube
|__________________________________________________________________________________________________
\*************************************************************************************************/


fp32 Vertices_ECubeMesh_Cube[] = 
{
	0.5, 0.5, 0 // Front
	,0.5, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0

	,-0.5, 0.5, -1.0 // Back
	,-0.5, -0.5, -1.0
	,0.5, -0.5, -1.0
	,0.5, 0.5, -1.0

	,0.5, 0.5, -1.0 // Left
	,0.5, -0.5, -1.0
	,0.5, -0.5, 0
	,0.5, 0.5, 0

	,-0.5, 0.5, 0 // Right
	,-0.5, -0.5, 0
	,-0.5, -0.5, -1.0
	,-0.5, 0.5, -1.0

	,0.5, -0.5, 0 // Top
	,0.5, -0.5, -1.0
	,-0.5, -0.5, -1.0
	,-0.5, -0.5, 0

	,0.5, 0.5, -1.0 // Bottom
	,0.5, 0.5, 0
	,-0.5, 0.5, 0
	,-0.5, 0.5, -1.0

};

fp32 Normals_ECubeMesh_Cube[] = 
{
	0, 0, 1 // Front
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1

	,0, 0, -1 // Back
	,0, 0, -1
	,0, 0, -1
	,0, 0, -1

	,1, 0, 0 // Left
	,1, 0, 0
	,1, 0, 0
	,1, 0, 0

	,-1, 0, 0 // Right
	,-1, 0, 0
	,-1, 0, 0
	,-1, 0, 0

	,0, -1, 0 // Top
	,0, -1, 0
	,0, -1, 0
	,0, -1, 0

	,0, 1, 0 // Bottom
	,0, 1, 0
	,0, 1, 0
	,0, 1, 0

};

const static fp32 TextureSizeX = 1.0f/16.0f;
const static fp32 TextureSizeY = 1.0f/32.0f;
fp32 DiffuseTexture_ECubeMesh_Cube[] = 
{
	0,				TextureSizeY // Front
	,0,				0
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY

	,0,				TextureSizeY // Back
	,0,				0
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY

	,TextureSizeX,	TextureSizeY // Left
	,0,				TextureSizeY
	,0,				0
	,TextureSizeX,	0

	,0,				0 // Right
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY
	,0,				TextureSizeY

	,TextureSizeX,	0 // Top
	,TextureSizeX,	TextureSizeY
	,0,				TextureSizeY
	,0,				0

	,0,				TextureSizeY // Bottom
	,0,				0
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY

};

uint8 TextureMatrixPalette_ECubeMesh_Cube[] = 
{
		1,1,1,1 // Front
		,0,0,0,0 // Back
		,0,0,0,0 // Left
		,0,0,0,0 // Right
		,0,0,0,0 // Top
		,0,0,0,0 // Bottom
};

uint8 TextureScaleZ_ECubeMesh_Cube[] = 
{
		0,0,0,0 // Front
		,0,0,0,0 // Back
		,1,1,1,1 // Left
		,1,1,1,1 // Right
		,1,1,1,1 // Top
		,1,1,1,1 // Bottom
};

uint16 Indices_ECubeMesh_Cube[] = 
{
	0, 1, 2 // Front
	,0, 2, 3

	,4+0, 4+1, 4+2 // Back
	,4+0, 4+2, 4+3

	,8+0, 8+1, 8+2 // Left
	,8+0, 8+2, 8+3

	,12+0, 12+1, 12+2 // Right
	,12+0, 12+2, 12+3

	,16+0, 16+1, 16+2 // Top
	,16+0, 16+2, 16+3

	,20+0, 20+1, 20+2 // Bottom
	,20+0, 20+2, 20+3

};

enum
{
	 EFront = 0
	,EBack = 4
	,ELeft = 8
	,ERight = 12
	,ETop = 16
	,EBottom = 20
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| SplitCube
|__________________________________________________________________________________________________
\*************************************************************************************************/


fp32 Vertices_ECubeMesh_SplitCube[] = 
{
	 0.5, 0.5, 0 // Left
	,0.5, -0.5, 0
	,0.0, -0.5, 0
	,0.0, 0.5, 0

	,0.0, 0.5, 0 // Right
	,0.0, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0

	,-0.5, 0.5, -1.0 // Back
	,-0.5, -0.5, -1.0
	,0.5, -0.5, -1.0
	,0.5, 0.5, -1.0

	,0.5, 0.5, -1.0 // Left
	,0.5, -0.5, -1.0
	,0.5, -0.5, 0
	,0.5, 0.5, 0

	,-0.5, 0.5, 0 // Right
	,-0.5, -0.5, 0
	,-0.5, -0.5, -1.0
	,-0.5, 0.5, -1.0

	,0.5, -0.5, 0 // Top
	,0.5, -0.5, -1.0
	,-0.5, -0.5, -1.0
	,-0.5, -0.5, 0

	,0.5, 0.5, -1.0 // Bottom
	,0.5, 0.5, 0
	,-0.5, 0.5, 0
	,-0.5, 0.5, -1.0

};

fp32 Normals_ECubeMesh_SplitCube[] = 
{
	0, 0, 1 // Left
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1

	,0, 0, 1 // Right
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1

	,0, 0, -1 // Back
	,0, 0, -1
	,0, 0, -1
	,0, 0, -1

	,1, 0, 0 // Left
	,1, 0, 0
	,1, 0, 0
	,1, 0, 0

	,-1, 0, 0 // Right
	,-1, 0, 0
	,-1, 0, 0
	,-1, 0, 0

	,0, -1, 0 // Top
	,0, -1, 0
	,0, -1, 0
	,0, -1, 0

	,0, 1, 0 // Bottom
	,0, 1, 0
	,0, 1, 0
	,0, 1, 0

};

fp32 DiffuseTexture_ECubeMesh_SplitCube[] = 
{
	0,				TextureSizeY // Left
	,0,				0
	,TextureSizeX/2,	0
	,TextureSizeX/2,	TextureSizeY

	,0,				TextureSizeY // Right
	,0,				0
	,TextureSizeX/2,	0
	,TextureSizeX/2,	TextureSizeY

	,0,				TextureSizeY // Back
	,0,				0
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY

	,TextureSizeX,	TextureSizeY // Left
	,0,				TextureSizeY
	,0,				0
	,TextureSizeX,	0

	,0,				0 // Right
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY
	,0,				TextureSizeY

	,TextureSizeX,	0 // Top
	,TextureSizeX,	TextureSizeY
	,0,				TextureSizeY
	,0,				0

	,0,				TextureSizeY // Bottom
	,0,				0
	,TextureSizeX,	0
	,TextureSizeX,	TextureSizeY

};

uint8 TextureMatrixPalette_ECubeMesh_SplitCube[] = 
{
		1,1,1,1 // Left
		,2,2,2,2 // Right
		,0,0,0,0 // Back
		,0,0,0,0 // Left
		,0,0,0,0 // Right
		,0,0,0,0 // Top
		,0,0,0,0 // Bottom
};

uint8 TextureScaleZ_ECubeMesh_SplitCube[] = 
{
		0,0,0,0 // Left
		,0,0,0,0 // Right
		,0,0,0,0 // Back
		,1,1,1,1 // Left
		,1,1,1,1 // Right
		,1,1,1,1 // Top
		,1,1,1,1 // Bottom
};

uint16 Indices_ECubeMesh_SplitCube[] = 
{
	0, 1, 2 // Front Left
	,0, 2, 3

	,4+0, 4+1, 4+2 // Front Left
	,4+0, 4+2, 4+3

	,8+0, 8+1, 8+2
	,8+0, 8+2, 8+3

	,12+0, 12+1, 12+2 
	,12+0, 12+2, 12+3

	,16+0, 16+1, 16+2 
	,16+0, 16+2, 16+3

	,20+0, 20+1, 20+2 
	,20+0, 20+2, 20+3

	,24+0, 24+1, 24+2 
	,24+0, 24+2, 24+3

};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Split Front
|__________________________________________________________________________________________________
\*************************************************************************************************/

fp32 Vertices_ECubeMesh_SplitFront[] = 
{
	 0.5, 0.5, 0 // Left
	,0.5, -0.5, 0
	,0.0, -0.5, 0
	,0.0, 0.5, 0

	,0.0, 0.5, 0 // Right
	,0.0, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0
	
};

fp32 Normals_ECubeMesh_SplitFront[] = 
{
	0, 0, 1 // Left
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1

	,0, 0, 1 // Right
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1
};

fp32 DiffuseTexture_ECubeMesh_SplitFront[] = 
{
	0,				TextureSizeY // Left
	,0,				0
	,TextureSizeX/2,	0
	,TextureSizeX/2,	TextureSizeY

	,0,				TextureSizeY // Right
	,0,				0
	,TextureSizeX/2,	0
	,TextureSizeX/2,	TextureSizeY
};

uint8 TextureMatrixPalette_ECubeMesh_SplitFront[] = 
{
		1,1,1,1 // Left
		,2,2,2,2 // Right
};

uint8 TextureScaleZ_ECubeMesh_SplitFront[] = 
{
		0,0,0,0 // Left
		,0,0,0,0 // Right
};

uint16 Indices_ECubeMesh_SplitFront[] = 
{
	0, 1, 2 // Front Left
	,0, 2, 3

	,4+0, 4+1, 4+2 // Front Left
	,4+0, 4+2, 4+3
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Shadow Front
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifndef DShadowDebug
fp32 Vertices_ECubeMesh_ShadowFront[] = 
{
	0.5, 0.5, 0
	,0.5, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0

	,-0.5, 0.5, 0
	,-0.5, -0.5, 0
	,0.5, -0.5, 0
	,0.5, 0.5, 0

};
#else
fp32 Vertices_ECubeMesh_ShadowFront[] = 
{
		0.5, 0.5, 0
	,0.5, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0

	,-0.5, 0.5, -0.5
	,-0.5, -0.5, -0.5
	,0.5, -0.5, -0.5
	,0.5, 0.5, -0.5

};
#endif

fp32 Normals_ECubeMesh_ShadowFront[] = 
{
		0, 0, 1
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1
	,0, 0, -1
	,0, 0, -1
	,0, 0, -1
	,0, 0, -1
};

uint16 Indices_ECubeMesh_ShadowFront[] = 
{
	 0, 1, 2 // Front
	,0, 2, 3

	,4, 5, 6 // Back
	,4, 6, 7

	,EBack+3, EBack+2, EFront+1 // Left
	,EBack+3, EFront+1, EFront+0

	,EFront+3, EFront+2, EBack+1 // Right
	,EFront+3, EBack+1, EBack+0

	,EFront+1, EBack+2, EBack+1 // Top
	,EFront+1, EBack+1, EFront+2

	,EBack+3, EFront+0, EFront+3 // Bottom
	,EBack+3, EFront+3, EBack+0
};

#ifndef DShadowDebug
fp32 Vertices_ECubeMesh_ShadowCube[] = 
{
	0.5, 0.5, 0 // Front
	,0.5, -0.5, 0
	,-0.5, -0.5, 0
	,-0.5, 0.5, 0

	,-0.5, 0.5, -1.0 // Back
	,-0.5, -0.5, -1.0
	,0.5, -0.5, -1.0
	,0.5, 0.5, -1.0

	,0.5, 0.5, -1.0 // Left
	,0.5, -0.5, -1.0
	,0.5, -0.5, 0
	,0.5, 0.5, 0

	,-0.5, 0.5, 0 // Right
	,-0.5, -0.5, 0
	,-0.5, -0.5, -1.0
	,-0.5, 0.5, -1.0

	,0.5, -0.5, 0 // Top
	,0.5, -0.5, -1.0
	,-0.5, -0.5, -1.0
	,-0.5, -0.5, 0

	,0.5, 0.5, -1.0 // Bottom
	,0.5, 0.5, 0
	,-0.5, 0.5, 0
	,-0.5, 0.5, -1.0
};
#else
fp32 Vertices_ECubeMesh_ShadowCube[] = 
{
		0.5, 0.5, 0.7 // Front
	,0.5, -0.5, 0.7
	,-0.5, -0.5, 0.7
	,-0.5, 0.5, 0.7

	,-0.5, 0.5, -0.7 // Back
	,-0.5, -0.5, -0.7
	,0.5, -0.5, -0.7
	,0.5, 0.5, -0.7

	,0.7, 0.5, -0.5 // Left
	,0.7, -0.5, -0.5
	,0.7, -0.5, 0.5
	,0.7, 0.5, 0.5

	,-0.7, 0.5, 0.5 // Right
	,-0.7, -0.5, 0.5
	,-0.7, -0.5, -0.5
	,-0.7, 0.5, -0.5

	,0.5, -0.7, 0.5 // Top
	,0.5, -0.7, -0.5
	,-0.5, -0.7, -0.5
	,-0.5, -0.7, 0.5

	,0.5, 0.7, -0.5 // Bottom
	,0.5, 0.7, 0.5
	,-0.5, 0.7, 0.5
	,-0.5, 0.7, -0.5
};
#endif

fp32 Normals_ECubeMesh_ShadowCube[] = 
{
		0, 0, 1 // Front
	,0, 0, 1
	,0, 0, 1
	,0, 0, 1

	,0, 0, -1 // Back
	,0, 0, -1
	,0, 0, -1
	,0, 0, -1

	,1, 0, 0 // Left
	,1, 0, 0
	,1, 0, 0
	,1, 0, 0

	,-1, 0, 0 // Right
	,-1, 0, 0
	,-1, 0, 0
	,-1, 0, 0

	,0, -1, 0 // Top
	,0, -1, 0
	,0, -1, 0
	,0, -1, 0

	,0, 1, 0 // Bottom
	,0, 1, 0
	,0, 1, 0
	,0, 1, 0
};

uint16 Indices_ECubeMesh_ShadowCube[] = 
{
	EFront+0, EFront+1, EFront+2 // Front
	,EFront+0, EFront+2, EFront+3

	,EBack+0, EBack+1, EBack+2 // Back
	,EBack+0, EBack+2, EBack+3

	,ELeft+0, ELeft+1, ELeft+2 // Left
	,ELeft+0, ELeft+2, ELeft+3

	,ERight+0, ERight+1, ERight+2 // Right
	,ERight+0, ERight+2, ERight+3

	,ETop+0, ETop+1, ETop+2 // Top
	,ETop+0, ETop+2, ETop+3

	,EBottom+0, EBottom+1, EBottom+2 // Bottom
	,EBottom+0, EBottom+2, EBottom+3

	,ELeft+3, ELeft+2, EFront+1 // Left(3,2)Front(1,0)
	,ELeft+3, EFront+1, EFront+0

	,EFront+3, EFront+2, ERight+1 // Front(3,2)Right(1,0)
	,EFront+3, ERight+1, ERight+0

	,EFront+2, EFront+1, ETop+0 // Front(2,1)Top(0,3)
	,EFront+2, ETop+0, ETop+3

	,EBottom+2, EBottom+1, EFront+0 // Bottom(2,1)Front(0,3)
	,EBottom+2, EFront+0, EFront+3

	,ELeft+2, ELeft+1, ETop+1 // Left(2,1)Top(1,0)
	,ELeft+2, ETop+1, ETop+0

	,ETop+3, ETop+2, ERight+2 // Top(3,2)Right(2,1)
	,ETop+3, ERight+2, ERight+1

	,ELeft+0, ELeft+3, EBottom+1 // Left(0,3)Bottom(1,0)
	,ELeft+0, EBottom+1, EBottom+0

	,EBottom+3, EBottom+2, ERight+0 // Bottom(3,2)Right(0,3)
	,EBottom+3, ERight+0, ERight+3

	,EBack+3, EBack+2, ELeft+1 // Back(3,2)Left(1,0)
	,EBack+3, ELeft+1, ELeft+0

	,ERight+3, ERight+2, EBack+1 // Right(3,2)Back(1,0)
	,ERight+3, EBack+1, EBack+0

	,EBack+2, EBack+1, ETop+2 // Back(2,1)Top(2,1)
	,EBack+2, ETop+2, ETop+1

	,EBack+0, EBack+3, EBottom+0 // Back(0,3)Bottom(0,3)
	,EBack+0, EBottom+0, EBottom+3

};


fp32 *CCube::CVertexBuffers::ms_pVertices[CCube::CVertexBuffers::ENumIds] = {NULL};
fp32 *CCube::CVertexBuffers::ms_pNormals[CCube::CVertexBuffers::ENumIds] = {NULL};
fp32 *CCube::CVertexBuffers::ms_pDiffuseTexture[CCube::CVertexBuffers::ENumIds] = {NULL};
uint16 *CCube::CVertexBuffers::ms_pIndices[CCube::CVertexBuffers::ENumIds] = {NULL};
uint8  *CCube::CVertexBuffers::ms_pTextureMatrixPalette[CCube::CVertexBuffers::ENumIds] = {NULL};
uint8  *CCube::CVertexBuffers::ms_pTextureScaleZ[CCube::CVertexBuffers::ENumIds] = {NULL};
int CCube::CVertexBuffers::ms_nVertices[CCube::CVertexBuffers::ENumIds] = {0};
int CCube::CVertexBuffers::ms_nIndices[CCube::CVertexBuffers::ENumIds] = {0};

CCube::CVertexBuffers::CVertexBuffers()
{
	// Normal
	ms_pVertices[ECubeMesh_Front] = Vertices_ECubeMesh_Cube;
	ms_pNormals[ECubeMesh_Front] = Normals_ECubeMesh_Cube;
	ms_pDiffuseTexture[ECubeMesh_Front] = DiffuseTexture_ECubeMesh_Cube;	
	ms_pIndices[ECubeMesh_Front] = Indices_ECubeMesh_Cube;
	ms_pTextureMatrixPalette[ECubeMesh_Front] = TextureMatrixPalette_ECubeMesh_Cube;
	ms_pTextureScaleZ[ECubeMesh_Front] = TextureScaleZ_ECubeMesh_Cube;
	ms_nVertices[ECubeMesh_Front] = 4*1;
	ms_nIndices[ECubeMesh_Front] = 6*1;

	ms_pVertices[ECubeMesh_Cube] = Vertices_ECubeMesh_Cube;
	ms_pNormals[ECubeMesh_Cube] = Normals_ECubeMesh_Cube;
	ms_pDiffuseTexture[ECubeMesh_Cube] = DiffuseTexture_ECubeMesh_Cube;	
	ms_pIndices[ECubeMesh_Cube] = Indices_ECubeMesh_Cube;
	ms_pTextureMatrixPalette[ECubeMesh_Cube] = TextureMatrixPalette_ECubeMesh_Cube;
	ms_pTextureScaleZ[ECubeMesh_Cube] = TextureScaleZ_ECubeMesh_Cube;
	ms_nVertices[ECubeMesh_Cube] = 4*6;
	ms_nIndices[ECubeMesh_Cube] = 6*6;

	ms_pVertices[ECubeMesh_SplitFront] = Vertices_ECubeMesh_SplitFront;
	ms_pNormals[ECubeMesh_SplitFront] = Normals_ECubeMesh_SplitFront;
	ms_pDiffuseTexture[ECubeMesh_SplitFront] = DiffuseTexture_ECubeMesh_SplitFront;	
	ms_pIndices[ECubeMesh_SplitFront] = Indices_ECubeMesh_SplitFront;
	ms_pTextureMatrixPalette[ECubeMesh_SplitFront] = TextureMatrixPalette_ECubeMesh_SplitFront;
	ms_pTextureScaleZ[ECubeMesh_SplitFront] = TextureScaleZ_ECubeMesh_SplitFront;
	ms_nVertices[ECubeMesh_SplitFront] = 4*2;
	ms_nIndices[ECubeMesh_SplitFront] = 6*2;

	ms_pVertices[ECubeMesh_SplitCube] = Vertices_ECubeMesh_SplitCube;
	ms_pNormals[ECubeMesh_SplitCube] = Normals_ECubeMesh_SplitCube;
	ms_pDiffuseTexture[ECubeMesh_SplitCube] = DiffuseTexture_ECubeMesh_SplitCube;	
	ms_pIndices[ECubeMesh_SplitCube] = Indices_ECubeMesh_SplitCube;
	ms_pTextureMatrixPalette[ECubeMesh_SplitCube] = TextureMatrixPalette_ECubeMesh_SplitCube;
	ms_pTextureScaleZ[ECubeMesh_SplitCube] = TextureScaleZ_ECubeMesh_SplitCube;
	ms_nVertices[ECubeMesh_SplitCube] = sizeof(Vertices_ECubeMesh_SplitCube) / sizeof(CVec3Dfp32);
	ms_nIndices[ECubeMesh_SplitCube] = sizeof(Indices_ECubeMesh_SplitCube) / 2;

	ms_pVertices[ECubeMesh_Sides] = Vertices_ECubeMesh_Cube;
	ms_pNormals[ECubeMesh_Sides] = Normals_ECubeMesh_Cube;
	ms_pDiffuseTexture[ECubeMesh_Sides] = DiffuseTexture_ECubeMesh_Cube;
	ms_pIndices[ECubeMesh_Sides] = Indices_ECubeMesh_Cube + 6;
	ms_pTextureMatrixPalette[ECubeMesh_Sides] = TextureMatrixPalette_ECubeMesh_Cube;
	ms_pTextureScaleZ[ECubeMesh_Sides] = TextureScaleZ_ECubeMesh_Cube;
	ms_nVertices[ECubeMesh_Sides] = 4*6;
	ms_nIndices[ECubeMesh_Sides] = 6*5;

	// Shadows
	ms_pVertices[ECubeMesh_ShadowFront] = Vertices_ECubeMesh_ShadowFront;
	ms_pNormals[ECubeMesh_ShadowFront] = Normals_ECubeMesh_ShadowFront;
	ms_pIndices[ECubeMesh_ShadowFront] = Indices_ECubeMesh_ShadowFront;
	ms_nVertices[ECubeMesh_ShadowFront] = 4*2;
	ms_nIndices[ECubeMesh_ShadowFront] = 6*6;

	ms_pVertices[ECubeMesh_ShadowCube] = Vertices_ECubeMesh_ShadowCube;
	ms_pNormals[ECubeMesh_ShadowCube] = Normals_ECubeMesh_ShadowCube;
	ms_pIndices[ECubeMesh_ShadowCube] = Indices_ECubeMesh_ShadowCube;
	ms_nVertices[ECubeMesh_ShadowCube] = 4*6;
	ms_nIndices[ECubeMesh_ShadowCube] = 6*6 + 12*6;

	M_TRACEALWAYS("%d\n", ECubeMaxMPEntries);
	for (int i = 0; i < ENumIds; ++i)
	{
		m_IDs[i] = m_pVBCtx->AllocID(m_iVBC, i);
	}
}
