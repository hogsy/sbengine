#ifndef _INC_MRndrPS3_RSX
#define _INC_MRndrPS3_RSX

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			RSX command buffer management

Author:			Magnus Högdahl
				

Copyright:		Copyright 2006 Starbreeze Studios AB

History:		
				2006-07-17:		Created File
\*____________________________________________________________________________________________*/


namespace NRenderPS3GCM
{

// -------------------------------------------------------------------
// RSX COMMAND DEFINITIONS
// -------------------------------------------------------------------

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
RSX Command reference:

Opcode is					0x?SSSOOOO, OOOO = opcode, SSS = Data size, ? = ?

NOp							0x00000000
Attribute N Format			0x00041740 + 4N, (Stride << 8) + (Components << 4) + Format
Attribute N Offset			0x00041680 + 4N, RSX local offset to attribute data
Invalidate vertex cache		0x00041710, 0x00000000, 0x00041714, 0x00000000, 0x00041714, 0x00000000, 0x00041714, 0x00000000
Set VP param write index	0x00041efc, Start const index, 
Write VP param				(Size << 16) + 0x1f00, Constant data (Size bytes)
SetColorMask				0x00040324, Mask
SetColorMaskMrt				0x00040370, Mask
SetDepthTestEnable			0x00040a74, 1/0
SetBlendFunc				0x00080314, SrcColor + (SrcAlpha << 16), DstColor + (DstAlpha << 16)
SetStencilFunc				0x000c0330, Func, StencilRef, StencilMask
SetStencilTestEnable		0x00040328, 1/0
SetStencilMask				0x0004032c, Mask
SetStencilOp				0x000c033c, failop, depthFailop, depthPassop
SetDepthFunc				0x00040a6c, Func


DrawIndexArray:
Set index array offset		0x0008181c, RSX local offset to indices, 0x10
Set primitive type			0x00041808, PrimType
Render indices				0x40001824 + (nBatches << 18), nBatches of: ((Count-1) << 24) + Index array offset,   (0x4 = Stride??)
End indexed primitives		0x00041808, 0x00

\*____________________________________________________________________________*/

/*
(CmdDump) gcmSetVertexProgramConstants04, 76 bytes : 00041efc, 00000000, 00401f00, 3f3c7c9
8, bdee048c, bdaa00d5, 00000000, be49297c, bfa85481, 3dca459b, 00000000, 3df78f1f, 3d6783c
4, 3f7e3750, c0804020, 3df71358, 3d671002, 3f7db834, 00000000, 

(CmdDump) gcmSetVertexProgramConstants44, 76 bytes : 00041efc, 00000004, 00401f00, 3f7b50c
a, be1eadb2, bde2abc5, 00000000, 3e16df1c, 3f7c7ebf, bd97b433, 00000000, 3df71358, 3d67100
2, 3f7db834, 00000000, 417542de, 4393e9fc, 445ce6e5, 00000000, 

(CmdDump) gcmSetDrawIndexArray, 232 bytes : 0008181c, 01386010, 00000010, 00041808, 000000
05, 00041824, 37000000, 40c01824, ff000038, ff000138, ff000238, ff000338, ff000438, ff0005
38, ff000638, ff000738, ff000838, ff000938, ff000a38, ff000b38, ff000c38, ff000d38, ff000e
38, ff000f38, ff001038, ff001138, ff001238, ff001338, ff001438, ff001538, ff001638, ff0017
38, ff001838, ff001938, ff001a38, ff001b38, ff001c38, ff001d38, ff001e38, ff001f38, ff0020
38, ff002138, ff002238, ff002338, ff002438, ff002538, ff002638, ff002738, ff002838, ff0029
38, ff002a38, ff002b38, ff002c38, ff002d38, ff002e38, c7002f38, 00041808, 00000000, 
(CmdDump) gcmSetColorMask, 8 bytes : 00040324, 01010101, 
(CmdDump) gcmSetColorMaskMrt, 16 bytes : 00040370, 00008000, 00040370, 00000000, 
(CmdDump) gcmSetDepthTestEnable, 8 bytes : 00040a74, 00000001, 
(CmdDump) gcmSetBlendFunc, 24 bytes : 00080314, 03020300, 03030301, 00080314, 00010001, 00
000000, 
(CmdDump) gcmSetStencilFunc, 16 bytes : 000c0330, 00000200, 000000aa, 00000055, 
(CmdDump) gcmSetStencilTestEnable, 8 bytes : 00040328, 00000001, 
(CmdDump) gcmSetStencilMask, 8 bytes : 0004032c, 000000aa, 
(CmdDump) gcmSetStencilOp, 16 bytes : 000c033c, 00001e00, 00001e01, 00001e02, 
(CmdDump) gcmSetStencilTestEnable, 8 bytes : 00040328, 00000000, 
(CmdDump) gcmSetDepthFunc, 16 bytes : 00040a6c, 00000200, 00040a6c, 00000201, 

(CmdDump) gcmSetDrawIndexArray, 392 bytes : 0008181c, 01386010, 00000010, 00041808, 000000
05, 00041824, 37000000, 40c01824, ff000038, ff000138, ff000238, ff000338, ff000438, ff0005
38, ff000638, ff000738, ff000838, ff000938, ff000a38, ff000b38, ff000c38, ff000d38, ff000e
38, ff000f38, ff001038, ff001138, ff001238, ff001338, ff001438, ff001538, ff001638, ff0017
38, ff001838, ff001938, ff001a38, ff001b38, ff001c38, ff001d38, ff001e38, ff001f38, ff0020
38, ff002138, ff002238, ff002338, ff002438, ff002538, ff002638, ff002738, ff002838, ff0029
38, ff002a38, ff002b38, ff002c38, ff002d38, ff002e38, c7002f38, 00041808, 00000000, 
0008181c, 01386010, 00000010, 00041808, 00000004, 00041824, 37000000, 40041824, 1c000038, 00041808, 00000000, 
0008181c, 01386010, 00000010, 00041808, 00000004, 00041824, 37000000, 40041824, 71000038, 00041808, 00000000, 
0008181c, 01386010, 00000010, 00041808, 00000004, 40041824, 01000000, 00041808, 00000000, 
0008181c, 01386010, 00000010, 00041808, 00000004, 40041824, 00000000, 00041808, 00000000,

m_ContextTexture.Bind(i, TexID);
(CmdDump) Bind, 64 bytes : 
00041a04, 00088729, 
00041a10, 0000aae4, 
00041a00, 013a2200, 
00041840, 00100000, 
00041a18, 00800080, 
00041a08, 00030101, 
00041a0c, 80040000, 
00041a14, 02062000, 

(CmdDump) Bind, 64 bytes : 
00041a04, 00088729, 
00041a10, 0000aae4, 
00041a00, 013a2200, 
00041840, 00100000, 
00041a18, 00800080, 
00041a08, 00030101, 

00041a0c, 80040000, 
00041a14, 02062000, 

Texture: 87090200, 0000aae4, 0100x0100, 0001, 00, 00, 00000000, 01370500
AddrMode: 00, 01, 01, 03, 0, 00, %2x
Filter: 00, 06, 02, 01
(CmdDump) Bind, 64 bytes : 
	00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 
	00041840, 00100000, 00041a18, 01000100, 00041a08, 00030101, 
	00041a0c, 80048000, 00041a14, 02062000, 

Texture: 87090200, 0000aae4, 0100x0020, 0001, 00, 00, 00000000, 01386180
AddrMode: 00, 01, 01, 03, 0, 00, %2x
Filter: 00, 06, 02, 01
(CmdDump) Bind, 64 bytes : 
	gcmSetTexture
		00041a04, 00098729,	// m_RSXTexture.format, m_RSXTexture.mipmap, m_RSXTexture.dimension, m_RSXTexture.cubemap
		00041a10, 0000aae4, // remap
		00041a00, 01386180, // address
		00041840, 00100000, ?
		00041a18, 01000020, // width, height
	gcmSetTextureAddress
		00041a08, 00030101, // addressing mode
	gcmSetTextureControl
		00041a0c, 80048000, // more addr mode?
	gcmSetTextureFilter
		00041a14, 02062000, // Filter

Texture: 87090200, 0000aae4, 0100x0080, 0001, 00, 00, 00000000, 0138d700
AddrMode: 00, 01, 01, 03, 0, 00, %2x
Filter: 00, 06, 02, 01
(CmdDump) Bind, 64 bytes : 
	00041a04, 00098729, 
	00041a10, 0000aae4, 
	00041a00, 0138d700, 
	00041840, 00100000, 
	00041a18, 01000Texture: a5010200, 0000aa1b, 0080x0040, 0001, 00, 00, 00000200, 013c4300


	(CmdDump) gcmSetTextureAddress aa,55,44,1,7,f, 8 bytes : 00041a08, 70f4150a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,0,7,f, 8 bytes : 00041a08, 70f4050a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,1,0,f, 8 bytes : 00041a08, 00f4150a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,1,7,3, 8 bytes : 00041a08, 7034150a, 
	(CmdDump) gcmSetTextureFilter aaaa,55,44,3, 8 bytes : 00041a14, 04556aaa, 
	(CmdDump) gcmSetTextureFilter aaaa,55,44,2, 8 bytes : 00041a14, 04554aaa, 
	(CmdDump) gcmSetTextureControl true, abcd,1234,99, 8 bytes : 00041a0c, de691a10, 
	(CmdDump) gcmSetTextureControl false abcd,1234,99, 8 bytes : 00041a0c, 5e691a10, 
	(CmdDump) gcmSetTexture depth pitch, 40 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture fmt cube, 40 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture dim mipmap, 40 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	Texture: 87090200, 0000aae4, 0100x0100, 0001, 00, 00, 00000000, 01370500
	AddrMode: 00, 01, 01, 03, 0, 00, 00
	Filter: 00, 06, 02, 01
	(CmdDump) Bind, 248 bytes : 00041a08, 70f4150a, 00041a08, 70f4050a, 00041a08, 00f4150a, 00041a08, 7034150a, 00041a14, 04556aaa, 00041a14, 04554aaa, 00041a0c, de691a10, 00041a0c, 5e691a10, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a08, 00030101, 00041a0c, 80048000, 00041a14, 02062000, 

	(CmdDump) gcmSetTextureAddress aa,55,44,1,7,f, 8 bytes : 00041a08, 70f4150a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,0,7,f, 8 bytes : 00041a08, 70f4050a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,1,0,f, 8 bytes : 00041a08, 00f4150a, 
	(CmdDump) gcmSetTextureAddress aa,55,44,1,7,3, 8 bytes : 00041a08, 7034150a, 
	(CmdDump) gcmSetTextureFilter aaaa,55,44,3, 8 bytes : 00041a14, 04557fff, 
	(CmdDump) gcmSetTextureFilter aaaa,55,44,2, 8 bytes : 00041a14, 04556000, 
	(CmdDump) gcmSetTextureControl 0, ffff,0,0, 8 bytes : 00041a0c, 7ff80000, 
	(CmdDump) gcmSetTextureControl 0, 0, ffff,0, 8 bytes : 00041a0c, 0007ff80, 
	(CmdDump) gcmSetTextureControl 0, 0, 0, ff, 8 bytes : 00041a0c, 00000070, 
	(CmdDump) gcmSetTexture depth , 40 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 3ff00000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture pitch, 40 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 0003ffff, 00041a18, 01000100, 

	(CmdDump) gcmSetTexture fmt, 40 bytes : 00041a04, 0000ff09, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture cubemap, 40 bytes : 00041a04, 0000000d, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture dimension, 40 bytes : 00041a04, 000000f9, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 
	(CmdDump) gcmSetTexture mipmap, 40 bytes : 00041a04, 000f0009, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 

	Texture: 87090200, 0000aae4, 0100x0100, 0001, 00, 00, 00000000, 01370500
	AddrMode: 00, 01, 01, 03, 0, 00, 00
	Filter: 00, 06, 02, 01
	(CmdDump) Bind, 376 bytes : 00041a08, 70f4150a, 00041a08, 70f4050a, 00041a08, 00f4150a, 00041a08, 7034150a, 00041a14, 04557fff, 00041a14, 04556000, 00041a0c, 7ff80000, 00041a0c, 0007ff80, 00041a0c, 00000070, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 3ff00000, 00041a18, 01000100, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 0003ffff, 00041a18, 01000100, 00041a04, 0000ff09, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 0000000d, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 000000f9, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 000f0009, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a08, 00030101, 00041a0c, 80048000, 00041a14, 02062000, 

	(CmdDump) Bind    , 1024 bytes : 00041a04, 00098729, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a08, 00030101, 00041a0c, 80048000, 00041a14, 02062000, 00041a24, 00098729, 00041a30, 0000aae4, 00041a20, 01370500, 00041844, 00100000, 00041a38, 01000100, 00041a28, 00030101, 00041a2c, 80048000, 00041a34, 02062000, 00041a44, 00098729, 00041a50, 0000aae4, 00041a40, 01370500, 00041848, 00100000, 00041a58, 01000100, 00041a48, 00030101, 00041a4c, 80048000, 00041a54, 02062000, 00041a64, 00098729, 00041a70, 0000aae4, 00041a60, 01370500, 0004184c, 00100000, 00041a78, 01000100, 00041a68, 00030101, 00041a6c, 80048000, 00041a74, 02062000, 00041a84, 00098729, 00041a90, 0000aae4, 00041a80, 01370500, 00041850, 00100000, 00041a98, 01000100, 00041a88, 00030101, 00041a8c, 80048000, 00041a94, 02062000, 00041aa4, 00098729, 00041ab0, 0000aae4, 00041aa0, 01370500, 00041854, 00100000, 00041ab8, 01000100, 00041aa8, 00030101, 00041aac, 80048000, 00041ab4, 02062000, 00041ac4, 00098729, 00041ad0, 0000aae4, 00041ac0, 01370500, 00041858, 00100000, 00041ad8, 01000100, 00041ac8, 00030101, 00041acc, 80048000, 00041ad4, 02062000, 00041ae4, 00098729, 00041af0, 0000aae4, 00041ae0, 01370500, 0004185c, 00100000, 00041af8, 01000100, 00041ae8, 00030101, 00041aec, 80048000, 00041af4, 02062000, 00041b04, 00098729, 00041b10, 0000aae4, 00041b00, 01370500, 00041860, 00100000, 00041b18, 01000100, 00041b08, 00030101, 00041b0c, 80048000, 00041b14, 02062000, 00041b24, 00098729, 00041b30, 0000aae4, 00041b20, 01370500, 00041864, 00100000, 00041b38, 01000100, 00041b28, 00030101, 00041b2c, 80048000, 00041b34, 02062000, 00041b44, 00098729, 00041b50, 0000aae4, 00041b40, 01370500, 00041868, 00100000, 00041b58, 01000100, 00041b48, 00030101, 00041b4c, 80048000, 00041b54, 02062000, 00041b64, 00098729, 00041b70, 0000aae4, 00041b60, 01370500, 0004186c, 00100000, 00041b78, 01000100, 00041b68, 00030101, 00041b6c, 80048000, 00041b74, 02062000, 00041b84, 00098729, 00041b90, 0000aae4, 00041b80, 01370500, 00041870, 00100000, 00041b98, 01000100, 00041b88, 00030101, 00041b8c, 80048000, 00041b94, 02062000, 00041ba4, 00098729, 00041bb0, 0000aae4, 00041ba0, 01370500, 00041874, 00100000, 00041bb8, 01000100, 00041ba8, 00030101, 00041bac, 80048000, 00041bb4, 02062000, 00041bc4, 00098729, 00041bd0, 0000aae4, 00041bc0, 01370500, 00041878, 00100000, 00041bd8, 01000100, 00041bc8, 00030101, 00041bcc, 80048000, 00041bd4, 02062000, 00041be4, 00098729, 00041bf0, 0000aae4, 00041be0, 01370500, 0004187c, 00100000, 00041bf8, 01000100, 00041be8, 00030101, 00041bec, 80048000, 00041bf4, 02062000, 
	(CmdDump) RSX_Bind, 1024 bytes : 00041a04, 00098720, 00041a10, 0000aae4, 00041a00, 01370500, 00041840, 00100000, 00041a18, 01000100, 00041a08, 00030101, 00041a0c, 80048000, 00041a14, 02062000, 00041a24, 00098720, 00041a30, 0000aae4, 00041a20, 01370500, 00041860, 00100000, 00041a38, 01000100, 00041a28, 00030101, 00041a2c, 80048000, 00041a34, 02062000, 00041a44, 00098720, 00041a50, 0000aae4, 00041a40, 01370500, 00041880, 00100000, 00041a58, 01000100, 00041a48, 00030101, 00041a4c, 80048000, 00041a54, 02062000, 00041a64, 00098720, 00041a70, 0000aae4, 00041a60, 01370500, 000418a0, 00100000, 00041a78, 01000100, 00041a68, 00030101, 00041a6c, 80048000, 00041a74, 02062000, 00041a84, 00098720, 00041a90, 0000aae4, 00041a80, 01370500, 000418c0, 00100000, 00041a98, 01000100, 00041a88, 00030101, 00041a8c, 80048000, 00041a94, 02062000, 00041aa4, 00098720, 00041ab0, 0000aae4, 00041aa0, 01370500, 000418e0, 00100000, 00041ab8, 01000100, 00041aa8, 00030101, 00041aac, 80048000, 00041ab4, 02062000, 00041ac4, 00098720, 00041ad0, 0000aae4, 00041ac0, 01370500, 00041900, 00100000, 00041ad8, 01000100, 00041ac8, 00030101, 00041acc, 80048000, 00041ad4, 02062000, 00041ae4, 00098720, 00041af0, 0000aae4, 00041ae0, 01370500, 00041920, 00100000, 00041af8, 01000100, 00041ae8, 00030101, 00041aec, 80048000, 00041af4, 02062000, 00041b04, 00098720, 00041b10, 0000aae4, 00041b00, 01370500, 00041940, 00100000, 00041b18, 01000100, 00041b08, 00030101, 00041b0c, 80048000, 00041b14, 02062000, 00041b24, 00098720, 00041b30, 0000aae4, 00041b20, 01370500, 00041960, 00100000, 00041b38, 01000100, 00041b28, 00030101, 00041b2c, 80048000, 00041b34, 02062000, 00041b44, 00098720, 00041b50, 0000aae4, 00041b40, 01370500, 00041980, 00100000, 00041b58, 01000100, 00041b48, 00030101, 00041b4c, 80048000, 00041b54, 02062000, 00041b64, 00098720, 00041b70, 0000aae4, 00041b60, 01370500, 000419a0, 00100000, 00041b78, 01000100, 00041b68, 00030101, 00041b6c, 80048000, 00041b74, 02062000, 00041b84, 00098720, 00041b90, 0000aae4, 00041b80, 01370500, 000419c0, 00100000, 00041b98, 01000100, 00041b88, 00030101, 00041b8c, 80048000, 00041b94, 02062000, 00041ba4, 00098720, 00041bb0, 0000aae4, 00041ba0, 01370500, 000419e0, 00100000, 00041bb8, 01000100, 00041ba8, 00030101, 00041bac, 80048000, 00041bb4, 02062000, 00041bc4, 00098720, 00041bd0, 0000aae4, 00041bc0, 01370500, 00041a00, 00100000, 00041bd8, 01000100, 00041bc8, 00030101, 00041bcc, 80048000, 00041bd4, 02062000, 00041be4, 00098720, 00041bf0, 0000aae4, 00041be0, 01370500, 00041a20, 00100000, 00041bf8, 01000100, 00041be8, 00030101, 00041bec, 80048000, 00041bf4, 02062000, 


	(CmdDump) AlphaFunc, 20 bytes : 00080308, 00000207, 00000080, 00040304, 00000000, 
	(CmdDump) Scissor, 48 bytes : 000808c0, 0000ffff, 00000000, 000808c0, 00000000, 0000ffff, 000808c0, ffff0000, 00000000, 000808c0, 00000000, ffff0000, 

*/

enum
{
	RSXCMD_NOP =						0x00000000,
	RSXCMD_ATTRIBFORMAT0 =				0x00041740,
	RSXCMD_ATTRIBOFFSET0 =				0x00041680,
	RSXCMD_INVALIDATEVERTEXCACHE1 =		0x00041710,
	RSXCMD_INVALIDATEVERTEXCACHE2 =		0x00041714,

