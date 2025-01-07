

void CollectName(CollectAll_, CN_Vertex, CN_Normal, CN_Color, CN_Specular, CN_Fog, CN_NumTexture, CN_NonStandard) (SCollectFunctionParams *Params)
{

	int Stride = 0;
	int Num = Params->m_Num;
	int CurStride = 0;

#if (CN_Vertex == 1)
	const VECTOR *Vertex = Params->m_pVertex;
	Stride += 3;
#endif 

#if (CN_Normal == 1)
	const VECTOR *Normal = Params->m_pNormal;
	Stride += 3;
#endif 

#if (CN_Color == 1)
	const uint32 *Color = Params->m_pColor;
	Stride += 1;
#else
	uint32 AllColor = Params->m_AllColor;
	Stride += 1;
#endif 

#if (CN_Specular == 1)
	const uint32 *Specular = Params->m_pSpecular;
#endif 

#if (CN_Fog == 1)
	const float *Fog = Params->m_pFog;
#endif 

#if ((CN_Fog == 1) || (CN_Specular == 1))
	Stride += 1;
#endif

#if (CN_NonStandard == 1)

#if (CN_NumTexture > 0)
	const float *Texture1 = Params->m_pTexture1;
	int TextureCordSize1 = Params->m_TextureCordSize1;
	int DestTextureCordSize1 = Params->m_DestTextureCordSize1;
	Stride += DestTextureCordSize1;
#endif 

#if (CN_NumTexture > 1)
	const float *Texture2 = Params->m_pTexture2;
	int TextureCordSize2 = Params->m_TextureCordSize2;
	int DestTextureCordSize2 = Params->m_DestTextureCordSize2;
	Stride += DestTextureCordSize2;
#endif 

#if (CN_NumTexture > 2)
	const float *Texture3 = Params->m_pTexture3;
	int TextureCordSize3 = Params->m_TextureCordSize3;
	int DestTextureCordSize3 = Params->m_DestTextureCordSize3;
	Stride += DestTextureCordSize3;
#endif 

#if (CN_NumTexture > 3)
	const float *Texture4 = Params->m_pTexture4;
	int TextureCordSize4 = Params->m_TextureCordSize4;
	int DestTextureCordSize4 = Params->m_DestTextureCordSize4;
	Stride += DestTextureCordSize4;
#endif 

#else

#if (CN_NumTexture > 0)
	const float *Texture1 = Params->m_pTexture1;
	Stride += 2;
#endif 

#if (CN_NumTexture > 1)
	const float *Texture2 = Params->m_pTexture2;
	Stride += 2;
#endif 

#if (CN_NumTexture > 2)
	const float *Texture3 = Params->m_pTexture3;
	Stride += 2;
#endif 

#if (CN_NumTexture > 3)
	const float *Texture4 = Params->m_pTexture4;
	Stride += 2;
#endif 

#endif

	float *VertexPointer = Params->m_pVertexList;
	const float *EndVertexPointer = VertexPointer + Stride * Num;
	
	while (VertexPointer < EndVertexPointer)
	{
		
#if (CN_Vertex == 1)
		*((VECTOR *)VertexPointer) = *Vertex; 
		VertexPointer += 3;
		++Vertex;
#endif
		
#if (CN_Normal == 1)
		*((VECTOR *)VertexPointer) = *Normal; 
		VertexPointer += 3;
		++Normal;
#endif

#if (CN_Color == 1)
		*((uint32 *)VertexPointer) = *Color;
		VertexPointer+= 1;
		++Color;
#else
		*((uint32 *)VertexPointer) = AllColor;
		VertexPointer+= 1;
#endif

#if ((CN_Fog == 1) && (CN_Specular == 1))
		*((uint32 *)VertexPointer) = ((*Specular) & 0x00FFFFFF) | (RoundToInt((*Fog) * 255.0f)) << 24;
		VertexPointer+= 1;
		++Specular;
		++Fog;
#else

	#if (CN_Specular == 1)
			*((uint32 *)VertexPointer) = (*Specular);
			VertexPointer+= 1;
			++Specular;
	#endif

	#if (CN_Fog == 1)
			*((uint32 *)VertexPointer) = (RoundToInt((*Fog) * 255.0f)) << 24;
			VertexPointer+= 1;
			++Fog;
	#endif

#endif

#if (CN_NonStandard == 1)

#if (CN_NumTexture > 0)
		switch (DestTextureCordSize1)
		{
		case 2:
			*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture1);
			VertexPointer += 2;
			break;
		case 3:
			*((VECTOR*)VertexPointer) = *((const VECTOR*)Texture1);
			VertexPointer += 3;
			break;
		case 4:
			*((VECTOR4D*)VertexPointer) = *((const VECTOR4D*)Texture1);
			VertexPointer += 4;
			break;
		case 1:
			*((float*)VertexPointer) = *((const float*)Texture1);
			VertexPointer += 1;
			break;
		}

		Texture1 += TextureCordSize1;
#endif

#if (CN_NumTexture > 1)
		switch (DestTextureCordSize2)
		{
		case 2:
			*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture2);
			VertexPointer += 2;
			break;
		case 3:
			*((VECTOR*)VertexPointer) = *((const VECTOR*)Texture2);
			VertexPointer += 3;
			break;
		case 4:
			*((VECTOR4D*)VertexPointer) = *((const VECTOR4D*)Texture2);
			VertexPointer += 4;
			break;
		case 1:
			*((float*)VertexPointer) = *((const float*)Texture2);
			VertexPointer += 1;
			break;
		}
		Texture2 += TextureCordSize2;
