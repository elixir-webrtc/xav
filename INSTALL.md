Xav requires FFmpeg development packages to be installed on your system.
You can install them with one of the following one-liners.

**Fedora**

```bash
dnf install ffmpeg-devel ffmpeg-libs
```

**Ubuntu**

```bash
apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev
```

**MacOS x86_64**

```bash
brew install ffmpeg
```

**MacOS arm64**

On arm64, we use pkg-config to determine ffmpeg's include and lib directories.

```bash
brew install pkg-config ffmpeg
```

**Windows**

Windows is not supported but PRs are welcomed.

