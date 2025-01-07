
#include <machine/regdef.h>


/*
#define zero	$0
#define v0	$2
#define v1	$3
#define a0	$4
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24
#define t9	$25
#define k0	$26
#define k1	$27
#define gp	$gp
#define sp	$sp
#define s8	$30
#define ra	$31
#define pc	$pc
*/

	.set	noreorder
//	.set	nomips16


	.global CMat4Dfp32_ASM_Multiply4x4
	.global CMat4Dfp32_ASM_Multiply3x3

	.global CVec3Dfp32_ASM_Mul_Mat4D
	.global CVec3Dfp32_ASM_CrossProduct

	.global CMat4Dfp32_ASM_InverseOrthogonal
	.global CMat4Dfp32_ASM_Inverse
	.global CVec3Dfp32Normalize
	
	.global	FooBreak

	.section .text                                                    



// a0 = Input1 ( Matrix pointer )
// a1 = Input2 ( Matrix pointer )
// a2 = Output ( Matrix pointer )

CMat4Dfp32_ASM_Multiply3x3:
    mtsab			a0,	0
    lq				t0,	0x00(a0)
    lq				t1,	0x10(a0)
    lq				t2,	0x20(a0)
    lq				t3,	0x30(a0)
    lq				t4,	0x40(a0)
    qfsrv			t0,	t1,	t0
    qfsrv			t1,	t2,	t1
    qfsrv			t2,	t3,	t2
    qfsrv			t3,	t4,	t3
    qmtc2			t0,	vf04
    qmtc2			t1,	vf05
    qmtc2			t2,	vf06
    qmtc2			t3,	vf07

    mtsab		    a1,	0
    lq				t0,	0x00(a1)
    lq				t1,	0x10(a1)
    lq				t2,	0x20(a1)
    lq				t3,	0x30(a1)
    lq				t4,	0x40(a1)
    qfsrv			t0,	t1,	t0
    qfsrv			t1,	t2,	t1
    qfsrv			t2,	t3,	t2
    qfsrv			t3,	t4,	t3
    qmtc2			t0,	vf08
    qmtc2			t1,	vf09
    qmtc2			t2,	vf10
    qmtc2			t3,	vf11


	vmulax.xyz		ACC, vf08,vf04
	vmadday.xyz		ACC, vf09,vf04
	vmaddz.xyz		vf04, vf10,vf04

	vmulax.xyz		ACC, vf08,vf05
	vmadday.xyz		ACC, vf09,vf05
	vmaddz.xyz		vf05, vf10,vf05

	vmulax.xyz		ACC, vf08,vf06
	vmadday.xyz		ACC, vf09,vf06
	vmaddz.xyz		vf06, vf10,vf06

	
	andi			t0, a2,15
	beq				t0, zero, noshift3x3

	mtsab			a2,0
	lq				t0, 0x00(a2)
	lq				t5, 0x40(a2)
	qfsrv			t0, t0, t0
	qfsrv			t5, t5, t5

	nop
	nop
	
	subi			t1, a2, 1
	mtsab			t1,	65535

	qmfc2			t1, vf04
	qmfc2			t2, vf05
	qmfc2			t3, vf06
	qmfc2			t4, vf07

	qfsrv			t0, t1, t0
	qfsrv			t1, t2, t1
	qfsrv			t2, t3, t2
	qfsrv			t3, t4, t3
	qfsrv			t4, t5, t4
	
	sq				t0,0x00(a2)
	sq				t1,0x10(a2)
	sq				t2,0x20(a2)
	sq				t3,0x30(a2)
	sq				t4,0x40(a2)

	j				ra
	nop

noshift3x3:
	sqc2			vf04,0x00(a2)
	sqc2			vf05,0x10(a2)
	sqc2			vf06,0x20(a2)
	sqc2			vf07,0x30(a2)

	j				ra
	nop
	


// Convert this to inline assembler later!