#endif	

#if (CN_NumTexture > 2)
		if (DestTextureCordSize3 == 2)
		{
			*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture3);
			VertexPointer += 2;
		}
		else if (DestTextureCordSize3 == 3)
		{
			*((VECTOR*)VertexPointer) = *((const VECTOR*)Texture3);
			VertexPointer += 3;
		}
		else if (DestTextureCordSize3 == 4)
		{
			*((VECTOR4D*)VertexPointer) = *((const VECTOR4D*)Texture3);
			VertexPointer += 4;
		}
		else if (DestTextureCordSize3 == 1)
		{
			*((float*)VertexPointer) = *((const float*)Texture3);
			VertexPointer += 1;
		}
		Texture3 += TextureCordSize3;
#endif
		
#if (CN_NumTexture > 3)
		if (DestTextureCordSize4 == 2)
		{
			*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture4);
			VertexPointer += 2;
		}
		else if (DestTextureCordSize4 == 3)
		{
			*((VECTOR*)VertexPointer) = *((const VECTOR*)Texture4);
			VertexPointer += 3;
		}
		else if (DestTextureCordSize4 == 4)
		{
			*((VECTOR4D*)VertexPointer) = *((const VECTOR4D*)Texture4);
			VertexPointer += 4;
		}
		else if (DestTextureCordSize4 == 1)
		{
			*((float*)VertexPointer) = *((const float*)Texture4);
			VertexPointer += 1;
		}
		Texture4 += TextureCordSize4;
#endif

#else

#if (CN_NumTexture > 0)
		*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture1);
		VertexPointer += 2;
		Texture1 += 2;
#endif

#if (CN_NumTexture > 1)
		*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture2);
		VertexPointer += 2;
		Texture2 += 2;
#endif	

#if (CN_NumTexture > 2)
		*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture3);
		VertexPointer += 2;
		Texture3 += 2;
#endif
		
#if (CN_NumTexture > 3)
		*((VECTOR2D*)VertexPointer) = *((const VECTOR2D*)Texture4);
		VertexPointer += 2;
		Texture4 += 2;
#endif

#endif
	}

}

void CollectName(CollectIndexed_, CN_Vertex, CN_Normal, CN_Color, CN_Specular, CN_Fog, CN_NumTexture, CN_NonStandard) (SCollectFunctionParams *Params)
{

//	int Stride = 0;
	int Num = Params->m_Num;

#if (CN_Vertex == 1)
	const VECTOR *Vertex = (VECTOR *)Params->m_pVertex;
//	Stride += 3;
#endif 

#if (CN_Normal == 1)
	const VECTOR *Normal = Params->m_pNormal;
//	Stride += 3;
#endif 

#if (CN_Color == 1)
	const uint32 *Color = Params->m_pColor;
//	Stride += 1;
#else
	uint32 AllColor = Params->m_AllColor;
//	Stride += 1;
#endif 

#if (CN_Specular == 1)
	const uint32 *Specular = Params->m_pSpecular;
#endif 

#if (CN_Fog == 1)
	const float *Fog = Params->m_pFog;
#endif 

#if ((CN_Fog == 1) || (CN_Specular == 1))
//Stride += 1;
#endif

#if (CN_NonStandard == 1)

#if (CN_NumTexture > 0)
	const float *Texture1 = Params->m_pTexture1;
	int TextureCordSize1 = Params->m_TextureCordSize1;
	int DestTextureCordSize1 = Params->m_DestTextureCordSize1;
#endif 

#if (CN_NumTexture > 1)
	const float *Texture2 = Params->m_pTexture2;
	int TextureCordSize2 = Params->m_TextureCordSize2;
	int DestTextureCordSize2 = Params->m_DestTextureCordSize2;
#endif 

#if (CN_NumTexture > 2)
	const float *Texture3 = Params->m_pTexture3;
	int TextureCordSize3 = Params->m_TextureCordSize3;
	int DestTextureCordSize3 = Params->m_DestTextureCordSize3;
#endif 

#if (CN_NumTexture > 3)
	const float *Texture4 = Params->m_pTexture4;
	int TextureCordSize4 = Params->m_TextureCordSize4;
	int DestTextureCordSize4 = Params->m_DestTextureCordSize4;
#endif 

#else

#if (CN_NumTexture > 0)
	const float *Texture1 = Params->m_pTexture1;
#endif 

#if (CN_NumTexture > 1)
	const float *Texture2 = Params->m_pTexture2;
#endif 

#if (CN_NumTexture > 2)
	const float *Texture3 = Params->m_pTexture3;
#endif 

#if (CN_NumTexture > 3)
	const float *Texture4 = Params->m_pTexture4;
#endif 

#endif

	float *VertexPointer = Params->m_pVertexList;
	const uint16 *Indices = Params->m_pIndices;
	const uint16 *EndIndex = Indices + Num;
	
	while (Indices < EndIndex)
	{		
		int CurrentIndex = *Indices;
		
#if (CN_Vertex == 1)		
		*((VECTOR *)VertexPointer) = Vertex[CurrentIndex];
		VertexPointer += 3;
#endif

#if (CN_Normal == 1)
		*((VECTOR *)VertexPointer) = Normal[CurrentIndex];
		VertexPointer += 3;
#endif

#if (CN_Color == 1)
		*((uint32 *)VertexPointer) = Color[CurrentIndex];
		VertexPointer+= 1;
#else
		*((uint32 *)VertexPointer) = AllColor;
		VertexPointer+= 1;
#endif

#if ((CN_Fog == 1) && (CN_Specular == 1))
		*((uint32 *)VertexPointer) = ((Specular[CurrentIndex]) & 0x00FFFFFF) | (RoundToInt((Fog[CurrentIndex]) * 255.0f)) << 24;
		VertexPointer+= 1;
#else

	#if (CN_Specular == 1)
			*((uint32 *)VertexPointer) = Specular[CurrentIndex];
			VertexPointer+= 1;
	#endif

	#if (CN_Fog == 1)
			*((uint32 *)VertexPointer) = (RoundToInt((Fog[CurrentIndex]) * 255.0f)) << 24;
			VertexPointer+= 1;
	#endif

#endif

#if (CN_NonStandard == 1)

#if (CN_NumTexture > 0)
		if (DestTextureCordSize1 == 2)
		{
			*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture1)[CurrentIndex];
			VertexPointer += 2;
		}
		else if (DestTextureCordSize1 == 3)
		{
			*((VECTOR*)VertexPointer) = ((const VECTOR*)Texture1)[CurrentIndex];
			VertexPointer += 3;
		}
		else if (DestTextureCordSize1 == 4)
		{
			*((VECTOR4D*)VertexPointer) = ((const VECTOR4D*)Texture1)[CurrentIndex];
			VertexPointer += 4;
		}
		else if (DestTextureCordSize1 == 1)
		{
			*((float*)VertexPointer) = ((const float*)Texture1)[CurrentIndex];
			VertexPointer += 1;
		}

		//memcpy(VertexPointer, Texture1, TextureCordSize1 << 2);
//		VertexPointer += DestTextureCordSize1;
#endif

#if (CN_NumTexture > 1)
		if (DestTextureCordSize2 == 2)
		{
			*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture2)[CurrentIndex];
			VertexPointer += 2;
		}
		else if (DestTextureCordSize2 == 3)
		{
			*((VECTOR*)VertexPointer) = ((const VECTOR*)Texture2)[CurrentIndex];
			VertexPointer += 3;
		}
		else if (DestTextureCordSize2 == 4)
		{
			*((VECTOR4D*)VertexPointer) = ((const VECTOR4D*)Texture2)[CurrentIndex];
			VertexPointer += 4;
		}
		else if (DestTextureCordSize2 == 1)
		{
			*((float*)VertexPointer) = ((const float*)Texture2)[CurrentIndex];
			VertexPointer += 1;
		}
