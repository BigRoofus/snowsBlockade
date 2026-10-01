# Blockade

> manipulate very high end frequencies to make identification of the audio almost impossible.

---

## Overview

Blockade is an audio-manipulation algorithm, not an application. It takes a WAV file in, adds
imperceptible high-frequency information, and produces a WAV file out that sounds identical to a
human listener but disrupts automated acoustic-fingerprinting/identification systems (e.g.
landmark-based matchers like Shazam, or ML models trained on full-bandwidth spectrograms).

This is a research / adversarial-ML project — studying how acoustic fingerprinting can be defeated
by inaudible perturbations, not a tool for evading copyright enforcement or covert surveillance.

Prior art this builds on:
- Saadatpanah et al., [*Adversarial Attacks on Copyright Detection Systems*](https://arxiv.org/pdf/1906.07153) —
  imperceptible perturbations that fooled YouTube Content ID and AudioTag.
- [*Inaudible Adversarial Perturbation*](https://www.ndss-symposium.org/wp-content/uploads/2024-30-paper.pdf) (NDSS 2024) —
  exploits the gap between human hearing (~20Hz-20kHz) and the full-bandwidth audio most
  recognition pipelines actually ingest.
- Standard psychoacoustic masking theory (the same math MP3 encoding uses) — computes, per frame,
  the maximum noise energy that stays inaudible under the original signal.
- Zhang et al., [*AntiFake*](https://arxiv.org/abs/2308.01187) (CCS 2023) — gradient-based
  adversarial perturbation against speaker-embedding models, disrupting voice-cloning pipelines'
  ability to extract usable speaker characteristics from a reference recording.

## Approach

Five techniques. The first four are applied per audio frame; the fifth runs once over the whole
utterance.

1. **Masking-threshold-constrained perturbation** — compute a Bark-scale psychoacoustic masking
   threshold per frame (simultaneous masking + absolute threshold of hearing). Inject noise-like
   energy shaped to stay under that threshold across the audible spectrum. This is inaudible by
   construction, not by ear-testing luck.
2. **Ultrasonic-band injection** — separately shape and inject energy above ~18kHz (bounded by
   Nyquist), which is at or beyond the edge of human hearing but is often included in full-bandwidth
   audio fed to fingerprinting/ML systems. This band uses a configurable gain rather than a strict
   masking derivation, since absolute-threshold modeling gets unreliable that close to Nyquist —
   treat the ceiling as a tuning knob, not a guarantee.
3. **Phase-inversion / peak disruption** (`core/phase_invert.py`) — landmark-based fingerprinters
   (Shazam-style) key on a constellation map of dominant time-frequency peaks. Each frame's top-K
   spectral peaks get an inverted (180°), bin-shifted copy injected back in, blurring peak position
   without adding audible energy — this is the "target the specific spectral peaks" idea the
   previous version of this doc flagged as future work.
4. **Data poisoning** (`core/poison.py`) — shapes perturbation energy toward the mel bands an
   MFCC-style feature front-end actually pools over (via a standard triangular mel filterbank
   weighting), rather than shaping noise blindly. A model that scrapes and trains on this audio
   learns spurious artifacts concentrated exactly where its own feature pipeline weights them most.
   Bounded by the same masking threshold as everything else — this shapes *where* energy goes, not
   whether it's audible.
5. **Anti-cloning adversarial attack** (`core/anticlone.py`, AntiFake-style) — a real, pretrained
   speech embedding model (WavLM) is run in a projected-gradient-descent (PGD) loop, pushing the
   embedding this clip produces away from its own true embedding, bounded by an L-inf epsilon.
   Directly targets what voice-cloning pipelines depend on (a speaker embedding extracted from
   reference audio) rather than shaping noise heuristically. Adds a `torch`/`torchaudio` dependency
   and downloads a pretrained model on first use.

## Architecture

Techniques 1-4 are folded into one pure, stateless function: `process_frame(frame, sample_rate,
config) -> frame`. It operates on a single fixed-size block of samples and has no dependency on
past or future frames, so it works identically whether it's:
- called in a loop over overlapping frames of a WAV file (the current, file-in/file-out use case), or
- fed live audio buffers in a real-time pipeline (not built yet, but requires no redesign — just a
  different caller).

Technique 5 (anti-cloning) breaks this pattern deliberately: WavLM's embeddings are contextual
across an entire clip, so there's no meaningful per-frame decomposition. It runs once per file, in
`audio_io.process_file`, over the whole (resampled) signal — not inside `process_frame`, and not
callable from a per-buffer real-time pipeline without redesign. All five techniques are toggled
independently via `.env` (`ENABLE_PHASE_INVERT`, `ENABLE_POISON`, `ENABLE_ANTICLONE`).

## Usage

```
python main.py --input path/to/in.wav --output path/to/out.wav
```

## Status

Early scaffolding. Masking model is a simplified approximation (Schroeder spreading function on the
Bark scale), not the full ISO/MPEG psychoacoustic model. Not yet validated against real fingerprinting
systems or listening tests. The anti-cloning PGD attack has been smoke-tested (it measurably drives
WavLM embedding similarity from ~1.0 toward negative values on synthetic audio) but not validated
against an actual voice-cloning pipeline or real speech.

**Data poisoning has a validation harness (`validation/run_poison_validation.py`) and the result is
a negative finding worth flagging**: it trains a small MFCC classifier on a synthetic 5-class vowel
task, once on clean audio and once on the same audio run through `poison_signal`, and compares
accuracy on a shared clean test set across 10 paired classifier-init trials. At the real pipeline's
imperceptibility bound (`PERTURBATION_CEILING=0.02`), the accuracy drop was **-1.0 ± 3.3 points —
statistically indistinguishable from training noise**. A sanity check with poisoning energy relaxed
to 15x the real ceiling (clearly audible, well outside what the algorithm would ever ship) did
produce a real 10-point drop, confirming the harness itself is sensitive enough to detect an effect
when one exists. Conclusion: at the amplitude this algorithm is constrained to for inaudibility, the
mel-band-targeted poisoning shaping in `core/poison.py` is currently not doing much — this needs a
stronger targeting strategy (or acceptance that poisoning may not be achievable under a strict
imperceptibility bound) rather than being treated as validated.
