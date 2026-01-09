.syntax unified
.cpu cortex-m33
.fpu softvfp
.thumb

// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// Place RAW WAV bytes in a dedicated section without padding
// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

.section .audio, "a"        // "a" = alloc in flash (no RAM copy)
.global snare2_wav
.global snare2_wav_len

snare2_wav:
    .incbin "../Core/Audio/Snare2.wav"    // MUST be relative to project root
snare2_wav_end:

// length in bytes (32-bit)
snare2_wav_len:
    .word snare2_wav_end - snare2_wav
