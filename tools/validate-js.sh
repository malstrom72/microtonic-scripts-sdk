#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

config="tools/eslint.microtonic.config.mjs"

if [ "$#" -eq 0 ]; then
	set -- "JSConsole.mtscript" "examples"
fi

paths=()
for target in "$@"; do
	if [ -d "$target" ]; then
		while IFS= read -r -d '' file; do
			paths+=("$file")
		done < <(find "$target" -name '*.js' -print0)
	else
		paths+=("$target")
	fi
done

if [ "${#paths[@]}" -eq 0 ]; then
	echo "No JavaScript files found." >&2
	exit 0
fi

if [ -x "node_modules/.bin/eslint" ]; then
	exec "node_modules/.bin/eslint" --no-config-lookup -c "$config" "${paths[@]}"
fi

exec npx eslint --no-config-lookup -c "$config" "${paths[@]}"
