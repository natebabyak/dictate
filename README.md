# Dictate

Local speech-to-text with push-to-talk.

## Release layout

Put these files **next to the `dictate` binary**:

```
dictate
config.json
ggml-<model>.bin        # see Models below
```

## Build

```sh
cmake --preset default
cmake --build build
```

## Run

```sh
cp config.json build/config.json
cp ggml-tiny.en.bin build/                 # or your chosen model
./build/dictate
```

**macOS:** grant **Accessibility** and **Input Monitoring** to your terminal (or `dictate`), then restart.

## config.json

Validated against `schema.json` at startup.

```json
{
  "mode": "hold",
  "hotkey": "control+shift+d",
  "model": "tiny.en"
}
```

### mode

- **hold** — record while the hotkey is held; transcribe on release.
- **toggle** — press once to start, again to stop and transcribe.

### hotkey

Modifiers joined with `+`, then the key. Supported modifiers:

- `control` (or `ctrl`)
- `shift`
- `option` (or `alt`)
- `command` (or `cmd`)

Keys: `a`–`z`, `0`–`9`, `space`, `f5`, …

Example: `control+shift+d`

### model

Set `model` to one of the values below. Place the matching `.bin` file next to the binary.

| Config value | File | Size | Notes |
|---|---|---:|---|
| `tiny.en` | `ggml-tiny.en.bin` | ~75 MB | Fastest, lowest accuracy |
| `base.en` | `ggml-base.en.bin` | ~142 MB | Good speed/quality balance |
| `small.en` | `ggml-small.en.bin` | ~466 MB | Better accuracy |
| `medium.en` | `ggml-medium.en.bin` | ~1.5 GB | High accuracy |
| `large-v3` | `ggml-large-v3.bin` | ~3.1 GB | Best accuracy, slowest |

Download models from [ggerganov/whisper.cpp on Hugging Face](https://huggingface.co/ggerganov/whisper.cpp/tree/main).

## Behavior

Transcribed text is pasted into the focused app (macOS). On Windows and Linux, text is printed to stderr until injection is implemented.
