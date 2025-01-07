#ifndef WDYNAMICS_SUPPORT_H
#define WDYNAMICS_SUPPORT_H

/*
	Temporary support functions.
	Subjected to removal...	
 */

M_INLINE CVec3Dfp64 ConvertVector(const CVec3Dfp32& _Vec)
{
	CVec3Dfp64 ret;
	ret.k[0] = _Vec.k[0];
	ret.k[1] = _Vec.k[1];
	ret.k[2] = _Vec.k[2];
	return ret;
}

M_INLINE CQuatfp64 ConvertQuat(const CQuatfp32& _Quat)
{
	CQuatfp64 ret;
	ret.k[0] = _Quat.k[0];
	ret.k[1] = _Quat.k[1];
	ret.k[2] = _Quat.k[2];
	ret.k[3] = _Quat.k[3];
	return ret;
}


#endif

