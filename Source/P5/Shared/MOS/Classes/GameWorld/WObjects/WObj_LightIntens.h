

class CLightIntens
{
public:
	enum
	{
		ENone		= 0,
		EPulse		= 1,
		ESine		= 2,
		ERandPulse	= 3,
		EBreak		= 4,
		ERandSine	= 5,
	};

	inline static fp32 EvaluatePulse(fp32 _Time, fp32 _Hz, fp32 _Spread)
	{
		if(EvaluateSine(_Time, _Hz, 1) > 0.5f)
			return 1;

		return 1-_Spread;
	}

	inline static fp32 EvaluateSine(fp32 _Time, fp32 _Hz, fp32 _Spread)
	{
		return 1 - ((M_Sin(_Time*_PI*_Hz)+1)/2.0f*_Spread);
	}

	inline static fp32 EvaluateRandPulse(fp32 _Time, fp32 _Hz, fp32 _Spread)
	{
		if(MFloat_GetRand(TruncToInt(_Time * _Hz)) < _Spread)
			return 0.01f;
		return 1;
	}

	inline static fp32 EvaluateBreak(fp32 _Time)
	{
		//fp32 Time = fmod(_Time, 5);
		if(_Time < 0.15f)
			return 1+M_Pow(_Time+0.9f,50);
		else if(_Time < 1)
		{
			fp32 Time1 = _Time-1;
			return EvaluateRandPulse(_Time, 15, (Time1*Time1))*(1-_Time);
		}

		return 0;
	}

	inline static fp32 EvaluateRandSine(fp32 _Time, fp32 _Hz, fp32 _Spread)
	{
		if(MFloat_GetRand(TruncToInt(_Time * _Hz)) < _Spread)
			return 1 - ((M_Sin(_Time*_PI*_Hz)+1)/2.0f*_Spread);
		return 1;
	}

	static bool GetActive(fp32 _Time, int32 _Type, fp32 _Hz, fp32 _Spread)
	{
		switch(_Type)
		{
		case EBreak : return 0; break;
		case EPulse : return EvaluatePulse(_Time, _Hz, _Spread) > 1-_Spread*0.5f; break;
		case ESine : return 1; break;
		case ERandPulse : return EvaluateRandPulse(_Time, _Hz, _Spread)>0.5f; break;
		case ERandSine : return 1; break;
		}

		return 1;
	}

	static fp32 GetIntens(fp32 _Time, int32 _Type, fp32 _Hz, fp32 _Spread)
	{
		// Apply _Type
		switch(_Type)
		{
		case EBreak : return EvaluateBreak(_Time); break;
		case EPulse : return EvaluatePulse(_Time, _Hz, _Spread); break;
		case ESine : return EvaluateSine(_Time, _Hz, _Spread); break;
		case ERandPulse : return EvaluateRandPulse(_Time, _Hz, _Spread); break;
		case ERandSine : return EvaluateRandSine(_Time, _Hz, _Spread); break;
		}

		return 1;
	}
};
