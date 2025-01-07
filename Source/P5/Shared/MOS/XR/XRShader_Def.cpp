
//-----------------------------------------------------------------------------
void CXR_Shader::RenderDeferred_VB(CXR_VertexBuffer * _pVB,CXR_VertexBuffer *_pSrc,fp32 _BasePrio,fp32 _OffsPrio)
{
		fp32 Prio = _BasePrio;
	CXR_VirtualAttributes_Deferred *M_RESTRICT pA = (CXR_VirtualAttributes_Deferred*)(_pSrc + 3);
	for(uint8 i = 0;i < 3;i++)
	{
		if( pA[i].m_TextureID )
		{
			_pSrc[i].m_Priority = Prio;
			m_pVBM->AddVB(_pSrc+i);
		}

		Prio += _OffsPrio;
	}
}