CMat4Dfp32_ASM_Multiply4x4:
    mtsab			a0,	0
    lq				t0,	0x00(a0)
    lq				t1,	0x10(a0)
    lq				t2,	0x20(a0)
    lq				t3,	0x30(a0)
    lq				t4,	0x40(a0)
    qfsrv			t0,	t1,	t0
    qfsrv			t1,	t2,	t1
    qfsrv			t2,	t3,	t2
    qfsrv			t3,	t4,	t3
    qmtc2			t0,	vf04
    qmtc2			t1,	vf05
    qmtc2			t2,	vf06
    qmtc2			t3,	vf07

    mtsab		    a1,	0
    lq				t0,	0x00(a1)
    lq				t1,	0x10(a1)
    lq				t2,	0x20(a1)
    lq				t3,	0x30(a1)
    lq				t4,	0x40(a1)
    qfsrv			t0,	t1,	t0
    qfsrv			t1,	t2,	t1
    qfsrv			t2,	t3,	t2
    qfsrv			t3,	t4,	t3
    qmtc2			t0,	vf08
    qmtc2			t1,	vf09
    qmtc2			t2,	vf10
    qmtc2			t3,	vf11


	vmulax.xyzw     ACC, vf08,vf04
	vmadday.xyzw    ACC, vf09,vf04
	vmaddaz.xyzw    ACC, vf10,vf04
	vmaddw.xyzw     vf04, vf11,vf04

	vmulax.xyzw     ACC, vf08,vf05
	vmadday.xyzw    ACC, vf09,vf05
	vmaddaz.xyzw    ACC, vf10,vf05
	vmaddw.xyzw     vf05, vf11,vf05

	vmulax.xyzw     ACC, vf08,vf06
	vmadday.xyzw    ACC, vf09,vf06
	vmaddaz.xyzw    ACC, vf10,vf06
	vmaddw.xyzw     vf06, vf11,vf06

	vmulax.xyzw     ACC, vf08,vf07
	vmadday.xyzw    ACC, vf09,vf07
	vmaddaz.xyzw    ACC, vf10,vf07
	vmaddw.xyzw     vf07, vf11,vf07

	andi			t0, a2,15
	beq				t0, zero, noshift4x4

	mtsab			a2,0
	lq				t0, 0x00(a2)
	lq				t5, 0x40(a2)
	qfsrv			t0, t0, t0
	qfsrv			t5, t5, t5

	nop
	nop
	
	subi			t1, a2, 1
	mtsab			t1,	65535

	qmfc2			t1, vf04
	qmfc2			t2, vf05
	qmfc2			t3, vf06
	qmfc2			t4, vf07

	qfsrv			t0, t1, t0
	qfsrv			t1, t2, t1
	qfsrv			t2, t3, t2
	qfsrv			t3, t4, t3
	qfsrv			t4, t5, t4
	
	sq				t0,0x00(a2)
	sq				t1,0x10(a2)
	sq				t2,0x20(a2)
	sq				t3,0x30(a2)
	sq				t4,0x40(a2)

	j				ra
	nop

noshift4x4:
	sqc2			vf04,0x00(a2)
	sqc2			vf05,0x10(a2)
	sqc2			vf06,0x20(a2)
	sqc2			vf07,0x30(a2)

	j				ra
	nop


// Vector * Matrix
// a0 = In / Out	( Vector pointer )
// a1 = In 			( Matrix pointer )

CVec3Dfp32_ASM_Mul_Mat4D:

    lq				t0,	0x00(a0)			// Load Vector (t0)
    mtsab			a0,	0					// shift count to align vector
    lq				t1, 0x10(a0)			// Load Vector
    qfsrv			t0,	t1,	t0				// Align vector
    
    lq				t2,	0x00(a1)			// Load Matrix (t1-t4)
    lq				t3,	0x10(a1)			// Load Matrix
    lq				t4,	0x20(a1)			// Load Matrix
    mtsab			a1,	0					// shift count to align matrix
    lq				t5,	0x30(a1)			// Load Matrix
    qfsrv			t1,	t3,	t2				// Align matrix
    lq				t6,	0x40(a1)			// Load Matrix
    qfsrv			t2,	t4,	t3				// Align matrix
    qfsrv			t3,	t5,	t4				// Align matrix
    qfsrv			t4,	t6,	t5				// Align matrix

    qmtc2			t0,	vf04				// Move vector to COP2 VF04
    qmtc2			t1,	vf05				// Move matrix to COP2 VF05-VF08
    qmtc2			t2,	vf06
    qmtc2			t3,	vf07
    qmtc2			t4,	vf08

	vmulax.xyz		ACC, vf05, vf04			// VF04 =  vx * m_row0
	vmadday.xyz		ACC, vf06, vf04			// 		+ vy * m_row1
	vmaddaz.xyz		ACC, vf07, vf04			// 		+ vz * m_row2
	vmaddw.xyz		vf04,vf08, vf00			// 		+  1 * m_row3

	qmfc2      		t2, vf4             	// Get result to t2
	sw         		t2, 0x00(a0)        	// Store x
	prot3w     		t1, t2              	// Rotate y into LSW of t1
	sw         		t1, 0x04(a0)        	// Store y
	prot3w     		t2, t1              	// Rotate z into LSW of t2
	sw         		t2, 0x08(a0)        	// Store z
	
	j				ra
	nop

