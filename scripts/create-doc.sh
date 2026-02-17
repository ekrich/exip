#!/usr/bin/env bash
# Create the user documentation PDF
set -eux

# Change the the source directory
cd "$(dirname "$0")/../docs/user"

# Create the PDF and move into the www directory
tectonic -X compile exip.tex && mv exip.pdf ../www/exip-user-guide-1.pdf
