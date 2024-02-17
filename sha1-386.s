/*
* void viasha(uchar *buf, uint count, *uint sum)
/*
TEXT viasha(SB), $0
	MOVL $0, AX
	MOVL buf+0(FP), SI
	MOVL count+4(FP), CX
	MOVL sum+8(FP), DI
	BYTE $0xF3; BYTE $0x0F; BYTE $0xA6; BYTE $0xC8
	RET
