#!/bin/bash
# NetHack Symbol Set Editor launcher
# Usage: ./symset_editor.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EDITOR_DIR="$SCRIPT_DIR/symset_editor"
VENV_DIR="$EDITOR_DIR/.venv"

# Create venv if it doesn't exist
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
    source "$VENV_DIR/bin/activate"
    pip install textual
else
    source "$VENV_DIR/bin/activate"
fi

# Run the editor
cd "$SCRIPT_DIR/.."
python -m util.symset_editor "$@"
