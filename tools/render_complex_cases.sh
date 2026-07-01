#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CASE_FILE="${1:-"$ROOT_DIR/test_cases_complex.tsv"}"
OUTPUT_DIR="${2:-"$ROOT_DIR/output_complex"}"
PARSER="$ROOT_DIR/parser"

if [[ ! -f "$CASE_FILE" ]]; then
	echo "case file not found: $CASE_FILE" >&2
	exit 1
fi

if [[ ! -x "$PARSER" ]]; then
	echo "parser executable not found; run: cmake --build . -j2" >&2
	exit 1
fi

mkdir -p "$OUTPUT_DIR"

count=0
while IFS=$'\t' read -r code name expr rest; do
	if [[ -z "${code:-}" || "${code:0:1}" == "#" ]]; then
		continue
	fi
	if [[ -z "${expr:-}" ]]; then
		echo "skip malformed row: $code ${name:-}" >&2
		continue
	fi

	base="${code}_${name}"
	svg_path="$OUTPUT_DIR/${base}.svg"
	log_path="$OUTPUT_DIR/${base}.log"
	tmp_svg="$OUTPUT_DIR/.${base}.tmp.svg"

	(
		cd "$OUTPUT_DIR"
		echo "$expr" | "$PARSER"
	) > "$log_path"

	if [[ ! -f "$OUTPUT_DIR/output.svg" ]]; then
		echo "parser did not produce output.svg for $base" >&2
		exit 1
	fi

	mv "$OUTPUT_DIR/output.svg" "$tmp_svg"
	mv "$tmp_svg" "$svg_path"
	((count += 1))
	echo "rendered $base -> $svg_path"
done < "$CASE_FILE"

echo "rendered $count complex case(s) into $OUTPUT_DIR"
