template <typename t_CType>
DIdsPInlineS t_CType RotateLeft(t_CType _Data, t_CType _nBits)
{
	return (_Data << _nBits) | (_Data >> (sizeof(_Data)*8 - _nBits));
}

template <typename t_CType>
DIdsPInlineS t_CType RotateRight(t_CType _Data, t_CType _nBits)
{
	return (_Data >> _nBits) | (_Data << (sizeof(_Data)*8 - _nBits));
}

template <typename t_CType>
DIdsPInlineS t_CType LittleEndianEncode(t_CType _Input)
{
#		ifdef CPU_LITTLEENDIAN
		return _Input;
#		else
		t_CType Temp;
		uint8 *pData = (uint8 *)&Temp;
		mint Len = sizeof(t_CType);
		for (mint i = 0; i < Len; ++i) 
		{
			pData[i] = (uint8)((_Input >> (i * 8)) & (t_CType)0xff);
		}
		return Temp;
#		endif
}

template <typename t_CType>
DIdsPInlineS t_CType LittleEndianDecode(t_CType _Input)
{
#		ifdef CPU_LITTLEENDIAN
		t_CType Temp = 0;
		uint8 *pData = (uint8 *)(&_Input);
		mint Len = sizeof(t_CType);
		for (mint i = 0; i < Len; ++i) 
		{
			Temp |= ((t_CType)pData[i]) << ((i) * 8);
		}
		return Temp;
#		else
		return _Input;
#		endif
}

class CMD5
{

protected:

	DIdsPInlineS uint32 fp_Transform0(uint32 _Input0, uint32 _Input1, uint32 _Input2, uint32 _Input3, uint32 _Data, uint32 _Rotate, uint32 _Add) 
	{
		return RotateLeft(_Input0 + (((_Input1 & _Input2) | ((~_Input1) & _Input3)) + _Data + _Add), _Rotate) + _Input1;
	}
	DIdsPInlineS uint32 fp_Transform1(uint32 _Input0, uint32 _Input1, uint32 _Input2, uint32 _Input3, uint32 _Data, uint32 _Rotate, uint32 _Add) 
	{
		return RotateLeft(_Input0 + (((_Input1 & _Input3) | (_Input2 & (~_Input3))) + _Data + _Add), _Rotate) + _Input1;
	}
	DIdsPInlineS uint32 fp_Transform2(uint32 _Input0, uint32 _Input1, uint32 _Input2, uint32 _Input3, uint32 _Data, uint32 _Rotate, uint32 _Add) 
	{
		return RotateLeft(_Input0 + ((_Input1 ^ _Input2 ^ _Input3) + _Data + _Add), _Rotate) + _Input1;
	}
	DIdsPInlineS uint32 fp_Transform3(uint32 _Input0, uint32 _Input1, uint32 _Input2, uint32 _Input3, uint32 _Data, uint32 _Rotate, uint32 _Add) 
	{
		return RotateLeft(_Input0 + ((_Input2 ^ (_Input1 | (~_Input3))) + _Data + _Add), _Rotate) + _Input1;
	}

	uint64 m_nBytes;
	uint32 m_State[4];
	uint8 m_Block[64];
	aint m_iBlockPos;

	const static aint msc_TransformOrder[];
	const static aint msc_Rotate0[];
	const static aint msc_Rotate1[];
	const static aint msc_Rotate2[];
	const static aint msc_Rotate3[];
	const static uint32 msc_Add0[];
	const static uint32 msc_Add1[];
	const static uint32 msc_Add2[];
	const static uint32 msc_Add3[];
	const static aint msc_DataOrder1[];
	const static aint msc_DataOrder2[];
	const static aint msc_DataOrder3[];

	void fp_AddBlock(void *_pBlock)
	{
		uint32 State[4];
		State[0] = m_State[0];
		State[1] = m_State[1];
		State[2] = m_State[2];
		State[3] = m_State[3];
#				ifndef CPU_LITTLEENDIAN
			uint32 Block[16];
			for (int i = 0; i < 16; ++i)
				Block[i] = LittleEndianDecode(((uint32 *)_pBlock)[i]);
			uint32 *pData = Block;
#				else
			uint32 *pData = (uint32 *)_pBlock;
#				endif

		for (int i = 0; i < 16; ++i)
		{
			int iTransformStart = msc_TransformOrder[(i%4)];
			State[iTransformStart] = fp_Transform0(State[iTransformStart], State[(iTransformStart + 1)%4],
				State[(iTransformStart + 2)%4], State[(iTransformStart + 3)%4], pData[i], msc_Rotate0[i%4], msc_Add0[i]);
		}

		for (int i = 0; i < 16; ++i)
		{
			int iTransformStart = msc_TransformOrder[(i%4)];
			State[iTransformStart] = fp_Transform1(State[iTransformStart], State[(iTransformStart + 1)%4],
				State[(iTransformStart + 2)%4], State[(iTransformStart + 3)%4], pData[msc_DataOrder1[i]], msc_Rotate1[i%4], msc_Add1[i]);
		}

		for (int i = 0; i < 16; ++i)
		{
			int iTransformStart = msc_TransformOrder[(i%4)];
			State[iTransformStart] = fp_Transform2(State[iTransformStart], State[(iTransformStart + 1)%4],
				State[(iTransformStart + 2)%4], State[(iTransformStart + 3)%4], pData[msc_DataOrder2[i]], msc_Rotate2[i%4], msc_Add2[i]);
		}

		for (int i = 0; i < 16; ++i)
		{
			int iTransformStart = msc_TransformOrder[(i%4)];
			State[iTransformStart] = fp_Transform3(State[iTransformStart], State[(iTransformStart + 1)%4],
				State[(iTransformStart + 2)%4], State[(iTransformStart + 3)%4], pData[msc_DataOrder3[i]], msc_Rotate3[i%4], msc_Add3[i]);
		}

		m_State[0] += State[0];
		m_State[1] += State[1];
		m_State[2] += State[2];
		m_State[3] += State[3];
#				ifndef CPU_LITTLEENDIAN
			memset(Block, 0, sizeof(Block));
#				endif
	}

