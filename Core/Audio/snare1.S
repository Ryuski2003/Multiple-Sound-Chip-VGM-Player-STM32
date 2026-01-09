.syntax unified
.cpu cortex-m33
.fpu softvfp
.thumb

// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// Place RAW WAV bytes in a dedicated section without padding
// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

.section .audio, "a"        // "a" = alloc in flash (no RAM copy)
.global snare1_wav
.global snare1_wav_len

snare1_wav:
    .incbin "../Core/Audio/Snare1.wav"    // MUST be relative to project root
snare1_wav_end:

// length in bytes (32-bit)
snare1_wav_len:
    .word snare1_wav_end - snare1_wav
