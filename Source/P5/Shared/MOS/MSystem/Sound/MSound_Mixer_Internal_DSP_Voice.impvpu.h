
/////////////////////////////////////////////////////////////////////////////////////////
// Handles resampling and volume etc

// Todo: Add optimized code for SampleRate = 0.5 case and nChannels == 1 or 2

#ifdef PLATFORM_SPU

#define Macro_DoIntConv0(_iIndex) \
		vec128 Pos##_iIndex = M_VTrunc(SPos);\
		vec128 SPosSave##_iIndex = SPos;\
		SPos = M_VAdd(SPos, SStepSize);\
		int32vec128 iiPos##_iIndex = (int32vec128)M_VCnv_f32_i32(Pos##_iIndex);

#define Macro_DoIntConv1(_iIndex) \
		vec128 Frac##_iIndex = M_VSub(SPosSave##_iIndex, Pos##_iIndex );\
		vec128 FracInv##_iIndex = M_VSub(One, Frac##_iIndex);

#define Macro_DoIntConv2(_iIndex) \
		int32 iPos##_iIndex = spu_extract(iiPos##_iIndex, 0) * _nChannels;

#else

#define Macro_DoIntConv0(_iIndex) \
		vec128 Pos##_iIndex = M_VTrunc(SPos);\
		vec128 SPosSave##_iIndex = SPos;\
		SPos = M_VAdd(SPos, SStepSize);\
		vec128 IntPos##_iIndex = M_VCnv_f32_i32(Pos##_iIndex );\
		M_VStAny32Ex(IntPos##_iIndex, &TempInts[0], _iIndex * 4);

#define Macro_DoIntConv1(_iIndex) \
		vec128 Frac##_iIndex = M_VSub(SPosSave##_iIndex, Pos##_iIndex );\
		vec128 FracInv##_iIndex = M_VSub(One, Frac##_iIndex);

#define Macro_DoIntConv2(_iIndex) \
		int32 iPos##_iIndex = TempInts[_iIndex] * _nChannels;\

#endif

#ifdef PLATFORM_SPU
#define M_REDUCECODESIZE
#endif

#ifndef PLATFORM_SPU

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_1_int16_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const int16 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VMul(M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f), M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 VolumeDelta = M_VMul(_VolumeDelta, MulInt16);
	
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 Value##_iIndex##1;\
		if (_nChannels == 4)\
		{\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_VCnvH_i16_i32(Value##_iIndex##0);\
		}\
		else if (_nChannels == 1)\
		{\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_VSplat(Value##_iIndex##0, 1);\
		}\
		else if (_nChannels == 2)\
		{\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_VShuf(Value##_iIndex##0, M_VSHUF(2,3,2,3));\
		}\
		else\
		{\
			vec128 Temp = M_VShl8_u128(Value##_iIndex##0, _nChannels*2);\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_VCnvL_i16_i32(Temp);\
		}\
		Value##_iIndex##0 = M_VCnv_i32_f32(Value##_iIndex##0);\
		Value##_iIndex##1 = M_VCnv_i32_f32(Value##_iIndex##1);\
		vec128 Temp##_iIndex##0 = M_VMul(Value##_iIndex##0, FracInv##_iIndex );\
		Temp##_iIndex##0 = M_VMAdd(Value##_iIndex##1, Frac##_iIndex , Temp##_iIndex##0);\
		pDestination[_iIndex] = M_VMul(Temp##_iIndex##0, Volume0);\
		Volume0 = M_VAdd(Volume0, VolumeDelta);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_1_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const int16 * pSource = (int16 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
	case 1:
		{
			SourcePos = ProcessLinear_1_int16_Helper<1, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_1_int16_Helper<2, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_1_int16_Helper<3, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 4:
		{
			SourcePos = ProcessLinear_1_int16_Helper<4, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_2_int16_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const int16 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
	
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 ValueS##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 ValueS##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(ValueS##_iIndex##0);\
		vec128 Value0_##_iIndex##1 = M_VCnvL_i16_i32(ValueS##_iIndex##1);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(ValueS##_iIndex##0);\
		vec128 Value1_##_iIndex##1 = M_VCnvH_i16_i32(ValueS##_iIndex##1);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value0_##_iIndex##1 = M_Vi32tofp32(Value0_##_iIndex##1);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		Value1_##_iIndex##1 = M_Vi32tofp32(Value1_##_iIndex##1);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		pDestination[_iIndex*2+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*2+1] = M_VMul(Temp1_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}


fp32 ProcessLinear_2_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const int16 * pSource = (int16 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_2_int16_Helper<5, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 2:
		{
			SourcePos = ProcessLinear_2_int16_Helper<6, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#ifndef M_REDUCECODESIZE
	case 3:
		{
			SourcePos = ProcessLinear_2_int16_Helper<7, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_2_int16_Helper<8, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_3_int16_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const int16 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 ValueS0_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 ValueS1_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 8);\
		vec128 ValueS0_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 ValueS1_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels + 8);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(ValueS0_##_iIndex##0);\
		vec128 Value0_##_iIndex##1 = M_VCnvL_i16_i32(ValueS0_##_iIndex##1);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(ValueS0_##_iIndex##0);\
		vec128 Value1_##_iIndex##1 = M_VCnvH_i16_i32(ValueS0_##_iIndex##1);\
		vec128 Value2_##_iIndex##0 = M_VCnvL_i16_i32(ValueS1_##_iIndex##0);\
		vec128 Value2_##_iIndex##1 = M_VCnvL_i16_i32(ValueS1_##_iIndex##1);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value0_##_iIndex##1 = M_Vi32tofp32(Value0_##_iIndex##1);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		Value1_##_iIndex##1 = M_Vi32tofp32(Value1_##_iIndex##1);\
		Value2_##_iIndex##0 = M_Vi32tofp32(Value2_##_iIndex##0);\
		Value2_##_iIndex##1 = M_Vi32tofp32(Value2_##_iIndex##1);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		vec128 Temp2_##_iIndex##0 = M_VMul(Value2_##_iIndex##0, FracInv##_iIndex );\
		Temp2_##_iIndex##0 = M_VMAdd(Value2_##_iIndex##1, Frac##_iIndex , Temp2_##_iIndex##0);\
		pDestination[_iIndex*3+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+1] = M_VMul(Temp1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+2] = M_VMul(Temp2_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_3_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const int16 * pSource = (int16 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_3_int16_Helper<9, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_3_int16_Helper<10, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_3_int16_Helper<11, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_3_int16_Helper<12, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_4_int16_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const int16 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 ValueS0_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 ValueS1_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 8);\
		vec128 ValueS0_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 ValueS1_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels + 8);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(ValueS0_##_iIndex##0);\
		vec128 Value0_##_iIndex##1 = M_VCnvL_i16_i32(ValueS0_##_iIndex##1);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(ValueS0_##_iIndex##0);\
		vec128 Value1_##_iIndex##1 = M_VCnvH_i16_i32(ValueS0_##_iIndex##1);\
		vec128 Value2_##_iIndex##0 = M_VCnvL_i16_i32(ValueS1_##_iIndex##0);\
		vec128 Value2_##_iIndex##1 = M_VCnvL_i16_i32(ValueS1_##_iIndex##1);\
		vec128 Value3_##_iIndex##0 = M_VCnvH_i16_i32(ValueS1_##_iIndex##0);\
		vec128 Value3_##_iIndex##1 = M_VCnvH_i16_i32(ValueS1_##_iIndex##1);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value0_##_iIndex##1 = M_Vi32tofp32(Value0_##_iIndex##1);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		Value1_##_iIndex##1 = M_Vi32tofp32(Value1_##_iIndex##1);\
		Value2_##_iIndex##0 = M_Vi32tofp32(Value2_##_iIndex##0);\
		Value2_##_iIndex##1 = M_Vi32tofp32(Value2_##_iIndex##1);\
		Value3_##_iIndex##0 = M_Vi32tofp32(Value3_##_iIndex##0);\
		Value3_##_iIndex##1 = M_Vi32tofp32(Value3_##_iIndex##1);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		vec128 Temp2_##_iIndex##0 = M_VMul(Value2_##_iIndex##0, FracInv##_iIndex );\
		Temp2_##_iIndex##0 = M_VMAdd(Value2_##_iIndex##1, Frac##_iIndex , Temp2_##_iIndex##0);\
		vec128 Temp3_##_iIndex##0 = M_VMul(Value3_##_iIndex##0, FracInv##_iIndex );\
		Temp3_##_iIndex##0 = M_VMAdd(Value3_##_iIndex##1, Frac##_iIndex , Temp3_##_iIndex##0);\
		pDestination[_iIndex*4+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+1] = M_VMul(Temp1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+2] = M_VMul(Temp2_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+3] = M_VMul(Temp3_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_4_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const int16 * pSource = (int16 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_4_int16_Helper<13, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_4_int16_Helper<14, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_4_int16_Helper<15, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_4_int16_Helper<16, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_1_int16_Helper(fp32 _SourcePos, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const int16 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VMul(M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f), M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
	vec128 Volume0 = M_VMul(MulInt16, _Volume0);
	vec128 VolumeDelta = M_VMul(_VolumeDelta, MulInt16);


#define Macro_DoWork1(_iIndex) \
			vec128 Value##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels);\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##0 = M_Vi32tofp32(Value##_iIndex##0);\
			pDestination[_iIndex] = M_VMul(Value##_iIndex##0, Volume0);\
			Volume0 = M_VAdd(Volume0, VolumeDelta);

	if (_nChannels == 1)
	{
		vec128 WorkVolume0 = M_VSplat(Volume0, 0);
		// Lerp
#define Macro_DoWork(_iIndex) \
			vec128 Value##_iIndex##0 = M_VLdU(pSource + _iIndex * 8);\
			vec128 Value##_iIndex##1 = M_VCnvH_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##0 = M_Vi32tofp32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_Vi32tofp32(Value##_iIndex##1);\
			Value##_iIndex##0 = M_VMul(Value##_iIndex##0, WorkVolume0);\
			Value##_iIndex##1 = M_VMul(Value##_iIndex##1, WorkVolume0);\
			pDestination[_iIndex*8+0] = M_VPerm(Value##_iIndex##0, Zero, 0, 4, 4, 4);\
			pDestination[_iIndex*8+1] = M_VPerm(Value##_iIndex##0, Zero, 1, 4, 4, 4);\
			pDestination[_iIndex*8+2] = M_VPerm(Value##_iIndex##0, Zero, 2, 4, 4, 4);\
			pDestination[_iIndex*8+3] = M_VPerm(Value##_iIndex##0, Zero, 3, 4, 4, 4);\
			pDestination[_iIndex*8+4] = M_VPerm(Value##_iIndex##1, Zero, 0, 4, 4, 4);\
			pDestination[_iIndex*8+5] = M_VPerm(Value##_iIndex##1, Zero, 1, 4, 4, 4);\
			pDestination[_iIndex*8+6] = M_VPerm(Value##_iIndex##1, Zero, 2, 4, 4, 4);\
			pDestination[_iIndex*8+7] = M_VPerm(Value##_iIndex##1, Zero, 3, 4, 4, 4);\


		mint LastPrecache;
		LastPrecache = (((mint)pSource) + 127) & ~mint(127);
		M_PRECACHE128(LastPrecache, 0);
		M_PRECACHE128(LastPrecache + 128, 0);
		M_PRECACHE128(LastPrecache + 256, 0);
		M_PRECACHE128(LastPrecache + 384, 0);
		M_PRECACHE128(LastPrecache + 512, 0);

		mint LastZeroAddr = (mint)pDestination;
		if (!(LastZeroAddr & 127))
		{
			M_PREZERO128(LastZeroAddr, 0);
		}
		LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
		
		uint32 nToProcessUnroll4 = _nToProcess & (~uint32(31));
		for (uint32 i = 0; i < nToProcessUnroll4; i += 32)
		{
			mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				// Do 4 at a time
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(0, (void *)ZeroAddr0);
				M_PREZERO128(128, (void *)ZeroAddr0);
				M_PREZERO128(256, (void *)ZeroAddr0);
				M_PREZERO128(384, (void *)ZeroAddr0);
			}

			mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
			if (IPrecache != LastPrecache)
			{
				LastPrecache = IPrecache;
				M_PRECACHE128(IPrecache + 512, 0);
			}

			Macro_DoWork(0);
			Macro_DoWork(1);
			Macro_DoWork(2);
			Macro_DoWork(3);

			pDestination += 8*4;
			pSource += 8*4;
		}

		uint32 nToProcessUnroll = _nToProcess & (~uint32(7));
		for (uint32 i = nToProcessUnroll4; i < nToProcessUnroll; i += 8)
		{
			mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			Macro_DoWork(0);

			pDestination += 8;
			pSource += 8;
		}
		
#undef Macro_DoWork

		for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
		{
			mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			Macro_DoWork1(0);

			pSource += _nChannels;
			++pDestination;
		}


	}
	else if (_nChannels == 2)
	{
		vec128 WorkVolume0 = M_VShuf(Volume0, M_VSHUF(0,1,0,1));
		// Lerp
#define Macro_DoWork(_iIndex) \
			vec128 Value##_iIndex##0 = M_VLdU(pSource + _iIndex * 8);\
			vec128 Value##_iIndex##1 = M_VCnvH_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
			Value##_iIndex##0 = M_Vi32tofp32(Value##_iIndex##0);\
			Value##_iIndex##1 = M_Vi32tofp32(Value##_iIndex##1);\
			Value##_iIndex##0 = M_VMul(Value##_iIndex##0, WorkVolume0);\
			Value##_iIndex##1 = M_VMul(Value##_iIndex##1, WorkVolume0);\
			pDestination[_iIndex*4+0] = M_VPerm(Value##_iIndex##0, Zero, 0, 1, 4, 4);\
			pDestination[_iIndex*4+1] = M_VPerm(Value##_iIndex##0, Zero, 2, 3, 4, 4);\
			pDestination[_iIndex*4+2] = M_VPerm(Value##_iIndex##1, Zero, 0, 1, 4, 4);\
			pDestination[_iIndex*4+3] = M_VPerm(Value##_iIndex##1, Zero, 2, 3, 4, 4);\


		mint LastPrecache;
		LastPrecache = (((mint)pSource) + 127) & ~mint(127);
		M_PRECACHE128(LastPrecache, 0);
		M_PRECACHE128(LastPrecache + 128, 0);
		M_PRECACHE128(LastPrecache + 256, 0);
		M_PRECACHE128(LastPrecache + 384, 0);
		M_PRECACHE128(LastPrecache + 512, 0);

		mint LastZeroAddr = (mint)pDestination;
		if (!(LastZeroAddr & 127))
		{
			M_PREZERO128(LastZeroAddr, 0);
		}
		LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
		
		uint32 nToProcessUnroll4 = _nToProcess & (~uint32(15));
		for (uint32 i = 0; i < nToProcessUnroll4; i += 16)
		{
			mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				// Do 2 at a time
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(0, (void *)ZeroAddr0);
				M_PREZERO128(128, (void *)ZeroAddr0);
			}

			mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
			if (IPrecache != LastPrecache)
			{
				LastPrecache = IPrecache;
				M_PRECACHE128(IPrecache + 512, 0);
			}

			Macro_DoWork(0);
			Macro_DoWork(1);
			Macro_DoWork(2);
			Macro_DoWork(3);

			pDestination += 8*2;
			pSource += 8*4;
		}

		uint32 nToProcessUnroll = _nToProcess & (~uint32(7));
		for (uint32 i = nToProcessUnroll4; i < nToProcessUnroll; i += 4)
		{
			mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			Macro_DoWork(0);

			pDestination += 4;
			pSource += 8;
		}
		
#undef Macro_DoWork

		for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
		{
			mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			Macro_DoWork1(0);

			pSource += _nChannels;
			++pDestination;
		}


	}
	else
	{
		// Lerp

		uint32 nToProcessUnroll = _nToProcess & (~uint32(7));


		mint LastPrecache;
		LastPrecache = (((mint)pSource) + 127) & ~mint(127);
		M_PRECACHE128(LastPrecache, 0);
		M_PRECACHE128(LastPrecache + 128, 0);
		M_PRECACHE128(LastPrecache + 256, 0);
		M_PRECACHE128(LastPrecache + 384, 0);
		M_PRECACHE128(LastPrecache + 512, 0);

		mint LastZeroAddr = (mint)pDestination;
		if (!(LastZeroAddr & 127))
		{
			M_PREZERO128(LastZeroAddr, 0);
		}
		LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
		
		for (uint32 i = 0; i < nToProcessUnroll; i += 8)
		{
			mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
			if (IPrecache != LastPrecache)
			{
				LastPrecache = IPrecache;
				M_PRECACHE128(IPrecache + 512, 0);
			}

			Macro_DoWork1(0);
			Macro_DoWork1(1);
			Macro_DoWork1(2);
			Macro_DoWork1(3);
			Macro_DoWork1(4);
			Macro_DoWork1(5);
			Macro_DoWork1(6);
			Macro_DoWork1(7);

			pDestination += 8;
			pSource += 8 * _nChannels;
		}
		
		for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
		{
			mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
			if (ZeroAddr0 > LastZeroAddr)
			{
				LastZeroAddr = ZeroAddr0;
				M_PREZERO128(ZeroAddr0, 0);
			}

			Macro_DoWork1(0);

			pSource += _nChannels;
			++pDestination;
		}

#undef Macro_DoWork1
	}

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_1_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
	case 1:
		{
			SourcePos = ProcessNoResample_1_int16_Helper<1, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_1_int16_Helper<2, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_1_int16_Helper<3, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 4:
		{
			SourcePos = ProcessNoResample_1_int16_Helper<4, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_2_int16_Helper(fp32 _SourcePos, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const int16 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(Value##_iIndex##0);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(Value##_iIndex##0);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		pDestination[_iIndex*2+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*2+1] = M_VMul(Value1_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_2_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_2_int16_Helper<5, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 2:
		{
			SourcePos = ProcessNoResample_2_int16_Helper<6, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#ifndef M_REDUCECODESIZE
	case 3:
		{
			SourcePos = ProcessNoResample_2_int16_Helper<7, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_2_int16_Helper<8, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_3_int16_Helper(fp32 _SourcePos, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const int16 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 ValueS0##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 0);\
		vec128 ValueS1##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 8);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(ValueS0##_iIndex##0);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(ValueS0##_iIndex##0);\
		vec128 Value2_##_iIndex##0 = M_VCnvL_i16_i32(ValueS1##_iIndex##0);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		Value2_##_iIndex##0 = M_Vi32tofp32(Value2_##_iIndex##0);\
		pDestination[_iIndex*3+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+1] = M_VMul(Value1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+2] = M_VMul(Value2_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_3_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_3_int16_Helper<9, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_3_int16_Helper<10, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_3_int16_Helper<11, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_3_int16_Helper<12, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_4_int16_Helper(fp32 _SourcePos, uint32 _nToProcess, const int16 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const int16 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulInt16 = M_VConst(1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f, 1.0f/32767.0f);
	vec128 Volume0 = M_VMul(_Volume0, MulInt16);
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 ValueS0##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 0);\
		vec128 ValueS1##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 8);\
		vec128 Value0_##_iIndex##0 = M_VCnvL_i16_i32(ValueS0##_iIndex##0);\
		vec128 Value1_##_iIndex##0 = M_VCnvH_i16_i32(ValueS0##_iIndex##0);\
		vec128 Value2_##_iIndex##0 = M_VCnvL_i16_i32(ValueS1##_iIndex##0);\
		vec128 Value3_##_iIndex##0 = M_VCnvH_i16_i32(ValueS1##_iIndex##0);\
		Value0_##_iIndex##0 = M_Vi32tofp32(Value0_##_iIndex##0);\
		Value1_##_iIndex##0 = M_Vi32tofp32(Value1_##_iIndex##0);\
		Value2_##_iIndex##0 = M_Vi32tofp32(Value2_##_iIndex##0);\
		Value3_##_iIndex##0 = M_Vi32tofp32(Value3_##_iIndex##0);\
		pDestination[_iIndex*4+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+1] = M_VMul(Value1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+2] = M_VMul(Value2_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+3] = M_VMul(Value3_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_4_int16(uint32 _nChannels, const int16 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_4_int16_Helper<13, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_4_int16_Helper<14, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_4_int16_Helper<15, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_4_int16_Helper<16, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_1_fp32_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const fp32 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulVol = M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One);
	vec128 Volume0 = M_VMul(_Volume0, MulVol);
	vec128 VolumeDelta = M_VMul(_VolumeDelta, MulVol);

#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 Value##_iIndex##1;\
		if (_nChannels == 1)\
		{\
			Value##_iIndex##1 = M_VSplat(Value##_iIndex##0, 1);\
		}\
		else if (_nChannels == 2)\
		{\
			Value##_iIndex##1 = M_VShuf(Value##_iIndex##0, M_VSHUF(2,3,2,3));\
		}\
		else\
		{\
			Value##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 4);\
		}\
		vec128 Temp##_iIndex##0 = M_VMul(Value##_iIndex##0, FracInv##_iIndex );\
		Temp##_iIndex##0 = M_VMAdd(Value##_iIndex##1, Frac##_iIndex , Temp##_iIndex##0);\
		pDestination[_iIndex] = M_VMul(Temp##_iIndex##0, Volume0);\
		Volume0 = M_VAdd(Volume0, VolumeDelta);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_1_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const fp32 * pSource = _pInData;
	vec128 *pDestination = _Destination;

	switch (_nChannels)
	{
	case 1:
		{
			SourcePos = ProcessLinear_1_fp32_Helper<1, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_1_fp32_Helper<2, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_1_fp32_Helper<3, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 4:
		{
			SourcePos = ProcessLinear_1_fp32_Helper<4, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}


template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_2_fp32_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const fp32 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 4);\
		vec128 Value0_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 Value1_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 4 + _nChannels);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		pDestination[_iIndex*2+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*2+1] = M_VMul(Temp1_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}


fp32 ProcessLinear_2_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const fp32 * pSource = (fp32 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_2_fp32_Helper<5, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 2:
		{
			SourcePos = ProcessLinear_2_fp32_Helper<6, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#ifndef M_REDUCECODESIZE
	case 3:
		{
			SourcePos = ProcessLinear_2_fp32_Helper<7, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_2_fp32_Helper<8, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_3_fp32_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const fp32 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 4);\
		vec128 Value2_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 8);\
		vec128 Value0_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 Value1_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 4 + _nChannels);\
		vec128 Value2_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 8 + _nChannels);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		vec128 Temp2_##_iIndex##0 = M_VMul(Value2_##_iIndex##0, FracInv##_iIndex );\
		Temp2_##_iIndex##0 = M_VMAdd(Value2_##_iIndex##1, Frac##_iIndex , Temp2_##_iIndex##0);\
		pDestination[_iIndex*3+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+1] = M_VMul(Temp1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+2] = M_VMul(Temp2_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_3_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const fp32 * pSource = (fp32 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_3_fp32_Helper<9, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_3_fp32_Helper<10, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_3_fp32_Helper<11, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_3_fp32_Helper<12, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}


template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessLinear_4_fp32_Helper(fp32 _SourcePos, fp32 _StepSize, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	const fp32 * M_RESTRICT pSource = _pSource;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 SPos = M_VLdScalar(_SourcePos);
	vec128 SStepSize = M_VLdScalar(_StepSize);
	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));
#ifndef PLATFORM_SPU
	int32 TempInts[8];
#endif

		// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 0);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 4);\
		vec128 Value2_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 8);\
		vec128 Value3_##_iIndex##0 = M_VLdU(pSource + iPos##_iIndex + 12);\
		vec128 Value0_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + _nChannels);\
		vec128 Value1_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 4 + _nChannels);\
		vec128 Value2_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 8 + _nChannels);\
		vec128 Value3_##_iIndex##1 = M_VLdU(pSource + iPos##_iIndex + 12 + _nChannels);\
		vec128 Temp0_##_iIndex##0 = M_VMul(Value0_##_iIndex##0, FracInv##_iIndex );\
		Temp0_##_iIndex##0 = M_VMAdd(Value0_##_iIndex##1, Frac##_iIndex , Temp0_##_iIndex##0);\
		vec128 Temp1_##_iIndex##0 = M_VMul(Value1_##_iIndex##0, FracInv##_iIndex );\
		Temp1_##_iIndex##0 = M_VMAdd(Value1_##_iIndex##1, Frac##_iIndex , Temp1_##_iIndex##0);\
		vec128 Temp2_##_iIndex##0 = M_VMul(Value2_##_iIndex##0, FracInv##_iIndex );\
		Temp2_##_iIndex##0 = M_VMAdd(Value2_##_iIndex##1, Frac##_iIndex , Temp2_##_iIndex##0);\
		vec128 Temp3_##_iIndex##0 = M_VMul(Value3_##_iIndex##0, FracInv##_iIndex );\
		Temp3_##_iIndex##0 = M_VMAdd(Value3_##_iIndex##1, Frac##_iIndex , Temp3_##_iIndex##0);\
		pDestination[_iIndex*4+0] = M_VMul(Temp0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+1] = M_VMul(Temp1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+2] = M_VMul(Temp2_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+3] = M_VMul(Temp3_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv0(1);
		Macro_DoIntConv0(2);
		Macro_DoIntConv0(3);
		Macro_DoIntConv0(4);
		Macro_DoIntConv0(5);
		Macro_DoIntConv0(6);
		Macro_DoIntConv0(7);

		Macro_DoIntConv1(0);
		Macro_DoIntConv1(1);
		Macro_DoIntConv1(2);
		Macro_DoIntConv1(3);
		Macro_DoIntConv1(4);
		Macro_DoIntConv1(5);
		Macro_DoIntConv1(6);
		Macro_DoIntConv1(7);

		Macro_DoIntConv2(0);
		Macro_DoIntConv2(1);
		Macro_DoIntConv2(2);
		Macro_DoIntConv2(3);
		Macro_DoIntConv2(4);
		Macro_DoIntConv2(5);
		Macro_DoIntConv2(6);
		Macro_DoIntConv2(7);

		mint IPrecache = (((mint)pSource) + iPos0 + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);


		pDestination += 8;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoIntConv0(0);
		Macro_DoIntConv1(0);
		Macro_DoIntConv2(0);
		Macro_DoWork(0);

		++pDestination;
	}

#undef Macro_DoWork

	return ((CVec4Dfp32 &)SPos).k[0];
}

fp32 ProcessLinear_4_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, fp32 _SampleRate, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;
	fp32 StepSize = _SampleRate;

	nToProcess = Clamp((int32)TruncToInt(((_nMaxIn - SourcePos - 1) / StepSize + 0.999)), 0, int32(nToProcess));
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	const fp32 * pSource = (fp32 *)_pInData;
	vec128 *pDestination = (vec128 *)_Destination;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessLinear_4_fp32_Helper<13, 0, 1, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessLinear_4_fp32_Helper<14, 0, 0, 1, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessLinear_4_fp32_Helper<15, 0, 0, 0, 1>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessLinear_4_fp32_Helper<16, 0, 0, 0, 0>(SourcePos, StepSize, nToProcess, pSource, _Volume, _VolumeDelta, pDestination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_1_fp32_Helper(fp32 _SourcePos, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const fp32 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 MulVol = M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One);
	vec128 Volume0 = M_VMul(_Volume0, MulVol);
	vec128 VolumeDelta = M_VMul(_VolumeDelta, MulVol);

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels);\
		pDestination[_iIndex] = M_VMul(Value##_iIndex##0, Volume0);\
		Volume0 = M_VAdd(Volume0, VolumeDelta);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_1_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
	case 1:
		{
			SourcePos = ProcessNoResample_1_fp32_Helper<1, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_1_fp32_Helper<2, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_1_fp32_Helper<3, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 4:
		{
			SourcePos = ProcessNoResample_1_fp32_Helper<4, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_2_fp32_Helper(fp32 _SourcePos, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const fp32 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 4);\
		pDestination[_iIndex*2+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*2+1] = M_VMul(Value1_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_2_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_2_fp32_Helper<5, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 2:
		{
			SourcePos = ProcessNoResample_2_fp32_Helper<6, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#ifndef M_REDUCECODESIZE
	case 3:
		{
			SourcePos = ProcessNoResample_2_fp32_Helper<7, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_2_fp32_Helper<8, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_3_fp32_Helper(fp32 _SourcePos, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const fp32 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 0);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 4);\
		vec128 Value2_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 8);\
		pDestination[_iIndex*3+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+1] = M_VMul(Value1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*3+2] = M_VMul(Value2_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_3_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_3_fp32_Helper<9, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_3_fp32_Helper<10, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_3_fp32_Helper<11, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_3_fp32_Helper<12, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;	
}

template <uint32 _nChannels, uint32 _Mask0, uint32 _Mask1, uint32 _Mask2, uint32 _Mask3>
fp32 ProcessNoResample_4_fp32_Helper(fp32 _SourcePos, uint32 _nToProcess, const fp32 *_pSource, vec128 _Volume0, vec128 _VolumeDelta, vec128 *_pDestination)
{
	uint32 iPos = TruncToInt(_SourcePos);
	const fp32 * M_RESTRICT pSource = _pSource + iPos * _nChannels;

	vec128 * M_RESTRICT pDestination = _pDestination;

	vec128 One = M_VOne();
	vec128 Zero = M_VZero();
	vec128 Volume0 = _Volume0;
	vec128 Volume1 = M_VMul(Volume0, M_VSelMsk(M_VConstMsk(_Mask0,_Mask1,_Mask2,_Mask3),Zero,One));

	// Lerp
#define Macro_DoWork(_iIndex) \
		vec128 Value0_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 0);\
		vec128 Value1_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 4);\
		vec128 Value2_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 8);\
		vec128 Value3_##_iIndex##0 = M_VLdU(pSource + _iIndex * _nChannels + 12);\
		pDestination[_iIndex*4+0] = M_VMul(Value0_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+1] = M_VMul(Value1_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+2] = M_VMul(Value2_##_iIndex##0, Volume0);\
		pDestination[_iIndex*4+3] = M_VMul(Value3_##_iIndex##0, Volume1);

	uint32 nToProcessUnroll = _nToProcess & (~uint32(7));

	mint LastPrecache;
	LastPrecache = (((mint)pSource) + 127) & ~mint(127);
	M_PRECACHE128(LastPrecache, 0);
	M_PRECACHE128(LastPrecache + 128, 0);
	M_PRECACHE128(LastPrecache + 256, 0);
	M_PRECACHE128(LastPrecache + 384, 0);
	M_PRECACHE128(LastPrecache + 512, 0);

	mint LastZeroAddr = (mint)pDestination;
	if (!(LastZeroAddr & 127))
	{
		M_PREZERO128(LastZeroAddr, 0);
	}
	LastZeroAddr = (LastZeroAddr + 127) & ~mint(127);

//		int32 IntValues[8];
	
	for (uint32 i = 0; i < nToProcessUnroll; i += 8)
	{
		mint ZeroAddr0 = (((mint)pDestination) + 127) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		mint IPrecache = (((mint)pSource) + 127) & ~mint(127);
		if (IPrecache != LastPrecache)
		{
			LastPrecache = IPrecache;
			M_PRECACHE128(IPrecache + 512, 0);
		}

		Macro_DoWork(0);
		Macro_DoWork(1);
		Macro_DoWork(2);
		Macro_DoWork(3);
		Macro_DoWork(4);
		Macro_DoWork(5);
		Macro_DoWork(6);
		Macro_DoWork(7);

		pDestination += 8;
		pSource += 8 * _nChannels;
	}
	
	for (uint32 i = nToProcessUnroll; i < _nToProcess; ++i)
	{
		mint ZeroAddr0 = ((mint)pDestination) & (~mint(127));
		if (ZeroAddr0 > LastZeroAddr)
		{
			LastZeroAddr = ZeroAddr0;
			M_PREZERO128(ZeroAddr0, 0);
		}

		Macro_DoWork(0);

		pSource += _nChannels;
		++pDestination;
	}

#undef Macro_DoWork

	return _SourcePos + _nToProcess;
}

fp32 ProcessNoResample_4_fp32(uint32 _nChannels, const fp32 * _pInData, vec128 _Volume, vec128 _VolumeDelta, uint32 _nMaxIn, fp32 _SampleOffset, vec128 *_Destination, uint32 &_SamplesProduced)
{
	uint32 nToProcess = _SamplesProduced;
	fp32 SourcePos = _SampleOffset;

	nToProcess = Min((uint32)TruncToInt(((_nMaxIn - SourcePos))), nToProcess);
	nToProcess = Max(nToProcess, uint32(1));
	_SamplesProduced = nToProcess;

	switch (_nChannels)
	{
#ifndef M_REDUCECODESIZE
	case 1:
		{
			SourcePos = ProcessNoResample_4_fp32_Helper<13, 0, 1, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 2:
		{
			SourcePos = ProcessNoResample_4_fp32_Helper<14, 0, 0, 1, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	case 3:
		{
			SourcePos = ProcessNoResample_4_fp32_Helper<15, 0, 0, 0, 1>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
#endif
	case 4:
		{
			SourcePos = ProcessNoResample_4_fp32_Helper<16, 0, 0, 0, 0>(SourcePos, nToProcess, _pInData, _Volume, _VolumeDelta, _Destination);
		}
		break;
	}
	return SourcePos;
}

#undef Macro_DoIntConv0
#undef Macro_DoIntConv1
#undef Macro_DoIntConv2

static M_INLINE int FloatIsNAN(const fp32 &_Number)
{
	if (( (*((uint32*)&_Number)) & 0x7F800000) == 0x7F800000)
	{
		if ((*((uint32*)&_Number)) & 0x007FFFFF)
			return true;
		else
			return false;
	}
	else
		return false;
}

void CSC_Mixer_DSP_Voice::ProcessFrame(CSC_Mixer_ProcessingInfo *_pInfo, void *_pInternalData, const void *_pParams, void *_pLastParams)
{
//	M_TRACEALWAYS("CSC_Mixer_DSP_Voice ");

	const CParams *pParams = (const CParams *)_pParams;
	CLastParams *pLastParams = (CLastParams *)_pLastParams;

	CVoice &Voice = *pParams->m_pSourceVoice;

	uint32 nChannels = Voice.m_nChannels;
	_pInfo->m_nChannels = nChannels; // Copy number of channels

	uint32 nSamples = _pInfo->m_nSamples;

	fp32 OldVolume = pLastParams->m_SingleVolume;
//	fp32 ChangeSpeed = 0.5f;
//	M_TRACEALWAYS("%f\n", OldVolume);
	// This will take about 80 ms to reach target value which is too long
	//	ModerateFloat(pLastParams->m_SingleVolume, pParams->m_SingleVolume, pLastParams->m_SingleVolumePrim, ChangeSpeed);
	fp32 NewVolume = pParams->m_SingleVolume;
	pLastParams->m_SingleVolume = pParams->m_SingleVolume;

	vec128 SamplesInv = M_VRcp(M_VCnv_i32_f32(M_VLdScalar_u32(_pInfo->m_nSamples)));
	vec128 *pDest = _pInfo->m_pDataOut;
	vec128 Volume;
	Volume = M_VLdScalar(OldVolume);
	vec128 VolumeDelta = M_VMul(M_VSub(M_VLdScalar(NewVolume), Volume), SamplesInv);


//	pLastParams->m_SingleVolume = pParams->m_SingleVolume;

	bint bEndOfSample = false;
	while (nSamples)
	{
		while (!Voice.m_pCurrentPacket || Voice.m_LoopSkipCurrent)
		{
			if (!Voice.m_pCurrentPacket)
			{
				{
					Voice.m_pCurrentPacket = Voice.m_QueuedPackets.Pop();
				}
				if (!Voice.m_pCurrentPacket)
				{
					if (Voice.m_SamplePos >= Voice.m_nSamples)
					{
						// Loop?
						if (Voice.m_LoopPoint < 0)
							bEndOfSample = true;
					}

					break;
				}
			}

			if (Voice.m_LoopSkipCurrent)
			{
				uint32 nMax = Voice.m_pCurrentPacket->m_nSamples - Voice.m_CurrentPacketPos;
				nMax = Min(nMax, Voice.m_LoopSkipCurrent);

				if (nMax)
				{
//					CSC_Mixer_Packet_fp32 *p32 = (CSC_Mixer_Packet_fp32 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)Voice.m_pCurrentPacket);
//					fp32 *pTemp = p32->m_pData + Voice.m_CurrentPacketPos;
					Voice.m_LoopSkipCurrent -= nMax;
					Voice.m_CurrentPacketPos += nMax;
				}

				if (Voice.m_pCurrentPacket && Voice.m_CurrentPacketPos == Voice.m_pCurrentPacket->m_nSamples)
				{
					Voice.m_CurrentPacketPos = 0;
					{
						if (Voice.m_pCurrentPacket->m_bCircular)
							Voice.m_QueuedPackets.Insert(Voice.m_pCurrentPacket);
						else
							Voice.m_PendingFinishedPackets.Insert(Voice.m_pCurrentPacket);
						Voice.m_pCurrentPacket = NULL;
					}
				}
			}
			else
				break;
		}
		if (!Voice.m_pCurrentPacket)
			break;

		fp32 nSamplesConsumed = 0;
		fp32 SampleOffset = Voice.m_SamplesLeft;
		fp32 SampleRate = Voice.m_CurrentPitch;
		if (SampleRate > 16.0f)
			SampleRate = 16.0f;
		uint32 nSamplesProduced = nSamples;

		int32 nMaxSamples = (Voice.m_pCurrentPacket->m_nSamples - Voice.m_CurrentPacketPos);
		int32 nMaxSamplesReal = nMaxSamples;

		int32 nSamplesLeft = 0;
		if (Voice.m_LoopPoint < 0)
		{
			nSamplesLeft = Voice.m_nSamples - Voice.m_SamplePos;
			nMaxSamples = Min(nMaxSamples, nSamplesLeft);
		}
		else
		{
			if (Voice.m_SamplePos == 0)
			{
				if (!Voice.m_bLoopSkipDone && Voice.m_LoopSkipStart)
				{
					// We are at wrap point
					Voice.m_LoopSkipCurrent = Voice.m_LoopSkipStart;
					Voice.m_bLoopSkipDone = true;
					continue;
				}			
				else
				{
					if (nMaxSamples)
						Voice.m_bLoopSkipDone = false;
				}
			}
			else
			{
				uint32 Wrapped = Voice.m_SamplePos % Voice.m_nSamples;
				int32 nMax = Voice.m_nSamples - Wrapped;
				nMaxSamples = Min(nMaxSamples, nMax);
				if (Wrapped == 0 && Voice.m_SamplePos >= Voice.m_nSamples && !Voice.m_bLoopSkipDone && Voice.m_LoopSkipBoth)
				{
					// We are at wrap point
					Voice.m_LoopSkipCurrent = Voice.m_LoopSkipBoth;
					Voice.m_bLoopSkipDone = true;
					continue;
				}			
				else
				{
					if (nMaxSamples)
						Voice.m_bLoopSkipDone = false;
				}
			}
		}


		int32 nSamplesNeedeApprox = Min(int32(TruncToInt(nSamples * SampleRate)) + 16, nMaxSamples);
		
//			if (SampleRate > 1.0f)
//				 nMaxSamples -= TruncToInt(2.0f * SampleRate);

		if (nMaxSamplesReal == 1)
		{
			// We are at edge
			CSC_Mixer_WorkerContext::CSC_Mixer_Packet *pNextPacket;
			{
//				DLock(m_pMixer->m_VoiceQueueLock);
				if (Voice.m_pCurrentPacket->m_bCircular)
					pNextPacket = Voice.m_pCurrentPacket;
				else
					pNextPacket = Voice.m_QueuedPackets.GetFirst();
			}
			if (!pNextPacket && nSamplesLeft != 1)
				break; // Starving

			switch (Voice.m_pCurrentPacket->m_PacketType)
			{
#ifndef PLATFORM_SPU
			case ESCMixer_PacketType_int16:
				{
					// 16 to 1 max resampling
					int16 TempBuffer[16 * 2];

					CSC_Mixer_Packet_int16 *pPacket = (CSC_Mixer_Packet_int16 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)Voice.m_pCurrentPacket);
					int16 *pPacketData = pPacket->m_pData;
					pPacketData += Voice.m_CurrentPacketPos * nChannels;
					DataCopy(TempBuffer, pPacketData, nChannels);

					if (pNextPacket)
					{
						pPacket = (CSC_Mixer_Packet_int16 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)pNextPacket);
						pPacketData = pPacket->m_pData;
						
						DataCopy(TempBuffer + nChannels, pPacketData, nChannels);
						nMaxSamples = Min(int32(2), nMaxSamples);
					}
					else
					{
						nMaxSamples = 1;
					}

					if (SampleRate == 1.0)
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessNoResample_1_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessNoResample_2_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessNoResample_3_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessNoResample_4_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
					}
					else
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessLinear_1_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessLinear_2_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessLinear_3_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessLinear_4_int16(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
					}
				}
				break;
#endif
			case ESCMixer_PacketType_fp32:
				{
					// 16 to 1 max resampling
					fp32 TempBuffer[16 * 2];

					CSC_Mixer_Packet_fp32 *pPacket = (CSC_Mixer_Packet_fp32 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)Voice.m_pCurrentPacket);
					fp32 *pPacketData = pPacket->m_pData;
					pPacketData += Voice.m_CurrentPacketPos * nChannels;

#ifdef PLATFORM_SPU
					{
						int32 nToTransfer = 1;
						mint Offset = (mint(pPacketData) - (mint(pPacketData) & (~mint(15))))/sizeof(fp32);
						fp32 *pEndSample = pPacketData + nToTransfer * nChannels;
						pEndSample = (fp32 *)((mint(pEndSample) + 15) & (~mint(15)));
						fp32 *pStartSample = pPacketData - Offset;

						MRTC_System_VPU::OS_DMATransferFromSys(g_pPacketCache, (mint)pStartSample, (mint)pEndSample - (mint)pStartSample);

						pPacketData = ((fp32 *)g_pPacketCache) + Offset;
					}
#endif

					DataCopy(TempBuffer, pPacketData, nChannels);



					if (pNextPacket)
					{
						pPacket = (CSC_Mixer_Packet_fp32 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)pNextPacket);
						pPacketData = pPacket->m_pData;
#ifdef PLATFORM_SPU
						{
							int32 nToTransfer = 1;
							mint Offset = (mint(pPacketData) - (mint(pPacketData) & (~mint(15))))/sizeof(fp32);
							fp32 *pEndSample = pPacketData + nToTransfer * nChannels;
							pEndSample = (fp32 *)((mint(pEndSample) + 15) & (~mint(15)));
							fp32 *pStartSample = pPacketData - Offset;

							MRTC_System_VPU::OS_DMATransferFromSys(g_pPacketCache, (mint)pStartSample, (mint)pEndSample - (mint)pStartSample);

							pPacketData = ((fp32 *)g_pPacketCache) + Offset;
						}
#endif

						DataCopy(TempBuffer + nChannels, pPacketData, nChannels);
						nMaxSamples = Min(int32(2), nMaxSamples);
					}
					else
					{
						nMaxSamples = 1;
					}

					if (SampleRate == 1.0)
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessNoResample_1_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessNoResample_2_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessNoResample_3_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessNoResample_4_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
					}
					else
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessLinear_1_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessLinear_2_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessLinear_3_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessLinear_4_fp32(nChannels, TempBuffer, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
					}
				}
				break;
			}

			if (nSamplesConsumed >= 1.0f)
			{
				if (Voice.m_pCurrentPacket->m_bCircular)
					Voice.m_QueuedPackets.Insert(Voice.m_pCurrentPacket);
				else
					Voice.m_PendingFinishedPackets.Insert(Voice.m_pCurrentPacket);

				Voice.m_pCurrentPacket = Voice.m_QueuedPackets.Pop();
				Voice.m_CurrentPacketPos = -1;
			}
		}
		else if (nMaxSamples)
		{

			switch (Voice.m_pCurrentPacket->m_PacketType)
			{
#ifndef PLATFORM_SPU
			case ESCMixer_PacketType_int16:
				{
					CSC_Mixer_Packet_int16 *pPacket = (CSC_Mixer_Packet_int16 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)Voice.m_pCurrentPacket);
					int16 *pPacketData = pPacket->m_pData;
					pPacketData += Voice.m_CurrentPacketPos * nChannels;
					if (SampleRate == 1.0)
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessNoResample_1_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessNoResample_2_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessNoResample_3_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessNoResample_4_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
					}
					else
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessLinear_1_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessLinear_2_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessLinear_3_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessLinear_4_int16(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
					}
				}
				break;
#endif
			case ESCMixer_PacketType_fp32:
				{
					CSC_Mixer_Packet_fp32 *pPacket = (CSC_Mixer_Packet_fp32 *)((CSC_Mixer_WorkerContext::CSC_Mixer_Packet *)Voice.m_pCurrentPacket);
					fp32 *pPacketData = pPacket->m_pData;
					pPacketData += Voice.m_CurrentPacketPos * nChannels;
#ifdef PLATFORM_SPU
					int32 nMaxSamplesInCache = uint32(EPacketCacheSize / (nChannels * sizeof(fp32)) - 16) & (~uint32(15));
					nSamplesNeedeApprox = Min(nSamplesNeedeApprox, nMaxSamplesInCache);
					nMaxSamples = Min(nSamplesNeedeApprox, nMaxSamples);
					mint Offset = (mint(pPacketData) - (mint(pPacketData) & (~mint(15))))/sizeof(fp32);
					fp32 *pEndSample = pPacketData + nMaxSamples * nChannels;
					pEndSample = (fp32 *)((mint(pEndSample) + 15) & (~mint(15)));
					fp32 *pStartSample = pPacketData - Offset;

					MRTC_System_VPU::OS_DMATransferFromSys(g_pPacketCache, (mint)pStartSample, (mint)pEndSample - (mint)pStartSample);

					pPacketData = ((fp32 *)g_pPacketCache) + Offset;
#endif
					if (SampleRate == 1.0)
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessNoResample_1_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessNoResample_2_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessNoResample_3_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessNoResample_4_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, pDest, nSamplesProduced);
					}
					else
					{
						if (nChannels <= 4)
							nSamplesConsumed = ProcessLinear_1_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 8)
							nSamplesConsumed = ProcessLinear_2_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 12)
							nSamplesConsumed = ProcessLinear_3_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
						else if (nChannels <= 16)
							nSamplesConsumed = ProcessLinear_4_fp32(nChannels, pPacketData, Volume, VolumeDelta, nMaxSamples, SampleOffset, SampleRate, pDest, nSamplesProduced);
					}
				}
				break;
			}
		}

		uint32 Temp = TruncToInt(nSamplesConsumed);
		Voice.m_CurrentPacketPos += Temp;
		Voice.m_SamplePos += Temp;
		Voice.m_SamplesLeft = nSamplesConsumed - Temp;

		pDest += nSamplesProduced;
		nSamples -= nSamplesProduced;

		if (Voice.m_pCurrentPacket && Voice.m_CurrentPacketPos >= Voice.m_pCurrentPacket->m_nSamples)
		{
			Voice.m_CurrentPacketPos = 0;
			{
//				DLock(m_pMixer->m_VoiceQueueLock);
				if (Voice.m_pCurrentPacket->m_bCircular)
					Voice.m_QueuedPackets.Insert(Voice.m_pCurrentPacket);
				else
					Voice.m_PendingFinishedPackets.Insert(Voice.m_pCurrentPacket);
				Voice.m_pCurrentPacket = NULL;
			}
		}
		if (RoundToInt(Voice.m_SamplePos + Voice.m_SamplesLeft + 0.5) >= Voice.m_nSamples)
		{
			// Loop?
			if (Voice.m_LoopPoint < 0)
			{
				Voice.m_SamplePos = RoundToInt(Voice.m_SamplePos + Voice.m_SamplesLeft + 0.5);
				bEndOfSample = true;
				break;
			}
		}
	}

	if (nSamples)
	{
//		if (!bEndOfSample)
//			M_TRACEALWAYS("Stream is starving\n");
		// Starving, lets fill the rest with 0.0
		if (nChannels <= 4)
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				pDest[i] = M_VZero();
			}
		}
		else if (nChannels <= 8)
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				pDest[i * 2 + 0] = M_VZero();
				pDest[i * 2 + 1] = M_VZero();
				
			}
		}
		else if (nChannels <= 12)
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				pDest[i * 3 + 0] = M_VZero();
				pDest[i * 3 + 1] = M_VZero();
				pDest[i * 3 + 2] = M_VZero();
			}
		}
		else if (nChannels <= 16)
		{
			for (uint32 i = 0; i < nSamples; ++i)
			{
				pDest[i * 4 + 0] = M_VZero();
				pDest[i * 4 + 1] = M_VZero();
				pDest[i * 4 + 2] = M_VZero();
				pDest[i * 4 + 3] = M_VZero();
			}
		}
	}

#if 0
	{
		vec128 *pDest = _pInfo->m_pDataOut;
		for (uint32 i = 0; i < 256; ++i)
		{
			if (pDest[i].x > 1.1 ||  pDest[i].x < -1.1 || FloatIsNAN(pDest[i].x))
				int nethu= 0;
		}
	}
#endif

}
