#!/bin/sh
# CI on a desktop: the native side against the real store (tools/native_smoke.c), then the
# example built with the ui tool for that desktop's native host. `windows` builds the example
# (from Linux too, with CXX naming a MinGW-w64 compiler); `smoke-windows` runs the check on a
# Windows machine. tools/ci_desktop.sh linux|windows|macos|smoke-windows
set -e
host="$1"
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mkdir -p build
case "$host" in
    linux)
        cc -Wall -Wextra -Werror $(pkg-config --cflags glib-2.0) native/linux/securestore.c tools/native_smoke.c \
            $(pkg-config --libs glib-2.0) -ldl -o build/smoke
        # Without a Secret Service: nothing is kept, and the store says it is not secure.
        DBUS_SESSION_BUS_ADDRESS=unix:path=/nonexistent ./build/smoke > build/missing.txt || true
        cat build/missing.txt
        grep -q "FAIL the store is secure" build/missing.txt && grep -q "FAIL set" build/missing.txt
        # With GNOME Keyring unlocked on a session bus of its own.
        dbus-run-session -- sh -c 'echo ci | gnome-keyring-daemon --unlock --components=secrets > /dev/null && ./build/smoke'
        ;;
    smoke-windows)
        clang -Wall -Wextra -Werror native/windows/securestore.c tools/native_smoke.c -o build/smoke.exe
        ./build/smoke.exe
        exit 0
        ;;
    windows) ;;
    macos)
        clang -fobjc-arc -Wall -Werror native/macos/securestore.m tools/native_smoke.c -framework Foundation \
            -framework Security -o build/smoke
        ./build/smoke
        ;;
    *) echo "usage: tools/ci_desktop.sh linux|windows|macos|smoke-windows"; exit 2 ;;
esac
cd examples/vault
eskiuc fetch
ui_src="${ESKIU_HOME:-$HOME/.eskiu}/pkgs/ui/$(awk -F'\t' '$1 == "ui" { print $5 }' eskiu.lock)"
tool="$root/build/ui"
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) tool="$tool.exe" ;; esac
[ -x "$tool" ] || eskiuc "$ui_src/tools/ui/main.esk" -o "$tool"
"$tool" build "$host"
echo "PASS desktop $host"
