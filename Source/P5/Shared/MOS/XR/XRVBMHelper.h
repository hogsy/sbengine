
#ifndef __XRVBMHELPER_H_INCLUDED
#define __XRVBMHELPER_H_INCLUDED

class CXR_VBMHelper
{
public:
	static class CXR_SkeletonInstance* Alloc_SkeletonInst(CXR_VBManager* _pVBM, int _nNodes, int _nRotTracks, int _nMoveTracks);
};

#endif
