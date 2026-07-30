# SN76489_STM32

*(Türkçe aşağıda / English below)*

---

## Türkçe

STM32H5 tabanlı, gerçek ses çipleri süren bir donanımsal VGM/VGZ chiptune çalar. microSD karttaki `.vgm`/`.vgz` dosyalarını okuyup **SN76489** (PSG), **YM2151** (OPM/FM) ve **MSM6295** (OKI ADPCM) çiplerine gerçek zamanlı register yazarak orijinal donanımla ses üretir; SPI TFT ekranda kendi arayüzü, rotary encoder ve butonlarla kontrol edilir.

### Donanım

- **MCU:** STM32H563ZIT6 (Cortex-M33, LQFP144), STM32CubeIDE + STM32CubeMX (`SN76489_STM32.ioc`) ile yapılandırılmış, TrustZone devre dışı.
- **Ses çipleri:**
  - **SN76489** (gerçek çip) — 8-bit paralel veri/CE/WE hatlarıyla (PD0-PD7, PC5/PC6) sürülüyor, kendi analog ses çıkışını üretiyor.
  - **YM2151** (gerçek çip) — 8-bit paralel arayüz (PF0-PF7 veri, PG9-PG13: IC/A0/WR/RD/CS), kendi analog ses çıkışını üretiyor.
  - **MSM6295** — gerçek çip değil; VGM verisindeki ADPCM ROM içeriği RAM'e yüklenip **yazılımda** (`msm6295.c`) çözülüyor (emülasyon), çıktısı dahili DAC'tan veriliyor.
  - Master clock, SN76489 ve YM2151 için **SI5351** (I2C1, PB7/PB8) üzerinden üretiliyor (MSM6295 yazılım olduğundan clock gerektirmiyor).
- **Ses çıkışı / mixing:** MSM6295 simülasyonu dahili DAC'tan (DAC1 CH1, TIM4 tetiklemeli örnekleme) veriliyor; SN76489, YM2151 ve DAC'tan gelen bu üç analog sinyal **TL072** op-amp ile kurulmuş bir **Summing Amplifier** (toplama amplifikatörü) devresinde mikslenip tek ses çıkışında birleştiriliyor.
- **Ekran:** ILI9341 240x320 TFT, SPI1 üzerinden (elle yazılmış sürücü, `ILI9341.c`), portrait/BGR modda.
- **Depolama:** microSD kart, SPI2 üzerinden, salt-okunur FAT32 (`fat32.c`, `sd_spi.c`).
- **Girdi:**
  - Rotary encoder (TIM3, X4 quadrature) — Now Playing modunda çalma hızını (0.5x-2.0x, pitch değişmeden, TIM2 periyodu ölçeklenerek), File List modunda sayfa gezintisini kontrol eder.
  - Butonlar (EXTI): SPEED_RST (hız sıfırlama), LOOP_SW/SHUFFLE_SW (kalıcı açık/kapalı switchler, pollanır), liste modu giriş/çıkış ve yukarı/aşağı navigasyon (basılı tutmada typematic tekrar).
<img width="2048" height="1439" alt="WhatsApp Image 2026-07-30 at 22 44 18" src="https://github.com/user-attachments/assets/d6af620e-1ddc-4578-b8b8-9aaa9d6e9e01" />

### Firmware yapısı (`Core/Src`)

| Dosya | Görev |
|---|---|
| `main.c` | Ana döngü: encoder/buton okuma, şarkı değiştirme, pause/resume, shuffle/loop durumu, UI güncelleme |
| `vgm.c` / `vgm.h` | VGM komut yorumlayıcı (SN76489/YM2151/MSM6295 yazmaları, wait komutları, header parse) |
| `vgz.c` / `puff.c` | `.vgz` (gzip) dosyalarını `.vgm`'e aç (deflate inflate) |
| `SN76489.c`, `YM2151.c`, `msm6295.c` | Çip-özel register/GPIO sürücü katmanları |
| `SI5351.c` | I2C clock generator - SN76489/YM2151 için master clock üretimi |
| `ILI9341.c` | SPI TFT sürücüsü (piksel/dikdörtgen/5x7 font metin çizimi) |
| `ui.c` / `ui.h` | Now Playing / File List ekranlarını yöneten el yapımı UI durum makinesi |
| `fat32.c`, `sd_spi.c` | Salt-okunur FAT32 + SPI SD kart erişimi |
| `shuffle.c` | "Bag" tabanlı shuffle algoritması (tüm şarkıları tekrarsız gezip yeniden karıştırır) |
| `storage.c` | Son çalınan şarkı indeksini dahili flash'a (Bank2 son sektör) kaydeder/okur |

### Özellikler

- SD karttan `.vgm` ve gzip'li `.vgz` dosya oynatma.
- Pitch değişmeden 0.5x-2.0x çalma hızı (encoder ile, TIM2 periyot ölçekleme).
- Loop ve shuffle açık/kapalı switchleri (shuffle "bag" algoritmasıyla tüm kütüphaneyi tekrarsız dolaşır).
- Dosya listesi tarama ekranı, sayfa bazlı hızlı gezinme ve basılı-tutma tekrarı.
- Son çalınan şarkı indeksi güç kesintisinden sonra da hatırlanıyor (dahili flash).

### Durum / Yol Haritası

