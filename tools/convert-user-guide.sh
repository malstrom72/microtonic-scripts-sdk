#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="$ROOT_DIR/.tools/docling"
DOCLING_BIN="$VENV_DIR/bin/docling"
GUIDE_DIR="$ROOT_DIR/docs"
PDF_PATH="$GUIDE_DIR/Microtonic User Guide.pdf"
MD_PATH="$GUIDE_DIR/Microtonic User Guide.md"
JSON_PATH="$GUIDE_DIR/Microtonic User Guide.json"
DOCLING_JSON_PATH="$GUIDE_DIR/Microtonic User Guide.docling.json"

if [ ! -x "$DOCLING_BIN" ]; then
	echo "Docling is not installed. Run tools/bootstrap-docling.sh first." >&2
	exit 1
fi

if [ ! -f "$PDF_PATH" ]; then
	echo "Missing PDF: $PDF_PATH" >&2
	exit 1
fi

"$DOCLING_BIN" "$PDF_PATH" \
	--to md \
	--to json \
	--no-ocr \
	--device cpu \
	--image-export-mode referenced \
	--output "$GUIDE_DIR"

if [ ! -f "$MD_PATH" ] || [ ! -f "$JSON_PATH" ]; then
	echo "Docling did not produce the expected output files:" >&2
	echo "  $MD_PATH" >&2
	echo "  $JSON_PATH" >&2
	exit 1
fi

mv "$JSON_PATH" "$DOCLING_JSON_PATH"

GUIDE_DIR_PREFIX="$GUIDE_DIR/" perl -0pi -e 's/\Q$ENV{GUIDE_DIR_PREFIX}\E//g' "$MD_PATH" "$DOCLING_JSON_PATH"
"$ROOT_DIR/tools/postprocess-user-guide.pl" "$MD_PATH"

echo "Converted $PDF_PATH"
