#!/usr/bin/env bash
#
# Validate the bundled example packages end to end: Cushy schema, JavaScript, and
# static IVG. This is the single "are the examples valid?" entry point; it
# orchestrates the per-language validators and adds the CushyLint pass that none
# of them cover.
#
# Scope:
#   - Cushy: every <Name>.mtscript package under examples/ plus JSConsole.mtscript
#   - JS:    delegated to validate-js.sh (defaults to the same example scope)
#   - IVG:   delegated to validate-static-ivg.sh (example .ivg plus the shared
#            Microtonic Resources the examples depend on)
#
# Exits non-zero if any section fails, after running them all so you see every
# problem in one run.

set -uo pipefail

sdk_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$sdk_root"

cushylint="$sdk_root/CushyLint/CushyLint"
resources="$sdk_root/Microtonic Resources"

# Example packages: everything under examples/ with a .mtscript extension, plus
# the top-level JSConsole reference package.
packages=()
for pkg in examples/*.mtscript JSConsole.mtscript; do
	[ -d "$pkg" ] && packages+=("$pkg")
done

failures=()

echo "== Cushy schema (CushyLint) =="
for pkg in "${packages[@]}"; do
	# Skip packages without a .cushy (nothing for CushyLint to check).
	if ! find "$pkg" -name '*.cushy' -print -quit | grep -q .; then
		echo "skipped (no .cushy) $pkg"
		continue
	fi
	if "$cushylint" "$sdk_root/$pkg/" "$resources" >/tmp/cushylint.$$.log 2>&1; then
		echo "ok   $pkg"
	else
		echo "FAIL $pkg" >&2
		cat /tmp/cushylint.$$.log >&2
		failures+=("cushy: $pkg")
	fi
	rm -f /tmp/cushylint.$$.log
done

echo
echo "== JavaScript (ESLint) =="
if tools/validate-js.sh "${packages[@]}"; then
	echo "ok   JavaScript"
else
	failures+=("javascript")
fi

echo
echo "== Static IVG =="
if tools/validate-static-ivg.sh; then
	echo "ok   static IVG"
else
	failures+=("static-ivg")
fi

echo
if [ "${#failures[@]}" -ne 0 ]; then
	echo "FAILED: ${failures[*]}" >&2
	exit 1
fi
echo "All example checks passed."
