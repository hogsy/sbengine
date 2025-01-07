

extern void MRTC_CreateObjectManager();
extern void MRTC_DestroyObjectManager();

#pragma warning(disable:4074)
#pragma init_seg(compiler)

class CInitFirst
{
public:
	CInitFirst()
	{
		MRTC_CreateObjectManager();
	}
	~CInitFirst()
	{
		MRTC_DestroyObjectManager();
	}
};
CInitFirst Init;

