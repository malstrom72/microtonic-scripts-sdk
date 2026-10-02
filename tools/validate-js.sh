#!/usr/bin/env bash
#
# Lint Sonic Charge script JavaScript (ES5 "script" syntax) with ESLint.
#
# Usage: tools/validate-js.sh [file-or-directory ...]
#
# Shared between the Microtonic and Synplant script SDKs. Only the product
# configuration block below differs between the two copies.

set -euo pipefail

# ---- Product configuration ----------------------------------------------------
# ESLint config file under tools/ and the console package linted by default.
eslint_config="eslint.microtonic.config.mjs"
console_package="JSConsole.mtscript"
# ---- End of product configuration --------------------------------------------

# ESLint used through npx when the SDK has no local node_modules/.bin/eslint.
# Pinned to a major version so a new ESLint major cannot change results
# unannounced; the configs are flat configs written for ESLint 10.
eslint_package="eslint@10"

sdk_root="$(cd "$(dirname "$0")/.." && pwd)"

config="$sdk_root/tools/$eslint_config"

# With no arguments, validate the console package plus everything under
# examples/ (packages, loose example scripts and subfolders). When paths are
# supplied, keep the caller's working directory so files outside the SDK
# checkout are validated instead of being ignored by ESLint's flat-config base
# path.
if [ "$#" -eq 0 ]; then
	cd "$sdk_root"
	set -- "$console_package" "examples"
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

if [ -x "$sdk_root/node_modules/.bin/eslint" ]; then
	exec "$sdk_root/node_modules/.bin/eslint" --no-config-lookup -c "$config" "${paths[@]}"
fi

exec npx --yes "$eslint_package" --no-config-lookup -c "$config" "${paths[@]}"
