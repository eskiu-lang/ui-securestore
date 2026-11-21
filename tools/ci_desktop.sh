#!/bin/sh
# CI on a desktop: the native side against the real store, then the example built with the ui
# tool for this desktop's native host. tools/ci_desktop.sh linux|windows|macos
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
    windows)
        clang -Wall -Wextra -Werror native/windows/securestore.c tools/native_smoke.c -o build/smoke.exe
        ./build/smoke.exe
        ;;
    macos)
        clang -fobjc-arc -Wall -Werror native/macos/securestore.m tools/native_smoke.c -framework Foundation \
            -framework Security -o build/smoke
        ./build/smoke
        ;;
    *) echo "usage: tools/ci_desktop.sh linux|windows|macos"; exit 2 ;;
esac
cd examples/vault
eskiuc fetch
ui_src="${ESKIU_HOME:-$HOME/.eskiu}/pkgs/ui/$(awk -F'\t' '$1 == "ui" { print $5 }' eskiu.lock)"
tool="$root/build/ui"
[ "$host" = windows ] && tool="$tool.exe"
eskiuc "$ui_src/tools/ui/main.esk" -o "$tool"
"$tool" build "$host"
echo "PASS desktop $host"
