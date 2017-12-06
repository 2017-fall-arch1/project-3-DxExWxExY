	.arch msp430g2553

	.data
	.global BIT0
BIT0:	.byte 0x01

	.data
	.global BIT1
BIT1:	.byte 0x02

	.data
	.global BIT2
BIT2:	.byte 0x04

	.data
	.global BIT3
BIT3:	.byte 0x08

	
	.text
	.global move
move:
	bit.b &BIT0, &P2IN	;Checs is SW1 was pressed
	jnz SW2
	call #lup
	;; mov #pLU, r12		;parameters for movlayerdraw
	;; mov #pl, r13
	;; call #movLayerDraw
	;; mov #fieldFence, r13	;parameters for mladvance
	;; call mlAdvance
	
SW2:
	bit.b &BIT1, &P2IN	;Checs is SW2 was pressed
	jnz SW3
	call #ldw
	;; mov &pLD, r12		;parameters for movlayerdraw
	;; mov &pl, r13
	;; call #movLayerDraw
	;; mov &fieldFence, r13	;parameters for mladvance
	;; call mlAdvance

SW3:	bit.b &BIT2, &P2IN	;Checs is SW3 was pressed
	jnz SW4
	call #rup
	;; mov #pRD, r12		;parameters for movlayerdraw
	;; mov #pl, r13
	;; call #movLayerDraw
	;; mov #fieldFence, r13	;parameters for mladvance
	;; call mlAdvance

SW4:
	bit.b &BIT3, &P2IN	;Checs is SW4 was pressed
	jnz RST
	call #rdw
	;; mov #pLU, r12		;parameters for movlayerdraw
	;; mov #pl, r13
	;; call #movLayerDraw
	;; mov #fieldFence, r13	;parameters for mladvance
	;; call mlAdvance
RST:
	bit.b &BIT0, &P2IN	;Checs is SW1 was pressed
	jnz END
	bit.b &BIT2, &P2IN	;Checs is SW3 was pressed
	jnz END
	bit.b &BIT3, &P2IN	;Checs is SW4 was pressed
	jnz END
	mov.b #0, &WDTCTL	;reset
END:	
	ret

