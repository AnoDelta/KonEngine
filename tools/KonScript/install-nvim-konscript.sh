#!/bin/bash
# Install KonScript syntax highlighting for Neovim
set -e

NVIM_CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}/nvim"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EDITOR_DIR="$SCRIPT_DIR/editor/nvim"

echo "Installing KonScript Neovim support..."
echo "Config dir: $NVIM_CONFIG"
echo ""

mkdir -p "$NVIM_CONFIG/syntax"
mkdir -p "$NVIM_CONFIG/ftplugin"
mkdir -p "$NVIM_CONFIG/ftdetect"

cp "$EDITOR_DIR/syntax/konscript.vim"  "$NVIM_CONFIG/syntax/konscript.vim"
echo "  ✓ $NVIM_CONFIG/syntax/konscript.vim"

cp "$EDITOR_DIR/ftplugin/konscript.vim" "$NVIM_CONFIG/ftplugin/konscript.vim"
echo "  ✓ $NVIM_CONFIG/ftplugin/konscript.vim"

cat > "$NVIM_CONFIG/ftdetect/konscript.vim" << 'VIMEOF'
autocmd BufNewFile,BufRead *.ks setfiletype konscript
VIMEOF
echo "  ✓ $NVIM_CONFIG/ftdetect/konscript.vim"

echo ""
echo "Done! Restart nvim and open a .ks file."
echo ""
echo "Key bindings in .ks files:"
echo "  F5     -- ksc run"
echo "  F6     -- ksc compile only"
echo "  Ctrl+/ -- toggle comment"
