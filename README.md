# Dictate

The free local WisprFlow alternative

## Installation

### From GitHub

### From Source

#### Prerequisites

- CMake
- Git
- vcpkg

```sh
cmake --preset default
```

```sh
cmake --build build
```

```sh
curl -L -o ggml-base.en.bin https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
```

```sh
./build/dictate
```
