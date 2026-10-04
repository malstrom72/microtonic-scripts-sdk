#!/usr/bin/env bash
set -e -o pipefail -u

script_dir="$(cd "$(dirname "$0")" && pwd)"
script_file="$script_dir/$(basename "$0")"
cd "$script_dir"

support_dir="$script_dir"
root_dir="$(cd .. && pwd)"
build_script="$support_dir/BuildCpp.sh"
output="$root_dir/build/MakaronCmd"
stamp="$output.sha256"
sources=("$support_dir/MakaronCmd.cpp" "$support_dir/Makaron.cpp" "$support_dir/Makaron.h" "$build_script" "$script_file")

mkdir -p "$root_dir/build"

# Hash file contents only (not paths) so a committed stamp is valid in any checkout.
current_stamp() {
	cat "${sources[@]}" | shasum -a 256 | awk '{ print $1 }'
}

needs_rebuild=false
if [ ! -x "$output" ]; then
	needs_rebuild=true
elif ! printf '' | "$output" - - >/dev/null 2>&1; then
	needs_rebuild=true
elif [ ! -f "$stamp" ]; then
	needs_rebuild=true
elif [ "$(cat "$stamp")" != "$(current_stamp)" ]; then
	needs_rebuild=true
fi

if [ "$needs_rebuild" = true ]; then
	tmp="$output.tmp"
	rm -f "$tmp"
	"$build_script" release native "$tmp" -I "$support_dir" "$support_dir/MakaronCmd.cpp" "$support_dir/Makaron.cpp"
	chmod +x "$tmp"
	printf '' | "$tmp" - - >/dev/null
	mv "$tmp" "$output"
	current_stamp > "$stamp"
fi
