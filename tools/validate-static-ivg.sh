#!/usr/bin/env bash
#
# Render every static .ivg under the product resources folder and examples/ to
# PNG with tools/IVG2PNG, so IVG syntax and renderer errors are caught before
# the files reach the product.
#
# Usage: tools/validate-static-ivg.sh [output-dir]
#
# Environment: IVG2PNG (renderer path), IVG_FONTS (font directory).
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

cd "$(dirname "$0")/.."
root="$(pwd)"

# Git Bash / MSYS / Cygwin cannot run the macOS binary, so use the Windows build.
case "$(uname -s)" in
	MINGW* | MSYS* | CYGWIN*) on_windows=1 default_renderer="tools/IVG2PNG/IVG2PNG.exe" ;;
	*) on_windows=0 default_renderer="tools/IVG2PNG/IVG2PNG" ;;
esac

renderer="${IVG2PNG:-$default_renderer}"
font_dir="${IVG_FONTS:-IVG/fonts}"
output_dir="${1:-${TMPDIR:-/tmp}/$product_id-static-ivg-validation}"

if [ ! -x "$renderer" ]; then
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
while IFS= read -r -d '' ivg_file; do
	relative="${ivg_file#./}"
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
done < <(find "$resources_dir" examples -name '*.ivg' -print0)

exit "$status"
