        .arch i8086,jumps
        .code16
        .att_syntax prefix
        .text

        .global _start

_start:
	mov	%cs, %ax
	cmp	$0xe000, %ax
	je	1f

	// ----------------------------------------------------------
	// copy BIOS image (0xf0000 -> 0xe0000)
	// ----------------------------------------------------------
	cld			// make sure that movsb copies forward
	movw	$0xe000, %ax
	movw	%ax, %es	// destination segment
	movw	$0, %di		// destination position (with ES)

	mov	%cs, %ax
	movw	%ax, %ds	// source segment
	movw	$0, %si		// source position (with DS)

	movw	$0x8000, %cx	// # words

	rep	movsw
	// ----------------------------------------------------------

	// Jump to the copied program
	ljmp	$0xe000, $_start

1:
	mov	%cs, %ax
	mov	%ax, %ds
	mov	%ax, %ss
	mov	%ax, %es

	// Clear the BSS region
	cld			// make sure that stosb store forward
	mov	$0, %ax
	mov	$__sbss,  %di
	mov	$__lbss0, %cx
	rep stosb

	// copy the initial DX reg. value to a variable defined in bios.c
	mov	%dx, initial_dx_value

	mov	$0xffff, %sp

//------------------------------------
//      Enter the main function
//------------------------------------
	cli
	call	main

//------------------------------------
//      Reset the cpu
//------------------------------------
	hlt

//------------------------------------
//      Stop the system
//------------------------------------
	cli
_inf_loop:
	jmp _inf_loop
//------------------------------------





	.global copy_data
copy_data:
	push	%bp
	mov	%sp, %bp
	push	%es

	push	%ax
	push	%cx
	push	%si
	push	%di

	// ----------------------------------------------------------
	cld			// make sure that movsb copies forward
	movw	6(%bp), %ax	// destination
	movw	%ax, %es

	movw	4(%bp), %ax
	movw	%ax, %si	// source position (with DS)

	movw	8(%bp), %ax
	movw	%ax, %di	// destination position (with ES)

	movw	10(%bp), %ax	// bytes
	movw	%ax, %cx
	
	rep	movsb
	// ----------------------------------------------------------

	pop	%di
	pop	%si
	pop	%cx
	pop	%ax

	pop	%es
	pop	%bp
	ret


	.global copy_data_word
copy_data_word:
	push	%bp
	mov	%sp, %bp
	push	%es

	push	%ax
	push	%cx
	push	%si
	push	%di

	// ----------------------------------------------------------
	cld			// make sure that movsb copies forward
	movw	6(%bp), %ax	// destination
	movw	%ax, %es

	movw	4(%bp), %ax
	movw	%ax, %si	// source position (with DS)

	movw	8(%bp), %ax
	movw	%ax, %di	// destination position (with ES)

	movw	10(%bp), %ax	// bytes
	movw	%ax, %cx
	
	rep	movsw
	// ----------------------------------------------------------

	pop	%di
	pop	%si
	pop	%cx
	pop	%ax

	pop	%es
	pop	%bp
	ret


	.global fetch_data
fetch_data:
	push	%bp
	mov	%sp, %bp
	push	%ds
	push	%es

	push	%ax
	push	%cx
	push	%si
	push	%di


	// ----------------------------------------------------------
	cld			// make sure that movsb copies forward
	movw	4(%bp), %ax	// source
	movw	%ax, %ds
	movw	%cs, %ax	// destination
	movw	%ax, %es

	movw	6(%bp), %ax
	movw	%ax, %si	// source position (with DS)

	movw	8(%bp), %ax
	movw	%ax, %di	// destination position (with ES)

	movw	10(%bp), %ax	// bytes
	movw	%ax, %cx

	rep	movsb
	// ----------------------------------------------------------

	pop	%di
	pop	%si
	pop	%cx
	pop	%ax

	pop	%es
	pop	%ds
	pop	%bp
	ret

	.global fetch_data_word
fetch_data_word:
	push	%bp
	mov	%sp, %bp
	push	%ds
	push	%es

	push	%ax
	push	%cx
	push	%si
	push	%di


	// ----------------------------------------------------------
	cld			// make sure that movsb copies forward
	movw	4(%bp), %ax	// source
	movw	%ax, %ds
	movw	%cs, %ax	// destination
	movw	%ax, %es

	movw	6(%bp), %ax
	movw	%ax, %si	// source position (with DS)

	movw	8(%bp), %ax
	movw	%ax, %di	// destination position (with ES)

	movw	10(%bp), %ax	// bytes
	movw	%ax, %cx

	rep	movsw
	// ----------------------------------------------------------

	pop	%di
	pop	%si
	pop	%cx
	pop	%ax

	pop	%es
	pop	%ds
	pop	%bp
	ret


	.global int11_handler_asm
int11_handler_asm:
	// Bits
	//    0 : IPL diskette installed
	//    1 : math coprocessor
	// 5- 4 : initial video mode (10 : 80x25 Color)
	// 7- 6 : # of diskette drives, less 1  when bit0 = 1
	//    8 : 0 if DMA installed
	//11- 9 : number of serial ports
	//   12 : game adapter installed
	//15-14 : number of printer ports
	//   
	// See: https://stanislavs.org/helppc/int_11.html
	//

	movw	$((1<<8)+(0x01<<6)+(0x02<<4)+1), %ax	// co-processor is not available
	// movw    $((1<<8)+(0x01<<6)+(0x02<<4)+2+1), %ax
	iret


	.global int12_handler_asm
int12_handler_asm:
	mov	%cs, %ax

	shrw 	%ax
	shrw 	%ax
	shrw 	%ax
	shrw 	%ax
	shrw 	%ax
	shrw 	%ax

	cmpw	$640, %ax
	jg	1f
	iret
1:
	movw	$640, %ax
	iret


