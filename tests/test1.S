.global _boot
.text

_boot:
	li t3, 0x10000
    li t4, 0xff0f0f0f

    ld t6, 0(t3)

    sw t4, 0(t3)
    lw t4, 0(t3)
    
    addi a0, x0, 1
    addi a1, x0, 0
    ecall

    addi t5, x0, 2
	addi t4, x0, 100
    addi t3, x0, 1
loop:
	mul t3, t3, t5
	blt t3, t4, loop
exit:
	addi x10, x0, 1
    add x11, x0, t3
    ecall

.data
variable:
	.word 0xdeadbeef
