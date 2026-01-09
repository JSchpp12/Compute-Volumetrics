#!/bin/sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"
docker build -f ./deps/rockyLinux_dockerfile -t starlight:rockyLinux .

docker run -it --rm -v "${PARENT_DIR}":/app -w /app starlight:rockyLinux ./dist/deps/scripts/build_install.sh
