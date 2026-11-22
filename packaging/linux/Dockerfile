# Debian 13 is the current Debian stable release. Pin the codename so the
# AppImage's minimum system library versions stay consistent.

FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        appstream build-essential ca-certificates cmake curl git ninja-build pkg-config \
        libegl-dev libgl-dev libgl1-mesa-dri libvulkan-dev libudev-dev libxkbcommon-dev \
        libdbus-1-3 libfontconfig1 libfreetype6 libgtk-3-0t64 libpulse0 libx11-xcb1 \
        libxkbcommon-x11-0 libxrandr2 \
        libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
        libxcb-render-util0 libxcb-shape0 libxcb-sync1 libxcb-xfixes0 libxcb-xkb1 \
        libasound2t64 libgstreamer-plugins-base1.0-0 libwayland-cursor0 libwayland-egl1 \
        squashfs-tools zlib1g-dev \
        file ca-certificates python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/aqt \
    && /opt/aqt/bin/pip install --no-cache-dir aqtinstall==3.3.0 \
    && case "$(dpkg --print-architecture)" in \
        amd64) host=linux; arch=linux_gcc_64 ;; \
        arm64) host=linux_arm64; arch=linux_gcc_arm64 ;; \
        *) echo "Unsupported AppImage build architecture" >&2; exit 1 ;; \
    esac \
    && /opt/aqt/bin/aqt install-qt "$host" desktop 6.12.0 "$arch" \
        --outputdir /opt/Qt --modules qtmultimedia qtshadertools


# linuxdeploy includes its AppImage output plugin.
ARG LINUXDEPLOY_TAG=continuous
ARG LINUXDEPLOY_QT_TAG=continuous
RUN case "$(dpkg --print-architecture)" in \
        amd64) arch=x86_64 ;; \
        arm64) arch=aarch64 ;; \
        *) echo "Unsupported AppImage build architecture" >&2; exit 1 ;; \
    esac \
    && curl -fL --retry 3 -o /usr/local/bin/linuxdeploy \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_TAG}/linuxdeploy-${arch}.AppImage" \
    && curl -fL --retry 3 -o /usr/local/bin/linuxdeploy-plugin-qt \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${LINUXDEPLOY_QT_TAG}/linuxdeploy-plugin-qt-${arch}.AppImage" \
    && chmod +x /usr/local/bin/linuxdeploy /usr/local/bin/linuxdeploy-plugin-qt

RUN sdk=$(find /opt/Qt -mindepth 2 -maxdepth 2 -type d \
        -exec test -x {}/bin/qmake \; -print -quit) \
    && test -n "$sdk" && ln -s "$sdk" /opt/qt-sdk
