#!/usr/bin/env bash
#
# Validate the bundled example packages end to end: Cushy schema, JavaScript, and
# static IVG. This is the single "are the examples valid?" entry point; it
# orchestrates the per-language validators and adds the CushyLint pass that none
# of them cover.
#
# Scope:
#   - Cushy: every package under examples/ with the product's package extension,
#            plus the top-level console package
#   - JS:    delegated to validate-js.sh with its default scope (the console
#            package plus everything under examples/, including loose example
#            scripts that are not inside a package)
#   - IVG:   delegated to validate-static-ivg.sh (example .ivg plus the shared
#            product resources folder the examples depend on)
#
# Exits non-zero if any section fails, after running them all so you see every
# problem in one run.
#
# Shared between the Microtonic and Synplant script SDKs. Only the product
# configuration block below differs between the two copies.

set -uo pipefail

# ---- Product configuration ----------------------------------------------------
product_id="microtonic"               # lower-case name for temporary paths
sdk_name="microtonic-scripts-sdk"     # SDK folder name under references/
resources_dir="Microtonic Resources"  # shared resources folder at the SDK root
package_ext="mtscript"                # script package folder extension
console_package="JSConsole.mtscript"  # console package at the SDK root
# ---- End of product configuration --------------------------------------------

sdk_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$sdk_root"

case "$(uname -s)" in
	MINGW* | MSYS* | CYGWIN*) on_windows=1 ;;
	*) on_windows=0 ;;
esac

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/$product_id-examples.XXXXXX")" || {
	echo "Could not create a temporary directory." >&2
	exit 1
}
mirror_ready=0

cleanup() {
	rm -rf "$tmp_root"
}
trap cleanup EXIT

# Run CushyLint on a package directory (absolute path, no trailing slash).
run_cushylint() {
	local pkg_dir="$1"
	if [ "$on_windows" -eq 1 ]; then
		# Git Bash / MSYS / Cygwin cannot run the POSIX launcher, so use the
		# Windows one. Run it from its own folder as .\CushyLint.bat so cmd.exe
		# sees an unquoted batch name (and finds it even when it does not search
		# the current directory), disable MSYS argument conversion, and pass a
		# C:/... path: CushyLint accepts / as a directory slash on Windows, and
		# the trailing slash tells it the argument is a package directory.
		(
			cd "$sdk_root/CushyLint" &&
				MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' \
					cmd /c '.\CushyLint.bat' "$(cygpath -m "$pkg_dir")/"
		)
	else
		"$sdk_root/CushyLint/CushyLint" "$pkg_dir/"
	fi
}

# Sets cushy_pkg to the directory CushyLint should check for package $1.
set_package_for_cushylint() {
	local pkg="$1"
	if ! grep -rqs --include='*.schema' "references/$sdk_name/" "$pkg"; then
		cushy_pkg="$sdk_root/$pkg"
		return
	fi

	# Some bundled examples double as copy-ready external-project examples, so
	# their .schema files point at ../../references/<sdk>/. Validate the same
	# package contents in a temporary project-shaped mirror instead of
	# rewriting the checked-in schema. The mirror holds copies (not links) of
	# the resources folder and the CushyLint schemas it includes, so cleanup
	# can never reach back into the SDK checkout.
	if [ "$mirror_ready" -eq 0 ]; then
		local ref_root="$tmp_root/project/references/$sdk_name"
		mkdir -p "$tmp_root/project/examples" "$ref_root/CushyLint"
		cp -R "$sdk_root/$resources_dir" "$ref_root/"
		cp "$sdk_root"/CushyLint/*.schema "$ref_root/CushyLint/"
		mirror_ready=1
	fi
	local mirrored="$tmp_root/project/$pkg"
	rm -rf "$mirrored"
	mkdir -p "$(dirname "$mirrored")"
	cp -R "$pkg" "$mirrored"
	cushy_pkg="$mirrored"
}

# Example packages: everything under examples/ with the product's package
# extension, plus the top-level console package.
packages=()
for pkg in examples/*."$package_ext" "$console_package"; do
	[ -d "$pkg" ] && packages+=("$pkg")
done

failures=()
log_file="$tmp_root/cushylint.log"

echo "== Cushy schema (CushyLint) =="
for pkg in "${packages[@]}"; do
	# Skip packages without a .cushy (nothing for CushyLint to check).
	if ! find "$pkg" -name '*.cushy' -print -quit | grep -q .; then
		echo "skipped (no .cushy) $pkg"
		continue
	fi
	cushy_pkg=""
	set_package_for_cushylint "$pkg"
	if run_cushylint "$cushy_pkg" >"$log_file" 2>&1; then
		echo "ok   $pkg"
	else
		echo "FAIL $pkg" >&2
		cat "$log_file" >&2
		failures+=("cushy: $pkg")
	fi
done

echo
echo "== JavaScript (ESLint) =="
if tools/validate-js.sh; then
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
