# Dictate

The free local WisprFlow alternative

## Installation

### From GitHub

### From Source

#### Prerequisites

- CMake
- Git
- vcpkg

Build for your machine (default preset):

```sh
cmake --preset default
cmake --build build
```

Platform-specific presets (same source, different `build/` dirs):

```sh
cmake --preset macos && cmake --build --preset macos
cmake --preset windows && cmake --build --preset windows
cmake --preset linux && cmake --build --preset linux
```

Project layout: `src/common/` (shared), `src/macos/`, `src/windows/`, `src/linux/` (platform adapters). Push-to-talk is implemented on **macOS** only for now.

```sh
curl -L -o ggml-tiny.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin
```

```sh
curl -L -o ggml-medium.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin
```

```sh
./build/dictate
```

On **macOS**, hold **Control+Shift+D** for a live preview; release for a polished transcription.

1. **Accessibility** — System Settings → Privacy & Security → **Accessibility** → enable **Cursor** (or Terminal/iTerm), or add `build/dictate` via the `+` button.
2. **Input Monitoring** (recommended) — same screen → **Input Monitoring** → enable the same app.
3. Quit and restart `./build/dictate`. You should see: `Push-to-talk active: hold Control+Shift+D`.
4. Debug keys: `DICTATE_DEBUG=1 ./build/dictate` logs key events to stderr.
5. Override the letter key only: `DICTATE_PTT_KEYCODE=2` (default `D` is keycode 2).

**Ctrl+C** in the terminal to quit. Windows and Linux builds compile but PTT is not implemented yet.

### Available Models

| Model               | Disk    | SHA                                        |
| ------------------- | ------- | ------------------------------------------ |
| tiny                | 75 MiB  | `bd577a113a864445d4c299885e0cb97d4ba92b5f` |
| tiny.en             | 75 MiB  | `c78c86eb1a8faa21b369bcd33207cc90d64ae9df` |
| base                | 142 MiB | `465707469ff3a37a2b9b8d8f89f2f99de7299dac` |
| base.en             | 142 MiB | `137c40403d78fd54d454da0f9bd998f78703390c` |
| small               | 466 MiB | `55356645c2b361a969dfd0ef2c5a50d530afd8d5` |
| small.en            | 466 MiB | `db8a495a91d927739e50b3fc1cc4c6b8f6c2d022` |
| small.en-tdrz       | 465 MiB | `b6c6e7e89af1a35c08e6de56b66ca6a02a2fdfa1` |
| medium              | 1.5 GiB | `fd9727b6e1217c2f614f9b698455c4ffd82463b4` |
| medium.en           | 1.5 GiB | `8c30f0e44ce9560643ebd10bbe50cd20eafd3723` |
| large-v1            | 2.9 GiB | `b1caaf735c4cc1429223d5a74f0f4d0b9b59a299` |
| large-v2            | 2.9 GiB | `0f4c8e34f21cf1a914c59d8b3ce882345ad349d6` |
| large-v2-q5_0       | 1.1 GiB | `00e39f2196344e901b3a2bd5814807a769bd1630` |
| large-v3            | 2.9 GiB | `ad82bf6a9043ceed055076d0fd39f5f186ff8062` |
| large-v3-q5_0       | 1.1 GiB | `e6e2ed78495d403bef4b7cff42ef4aaadcfea8de` |
| large-v3-turbo      | 1.5 GiB | `4af2b29d7ec73d781377bfd1758ca957a807e941` |
| large-v3-turbo-q5_0 | 547 MiB | `e050f7970618a659205450ad97eb95a18d69c9ee` |
