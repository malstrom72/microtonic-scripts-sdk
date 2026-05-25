#!/usr/bin/env bash
set -e -o pipefail -u

script_dir="$(cd "$(dirname "$0")" && pwd)"
script_file="$script_dir/$(basename "$0")"
cd "$script_dir"

support_dir="$script_dir"
root_dir="$(cd .. && pwd)"
pika_dir="$support_dir/PikaCmd"
source_file="$pika_dir/PikaCmdAmalgam.cpp"
build_script="$support_dir/BuildCpp.sh"
output="$root_dir/build/PikaCmd"
stamp="$output.sha256"

if [ ! -f "$source_file" ]; then
	echo "Missing PikaCmd source: $source_file"
	exit 1
fi

mkdir -p "$root_dir/build"

current_stamp() {
	shasum -a 256 "$source_file" "$build_script" "$script_file" | shasum -a 256 | awk '{ print $1 }'
}

expected_version() {
	sed -n 's/.*#define PIKA_SCRIPT_VERSION "\([^"]*\)".*/\1/p' "$source_file" | head -n 1
}

actual_version() {
	"$output" '{ print(VERSION) }' 2>/dev/null | sed -n '1p'
}

needs_rebuild=false
if [ ! -x "$output" ]; then
	needs_rebuild=true
elif ! "$output" -h >/dev/null 2>&1; then
	needs_rebuild=true
elif [ ! -f "$stamp" ]; then
	if [ "$(actual_version)" = "$(expected_version)" ]; then
		current_stamp > "$stamp"
	else
		needs_rebuild=true
	fi
elif [ "$(cat "$stamp")" != "$(current_stamp)" ]; then
	needs_rebuild=true
fi

if [ "$needs_rebuild" = true ]; then
	tmp="$output.tmp"
	rm -f "$tmp"
	"$build_script" release native "$tmp" -DPLATFORM_STRING=UNIX "$source_file"
	chmod +x "$tmp"

	(
		cd "$pika_dir"
		"$tmp" unittests.pika >/dev/null
		"$tmp" systoolsTests.pika
	)

	mv "$tmp" "$output"
	current_stamp > "$stamp"
fi