	void fp_AddData(const void *_pData, mint _Len)
	{
		m_nBytes += _Len;
		uint8 *pData = (uint8 *)_pData;
		// Start by transforming up to a whole block
		if (m_iBlockPos)
		{
			if (m_iBlockPos + _Len >= 64)
			{
				aint ToCopy = 64 - m_iBlockPos;
				memcpy(m_Block + m_iBlockPos, pData, ToCopy);
				pData += ToCopy;
				_Len -= ToCopy;

				fp_AddBlock(m_Block);
				m_iBlockPos = 0;
			}                    
		}

		// Add whole blocks
		while (_Len >= 64)
		{
			DIdsAssert(m_iBlockPos == 0, "Must not have saved block");
			fp_AddBlock(pData);
			pData += 64;
			_Len -= 64;
		}

		// Add remaining to buffer
		memcpy(m_Block + m_iBlockPos, pData, _Len);
		m_iBlockPos += _Len;
	}

public:

	CMD5()
	{
		f_Reset();
	}

	CMD5(const CMD5 &_Src)
	{
		memcpy(m_Block, _Src.m_Block, sizeof(m_Block));
		memcpy(m_State, _Src.m_State, sizeof(m_State));
		m_iBlockPos = _Src.m_iBlockPos;
		m_nBytes = _Src.m_nBytes;
	}

	CMD5 &operator = (const CMD5 &_Src)
	{
		memcpy(m_Block, _Src.m_Block, sizeof(m_Block));
		memcpy(m_State, _Src.m_State, sizeof(m_State));
		m_iBlockPos = _Src.m_iBlockPos;
		m_nBytes = _Src.m_nBytes;

		return *this;
	}

	~CMD5()
	{
		memset(m_Block, 0, sizeof(m_Block));
		memset(m_State, 0, sizeof(m_State));
	}

	void f_Reset()
	{
		memset(m_Block, 0, sizeof(m_Block));
		m_iBlockPos = 0;
		m_nBytes = 0;
		m_State[0] = 0x67452301;
		m_State[1] = 0xefcdab89;
		m_State[2] = 0x98badcfe;
		m_State[3] = 0x10325476;
	}

	DIdsPInlineS void f_AddData(const void *_pData, mint _Len)
	{
		fp_AddData(_pData, _Len);
	}

	void f_GetDigest(uint8 * _pDest) const
	{
		CMD5 TempState(*this);
		static uint8 Pad[64] = {0x80, 0};

		uint64 Size = LittleEndianEncode((m_nBytes % (0x1fffffffffffffffLL)) * 8);
		int Offset = (m_nBytes & 0x3f);

		TempState.f_AddData(Pad, (Offset < 56) ? (56 - Offset) : (120 - Offset));
		TempState.f_AddData(&Size, sizeof(Size));

		((uint32 *)_pDest)[0] = LittleEndianEncode(TempState.m_State[0]);
		((uint32 *)_pDest)[1] = LittleEndianEncode(TempState.m_State[1]);
		((uint32 *)_pDest)[2] = LittleEndianEncode(TempState.m_State[2]);
		((uint32 *)_pDest)[3] = LittleEndianEncode(TempState.m_State[3]);
	}

	dllvirtual void fv_AddData(const void* _pData, mint _Len);
	dllvirtual void fv_GetDigest(uint8 * _pDest) const;
};

class CMD5Digest 
{			
	uint8 m_Data[16];
public:
	CMD5Digest()
	{
		memset(m_Data, 0, sizeof(m_Data));
	}

	CMD5Digest(const CMD5Digest &_Src)
	{
		memcpy(m_Data, _Src.m_Data, sizeof(m_Data));
	}

	CMD5Digest(const CMD5 &_Src)
	{
		_Src.fv_GetDigest(m_Data);
	}

	CMD5Digest(void* _pData, mint _Len)
	{
		CMD5 Data;
		Data.fv_AddData(_pData, _Len);
		Data.fv_GetDigest(m_Data);
	}

	CMD5Digest &operator = (const CMD5Digest &_Src)
	{
		memcpy(m_Data, _Src.m_Data, sizeof(m_Data));
		return *this;
	}

	CMD5Digest &operator = (const CMD5 &_Src)
	{
		_Src.fv_GetDigest(m_Data);
		return *this;
	}

	bint operator != (const CMD5Digest &_Src) const
	{
		if (memcmp(m_Data, _Src.m_Data, 16))
			return true;

		return false;
	}

	bint operator != (const CMD5 &_Src) const
	{
		CMD5Digest Temp = _Src;

		return Temp != (*this);
	}

	bint operator == (const CMD5Digest &_Src) const
	{
		if (memcmp(m_Data, _Src.m_Data, 16))
			return false;

		return true;
	}

	bint operator == (const CMD5 &_Src) const
	{
		CMD5Digest Temp = _Src;

		return Temp == (*this);
	}

	uint8 *f_GetData()
	{
		return m_Data;
	}
};

CMD5Digest GetFileMD5(CStr _File);
