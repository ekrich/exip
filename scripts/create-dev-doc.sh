#!/usr/bin/env bash
# Generate developer documentation with Doxygen
set -eux

# Change to the dev docs directory
cd "$(dirname "$0")/../docs/dev"

# Generate HTML documentation
doxygen Doxyfile
