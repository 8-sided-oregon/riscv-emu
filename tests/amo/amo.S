.global _boot
.text

_boot:
    li a0, 0
    li x1, 0x1000
    li x5, 0x1004
    li x2, 10

    li x3, 8
    sw x3, (x1)

    lr.w x4, (x1)

    addi x4, x4, 10
    sc.w x5, x4, (x1)
    
    lw x6, (x1)
    ecall 

    li x4, 1
    amoadd.d x6, x4, (x1)
    ecall

    lw x6, (x1)
exit:
    li a0, 1
    ecall
