#!/bin/bash
# Install KonScript editor support
# Usage:
#   ./install.sh           -- install everything
#   ./install.sh --nvim    -- Neovim only
#   ./install.sh --vscode  -- VS Code only
#   ./install.sh --lsp     -- LSP server only

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# -----------------------------------------------------------------------
# Options
DO_NVIM=0
DO_VSCODE=0
DO_LSP=0

if [ $# -eq 0 ]; then
    DO_NVIM=1; DO_VSCODE=1; DO_LSP=1
fi
for arg in "$@"; do
    case "$arg" in
        --nvim)   DO_NVIM=1 ;;
        --vscode) DO_VSCODE=1 ;;
        --lsp)    DO_LSP=1 ;;
        --all)    DO_NVIM=1; DO_VSCODE=1; DO_LSP=1 ;;
    esac
done

# -----------------------------------------------------------------------
# Neovim
# -----------------------------------------------------------------------
install_nvim() {
    echo "Installing Neovim support..."

    # Detect config dir
    NVIM_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/nvim"

    mkdir -p "$NVIM_DIR/ftdetect"
    mkdir -p "$NVIM_DIR/syntax"
    mkdir -p "$NVIM_DIR/ftplugin"

    cp "$SCRIPT_DIR/nvim/ftdetect/konscript.vim" "$NVIM_DIR/ftdetect/"
    cp "$SCRIPT_DIR/nvim/syntax/konscript.vim"   "$NVIM_DIR/syntax/"
    cp "$SCRIPT_DIR/nvim/ftplugin/konscript.vim" "$NVIM_DIR/ftplugin/"

    echo "  Installed to $NVIM_DIR"

    # If using lazy.nvim, suggest adding LSP config
    if [ -f "$NVIM_DIR/init.lua" ]; then
        echo ""
        echo "  Detected Neovim with init.lua."
        echo "  To enable the LSP, add this to your config:"
        echo ""
        echo "    require('lspconfig').konscript_lsp.setup({"
        echo "      cmd = { 'konscript-lsp' },"
        echo "      filetypes = { 'konscript' },"
        echo "    })"
    fi
}

# -----------------------------------------------------------------------
# VS Code
# -----------------------------------------------------------------------
install_vscode() {
    echo "Installing VS Code support..."

    # Try to find VS Code extensions dir
    VSCODE_DIR=""
    for dir in \
        "$HOME/.vscode/extensions" \
        "$HOME/.vscode-server/extensions" \
        "$HOME/Library/Application Support/Code/User/extensions"
    do
        if [ -d "$dir" ]; then
            VSCODE_DIR="$dir"
            break
        fi
    done

    if [ -z "$VSCODE_DIR" ]; then
        echo "  VS Code extensions directory not found."
        echo "  Copy $SCRIPT_DIR/vscode/konscript/ to your extensions folder manually."
        return
    fi

    DEST="$VSCODE_DIR/konscript-0.1.0"
    mkdir -p "$DEST/syntaxes"
    cp "$SCRIPT_DIR/vscode/konscript/package.json"              "$DEST/"
    cp "$SCRIPT_DIR/vscode/konscript/language-configuration.json" "$DEST/"
    cp "$SCRIPT_DIR/vscode/konscript/syntaxes/konscript.tmLanguage.json" \
       "$DEST/syntaxes/"

    echo "  Installed to $DEST"
    echo "  Restart VS Code to activate."
}

# -----------------------------------------------------------------------
# LSP server
# -----------------------------------------------------------------------
install_lsp() {
    echo "Building and installing konscript-lsp..."

    KS_DIR="$(dirname "$SCRIPT_DIR")"

    g++ -std=c++17 -O2 \
        -I "$KS_DIR/include" \
        "$SCRIPT_DIR/lsp/konscript-lsp.cpp" \
        -o "$SCRIPT_DIR/lsp/konscript-lsp"

    sudo install -m 755 "$SCRIPT_DIR/lsp/konscript-lsp" /usr/local/bin/konscript-lsp

    echo "  Installed to /usr/local/bin/konscript-lsp"
    echo ""
    echo "  Neovim (with nvim-lspconfig):"
    echo "    require('lspconfig.configs').konscript_lsp = {"
    echo "      default_config = {"
    echo "        cmd = { 'konscript-lsp' },"
    echo "        filetypes = { 'konscript' },"
    echo "        root_dir = require('lspconfig.util').find_git_ancestor,"
    echo "      }"
    echo "    }"
    echo "    require('lspconfig').konscript_lsp.setup({})"
}

# -----------------------------------------------------------------------
# Run
# -----------------------------------------------------------------------
echo "KonScript Editor Support Installer"
echo "==================================="
echo ""

[ "$DO_NVIM"   = "1" ] && install_nvim
[ "$DO_VSCODE" = "1" ] && install_vscode
[ "$DO_LSP"    = "1" ] && install_lsp

echo ""
echo "==================================="
echo "Done!"
