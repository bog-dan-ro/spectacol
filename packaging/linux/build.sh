#!/usr/bin/env bash
# Builds the Linux AppImage in Debian stable with the image's Qt SDK.
# Copyright (c) 2015-2026, BogDan Vatra. GPL-3.0-or-later.
set -euo pipefail

scriptDir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
sourceDir=$(cd "$scriptDir/../.." && pwd)

usage()
{
    cat <<EOF
Usage: $0 [options]

  --build-dir DIR    build tree and packages (default: build/appimage-docker)
  --zxkit DIR        use this ZxKit checkout (default: the enclosing checkout,
                     or ZxKit from GitHub when building standalone Spectacol)
  --fetch-zxkit      use ZxKit from GitHub
  --jobs N           parallel build jobs (default: one per CPU)
  --rebuild-image    pull the base image and reinstall Qt and the deployment tools
  -h, --help         show this help
EOF
}

buildInContainer()
{
    local jobs=$1
    local zxkit=$2
    local sdk=/opt/qt-sdk
    local qtVersion
    qtVersion=$("$sdk/bin/qmake" -query QT_VERSION)
    local cmakeDir="/build/cmake/aqt-$qtVersion-$(uname -m)"
    mkdir -p "$HOME"

    echo "Using Qt $qtVersion from $sdk"
    cmake -S /src/spectacol -B "$cmakeDir" -G Ninja \
        -DCMAKE_PREFIX_PATH="$sdk" \
        -DCMAKE_BUILD_TYPE=Release \
        -DFETCHCONTENT_SOURCE_DIR_ZXKIT="$zxkit"
    cmake --build "$cmakeDir" --target appImage --parallel "$jobs"

    local version
    version=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$cmakeDir/CMakeCache.txt")
    local name="Spectacol-$version-linux-$(uname -m).AppImage"
    mkdir -p /build/packages
    cp "$cmakeDir/packages/$name" "/build/packages/$name"
    echo "Created packages/$name"
}

if [[ ${1:-} == --inside ]]; then
    buildInContainer "$2" "$3"
    exit
fi

buildDir="$sourceDir/build/appimage-docker"
zxkitDir=
fetchZxkit=false
jobs=$(nproc)
rebuildImage=false
while (($#)); do
    case $1 in
    --build-dir) buildDir=${2:?Missing build directory}; shift ;;
    --zxkit) zxkitDir=${2:?Missing ZxKit directory}; shift ;;
    --fetch-zxkit) fetchZxkit=true ;;
    --jobs) jobs=${2:?Missing build job count}; shift ;;
    --rebuild-image) rebuildImage=true ;;
    -h|--help) usage; exit ;;
    *) usage >&2; exit 2 ;;
    esac
    shift
done

if [[ -z $zxkitDir ]] && ! $fetchZxkit && [[ -f $sourceDir/../../include/zxkit/zxkit.h ]]; then
    zxkitDir=$sourceDir/../..
fi
if [[ -n $zxkitDir ]]; then
    if [[ ! -f $zxkitDir/include/zxkit/zxkit.h ]]; then
        echo "$zxkitDir is not a ZxKit checkout" >&2
        exit 2
    fi
    zxkitDir=$(cd "$zxkitDir" && pwd)
fi

imageTag=$(sha256sum < "$scriptDir/Dockerfile" | cut -c1-12)
image="spectacol-appimage-build:$imageTag"
if $rebuildImage || ! docker image inspect "$image" > /dev/null 2>&1; then
    imageArgs=()
    $rebuildImage && imageArgs+=(--pull --no-cache)
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
)
zxkit=
if [[ -n $zxkitDir ]]; then
    zxkit=/src/zxkit
    runArgs+=(--volume "$zxkitDir:$zxkit:ro")
fi
docker run "${runArgs[@]}" "$image" /src/spectacol/packaging/linux/build.sh --inside "$jobs" "$zxkit"
echo "Package: $buildDir/packages"
