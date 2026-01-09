.syntax unified
.cpu cortex-m33
.fpu softvfp
.thumb

// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// Place RAW WAV bytes in a dedicated section without padding
// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

.section .audio, "a"        // "a" = alloc in flash (no RAM copy)
.global hihat_wav
.global hihat_wav_len

hihat_wav:
    .incbin "../Core/Audio/HiHat.wav"    // MUST be relative to project root
hihat_wav_end:

// length in bytes (32-bit)
hihat_wav_len:
    .word hihat_wav_end - hihat_wav
