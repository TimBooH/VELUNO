#!/usr/bin/env bash
set -euo pipefail
render_test=$1
runtime_dir=$(mktemp -d)
chmod 700 "$runtime_dir"
compositor_pid=
cleanup() {
    if [[ -n "$compositor_pid" ]]; then
        kill "$compositor_pid" 2>/dev/null || true
        wait "$compositor_pid" 2>/dev/null || true
    fi
    rm -rf "$runtime_dir"
}
trap cleanup EXIT
export XDG_RUNTIME_DIR="$runtime_dir"
export WAYLAND_DISPLAY=veluno-render-test
export LIBGL_ALWAYS_SOFTWARE=1
export QT_QUICK_BACKEND=rhi
weston --backend=headless-backend.so --renderer=gl --shell=kiosk-shell.so --socket="$WAYLAND_DISPLAY" --idle-time=0 --no-config --log="$runtime_dir/weston.log" &
compositor_pid=$!
for ((attempt=0; attempt<100; ++attempt)); do
    [[ -S "$runtime_dir/$WAYLAND_DISPLAY" ]] && break
    if ! kill -0 "$compositor_pid" 2>/dev/null; then cat "$runtime_dir/weston.log"; exit 1; fi
    sleep 0.1
done
if [[ ! -S "$runtime_dir/$WAYLAND_DISPLAY" ]]; then cat "$runtime_dir/weston.log"; exit 1; fi
"$render_test" -platform wayland
