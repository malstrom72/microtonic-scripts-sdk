#!/usr/bin/env bash
#
# Render static .ivg files to PNG with tools/IVG2PNG, so IVG syntax and renderer
# errors are caught before the files reach the product.
#
# Usage: tools/validate-static-ivg.sh [--out <dir>] [<path> ...]
#
# Each <path> is an .ivg file or a folder that is searched recursively, relative
# to the current directory. Without paths, the SDK's own examples/ and product
# resources folder are checked. PNGs and logs go to --out, by default a temp
# folder named after the product.
#
# Environment: IVG2PNG (renderer path), IVG_FONTS (font directory), both
# relative to the SDK root unless absolute.
#
# Files the command-line renderer cannot render on its own are reported and
# skipped, not failed: dynamic files ("Variable <name> does not exist"), files
# that use IVG include statements ("Could not include file"; IVG2PNG has no
# filesystem include loader) and helper files without top-level bounds
# ("Undeclared bounds"). tools/validate-static-ivg.cmd applies the same rules.
#
# Shared between the Microtonic and Synplant script SDKs. Only the product
# configuration block below differs between the two copies.

set -euo pipefail

# ---- Product configuration ----------------------------------------------------
product_id="microtonic"              # lower-case name for the default output dir
resources_dir="Microtonic Resources" # shared resources folder at the SDK root
# ---- End of product configuration --------------------------------------------

usage() {
	echo "Usage: tools/validate-static-ivg.sh [--out <dir>] [<path> ...]" >&2
	exit 2
}

# Paths are relative to the caller's directory, so resolve them before the cd below.
output_dir=""
paths=()
while [ $# -gt 0 ]; do
	case "$1" in
		--out)
			[ $# -ge 2 ] || usage
			output_dir="$2"
			shift 2
			;;
		-h | --help) usage ;;
		-*) echo "Unknown option: $1" >&2; usage ;;
		*)
			if [ ! -e "$1" ]; then
				echo "No such file or folder: $1" >&2
				exit 2
			fi
			paths+=("$(cd "$(dirname "$1")" && pwd)/$(basename "$1")")
			shift
			;;
	esac
done
if [ -n "$output_dir" ]; then
	mkdir -p "$output_dir"
	output_dir="$(cd "$output_dir" && pwd)"
fi

cd "$(dirname "$0")/.."
root="$(pwd)"
if [ ${#paths[@]} -eq 0 ]; then
	paths=("$root/$resources_dir" "$root/examples")
fi

# Git Bash / MSYS / Cygwin cannot run the macOS binary, so use the Windows build.
case "$(uname -s)" in
	MINGW* | MSYS* | CYGWIN*) on_windows=1 default_renderer="tools/IVG2PNG/IVG2PNG.exe" ;;
	*) on_windows=0 default_renderer="tools/IVG2PNG/IVG2PNG" ;;
esac

renderer="${IVG2PNG:-$default_renderer}"
font_dir="${IVG_FONTS:-IVG/fonts}"
output_dir="${output_dir:-${TMPDIR:-/tmp}/$product_id-static-ivg-validation}"

# Rebuild when the renderer is missing or cannot run here, e.g. the macOS binary on Linux.
# Run without arguments it prints usage and exits 1; 126 or 127 means it did not run at all.
renderer_runs() {
	[ -x "$renderer" ] || return 1
	local rc=0
	"$renderer" >/dev/null 2>&1 || rc=$?
	[ "$rc" -lt 126 ]
}

if ! renderer_runs; then
	if [ "$on_windows" -eq 1 ]; then
		MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' cmd /c 'tools\build-ivg2png.cmd' release native
	else
		tools/build-ivg2png.sh release native nosimd
	fi
fi

mkdir -p "$output_dir"
output_dir="$(cd "$output_dir" && pwd)"

# Each file is rendered from its own directory, so make the tool paths absolute.
case "$renderer" in
	/*) renderer_path="$renderer" ;;
	*) renderer_path="$root/$renderer" ;;
esac

case "$font_dir" in
	/*) font_path="$font_dir" ;;
	*) font_path="$root/$font_dir" ;;
esac

status=0
count=0
for path in "${paths[@]}"; do
	# Name each output after the file's path below the given path's parent, which keeps names short.
	base="$(dirname "$path")"
	while IFS= read -r -d '' ivg_file; do
		count=$((count + 1))
		relative="${ivg_file#"$base"/}"
		output_file="$output_dir/${relative//\//__}.png"
		log_file="$output_file.log"
		ivg_dir="$(dirname "$ivg_file")"
		ivg_name="$(basename "$ivg_file")"
		if (cd "$ivg_dir" && "$renderer_path" --fast --fonts "$font_path" "$ivg_name" "$output_file") >"$log_file" 2>&1; then
			echo "rendered $relative -> $output_file"
		elif grep -q "Variable .* does not exist" "$log_file"; then
			echo "skipped dynamic $relative"
		elif grep -q "Could not include file" "$log_file"; then
			echo "skipped include-dependent $relative"
		elif grep -q "Undeclared bounds" "$log_file"; then
			echo "skipped helper $relative"
		else
			cat "$log_file" >&2
			echo "failed $relative" >&2
			status=1
		fi
	done < <(find "$path" -name '*.ivg' -print0)
done

if [ "$count" -eq 0 ]; then
	echo "No .ivg files found in: ${paths[*]}" >&2
fi
exit "$status"
