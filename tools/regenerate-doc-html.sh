#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCS_DIR="$ROOT_DIR/docs"

if ! command -v pandoc >/dev/null 2>&1; then
  echo "error: pandoc is required to regenerate checked-in HTML docs" >&2
  exit 1
fi

if [ "$#" -eq 0 ]; then
  set -- "$DOCS_DIR"/*.md
fi

for src in "$@"; do
  if [ ! -f "$src" ]; then
    echo "error: not a file: $src" >&2
    exit 1
  fi

  case "$src" in
    *.md) ;;
    *)
      echo "error: expected a Markdown file: $src" >&2
      exit 1
      ;;
  esac

  out="${src%.md}.html"
  if [ ! -f "$out" ]; then
    echo "skip: $src (no checked-in HTML target)"
    continue
  fi

  title="$(sed -n 's:.*<title>\(.*\)</title>.*:\1:p' "$out" | head -n 1)"
  if [ -z "$title" ]; then
    title="$(basename "${src%.md}")"
  fi

  echo "pandoc: $src -> $out"
  pandoc -s --no-highlight "$src" -o "$out" --metadata "title=$title"
done
