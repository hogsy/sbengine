#ifndef CDynamicVB_h
#define CDynamicVB_h

//----------------------------------------------------------------------

#include "MFloat.h"

//----------------------------------------------------------------------

#define TransparencyPriorityMin			15.0f
#define TransparencyPriorityMax			16.0f
#define TransparencyPriorityBaseBias	0.5f
#define TransparencyPriorityBiasUnit	0.01f

//----------------------------------------------------------------------

enum DVBFLAGS
{
	DVBFLAGS_POSONLY		= 0x00,
	DVBFLAGS_COLORS			= 0x01,
	DVBFLAGS_TEXTURE		= 0x02,
	DVBFLAGS_TEXTURE_TRIS	= 0x04,
	DVBFLAGS_TEXTURE_QUADS	= 0x08,
	DVBFLAGS_NORMALS		= 0x10,
	DVBFLAGS_FLATSHADED		= 0x20,
	DVBFLAGS_CULLSORT		= 0x40,

	DVBFLAGS_DEFAULT		= 0x03,
};

//----------------------------------------------------------------------
// CDynamicVB
//----------------------------------------------------------------------

class CDynamicVB
{
	public:
//	private:

		int32				m_MaxNumVertices;
		int32				m_MaxNumTriangles;
		
		int32				m_NumVertices;
		int32				m_NumTriangles;

		CVec3Dfp32*			m_pVertexPos;
		CVec3Dfp32*			m_pVertexNormal;
		CVec2Dfp32*			m_pVertexTex;
		CPixel32*			m_pVertexColor;
		uint16*				m_pIndex;
		uint16*				m_pFlippedIndex;

		CMat4Dfp32*			m_pMatrix;

		class CXR_VertexBuffer*	m_pVB;
		class CXR_VertexBuffer*	m_pFlippedVB;

		int32				m_Flags;

	public:

		CDynamicVB()
		{
			Clear();
		}

		void Clear();
		bool Create(class CXR_Model_Custom_RenderParams* _pRenderParams, class CXR_Model_Custom* _pCustomModel, class CXR_VBManager* _pVBM, int32 _NumVertices, int32 _NumTriangles, int _Flags = DVBFLAGS_DEFAULT);

		bool IsValid()
		{
			return ((m_NumVertices > 0) && (m_NumTriangles > 0));
		}

		void AddTriangle(int _v1, int _v2, int _v3, int _vbase = -1);

		void AddVertex(CVec3Dfp32 _pos, fp32 _tu, fp32 _tv, int32 _color);
		void AddVertex(CVec3Dfp32 _pos, CVec3Dfp32 _normal, CVec2Dfp32 _tex, int32 _color);

		void Render(const CMat4Dfp32& _matrix);

		int GetNumVertices()
		{
			return m_NumVertices;
		}

		int GetNumTriangles()
		{
			return m_NumTriangles;
		}

		class CXR_VertexBuffer* GetVB()
		{
			return m_pVB;
		}
		
		class CXR_VertexBuffer* GetFlippedVB()
		{
			return m_pFlippedVB;
		}
		
		void RecalculateNormals();
		void ConvertToFlatShaded();

//		bool ApplyDiffuseLight(CVec3Dfp32 &_Center, class CXR_Model_Custom* _pCustomModel, class CXR_VBManager* _pVBM);

		void DebugRenderNormals(class CWireContainer* _pWC, fp32 _Duration = 0.0f, int32 _Color = 0xFFFFFF00, fp32 _Length = 10.0f);

};

//----------------------------------------------------------------------

#endif /* CDynamicVB_h */
