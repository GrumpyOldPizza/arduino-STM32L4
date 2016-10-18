/*
 * Copyright (c) 2016 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

	.arch armv7-m

	.syntax unified

	.global	memcpy
	.global	memmove
	.global	memset
	.global	memzero

	.global	__aeabi_memcpy
	.global	__aeabi_memcpy4
	.global	__aeabi_memcpy8

	.global	__aeabi_memmove
	.global	__aeabi_memmove4
	.global	__aeabi_memmove8

	.global	__aeabi_memset
	.global	__aeabi_memset4
	.global	__aeabi_memset8

	.global	__aeabi_memclr
	.global	__aeabi_memclr4
	.global	__aeabi_memclr8
	
__aeabi_memcpy   = memcpy
__aeabi_memcpy4  = memcpy
__aeabi_memcpy8  = memcpy
__aeabi_memmove  = memmove
__aeabi_memmove4 = memmove
__aeabi_memmove8 = memmove
__aeabi_memclr   = memzero
__aeabi_memclr4  = memzero
__aeabi_memclr8  = memzero
__aeabi_memset4  = __aeabi_memset
__aeabi_memset8  = __aeabi_memset
	
        .section .text.memcpy, "ax"
	.thumb
	.thumb_func
	.type	memcpy, %function
	.align	3
	
	// R0  d
	// R1  s
	// R2  n
	// R3  scratch
	// R12 saved d
memcpy:
	mov	r12, r0

	/* Don't bother to be fancy about narrow copies. For n <= 16 simply
	 * copy byte by byte.
	 */
        cmp     r2, #16
        bls     9f

	/* Here there is a good sized chunk. First make sure the destination is 32 bit aligned,
	 * so that unaligned copies are only affecting the read path.
	 */
	lsls    r3, r0, #30
	bne     8f
	
	/* Now that the desintation is aligned, first copy in terms of 64 byte chunks.
	 * Offset d & s by -4, so that we can use the pre-increment instructions to update
 	 * the pointers.
	 */
0:	subs	r0, #4
	subs	r1, #4
	subs	r2, #64
	blo	2f

	.align	2