- Arayüz şu an tamamen elle yazılmış (`ui.c` + `ILI9341.c`), **TouchGFX entegre değil**.
- STM32H563'te LTDC olmadığından, TouchGFX eklenirse "MCU without display controller (SPI)" modu ve mevcut ILI9341 SPI sürücüsüne bağlanan özel bir framebuffer flush katmanı gerekecek — bu entegrasyon planlanıyor, henüz projede yok.
<img width="1428" height="2048" alt="WhatsApp Image 2026-07-30 at 22 44 18 (1)" src="https://github.com/user-attachments/assets/4557284e-bdae-4384-bb6c-56f83b79f610" />

### Örnek VGM dosyaları (repo kökünde)

`Ryu.vgm`, `Vortex.vgm` ve ilgili `.txt`/ses dosyaları test/referans amaçlıdır, firmware derlemesine dahil değildir.

---

## English

An STM32H5-based hardware VGM/VGZ chiptune player that drives real sound chips. It reads `.vgm`/`.vgz` files from a microSD card and writes register data in real time to genuine **SN76489** (PSG), **YM2151** (OPM/FM), and **MSM6295** (OKI ADPCM) chips, producing audio on the original hardware; controlled via a custom UI on an SPI TFT screen, a rotary encoder, and buttons.

### Hardware

- **MCU:** STM32H563ZIT6 (Cortex-M33, LQFP144), configured with STM32CubeIDE + STM32CubeMX (`SN76489_STM32.ioc`), TrustZone disabled.
- **Sound chips:**
  - **SN76489** (real chip) — driven via an 8-bit parallel data bus and CE/WE lines (PD0-PD7, PC5/PC6), produces its own analog audio output.
  - **YM2151** (real chip) — 8-bit parallel interface (PF0-PF7 data, PG9-PG13: IC/A0/WR/RD/CS), produces its own analog audio output.
  - **MSM6295** — not a real chip; ADPCM ROM data from the VGM file is loaded into RAM and decoded **in software** (`msm6295.c`, emulation), output through the internal DAC.
  - Master clock for SN76489 and YM2151 is generated via an **SI5351** (I2C1, PB7/PB8); the MSM6295 emulation runs in software and needs no clock.
- **Audio output / mixing:** The MSM6295 emulation is output via the internal DAC (DAC1 CH1, sampled via TIM4 trigger); the three analog signals from SN76489, YM2151, and the DAC are mixed into a single output by a **TL072** op-amp **summing amplifier** circuit.
- **Display:** ILI9341 240x320 TFT over SPI1 (hand-written driver, `ILI9341.c`), portrait/BGR mode.
- **Storage:** microSD card over SPI2, read-only FAT32 (`fat32.c`, `sd_spi.c`).
- **Input:**
  - Rotary encoder (TIM3, X4 quadrature) — controls playback speed (0.5x-2.0x without pitch shift, via TIM2 period scaling) in Now Playing mode, and page navigation in File List mode.
  - Buttons (EXTI): SPEED_RST (reset speed), LOOP_SW/SHUFFLE_SW (persistent on/off switches, polled), list mode enter/exit, and up/down navigation (with typematic repeat on hold).
<img width="2048" height="1439" alt="WhatsApp Image 2026-07-30 at 22 44 18" src="https://github.com/user-attachments/assets/98b6667b-1e7d-4ffd-a9fa-f2a7c2174fb6" />

### Firmware layout (`Core/Src`)

| File | Role |
|---|---|
| `main.c` | Main loop: encoder/button handling, song switching, pause/resume, shuffle/loop state, UI updates |
| `vgm.c` / `vgm.h` | VGM command interpreter (SN76489/YM2151/MSM6295 writes, wait commands, header parsing) |
| `vgz.c` / `puff.c` | Decompress `.vgz` (gzip) files into `.vgm` (deflate inflate) |
| `SN76489.c`, `YM2151.c`, `msm6295.c` | Chip-specific register/GPIO driver layers |
| `SI5351.c` | I2C clock generator - produces the master clock for SN76489/YM2151 |
| `ILI9341.c` | SPI TFT driver (pixel/rectangle/5x7-font text drawing) |
| `ui.c` / `ui.h` | Hand-rolled UI state machine driving the Now Playing / File List screens |
| `fat32.c`, `sd_spi.c` | Read-only FAT32 + SPI SD card access |
| `shuffle.c` | "Bag"-based shuffle algorithm (visits all songs without repeats, then reshuffles) |
| `storage.c` | Saves/loads the last-played song index to/from internal flash (last sector of Bank2) |

### Features

- Plays `.vgm` and gzip-compressed `.vgz` files from an SD card.
- 0.5x-2.0x playback speed without pitch shift (via encoder, TIM2 period scaling).
- Loop and shuffle on/off switches (shuffle uses a "bag" algorithm to visit the whole library without repeats).
- File browsing screen with page-based fast navigation and hold-to-repeat.
- Last-played song index is remembered across power cycles (internal flash).

### Status / Roadmap

- The UI is currently entirely hand-written (`ui.c` + `ILI9341.c`) — **TouchGFX is not integrated**.
- Since the STM32H563 has no LTDC, adding TouchGFX would require the "MCU without display controller (SPI)" mode plus a custom framebuffer flush layer hooked into the existing ILI9341 SPI driver — this integration is planned but not yet present in the project.
<img width="1428" height="2048" alt="WhatsApp Image 2026-07-30 at 22 44 18 (1)" src="https://github.com/user-attachments/assets/1c700e0b-f54d-4510-8b7a-5e4c557c2c0e" />

### Sample VGM files (repo root)

`Ryu.vgm`, `Vortex.vgm`, and the related `.txt`/audio files are for testing/reference only and are not part of the firmware build.
