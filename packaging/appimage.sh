#!/usr/bin/env bash
# Packages an already tested build. Never uploads or publishes anything.
set -euo pipefail
project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${1:-"$project_root/build"}
output_dir=${2:-"$project_root/build/package"}
mkdir -p "$output_dir/tools"
output_dir=$(cd "$output_dir" && pwd)
app_dir="$output_dir/AppDir"
if [[ -e "$app_dir" ]]; then
    echo "AppDir already exists; select a fresh output directory." >&2
    exit 1
fi
fetch_tool() {
    local name=$1 repository=$2 digest=$3
    curl --fail --location --retry 3 "https://github.com/$repository/releases/download/continuous/$name-x86_64.AppImage" --output "$output_dir/tools/$name"
    echo "$digest  $output_dir/tools/$name" | sha256sum --check
    chmod +x "$output_dir/tools/$name"
}
# continuous URLs are checksum locked: changed upstream assets fail closed.
fetch_tool linuxdeploy linuxdeploy/linuxdeploy 8aea8da0f7f7039d2a2cecb14657d752a222a5e1d3825caeef186c82f751cdd1
fetch_tool linuxdeploy-plugin-qt linuxdeploy/linuxdeploy-plugin-qt cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617
DESTDIR="$app_dir" cmake --install "$build_dir" --prefix /usr
export PATH="$output_dir/tools:$PATH"
export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE=${QMAKE:-qmake6}
export QML_SOURCES_PATHS="$project_root/qml"
export EXTRA_QT_MODULES="waylandcompositor"
export EXTRA_PLATFORM_PLUGINS="libqoffscreen.so;libqwayland-egl.so;libqwayland-generic.so"
"$build_dir/veluno" --build-info > "$output_dir/build-info.json"
VERSION=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["version"])' "$output_dir/build-info.json")
release_channel=$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["channel"])' "$output_dir/build-info.json")
if [[ "$release_channel" == stable && "$VERSION" != *-* ]]; then
    OUTPUT="VELUNO-$VERSION-x86_64.AppImage"
elif [[ "$release_channel" == unstable && "$VERSION" == *-dev ]]; then
    OUTPUT="VELUNO-${VERSION%-dev}-unstable-x86_64.AppImage"
else
    echo "Build version and channel do not match; refusing to package." >&2
    exit 1
fi
export OUTPUT VERSION
qt_plugins=$("$QMAKE" -query QT_INSTALL_PLUGINS)
if [[ ! -d "$qt_plugins/tls" && -n "${PKG_CONFIG_SYSROOT_DIR:-}" ]]; then qt_plugins="$PKG_CONFIG_SYSROOT_DIR$qt_plugins"; fi
mkdir -p "$app_dir/usr/plugins/tls"
cp "$qt_plugins/tls/libqopensslbackend.so" "$app_dir/usr/plugins/tls/"
openssl_libdir=$(pkg-config --variable=libdir openssl)
if [[ ! -r "$openssl_libdir/libssl.so" && -n "${PKG_CONFIG_SYSROOT_DIR:-}" ]]; then openssl_libdir="$PKG_CONFIG_SYSROOT_DIR$openssl_libdir"; fi
cd "$output_dir"
linuxdeploy --appdir "$app_dir" --library "$openssl_libdir/libssl.so" --library "$openssl_libdir/libcrypto.so" --plugin qt --output appimage
sha256sum "$OUTPUT" > "$OUTPUT.sha256"
# Exercise the resulting package, including bundled QML and SQLite plugins.
smoke_dir=$(mktemp -d)
trap 'rm -rf "$smoke_dir"' EXIT
mkdir -p "$smoke_dir/data" "$smoke_dir/cache" "$smoke_dir/media"
ffmpeg -hide_banner -loglevel error -f lavfi -i color=c=blue:s=96x64:r=1:d=1 -frames:v 1 -an -c:v mpeg4 "$smoke_dir/media/Smoke.Movie.2020.mp4"
env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH -u QML_IMPORT_PATH -u QML2_IMPORT_PATH XDG_DATA_HOME="$smoke_dir/data" XDG_CACHE_HOME="$smoke_dir/cache" QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software timeout 30s "./$OUTPUT" --smoke-test --smoke-scan "$smoke_dir/media"

# Force loading Qt's folder picker implementation inside the isolated package too.
env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH -u QML_IMPORT_PATH -u QML2_IMPORT_PATH XDG_DATA_HOME="$smoke_dir/data" XDG_CACHE_HOME="$smoke_dir/cache" QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software timeout 20s "./$OUTPUT" --smoke-test --smoke-dialog
