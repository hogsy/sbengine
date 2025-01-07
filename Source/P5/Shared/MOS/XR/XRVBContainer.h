
#ifndef _INC_XRVBCONTAINER
#define _INC_XRVBCONTAINER


#include "XR.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:				CXR_VBContainer
						
	Comments:			Vertex-buffer container base class.
\*____________________________________________________________________*/

enum
{
	VB_GETFLAGS_BUILD			= DBit(0),		// Get is called from render for VB build.	(Single call)
	VB_GETFLAGS_FALLBACK		= DBit(1),	// Get is called from rendersurface.		(Frequent calls)
	VB_GETFLAGS_ONLYPRIMITIVES	= DBit(2),	// Get is called from rendersurface.		(Frequent calls)

};

#if 0
enum EXR_VBGetFlags
{
	EXR_VBGetFlags_Compress0to1_Position = DBit(0),
	EXR_VBGetFlags_Compress0to1_Tex0 = DBit(1),
	EXR_VBGetFlags_Compress0to1_Tex1 = DBit(2),
	EXR_VBGetFlags_Compress0to1_Tex2 = DBit(3),
	EXR_VBGetFlags_Compress0to1_Tex3 = DBit(4),
	EXR_VBGetFlags_Compress0to1_MatrixWeights = DBit(5),

	EXR_VBGetFlags_CompressMinus1to1_Position = DBit(6),
	EXR_VBGetFlags_CompressMinus1to1_Normal = DBit(7),
	EXR_VBGetFlags_CompressMinus1to1_Tex0 = DBit(8),
	EXR_VBGetFlags_CompressMinus1to1_Tex1 = DBit(9),
	EXR_VBGetFlags_CompressMinus1to1_Tex2 = DBit(10),
	EXR_VBGetFlags_CompressMinus1to1_Tex3 = DBit(11),

	EXR_VBGetFlags_Compress0to255_MatrixIndex = DBit(12),

	EXR_VBGetFlags_CompressBox_Position = DBit(13),
	EXR_VBGetFlags_CompressBox_Tex0 = DBit(14),
	EXR_VBGetFlags_CompressBox_Tex1 = DBit(15),
	EXR_VBGetFlags_CompressBox_Tex2 = DBit(16),
	EXR_VBGetFlags_CompressBox_Tex3 = DBit(17)

};
class CXR_TexCoordBox
{
public:
	CVec4Dfp4 m_Min;
	CVec4Dfp4 m_Max;
};

class CXR_VBGetOptions
{
public:
	CXR_VBGetOptions()
	{
		m_Flags = 0;
//		m_pMatrixMap = NULL;
//		m_nMatrixMap = 0;
	}

	uint32 m_Flags;

	CBox3Dfp4 m_Bound_Vertex;

	CXR_TexCoordBox m_Bound_Texture0;
	CXR_TexCoordBox m_Bound_Texture1;
	CXR_TexCoordBox m_Bound_Texture2;
	CXR_TexCoordBox m_Bound_Texture3;

//	uint8 *m_pMatrixMap;
//	int m_nMatrixMap;
	
};
#endif

enum
{
	VBF_DUAL_MATRIX_LISTS	= DBit(0),
};

class CXR_VBContainer
{
protected:
	int m_iVBC;
	class CXR_VBContext* m_pVBCtx;

	virtual void VBC_Add();
	virtual void VBC_Remove();

public:

	CXR_VBContainer();
	virtual ~CXR_VBContainer();

	class CXR_VBContext* GetVBContext() { return m_pVBCtx; };

	virtual void OnRefresh();

	virtual int GetNumLocal() pure;
	virtual CFStr GetName(int _iLocal) { return CFStrF("VBClass %d, Local %d", m_iVBC, _iLocal); };
	virtual int GetLocal(const char* _pName) { return -1; };
	virtual int GetID(int _iLocal) pure;
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags) pure;
	virtual void Release(int _iLocal);
	virtual uint32 GetFlags(int _iLocal) { return 0; };

	virtual const char *GetContainerSortName() {return "";} // Must return an unique name if Get does any file IO
};

typedef TPtr<CXR_VBContainer> spCXR_VBContainer;

#endif // _INC_XRVBCONTAINER