	RSXCMD_ATTRIBCMDSTRIDE =			4,


	RSXCMD_VPPARAM_SIZE =				4,
	RSXCMD_INVALIDATEVERTEXCACHE_SIZE =	8,
};

#define RSX_SAMPLERCMDOFFSET(_iSampler) ((_iSampler) << 5)

#define RSX_MAKEADDRMODE(_WrapS, _WrapT, _WrapR, _URemap, _DepthFunc, _sRGBGamma) \
	(((_WrapS) << 0) + ((_WrapT) << 8) + ((_WrapR) << 16) + ((_URemap) << 12) + ((_DepthFunc) << 28) + ((_sRGBGamma) << 20))

#define RSX_MAKEFILTERMODE(_Bias, _MinFilter, _MagFilter, _ConvFilter)	\
	((((_Bias) & 0x1fff) << 0) + ((_ConvFilter) << 13) + ((_MinFilter) << 16) + ((_MagFilter) << 24))

#define RSX_MAKETEXTURECONTROL(_MinLod, _MaxLod, _MaxAniso)	\
	(M_Bit(31) + ((_MaxAniso) << 4) + (((_MinLod) & 0xfff) << 19) + (((_MaxLod) & 0xfff) << 7))

#define RSX_MAKEDEPTHPITCH(_Depth, _Pitch) \
	((((_Depth) & 0x3ff) << 20) + (((_Pitch) & 0x3ffff) << 0))

// What is teh 9?
#define RSX_MAKEFORMAT(_Format, _CubeMap, _Dimension, _MipMap) \
	(0x00000009 + ((_Format) << 8) + (((_CubeMap) & 1) << 2) + (((_Dimension) & 0x0f) << 4) + (((_MipMap) & 0x0f) << 16))

class CRSXTexture
{
public:
	uint32 m_RSXFormat;
	uint32 m_RSXDepthPitch;
	uint32 m_RSXFilter;
	uint32 m_RSXControl;
	uint32 m_RSXAddrMode;
};

M_FORCEINLINE void RSX_Texture(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, const CellGcmTexture& _Texture)
{
	// Set format
	_pCmd[0] = 0x00041a04 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = RSX_MAKEFORMAT(_Texture.format, _Texture.cubemap, _Texture.dimension, _Texture.mipmap);

	// Set remap
	_pCmd[2] = 0x00041a10 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[3] = _Texture.remap;

	// Set address
	_pCmd[4] = 0x00041a00 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[5] = _Texture.offset;

	// Set depth, pitch
	_pCmd[6] = 0x00041840 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[7] = RSX_MAKEDEPTHPITCH(_Texture.depth, _Texture.pitch);

	// Set width & height
	_pCmd[8] = 0x00041a18 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[9] = (_Texture.width << 16) + _Texture.height;

	_pCmd += 10;
}

M_FORCEINLINE void RSX_TextureAddress(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint8 _WrapS, uint8 _WrapT, uint8 _WrapR, uint8 _URemap, uint8 _DepthFunc, uint8 _sRGBGamma)
{
	// WrapS, 4 bit
	// WrapT, 4 bit
	// WrapR, 4 bit
	// URemap, 1 bit
	// DepthFunc, 4 bit
	// sRGBGamma, 4 bit

	_pCmd[0] = 0x00041a08 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = RSX_MAKEADDRMODE(_WrapS, _WrapT, _WrapR, _URemap, _DepthFunc, _sRGBGamma);
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureAddress(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint32 _AddrMode)
{
	_pCmd[0] = 0x00041a08 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = _AddrMode;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureControl(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint32 _MinLod, uint32 _MaxLod, uint32 _MaxAniso)
{
	_pCmd[0] = 0x00041a0c + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = RSX_MAKETEXTURECONTROL(_MinLod, _MaxLod, _MaxAniso);
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureControl(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint32 _TextureControl)
{
	_pCmd[0] = 0x00041a0c + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = _TextureControl;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureControlDisable(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler)
{
	_pCmd[0] = 0x00041a0c + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = 0;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureFilter(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint16 _Bias, uint8 _MinFilter, uint8 _MagFilter, uint8 _ConvFilter)
{
	_pCmd[0] = 0x00041a14 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = RSX_MAKEFILTERMODE(_Bias, _MinFilter, _MagFilter, _ConvFilter);
	_pCmd += 2;
}

M_FORCEINLINE void RSX_TextureFilter(uint32 M_RESTRICT* & _pCmd, uint32 _iSampler, uint32 _FilterMode)
{
	_pCmd[0] = 0x00041a14 + RSX_SAMPLERCMDOFFSET(_iSampler);
	_pCmd[1] = _FilterMode;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_ColorMask(uint32 M_RESTRICT* & _pCmd, uint32 _Mask)
{
	_pCmd[0] = 0x00040324;
	_pCmd[1] = _Mask;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_ColorMaskMRT(uint32 M_RESTRICT* & _pCmd, uint32 _Mask)
{
	_pCmd[0] = 0x00040370;
	_pCmd[1] = _Mask;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_StencilFunc(uint32 M_RESTRICT* & _pCmd, uint32 _Func, uint32 _Ref, uint32 _Mask)
{
	_pCmd[0] = 0x000c0330;
	_pCmd[1] = _Func;
	_pCmd[2] = _Ref;
	_pCmd[3] = _Mask;
	_pCmd += 4;
}

M_FORCEINLINE void RSX_StencilOp(uint32 M_RESTRICT* & _pCmd, uint32 _FailOp, uint32 _DepthFailOp, uint32 _DepthPassOp)
{
	_pCmd[0] = 0x000c033c;
	_pCmd[1] = _FailOp;
	_pCmd[2] = _DepthFailOp;
	_pCmd[3] = _DepthPassOp;
	_pCmd += 4;
}

M_FORCEINLINE void RSX_StencilMask(uint32 M_RESTRICT* & _pCmd, uint32 _Mask)
{
	_pCmd[0] = 0x0004032c;
	_pCmd[1] = _Mask;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_BackStencilFunc(uint32 M_RESTRICT* & _pCmd, uint32 _Func, uint32 _Ref, uint32 _Mask)
{
	_pCmd[0] = 0x000c0350;
	_pCmd[1] = _Func;
	_pCmd[2] = _Ref;
	_pCmd[3] = _Mask;
	_pCmd += 4;
}

M_FORCEINLINE void RSX_BackStencilOp(uint32 M_RESTRICT* & _pCmd, uint32 _FailOp, uint32 _DepthFailOp, uint32 _DepthPassOp)
{
	_pCmd[0] = 0x000c035c;
	_pCmd[1] = _FailOp;
	_pCmd[2] = _DepthFailOp;
	_pCmd[3] = _DepthPassOp;
	_pCmd += 4;
}

M_FORCEINLINE void RSX_BackStencilMask(uint32 M_RESTRICT* & _pCmd, uint32 _Mask)
{
	_pCmd[0] = 0x0004034c;
	_pCmd[1] = _Mask;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_StencilTestEnable(uint32 M_RESTRICT* & _pCmd, uint32 _bEnable)
{
	_pCmd[0] = 0x00040328;
	_pCmd[1] = _bEnable;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_DepthTestEnable(uint32 M_RESTRICT* & _pCmd, uint32 _bEnable)
{
	_pCmd[0] = 0x00040a74;
	_pCmd[1] = _bEnable;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_DepthFunc(uint32 M_RESTRICT* & _pCmd, uint32 _Func)
{
	_pCmd[0] = 0x00040a6c;
	_pCmd[1] = _Func;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_BlendFunc(uint32 M_RESTRICT* & _pCmd, uint32 _SrcColorBlend, uint32 _DstColorBlend, uint32 _SrcAlphaBlend, uint32 _DstAlphaBlend)
{
	_pCmd[0] = 0x00080314;
	_pCmd[1] = _SrcColorBlend + (_SrcAlphaBlend << 16);
	_pCmd[2] = _DstColorBlend + (_DstAlphaBlend << 16);
	_pCmd += 3;
}

M_FORCEINLINE void RSX_AlphaFunc(uint32 M_RESTRICT* & _pCmd, uint32 _Func, uint32 _Ref)
{
	_pCmd[0] = 0x00080308;
	_pCmd[1] = _Func;
	_pCmd[2] = _Ref;
	_pCmd += 3;
}

M_FORCEINLINE void RSX_AlphaTestEnable(uint32 M_RESTRICT* & _pCmd, uint32 _bEnable)
{
	_pCmd[0] = 0x00040304;
	_pCmd[1] = _bEnable;
	_pCmd += 2;
}

M_FORCEINLINE void RSX_Scissor(uint32 M_RESTRICT* & _pCmd, const CScissorRect& _Scissor)
{
	_pCmd[0] = 0x000808c0;
	_pCmd[1] = (_Scissor.GetMinX() + ((_Scissor.GetMaxX()-_Scissor.GetMinX()) << 16)) & 0x0fff0fff;
	_pCmd[2] = (_Scissor.GetMinY() + ((_Scissor.GetMaxY()-_Scissor.GetMinY()) << 16)) & 0x0fff0fff;
	_pCmd += 3;
}

M_FORCEINLINE void RSX_ScissorDisable(uint32 M_RESTRICT* & _pCmd)
{
	_pCmd[0] = 0x000808c0;
	_pCmd[1] = 0x0fff0000;
	_pCmd[2] = 0x0fff0000;
	_pCmd += 3;
}


// NOTE: RSX_VPParam must keep the write pointer aligned. Therefore a NOP is inserted to pad the command to 16 bytes.
M_FORCEINLINE void RSX_VPParam(uint32 M_RESTRICT* & _pCmd, uint32 _iParamStart, uint32 _nParams)
{
	_pCmd[0] = 0x00000000;
	_pCmd[1] = 0x00041efc;						// Start index command
	_pCmd[2] = _iParamStart;
	_pCmd[3] = (_nParams << (16+4)) + 0x1f00;	// Data command
	_pCmd += 4;
}



M_FORCEINLINE void RSX_InvalidateVertexCache(uint32 M_RESTRICT* & _pCmd)
{
	_pCmd[0] = RSXCMD_INVALIDATEVERTEXCACHE1;
	_pCmd[1] = 0;
	_pCmd[2] = RSXCMD_INVALIDATEVERTEXCACHE2;
	_pCmd[3] = 0;
	_pCmd[4] = RSXCMD_INVALIDATEVERTEXCACHE2;
	_pCmd[5] = 0;
	_pCmd[6] = RSXCMD_INVALIDATEVERTEXCACHE2;
	_pCmd[7] = 0;
	_pCmd += 8;
}



#define RSX_MAKEATTRIBFORMAT(_Stride, _Components, _Format)	\
	(((_Stride) << 8) + ((_Components) << 4) + (_Format))

#define RSX_MAKEATTRIBFORMATDISABLE RSX_MAKEATTRIBFORMAT(0, 0, CELL_GCM_VERTEX_F)

#define RSX_SETATTRIBFORMAT(_pBuffer, _iAttrib, _Fmt) \
{	\
	(_pBuffer)[0] = RSXCMD_ATTRIBFORMAT0+((_iAttrib) * RSXCMD_ATTRIBCMDSTRIDE);	\
	(_pBuffer)[1] = _Fmt;	\
}

#define RSX_SETATTRIBFORMATDISABLE(_pBuffer, _iAttrib) \
{	\
	(_pBuffer)[0] = RSXCMD_ATTRIBFORMAT0+((_iAttrib) * RSXCMD_ATTRIBCMDSTRIDE);	\
	(_pBuffer)[1] = RSX_MAKEATTRIBFORMATDISABLE;	\
}

#define RSX_SETATTRIBOFFSET(_pBuffer, _iAttrib, _pArray) \
{	\
	(_pBuffer)[0] = RSXCMD_ATTRIBOFFSET0+((_iAttrib) * RSXCMD_ATTRIBCMDSTRIDE);	\
	(_pBuffer)[1] = _pArray;	\
}

#define RSX_SETATTRIBFORMAT_SIZE 2
#define RSX_SETATTRIBOFFSET_SIZE 2

#define RSX_SETINVALIDATEVERTEXCACHE(_pBuffer)	\
{ \
	(_pBuffer)[0] = RSXCMD_INVALIDATEVERTEXCACHE1; \
	(_pBuffer)[1] = 0x00000000; \
	(_pBuffer)[2] = RSXCMD_INVALIDATEVERTEXCACHE2; \
	(_pBuffer)[3] = 0x00000000; \
	(_pBuffer)[4] = RSXCMD_INVALIDATEVERTEXCACHE2; \
	(_pBuffer)[5] = 0x00000000; \
	(_pBuffer)[6] = RSXCMD_INVALIDATEVERTEXCACHE2; \
	(_pBuffer)[7] = 0x00000000; \
}


struct gcmData
{
	uint32 m_DontCare[7];
	uint32* m_pWrite, *m_pWriteMax;
};

struct nvgle
{
	uint32 dontCare[196];
	gcmData *data;
};

#ifdef GCM_COMMANDBUFFERWRITE

extern "C" nvgle g_nvgle;

M_FORCEINLINE uint32* RSX_AllocateCommand(uint32 _WordCount)
{
	//	M_ASSERT((_Bytes & 3) == 0, "!");
	uint32 wordcount = _WordCount;

	// Common case, there's enough space: note that there always has to be one word left in the command buffer, hence > and not >=)
	if (g_nvgle.data->m_pWriteMax - g_nvgle.data->m_pWrite > (int)wordcount)
	{
		uint32* pWrite = g_nvgle.data->m_pWrite;
		g_nvgle.data->m_pWrite += wordcount;
		return pWrite;
	}
	else
	{
		gcmSetNopCommand(wordcount);
		uint32* pWrite = g_nvgle.data->m_pWrite - wordcount;
		return pWrite;
	}
}

M_FORCEINLINE uint32* RSX_AllocateCommandAlign16(uint32 _WordCount)
{
	//	M_ASSERT((_Bytes & 3) == 0, "!");
	uint32 wordcount = _WordCount;

	if (g_nvgle.data->m_pWriteMax - g_nvgle.data->m_pWrite > (int)wordcount+3)
	{
		// Common case, there's enough space: note that there always has to be one word left in the command buffer, hence > and not >=)
		uint32* pWrite =  g_nvgle.data->m_pWrite;
		if (uint32(pWrite) & 0x0f)
		{
			int nNop = 0x4 - ((uint32(pWrite) & 0x0f) >> 2);
			for(int i = 0; i < nNop; i++)
				pWrite[i] = RSXCMD_NOP;
			pWrite += nNop;
		}
		g_nvgle.data->m_pWrite = pWrite + wordcount;
		M_ASSERT((uint32(pWrite) & 0x0f) == 0, "!");
		return pWrite;
	}
	else
	{
		gcmSetNopCommand(wordcount+3);
		uint32* pWrite = g_nvgle.data->m_pWrite - wordcount - 3;
		if (uint32(pWrite) & 0x0f)
		{
			int nNop = 0x4 - ((uint32(pWrite) & 0x0f) >> 2);
			for(int i = 0; i < nNop; i++)
				pWrite[i] = RSXCMD_NOP;
			pWrite += nNop;
		}
		g_nvgle.data->m_pWrite = pWrite + wordcount;
		M_ASSERT((uint32(pWrite) & 0x0f) == 0, "!");
		return pWrite;
	}
}

M_FORCEINLINE uint32* RSX_BeginCommand(uint32 _WordCount)
{
	//	M_ASSERT((_Bytes & 3) == 0, "!");
	uint32 wordcount = _WordCount;

	if (g_nvgle.data->m_pWriteMax - g_nvgle.data->m_pWrite > (int)wordcount)
	{
		// Common case, there's enough space: note that there always has to be one word left in the command buffer, hence > and not >=)
		uint32* pWrite = g_nvgle.data->m_pWrite;
		return pWrite;
	}
	else
	{
		gcmSetNopCommand(wordcount);
		uint32* pWrite = g_nvgle.data->m_pWrite - wordcount;
		g_nvgle.data->m_pWrite = pWrite;
		return pWrite;
	}
}

M_FORCEINLINE uint32* RSX_BeginCommandAlign16(uint32 _WordCount)
{
	//	M_ASSERT((_Bytes & 3) == 0, "!");
	uint32 wordcount = _WordCount;

	if (g_nvgle.data->m_pWriteMax - g_nvgle.data->m_pWrite > (int)wordcount+3)
	{
		// Common case, there's enough space: note that there always has to be one word left in the command buffer, hence > and not >=)
		uint32* pWrite =  g_nvgle.data->m_pWrite;
		if (uint32(pWrite) & 0x0f)
		{
			int nNop = 0x4 - ((uint32(pWrite) & 0x0f) >> 2);
			for(int i = 0; i < nNop; i++)
				pWrite[i] = RSXCMD_NOP;
			pWrite += nNop;
			g_nvgle.data->m_pWrite = pWrite;
		}
		M_ASSERT((uint32(pWrite) & 0x0f) == 0, "!");
		return pWrite;
	}
	else
	{
		// Need to let GCM make some space
		gcmSetNopCommand(wordcount+3);
		uint32* pWrite = g_nvgle.data->m_pWrite - wordcount - 3;
		if (uint32(pWrite) & 0x0f)
		{
			int nNop = 0x4 - ((uint32(pWrite) & 0x0f) >> 2);
			for(int i = 0; i < nNop; i++)
				pWrite[i] = RSXCMD_NOP;
			pWrite += nNop;
		}
		g_nvgle.data->m_pWrite = pWrite;
		M_ASSERT((uint32(pWrite) & 0x0f) == 0, "!");
		return pWrite;
	}
}

M_FORCEINLINE void RSX_EndCommand(uint32 _WordCount)
{
	//	M_ASSERT((_Bytes & 3) == 0, "!");
	uint32 wordcount = _WordCount;

	// Common case, there's enough space:
	// (note that there always has to be one word left in the command buffer, 	hence > and not >=)
	if (g_nvgle.data->m_pWriteMax - g_nvgle.data->m_pWrite > (int)wordcount)
	{
		uint32* pWrite = g_nvgle.data->m_pWrite;
		g_nvgle.data->m_pWrite += wordcount;
	}
	else
	{
		// If we arrive here it means that we're trying to commit more bytes than we reserved using gcmBeginCommand()
		M_ASSERT(0, "!");

		//		gcmSetNopCommand(wordcount);
		//		uint32* pWrite = g_nvgle.data->m_pWrite - wordcount;
		//		return pWrite;
	}
}

// -------------------------------------------------------------------
class CDumpScopeCommands
{
public:
	uint32* m_pBegin;
	const char* m_pMsg;

	CDumpScopeCommands(const char* _pMsg)
	{
		m_pBegin = RSX_BeginCommand(1024);	// This will make sure that the command buffer won't wrap unless we write more than 4k
		m_pMsg = _pMsg;
	}

	~CDumpScopeCommands()
	{
		uint32* pEnd = RSX_BeginCommand(0);
		uint32* pBegin = m_pBegin;
		if (pEnd < pBegin)
		{
			M_TRACEALWAYS("(CmdDump) %s, Command buffer wrapped\n", m_pMsg);
		}
		else
		{
			M_TRACEALWAYS("(CmdDump) %s, %d bytes : ", m_pMsg, (pEnd - pBegin) * 4 );
			while(pBegin != pEnd)
			{
				M_TRACEALWAYS("%.8x, ", *pBegin);
				pBegin++;
			}
			M_TRACEALWAYS("\n");
		}
	}
};

#ifdef _DEBUG
#define MTRACE_SCOPECOMMANDS(_pMsg) CDumpScopeCommands ScopeDump(_pMsg)
#else
#define MTRACE_SCOPECOMMANDS(_pMsg) 
#endif

#define MTRACE_SCOPECOMMANDS_ALWAYS(_pMsg) CDumpScopeCommands ScopeDump(_pMsg)

#else

#define MTRACE_SCOPECOMMANDS(_pMsg)
#define MTRACE_SCOPECOMMANDS_ALWAYS(_pMsg)

#endif

}	// Bloody namespace

#endif
