# Dictate

Local speech-to-text with push-to-talk.

## Release layout

Put these files **next to the `dictate` binary**:

```
dictate
dictate.conf          # optional (created by config commands)
replacements.txt      # optional
ggml-tiny.en.bin      # whisper model
```

Copy `dictate.conf.example` → `dictate.conf` and `replacements.txt.example` → `replacements.txt` if you want templates.

## Build

```sh
cmake --preset default
cmake --build build
```

## Run

```sh
./build/dictate run
# or simply
./build/dictate
```

**macOS:** grant **Accessibility** and **Input Monitoring** to your terminal (or `dictate`), then restart.

## CLI

```sh
dictate run
dictate config show
dictate config mode hold
dictate config mode toggle
dictate config hotkey control+shift+d
```

Hotkey modifiers: `control` (or `ctrl`), `shift`, `option` (or `alt`), `command` (or `cmd`), plus a key (`a`–`z`, `0`–`9`, `space`, `f5`, …).

## replacements.txt

One rule per line:

```
um =>
myapp => MyApp
```

## dictate.conf

```
mode=hold
hotkey=control+shift+d
model=ggml-tiny.en.bin
```

- **hold** — record while the hotkey is held; transcribe on release.
- **toggle** — press once to start, again to stop and transcribe.
