#!/usr/bin/env bash
# Cross-compiles Spectacol for 64-bit Windows in a Docker container and packages
# it as a portable zip. The image (Debian testing's MinGW-w64, Qt built from
# source) is built on first use, and again when its Dockerfile changes.
#
# Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
set -euo pipefail

scriptDir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
sourceDir=$(cd "$scriptDir/../.." && pwd)

usage()
{
    cat <<EOF
Usage: $0 [options]

  --build-dir DIR    build tree and packages (default: build/windows-x86_64)
  --build-type TYPE  CMake build type (default: Release)
  --zxkit DIR        use this ZxKit checkout (default: the ZxKit checkout that
                     contains Spectacol, else ZxKit from GitHub)
  --fetch-zxkit      use ZxKit from GitHub, even inside a ZxKit checkout
  --jobs N           parallel build jobs (default: one per CPU)
  --rebuild-image    rebuild the image from scratch, e.g. for Debian updates
  -h, --help         show this help
EOF
}

# Runs in the container: /src/spectacol (read-only), /build, /src/zxkit (optional)
buildInContainer()
{
    local qt=/opt/qt6/mingw_64
    local cmakeDir=/build/cmake
    mkdir -p "$HOME"

    cmake -S /src/spectacol -B "$cmakeDir" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$qt/lib/cmake/Qt6/qt.toolchain.cmake" \
        -DCMAKE_BUILD_TYPE="$SPECTACOL_BUILD_TYPE" \
        -DFETCHCONTENT_SOURCE_DIR_ZXKIT="$SPECTACOL_ZXKIT"
    cmake --build "$cmakeDir" --parallel $SPECTACOL_JOBS

    local version
    version=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$cmakeDir/CMakeCache.txt")
    local name="Spectacol-$version-windows-x86_64"
    local app="/build/$name"
    rm -rf "$app"
    cmake --install "$cmakeDir" --component Runtime --prefix "$app"

    local deployArgs=(
        --qt "$qt"
        --qmlimportscanner /opt/qt6/host/libexec/qmlimportscanner
        --qml-root /src/spectacol/src/qml
        --import-path "$cmakeDir/qml"
    )
    local dll
    for dll in libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll; do
        dll=$(x86_64-w64-mingw32-g++-posix -print-file-name="$dll")
        [[ $dll == /* ]] && deployArgs+=(--dll-path "$(dirname "$dll")")
    done
    case $SPECTACOL_BUILD_TYPE in
    Release | MinSizeRel) deployArgs+=(--strip x86_64-w64-mingw32-strip) ;;
    esac
    python3 /src/spectacol/packaging/windows/deploy.py "$app" "${deployArgs[@]}"

    mkdir -p "$app/licenses/Qt"
    cp "$qt"/licenses/* "$app/licenses/Qt/"
    cp /usr/share/doc/mingw-w64-common/copyright "$app/licenses/MinGW-w64.txt"

    mkdir -p /build/packages
    rm -f "/build/packages/$name.zip"
    (cd /build && zip -qr9 "packages/$name.zip" "$name")
    echo "Created packages/$name.zip"
}

if [[ ${1:-} == --inside ]]; then
    buildInContainer
    exit
fi

buildDir="$sourceDir/build/windows-x86_64"
buildType=Release
zxkitDir=
fetchZxKit=false
jobs=
rebuildImage=false
while (($#)); do
    case $1 in
    --build-dir) buildDir=$2; shift ;;
    --build-type) buildType=$2; shift ;;
    --zxkit) zxkitDir=$2; shift ;;
    --fetch-zxkit) fetchZxKit=true ;;
    --jobs) jobs=$2; shift ;;
    --rebuild-image) rebuildImage=true ;;
    -h | --help) usage; exit ;;
    *) usage >&2; exit 2 ;;
    esac
    shift
done

if [[ -z $zxkitDir ]] && ! $fetchZxKit && [[ -f $sourceDir/../../include/zxkit/zxkit.h ]]; then
    zxkitDir=$sourceDir/../..
fi
if [[ -n $zxkitDir ]]; then
    if [[ ! -f $zxkitDir/include/zxkit/zxkit.h ]]; then
        echo "$zxkitDir is not a ZxKit checkout" >&2
        exit 2
    fi
    zxkitDir=$(cd "$zxkitDir" && pwd)
fi

# The tag follows the image's inputs, so a changed Dockerfile builds a new image
imageTag=$(cat "$scriptDir/Dockerfile" "$scriptDir/mingw-w64-x86_64.cmake" | sha256sum | cut -c1-12)
image="spectacol-windows-build:$imageTag"
if $rebuildImage || ! docker image inspect "$image" > /dev/null 2>&1; then
    echo "Building the $image image: MinGW-w64 and Qt, this takes a while"
    imageArgs=()
    $rebuildImage && imageArgs+=(--pull --no-cache)
    [[ -n $jobs ]] && imageArgs+=(--build-arg "BUILD_JOBS=$jobs")
    docker build "${imageArgs[@]}" -t "$image" "$scriptDir"
fi

mkdir -p "$buildDir"
buildDir=$(cd "$buildDir" && pwd)
runArgs=(
    --rm --init
    --user "$(id -u):$(id -g)"
    --volume "$sourceDir:/src/spectacol:ro"
    --volume "$buildDir:/build"
    --env HOME=/build/.home
    --env "SPECTACOL_BUILD_TYPE=$buildType"
    --env "SPECTACOL_JOBS=$jobs"
)
if [[ -n $zxkitDir ]]; then
    echo "Using ZxKit from $zxkitDir"
    runArgs+=(--volume "$zxkitDir:/src/zxkit:ro" --env SPECTACOL_ZXKIT=/src/zxkit)
else
    echo "Using ZxKit from GitHub"
    runArgs+=(--env SPECTACOL_ZXKIT=)
fi
[[ -t 1 ]] && runArgs+=(--tty)
docker run "${runArgs[@]}" "$image" /src/spectacol/packaging/windows/build.sh --inside
echo "Package: $buildDir/packages"
