#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"
cd deps
docker build -f rockyLinux_dockerfile -t starlight:rockyLinux .
cd ..
echo $PARENT_DIR
docker run -it -v $PARENT_DIR:/app -w /app starlight:rockyLinux /bin/bash ./dist/deps/scripts/build_install.sh
