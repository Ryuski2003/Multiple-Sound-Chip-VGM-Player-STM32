.section .rodata
.align 4
.global vgm_data_start
.global vgm_data_end
vgm_data_start:
    .incbin "../Core/Audio/Ryu.vgm"
vgm_data_end:
