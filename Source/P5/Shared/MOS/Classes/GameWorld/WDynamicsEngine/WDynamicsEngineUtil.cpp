#include "pch.h"
#include "WDynamicsEngine2.h"
#include "WDynamicsEngineUtil.h"

void PrintFixed(fp32 _x, int _ColumnWidth)
{
	char buf[100];

	sprintf(buf, "%1.1f", _x);

	int n = (int) (_ColumnWidth - strlen(buf));
	if (n < 0) n = 0;
	for (int i = 0; i < n; i++)
		M_TRACEALWAYS(" ");

	M_TRACEALWAYS(buf);
}

void PrintFixed(vec128 _v, int _ColumnWidth)
{
	CVec4Dfp32 v = _v;
	for (int i = 0; i < 4; i++)
	{
		PrintFixed(v[i],_ColumnWidth);
		if (i != 3) M_TRACEALWAYS(", ");
		else M_TRACEALWAYS(" ");
	}
}

void PrintFixed(const CVector4Pair& _v, int _ColumnWidth)
{
	CVec4Dfp32 v = _v.a;
	for (int i = 0; i < 4; i++)
	{
		PrintFixed(v[i],_ColumnWidth);
		M_TRACEALWAYS(", ");
	}

	v = _v.b;
	for (int i = 0; i < 4; i++)
	{
		PrintFixed(v[i],_ColumnWidth);
		if (i != 3) M_TRACEALWAYS(", ");
		else M_TRACEALWAYS(" ");
	}
}

