#!/usr/bin/env bash
set -euo pipefail
# Native GLib 2.90.1, built in a local prefix, with release optimization.
root="$(pwd)"
mkdir -p build/glib-overall-deps
cd build/glib-overall-deps
curl --fail --location --retry 3 -o glib-2.90.1.tar.xz https://download.gnome.org/sources/glib/2.90/glib-2.90.1.tar.xz
echo '93c941aa17d5eb1d53fe838365f29a8b4e539c222a256d974ec8f30fc413e396  glib-2.90.1.tar.xz' | sha256sum --check
tar -xf glib-2.90.1.tar.xz
python3 -m venv tooling
tooling/bin/pip install meson packaging
tooling/bin/meson setup glib-build glib-2.90.1 --prefix="$root/build/native-glib" --libdir=lib --buildtype=release --wrap-mode=nofallback -Db_ndebug=true -Dglib_debug=disabled -Dtests=false -Dinstalled_tests=false -Ddocumentation=false -Dintrospection=disabled -Dman-pages=disabled -Dlibmount=disabled -Dselinux=disabled -Dsysprof=disabled
ninja -C glib-build -j 4
ninja -C glib-build install
cd "$root"
out=build/glib-overall-linux-results
mkdir -p "$out"
cp build/glib-overall-deps/glib-build/meson-info/intro-buildoptions.json "$out/glib-buildoptions.json"
cp build/glib-overall-deps/glib-build/meson-logs/meson-log.txt "$out/glib-meson-log.txt"
sha256sum build/glib-overall-deps/glib-2.90.1.tar.xz > "$out/glib-source.sha256"
build/glib-overall-deps/tooling/bin/meson --version > "$out/meson-version.txt"
python3 benchmarks/research/full-v420-academic-evidence/run.py --glib-prefix build/native-glib --include-legacy --output "$out"
