# Cross-compilation environment for the 64-bit Windows build of Spectacol:
# Debian testing's MinGW-w64 and Qt built from source, once for the build
# machine (moc, rcc, qsb, qmlcachegen... run during the build) and once for
# Windows. packaging/windows/build.sh builds this image and the package.

ARG DEBIAN_IMAGE=debian:testing

FROM ${DEBIAN_IMAGE} AS toolchain
# Qt's tools warn without a UTF-8 locale
ENV LANG=C.UTF-8
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        binutils-mingw-w64-x86-64 \
        ca-certificates \
        cmake \
        curl \
        g++ \
        g++-mingw-w64-x86-64-posix \
        git \
        ninja-build \
        python3 \
        spirv-tools \
        xz-utils \
        zip \
    && rm -rf /var/lib/apt/lists/*
COPY mingw-w64-x86_64.cmake /opt/qt6/mingw-w64-x86_64.cmake

FROM toolchain AS qt
ARG QT_VERSION=6.12.0
# The build machine needs every module whose tools the Windows build runs
ARG QT_HOST_MODULES="qtbase qtsvg qtshadertools qtdeclarative"
ARG QT_WINDOWS_MODULES="qtbase qtsvg qtshadertools qtdeclarative qtmultimedia"
# Empty: as many jobs as CPUs
ARG BUILD_JOBS=

RUN <<'EOF'
set -eu
url="https://download.qt.io/official_releases/qt/${QT_VERSION%.*}/${QT_VERSION}/submodules"
mkdir -p /qt/src
cd /qt/src
for module in $QT_WINDOWS_MODULES; do
    archive="$module-everywhere-src-$QT_VERSION.tar.xz"
    curl -fsSL --retry 3 -O "$url/$archive" -O "$url/$archive.sha256"
    sha256sum -c "$archive.sha256"
    tar xf "$archive"
    mv "${archive%.tar.xz}" "$module"
    rm "$archive" "$archive.sha256"
done
EOF

RUN <<'EOF'
set -eu
for module in $QT_HOST_MODULES; do
    mkdir -p "/qt/build/host/$module"
    cd "/qt/build/host/$module"
    if [ "$module" = qtbase ]; then
        # Only the tools run here: no OpenGL and no platform plugins needed
        /qt/src/qtbase/configure -prefix /opt/qt6/host -release \
            -nomake examples -nomake tests -no-dbus -no-feature-sql -no-opengl
    else
        /opt/qt6/host/bin/qt-configure-module "/qt/src/$module"
    fi
    cmake --build . --parallel $BUILD_JOBS
    cmake --install .
done
rm -rf /qt/build/host
EOF

RUN <<'EOF'
set -eu
for module in $QT_WINDOWS_MODULES; do
    mkdir -p "/qt/build/windows/$module"
    cd "/qt/build/windows/$module"
    if [ "$module" = qtbase ]; then
        /qt/src/qtbase/configure -prefix /opt/qt6/mingw_64 -release \
            -xplatform win32-g++ -qt-host-path /opt/qt6/host \
            -nomake examples -nomake tests -no-dbus -no-feature-sql \
            -- -DCMAKE_TOOLCHAIN_FILE=/opt/qt6/mingw-w64-x86_64.cmake
    else
        /opt/qt6/mingw_64/bin/qt-configure-module "/qt/src/$module"
    fi
    cmake --build . --parallel $BUILD_JOBS
    cmake --install .
done
rm -rf /qt/build/windows
# The package ships Qt's libraries under the LGPL
mkdir -p /opt/qt6/mingw_64/licenses
cp /qt/src/qtbase/LICENSES/LGPL-3.0-only.txt /qt/src/qtbase/LICENSES/GPL-3.0-only.txt \
    /opt/qt6/mingw_64/licenses/
EOF

FROM toolchain
COPY --from=qt /opt/qt6 /opt/qt6