// a0 = in (matrix pointer)
// a1 = out (matrix pointer)
CMat4Dfp32_ASM_InverseOrthogonal:

	lq				t0,	0x00(a0)
	mtsab			a0,	0
	lq				t1,	0x10(a0)
	qfsrv			t0,	t1,	t0
	lq				t2,	0x20(a0)
	qfsrv			t1,	t2,	t1
	lq				t3,	0x30(a0)
	qfsrv			t2,	t3,	t2
	lq				t4,	0x40(a0)
	qfsrv			t3,	t4,	t3
	qmtc2			t0,	vf01
	qmtc2			t1,	vf02
	qmtc2			t2,	vf03
	qmtc2			t3,	vf04

	vmr32.w			vf01w, vf00					
	vmr32.w			vf02w, vf00
	vmr32.w			vf03w, vf00
	vmove.w			vf04w, vf00w

												// REG		x		y		z		w
	vaddx.y 		vf05y, vf00y, vf02x			// VF05: 	0		m21		0		0
	vmove.zw 		vf07zw, vf03zw              // VF07: 	0		0		m33		m34
	vaddx.z 		vf05z, vf00z, vf03x			// VF05: 	0		m21		m31		0
	vmr32.x 		vf06x, vf01					// VF06: 	m12		0		0		0
	vaddy.z 		vf06z, vf00z, vf03y			// VF06: 	m12		0		m32		0
	vmove.xw 		vf05xw, vf01xw				// VF05: 	m11		m21		m31		m14
	vaddz.x 		vf07x, vf00x, vf01z			// VF07: 	m13		0		m33		m34
	vmove.yw 		vf06yw, vf02yw				// VF06: 	m12		m22		m32		m24
	vnop
	vmr32.y 		vf07y, vf02					// VF07: 	m13		m23		m33		m34
	vadda 			ACC, vf00, vf00				// Clear ACC
	vmove.w 		vf08w, vf04w                // VF08:	0		0		0		m44
	vmsubax 		ACC, vf05, vf04x
	vmsubay 		ACC, vf06, vf04y
	vmsubz.xyz 		vf08, vf07, vf04z			// VF08:	-T*R1	-T*R2	-T*R3	m44	

	andi			t0, a1, 15
	bne				t0, zero, unaligned
	nop

	// adress aligned, store as is	
	sqc2 			vf05, 0x00(a1)
	sqc2 			vf06, 0x10(a1)
	sqc2 			vf07, 0x20(a1)
	sqc2 			vf08, 0x30(a1)

	j				ra
	nop

unaligned:

	// adress not aligned, fuck around
	mtsab			a1, 0
	lq				t0, 0x00(a1)
	qfsrv			t0, t0, t0
	lq				t5, 0x40(a1)
	qfsrv			t5, t5, t5

	nop
	nop
	
	subi			t1, a1, 1
	mtsab			t1,	65535

	qmfc2			t1, vf05
	qmfc2			t2, vf06
	qmfc2			t3, vf07
	qmfc2			t4, vf08

	qfsrv			t0, t1, t0
	qfsrv			t1, t2, t1
	qfsrv			t2, t3, t2
	qfsrv			t3, t4, t3
	qfsrv			t4, t5, t4
	
	sq				t0,0x00(a1)
	sq				t1,0x10(a1)
	sq				t2,0x20(a1)
	sq				t3,0x30(a1)
	sq				t4,0x40(a1)

	j				ra
	nop


// a0 -- in/out (vector pointer)
CVec3Dfp32Normalize:

    lq			$2,	0x00(a0)			//# Load/align vec
    lq			$3, 0x10(a0)
    mtsab		a0,	0
    qfsrv		$2,	$3,	$2

    qmtc2		$2,	vf01

	vmul.xyz	vf02, vf01, vf01
	vaddy.x 	vf02, vf02, vf02
	vaddz.x 	vf02, vf02, vf02
	
	vrsqrt 		Q, vf00w, vf02x
	vwaitq
	vmulq.xyz 	vf02, vf01, Q

	qmfc2      	$3, vf02             	//# Get result to t2
	sw         	$3, 0x00(a0)        	//# Store x
	prot3w     	$2, $3              	//# Rotate y into LSW of t1
	sw         	$2, 0x04(a0)        	//# Store y
	prot3w     	$3, $2              	//# Rotate z into LSW of t2
	sw         	$3, 0x08(a0)        	//# Store z

	j			ra
	

FooBreak:
	break
	nop
	j			ra
	nop
	
