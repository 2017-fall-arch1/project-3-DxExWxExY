	.arch msp430g2553
	.p2align 1,0
	.text  

	.data
	.global BIT0
BIT0:
	.byte 0x1

	.data
	.global BIT1
BIT1:
	.byte 0x2

	.data
	.global BIT2
BIT2:
	.byte 0x4

	.data
	.global BIT3
BIT3:
	.byte 0x8

	
	.text
	.global move
move:
	cmp.b &P2IN, &BIT0	;Checs is SW1 was pressed
	jnc SW2
	mov #pLU, r12		;parameters for movlayerdraw
	mov #pl, r13
	call #movLayerDraw
	mov #fieldFence, r13	;parameters for mladvance
	call mlAdvance
	ret
	
SW2:
	cmp.b &P2IN, &BIT1	;Checs is SW2 was pressed
	jnc SW3
	mov #pLD, r12		;parameters for movlayerdraw
	mov #pl, r13
	call #movLayerDraw
	mov #fieldFence, r13	;parameters for mladvance
	call mlAdvance
	ret

SW3:	cmp.b &P2IN, &BIT0	;Checs is SW3 was pressed
	jnc SW4
	mov #pRD, r12		;parameters for movlayerdraw
	mov #pl, r13
	call #movLayerDraw
	mov #fieldFence, r13	;parameters for mladvance
	call mlAdvance
	ret

SW4:
	cmp.b &P2IN, &BIT0	;Checs is SW4 was pressed
	jnc END
	mov #pLU, r12		;parameters for movlayerdraw
	mov #pl, r13
	call #movLayerDraw
	mov #fieldFence, r13	;parameters for mladvance
	call mlAdvance
END:
	ret
