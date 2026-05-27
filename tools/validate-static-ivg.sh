#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

renderer="${IVG2PNG:-IVG/output/IVG2PNG}"
output_dir="${1:-/tmp/microtonic-static-ivg-validation}"

if [ ! -x "$renderer" ]; then
	tools/build-ivg2png.sh release native nosimd
fi

mkdir -p "$output_dir"

status=0
while IFS= read -r -d '' ivg_file; do
	relative="${ivg_file#./}"
	output_file="$output_dir/${relative//\//__}.png"
	log_file="$output_file.log"
	if "$renderer" --fast "$ivg_file" "$output_file" >"$log_file" 2>&1; then
		echo "rendered $relative -> $output_file"
	elif grep -q "Variable .* does not exist" "$log_file"; then
		echo "skipped dynamic $relative"
	else
		cat "$log_file" >&2
		echo "failed $relative" >&2
		status=1
	fi
done < <(find "Microtonic Resources" examples -name '*.ivg' -print0)

exit "$status"
