# KonScript Editor Support

## Quick Install

```bash
cd tools/KonScript/editor
chmod +x install.sh
./install.sh           # install everything
./install.sh --nvim    # Neovim only
./install.sh --vscode  # VS Code only
./install.sh --lsp     # LSP server only
```

## What Gets Installed

### Neovim
- Filetype detection for `.ks` files
- Syntax highlighting (keywords, types, builtins, strings, comments)
- Indentation settings (4 spaces)
- Keybindings:
  - `F5` — compile and run current file (`ksc %`)
  - `F6` — typecheck only (`ksc --check %`)
  - `F7` — compile to `.cpp` only

### VS Code
- Syntax highlighting via TextMate grammar
- Bracket matching and auto-closing
- Comment toggling (`//` and `/* */`)
- The `.tmLanguage.json` grammar also works in **Zed**, **Sublime Text**, and any editor that supports TextMate grammars

### LSP Server (`konscript-lsp`)
Provides real-time diagnostics as you type — errors from the lexer, parser, and typechecker appear inline in your editor.

**Neovim with nvim-lspconfig:**
```lua
-- In your init.lua or a plugin config file
local lspconfig = require('lspconfig')
local configs   = require('lspconfig.configs')

if not configs.konscript_lsp then
    configs.konscript_lsp = {
        default_config = {
            cmd        = { 'konscript-lsp' },
            filetypes  = { 'konscript' },
            root_dir   = lspconfig.util.find_git_ancestor,
            settings   = {},
        },
    }
end

lspconfig.konscript_lsp.setup({})
```

**VS Code:**
Add a `.vscode/settings.json` to your project:
```json
{
    "konscript.lsp.enabled": true
}
```
*(Full VS Code LSP extension coming soon)*

## Supported Editors

| Editor | Syntax | LSP |
|--------|--------|-----|
| Neovim | ✅ | ✅ (with nvim-lspconfig) |
| VS Code | ✅ | 🔜 |
| Zed | ✅ (copy tmLanguage.json) | 🔜 |
| Sublime Text | ✅ (copy tmLanguage.json) | 🔜 |
| Helix | 🔜 | 🔜 |
| Emacs | 🔜 | 🔜 |

## Manual Install

### Neovim (manual)
Copy the files to your Neovim config:
```bash
~/.config/nvim/ftdetect/konscript.vim
~/.config/nvim/syntax/konscript.vim
~/.config/nvim/ftplugin/konscript.vim
```

### Zed
Copy `vscode/konscript/syntaxes/konscript.tmLanguage.json` to your Zed extensions folder and register it as a language grammar.

### Sublime Text
Copy `konscript.tmLanguage.json` to `~/.config/sublime-text/Packages/KonScript/`.
