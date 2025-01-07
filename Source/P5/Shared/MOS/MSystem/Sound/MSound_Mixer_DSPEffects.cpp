
#include "PCH.h"
#include "MSound_Mixer.h"
#include "MSound_Mixer_DSPEffects.h"

//#pragma optimize("", off)
//#pragma inline_depth(0)

#include "MSound_Mixer_Internal_DSP_Master.imp.h"
#include "MSound_Mixer_Internal_DSP_Voice.imp.h"
#include "MSound_Mixer_Internal_DSP_Router.imp.h"
#include "MSound_Mixer_Internal_DSP_SilenceGen.imp.h"
#include "MSound_Mixer_Internal_DSP_VolumeMatrix.imp.h"

#include "MSound_Mixer_DSP_Volume.imp.h"
#include "MSound_Mixer_DSP_Copy.imp.h"
#include "MSound_Mixer_DSP_BiQuad.imp.h"
#include "MSound_Mixer_DSP_Reverb.imp.h"

/*
	ESC_Mixer_DSP_Master = 0,
	ESC_Mixer_DSP_Voice,
	ESC_Mixer_DSP_Router,
	ESC_Mixer_DSP_Max,
	*/


uint32 CSC_Mixer_WorkerContext::ms_DSP_NumParams[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumParams,
	CSC_Mixer_DSP_Voice::ENumParams,
	CSC_Mixer_DSP_Router::ENumParams,
	CSC_Mixer_DSP_SilenceGen::ENumParams,
	CSC_Mixer_DSP_VolumeMatrix::ENumParams,
	CSC_Mixer_DSP_Reverb::ENumParams,
	CSC_Mixer_DSP_Volume::ENumParams,
	CSC_Mixer_DSP_Copy::ENumParams,
	CSC_Mixer_DSP_BiQuad::ENumParams,
};

uint32 CSC_Mixer_WorkerContext::ms_DSP_NumLastParams[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumLastParams,
	CSC_Mixer_DSP_Voice::ENumLastParams,
	CSC_Mixer_DSP_Router::ENumLastParams,
	CSC_Mixer_DSP_SilenceGen::ENumLastParams,
	CSC_Mixer_DSP_VolumeMatrix::ENumLastParams,
	CSC_Mixer_DSP_Reverb::ENumLastParams,
	CSC_Mixer_DSP_Volume::ENumLastParams,
	CSC_Mixer_DSP_Copy::ENumLastParams,
	CSC_Mixer_DSP_BiQuad::ENumLastParams,
};

uint32 CSC_Mixer_WorkerContext::ms_DSP_NumInternal[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::ENumInternal,
	CSC_Mixer_DSP_Voice::ENumInternal,
	CSC_Mixer_DSP_Router::ENumInternal,
	CSC_Mixer_DSP_SilenceGen::ENumInternal,
	CSC_Mixer_DSP_VolumeMatrix::ENumInternal,
	CSC_Mixer_DSP_Reverb::ENumInternal,
	CSC_Mixer_DSP_Volume::ENumInternal,
	CSC_Mixer_DSP_Copy::ENumInternal,
	CSC_Mixer_DSP_BiQuad::ENumInternal,
};
uint32 CSC_Mixer_WorkerContext::ms_DSP_ProcessingType[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::EProcessingType,
	CSC_Mixer_DSP_Voice::EProcessingType,
	CSC_Mixer_DSP_Router::EProcessingType,
	CSC_Mixer_DSP_SilenceGen::EProcessingType,
	CSC_Mixer_DSP_VolumeMatrix::EProcessingType,
	CSC_Mixer_DSP_Reverb::EProcessingType,
	CSC_Mixer_DSP_Volume::EProcessingType,
	CSC_Mixer_DSP_Copy::EProcessingType,
	CSC_Mixer_DSP_BiQuad::EProcessingType,
};
FSCMixer_DSPFunc_DestroyData *CSC_Mixer_WorkerContext::ms_DSP_Func_DestroyData[ESC_Mixer_DSP_Max] = 
{
	NULL,
	NULL,
	NULL,
	NULL,
	CSC_Mixer_DSP_VolumeMatrix::DestroyData,
	CSC_Mixer_DSP_Reverb::DestroyData,
	NULL,
	NULL,
	NULL,
};

FSCMixer_DSPFunc_CopyData *CSC_Mixer_WorkerContext::ms_DSP_Func_CopyData[ESC_Mixer_DSP_Max] = 
{
	NULL,
	NULL,
	NULL,
	NULL,
	CSC_Mixer_DSP_VolumeMatrix::CopyData,
	NULL,
	NULL,
	NULL,
	NULL,
};

FSCMixer_DSPFunc_InitData *CSC_Mixer_WorkerContext::ms_DSP_Func_InitData[ESC_Mixer_DSP_Max] = 
{
	CSC_Mixer_DSP_Master::InitData,
	CSC_Mixer_DSP_Voice::InitData,
	CSC_Mixer_DSP_Router::InitData,
	CSC_Mixer_DSP_SilenceGen::InitData,
	CSC_Mixer_DSP_VolumeMatrix::InitData,
	CSC_Mixer_DSP_Reverb::InitData,
	CSC_Mixer_DSP_Volume::InitData,
	NULL,
	CSC_Mixer_DSP_BiQuad::InitData,
};


FSCMixer_DSPFunc_Create *CSC_Mixer_WorkerContext::ms_DSP_Func_Create[ESC_Mixer_DSP_Max] = 
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	CSC_Mixer_DSP_Reverb::Create,
	CSC_Mixer_DSP_Volume::Create,
	NULL,
	CSC_Mixer_DSP_BiQuad::Create,
};