1:	ldr	r3, [r1, #4]
	str	r3, [r0, #4]
	ldr	r3, [r1, #8]
	str	r3, [r0, #8]
	ldr	r3, [r1, #12]
	str	r3, [r0, #12]
	ldr	r3, [r1, #16]
	str	r3, [r0, #16]
	ldr	r3, [r1, #20]
	str	r3, [r0, #20]
	ldr	r3, [r1, #24]
	str	r3, [r0, #24]
	ldr	r3, [r1, #28]
	str	r3, [r0, #28]
	ldr	r3, [r1, #32]
	str	r3, [r0, #32]
	ldr	r3, [r1, #36]
	str	r3, [r0, #36]
	ldr	r3, [r1, #40]
	str	r3, [r0, #40]
	ldr	r3, [r1, #44]
	str	r3, [r0, #44]
	ldr	r3, [r1, #48]
	str	r3, [r0, #48]
	ldr	r3, [r1, #52]
	str	r3, [r0, #52]
	ldr	r3, [r1, #56]
	str	r3, [r0, #56]
	ldr	r3, [r1, #60]
	str	r3, [r0, #60]
	ldr	r3, [r1, #64]!
	str	r3, [r0, #64]!
	subs	r2, #64
	bhs	1b

	/* Take care of the remaining 16 byte chunks.
	 */
2:	adds	r2, #64-16
	blo	4f
	
	.align	2
3:	ldr	r3, [r1, #4]
	str	r3, [r0, #4]
	ldr	r3, [r1, #8]
	str	r3, [r0, #8]
	ldr	r3, [r1, #12]
	str	r3, [r0, #12]
	ldr	r3, [r1, #16]!
	str	r3, [r0, #16]!
	subs	r2, #16
	bhs	3b
	
	/* Next copy the remaining 4 byte chunks.
	 */
4:	adds	r2, #16-4
	blo	6f

	.align	2
5:	ldr	r3, [r1, #4]!
	str	r3, [r0, #4]!
	subs	r2, #4
	bhs	5b

	/* At last take care of the trailing bytes
	*/
6:	adds	r2, r2, #4
	bne	7f
	mov	r0, r12
	bx	lr

7:	adds	r0, #4
	adds	r1, #4
	lsls    r3, r2, #31
	itt     cs
	ldrhcs  r3, [r1], #2
	strhcs  r3, [r0], #2
	itt     ne
	ldrbne  r3, [r1]
	strbne  r3, [r0]
	mov	r0, r12
	bx	lr

	/* Align the destination to be 32 bit aligned to avoid extra cycles on the stores.
	 */
8:	lsls    r3, r0, #31
	ittt    ne
	ldrbne  r3, [r1], #1
	strbne  r3, [r0], #1
	subne   r2, r2, #1
	ittt    cs
	ldrhcs  r3, [r1], #2
	strhcs  r3, [r0], #2
	subcs   r2, r2, #2
	b	0b
	
	/* Simple byte by byte copy loop for small batches.
	*/
9:	cbz	r2, 11f
10:	ldrb    r3, [r1], #1
	strb    r3, [r0], #1
        subs    r2, #1
        bne     10b
11:	mov	r0, r12
	bx	lr

	.size	memcpy,.-memcpy

	
        .section .text.memmove, "ax"
	.thumb
	.thumb_func
	.type	memmove, %function
	.align	3

	// R0  d
	// R1  s
	// R2  n
	// R3  scratch
	// R12 saved d
memmove:
	cmp     r1, r0
	bcs.w   memcpy
	adds    r3, r1, r2
	cmp     r0, r3
	bcs.w   memcpy
	mov	r12, r0

	/* Move d & s to the byte beyond the area to copy.
	*/
	add	r0, r2
	add	r1, r2

	/* Don't bother to be fancy about narrow copies. For n <= 16 simply
	 * copy byte by byte.
	 */
        cmp     r2, #16
        bls     9f

	/* Here there is a good sized chunk. First make sure the destination is 32 bit aligned,
	 * so that unaligned copies are only affecting the read path.
	 */
	lsls    r3, r0, #30
	bne     8f

	/* Now that the desintation is aligned, first copy in terms of 64 byte chunks.
	 */
0:	subs	r2, #64
	blo	2f

	.align	2
1:	subs	r1, #64
	subs	r0, #64
	ldr	r3, [r1, #60]
	str	r3, [r0, #60]
	ldr	r3, [r1, #56]
	str	r3, [r0, #56]
	ldr	r3, [r1, #52]
	str	r3, [r0, #52]
	ldr	r3, [r1, #48]
	str	r3, [r0, #48]
	ldr	r3, [r1, #44]
	str	r3, [r0, #44]
	ldr	r3, [r1, #40]
	str	r3, [r0, #40]
	ldr	r3, [r1, #36]
	str	r3, [r0, #36]
	ldr	r3, [r1, #32]
	str	r3, [r0, #32]
	ldr	r3, [r1, #28]
	str	r3, [r0, #28]
	ldr	r3, [r1, #24]
	str	r3, [r0, #24]
	ldr	r3, [r1, #20]
	str	r3, [r0, #20]
	ldr	r3, [r1, #16]
	str	r3, [r0, #16]
	ldr	r3, [r1, #12]
	str	r3, [r0, #12]
	ldr	r3, [r1, #8]
	str	r3, [r0, #8]
	ldr	r3, [r1, #4]
	str	r3, [r0, #4]
	ldr	r3, [r1, #0]
	str	r3, [r0, #0]
	subs	r2, #64
	bhs	1b

	/* Take care of the remaining 16 byte chunks.
	 */
2:	adds	r2, #64-16
	blo	4f

	.align	2
3:	subs	r1, #16
	subs	r0, #16
	ldr	r3, [r1, #12]
	str	r3, [r0, #12]
	ldr	r3, [r1, #8]
	str	r3, [r0, #8]
	ldr	r3, [r1, #4]
	str	r3, [r0, #4]
	ldr	r3, [r1, #0]
	str	r3, [r0, #0]
	subs	r2, #16
	bhs	3b

	/* Next copy the remaining 4 byte chunks.
	 */
4:	adds	r2, #16-4
	blo	6f
	
	.align	2
5:	ldr	r3, [r1, #-4]!
	str	r3, [r0, #-4]!
	subs	r2, #4
	bhs	5b

	/* At last take care of the trailing bytes
	*/
6:	adds	r2, r2, #4
	bne	7f
	mov	r0, r12
	bx	lr

7:	lsls    r3, r2, #31
	itt     cs
	ldrhcs  r3, [r1, #-2]!
	strhcs  r3, [r0, #-2]!
	itt     ne
	ldrbne  r3, [r1, #-1]
	strbne  r3, [r0, #-1]
	mov	r0, r12
	bx	lr

	/* Align the destination to be 32 bit aligned to avoid extra cycles on the stores.
	 */
8:	lsls    r3, r0, #31
	ittt    ne
	ldrbne  r3, [r1, #-1]!
	strbne  r3, [r0, #-1]!
	subne   r2, r2, #1
	ittt    cs
	ldrhcs  r3, [r1, #-2]!
	strhcs  r3, [r0, #-2]!
	subcs   r2, r2, #2
	b	0b
	
	/* Simple byte by byte copy loop for small batches.
	*/
9:	cbz	r2, 11f
10:     ldrb    r3, [r1, #-1]!
	strb    r3, [r0, #-1]!
        subs    r2, #1
        bne     10b
11:	mov	r0, r12
	bx	lr

	.size	memmove,.-memmove
	

        .section .text.memset, "ax"
	.thumb
	.thumb_func
	.type	memset, %function
	.align	3

	// R0  d
	// R1  c
	// R2  n
	// R3  scratch
	// R12 saved d
memset:
	mov	r12, r0

	/* Don't bother to be fancy about narrow stores. For n <= 16 simply
	 * store byte by byte.
	 */
        cmp     r2, #16
        bls     9f

	/* Here there is a good sized chunk. First make sure the destination is 32 bit aligned,
	 */
        uxtb    r1, r1
	orr     r1, r1, r1, lsl #8
	orr     r1, r1, r1, lsl #16
	lsls    r3, r0, #30
	bne     8f

	/* Now that the desintation is aligned, first store in terms of 64 byte chunks.
	 * Offset d by -4, so that we can use the pre-increment instructions to update
 	 * the pointers.
	 */
0:	subs	r0, #4
	subs	r2, #64
	blo	2f
	
	.align	2
1:	str	r1, [r0, #4]
	str	r1, [r0, #8]
	str	r1, [r0, #12]
	str	r1, [r0, #16]
	str	r1, [r0, #20]
	str	r1, [r0, #24]
	str	r1, [r0, #28]
	str	r1, [r0, #32]
	str	r1, [r0, #36]
	str	r1, [r0, #40]
	str	r1, [r0, #44]
	str	r1, [r0, #48]
	str	r1, [r0, #52]
	str	r1, [r0, #56]
	str	r1, [r0, #60]
	str	r1, [r0, #64]!
	subs	r2, #64
	bhs	1b

	/* Take care of the remaining 16 byte chunks.
	 */
2:	adds	r2, #64-16
	blo	4f

	.align	2
3:	str	r1, [r0, #4]
	str	r1, [r0, #8]
	str	r1, [r0, #12]
	str	r1, [r0, #16]!
	subs	r2, #16
	bhs	3b

	/* Next store the remaining 4 byte chunks.
	 */
4:	adds	r2, #16-4
	blo	6f

	.align	2
5:	str	r1, [r0, #4]!
	subs	r2, #4
	bhs	5b

	/* At last take care of the trailing bytes
	*/
6:	adds	r2, r2, #4
	bne	7f
	mov	r0, r12
	bx	lr

7:	adds	r0, #4
	lsls    r3, r2, #31
	it      cs
	strhcs  r1, [r0], #2
	it      ne
	strbne  r1, [r0]
	mov	r0, r12
	bx	lr

	/* Align the destination to be 32 bit aligned to avoid extra cycles on the stores.
	 */
8:	lsls    r3, r0, #31
	itt     ne
	strbne  r1, [r0], #1
	subne   r2, r2, #1
	itt     cs
	strhcs  r1, [r0], #2
	subcs   r2, r2, #2
	b	0b
	
9:	cbz	r2, 11f
10:	strb    r1, [r0], #1
        subs    r2, #1
        bne     10b
11:	mov	r0, r12
	bx	lr
	.size	memset,.-memset

	
        .section .text.memzero, "ax"
	.thumb
	.thumb_func
	.type	memzero, %function
memzero:
	mov	r2, r1
	mov	r1, #0
	b	memset
	.size	memzero,.-memzero

	
        .section .text.__aeabi_memset, "ax"
	.thumb
	.thumb_func
	.type	__aeabi_memset, %function
__aeabi_memset:
	mov	r3, r1
	mov	r1, r2
	mov	r2, r3
	b	memset
	.size	__aeabi_memset,.-__aeabi_memset
	
