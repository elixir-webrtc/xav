Xav requires FFmpeg development packages to be installed on your system.
You can install them with one of the following one-liners.

On some platforms, we use pkg-config to determine ffmpeg's include and lib directories.

**Fedora**

```bash
dnf install pkg-config ffmpeg-devel ffmpeg-libs
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

```bash
brew install pkg-config ffmpeg
```

**Windows**

Windows is not supported but PRs are welcomed.
