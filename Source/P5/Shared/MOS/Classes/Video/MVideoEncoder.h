
#ifndef DInc_MVideoEncoder_h
#define DInc_MVideoEncoder_h

class CMVideoEncoder : public CReferenceCount
{
	MRTC_DECLARE;
public:
	virtual void InitEncode(CCFile *_pFile, int _Width, int _Height, fp32 _FrameRate, int _Keyframe, fp32 _Quality = 1.0, fp32 _Bitrate = 0.0, fp32 _PixelAspect = 1.0, CStr _ExtraData = CStr()) pure;
	virtual void FinishEncode() pure;
	virtual void AddFrame(CImage &_Image, bool _bLast) pure;
};

#endif //DInc_MVideoEncoder_h