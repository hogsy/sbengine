#include "MRTC_VPU.h"

class CTM_BDVertexInfo
{
public:
	union {
		struct {
			uint8 m_Flags;				// Unused
			uint8 m_nBones;
			uint16 m_iBoneInfluence;	// Index into array of CTM_BDInfluence
		};
		uint32 m_dummy;
	};
//	CTM_BDVertexInfo() {}
	CTM_BDVertexInfo(uint32 _Data) : m_dummy(_Data) {}


};

class CTM_BDInfluence
{
public:
	union {
		struct {
			uint16 m_iBone;
			fp32 m_Influence;			// 0..1
		};
		uint64 m_dummy;
	};

	CTM_BDInfluence(uint64 _Data) : m_dummy(_Data) {}
};

class VertexTransformer
{
private:

	CVPU_CacheBuffer<CTM_BDInfluence,16,128>* m_pInfluenceBuffer;
	const CMat4Dfp32* m_pMatrixPal;
public:
	
	VertexTransformer() 
	{ 
		m_pInfluenceBuffer=NULL; 
		m_pMatrixPal=NULL;
	}
	void Create(CVPU_CacheBuffer<CTM_BDInfluence,16,128>* _pInfluenceBuffer,const CMat4Dfp32* _pMatrixPal,uint32 _nMatrix)
	{
		m_pInfluenceBuffer=_pInfluenceBuffer;
		M_ASSERT(mint(_pMatrixPal)%16==0,"Matrix pointer must be aligned");
		m_pMatrixPal=_pMatrixPal;
	}
	vec128 TransformVertex(vec128 Vertex,const CTM_BDVertexInfo& _VertexInfo)
	{
		uint32 nBones=_VertexInfo.m_nBones;
		uint32 iBoneInfluence=_VertexInfo.m_iBoneInfluence;
		if (nBones == 1)
		{
			const CTM_BDInfluence BI=m_pInfluenceBuffer->GetElement(iBoneInfluence);
			return M_VMulMat4x3(Vertex,m_pMatrixPal[BI.m_iBone]);
		}
		else if (nBones == 2)
		{
			const CTM_BDInfluence BI0=(m_pInfluenceBuffer->GetElement(iBoneInfluence));
			const CTM_BDInfluence BI1(m_pInfluenceBuffer->GetElement(iBoneInfluence+1));
			const CMat4Dfp32& m0=m_pMatrixPal[BI0.m_iBone];
			const CMat4Dfp32& m1=m_pMatrixPal[BI1.m_iBone];
			vec128 lrp=M_VLdScalar(BI1.m_Influence);
			vec128 a=M_VLrp(m0.r[0],m1.r[0],lrp);
			vec128 b=M_VLrp(m0.r[1],m1.r[1],lrp);
			vec128 c=M_VLrp(m0.r[2],m1.r[2],lrp);
			vec128 d=M_VLrp(m0.r[3],m1.r[3],lrp);
			M_VTranspose4x3(a, b, c, d);
			return M_VDp4x3(Vertex, a, Vertex, b, Vertex, c);
		}

		else
		{
			vec128 a=M_VZero();
			vec128 b=M_VZero();
			vec128 c=M_VZero();
			vec128 d=M_VZero();
			for(uint32 iiBone = 0; iiBone < nBones; iiBone++)
			{
				const CTM_BDInfluence& BI=(m_pInfluenceBuffer->GetElement(iBoneInfluence+iiBone));
				const CMat4Dfp32& mm=m_pMatrixPal[BI.m_iBone];
				const vec128 s=M_VLdScalar(BI.m_Influence);
				a=M_VAdd(a,M_VMul(mm.r[0],s));
				b=M_VAdd(b,M_VMul(mm.r[1],s));
				c=M_VAdd(c,M_VMul(mm.r[2],s));
				d=M_VAdd(d,M_VMul(mm.r[3],s));
			}
			M_VTranspose4x3(a, b, c, d);
			return M_VDp4x3(Vertex, a, Vertex, b, Vertex, c);
		}
	}
};