//		*((VECTOR2D*)VertexPointer) = *((VECTOR2D*)Texture2);
//		memcpy(VertexPointer, Texture2, TextureCordSize2 << 2);
#endif

#if (CN_NumTexture > 2)
		if (DestTextureCordSize3 == 2)
		{
			*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture3)[CurrentIndex];
			VertexPointer += 2;
		}
		else if (DestTextureCordSize3 == 3)
		{
			*((VECTOR*)VertexPointer) = ((const VECTOR*)Texture3)[CurrentIndex];
			VertexPointer += 3;
		}
		else if (DestTextureCordSize3 == 4)
		{
			*((VECTOR4D*)VertexPointer) = ((const VECTOR4D*)Texture3)[CurrentIndex];
			VertexPointer += 4;
		}
		else if (DestTextureCordSize3 == 1)
		{
			*((float*)VertexPointer) = ((const float*)Texture3)[CurrentIndex];
			VertexPointer += 1;
		}
#endif
		
#if (CN_NumTexture > 3)
		if (DestTextureCordSize4 == 2)
		{
			*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture4)[CurrentIndex];
			VertexPointer += 2;
		}
		else if (DestTextureCordSize4 == 3)
		{
			*((VECTOR*)VertexPointer) = ((const VECTOR*)Texture4)[CurrentIndex];
			VertexPointer += 3;
		}
		else if (DestTextureCordSize4 == 4)
		{
			*((VECTOR4D*)VertexPointer) = ((const VECTOR4D*)Texture4)[CurrentIndex];
			VertexPointer += 4;
		}
		else if (DestTextureCordSize4 == 1)
		{
			*((float*)VertexPointer) = ((const float*)Texture4)[CurrentIndex];
			VertexPointer += 1;
		}
#endif

#else

#if (CN_NumTexture > 0)
		*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture1)[CurrentIndex];
		VertexPointer += 2;
#endif

#if (CN_NumTexture > 1)
		*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture2)[CurrentIndex];
		VertexPointer += 2;
#endif

#if (CN_NumTexture > 2)
		*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture3)[CurrentIndex];
		VertexPointer += 2;
#endif
		
#if (CN_NumTexture > 3)
		*((VECTOR2D*)VertexPointer) = ((const VECTOR2D*)Texture4)[CurrentIndex];
		VertexPointer += 2;
#endif

#endif

		++Indices;

	}
}