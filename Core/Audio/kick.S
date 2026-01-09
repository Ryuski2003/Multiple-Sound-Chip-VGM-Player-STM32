.syntax unified
.cpu cortex-m33
.fpu softvfp
.thumb

// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// Place RAW WAV bytes in a dedicated section without padding
// ▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

.section .audio, "a"        // "a" = alloc in flash (no RAM copy)
.global kick_wav
.global kick_wav_len

kick_wav:
    .incbin "../Core/Audio/Kick.wav"    // MUST be relative to project root
kick_wav_end:

// length in bytes (32-bit)
kick_wav_len:
    .word kick_wav_end - kick_wav
