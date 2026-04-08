#!/usr/bin/env python3
"""Generate docs/index.html from all project markdown files.

Usage:
    python3 tools/gen_docs_html.py

Reads:  DOCS.md, tools/KonScript/DOCS.md, ROADMAP.md, README.md, etc.
Writes: docs/index.html (single self-contained HTML file)
"""
import os, re, html

REPO_ROOT = os.path.join(os.path.dirname(__file__), "..")

# Which docs to include, in order
DOC_FILES = [
    ("README.md",                  "README"),
    ("DOCS.md",                    "Engine Docs"),
    ("tools/KonScript/DOCS.md",    "KonScript Reference"),
    ("ROADMAP.md",                 "Roadmap"),
]

def read_file(path):
    full = os.path.join(REPO_ROOT, path)
    if not os.path.isfile(full):
        return None
    with open(full, "r") as f:
        return f.read()

def md_to_html(md):
    """Minimal markdown → HTML converter. Handles the subset used in our docs."""
    lines = md.split("\n")
    out = []
    in_code = False
    in_table = False
    in_list = False
    code_lang = ""

    for line in lines:
        # Fenced code blocks
        if line.strip().startswith("```"):
            if in_code:
                out.append("</code></pre>")
                in_code = False
            else:
                code_lang = line.strip()[3:].strip()
                cls = f' class="language-{code_lang}"' if code_lang else ""
                out.append(f"<pre><code{cls}>")
                in_code = True
            continue
        if in_code:
            out.append(html.escape(line))
            continue

        # Tables
        if "|" in line and line.strip().startswith("|"):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            # Skip separator rows (|---|---|)
            if all(re.match(r'^[-:]+$', c) for c in cells):
                continue
            if not in_table:
                out.append('<table>')
                tag = "th"
                in_table = True
            else:
                tag = "td"
            row = "".join(f"<{tag}>{inline(c)}</{tag}>" for c in cells)
            out.append(f"<tr>{row}</tr>")
            continue
        elif in_table:
            out.append("</table>")
            in_table = False

        stripped = line.strip()

        # Empty line
        if not stripped:
            if in_list:
                out.append("</ul>")
                in_list = False
            out.append("")
            continue

        # Blockquote
        if stripped.startswith("> "):
            out.append(f'<blockquote>{inline(stripped[2:])}</blockquote>')
            continue

        # Horizontal rule
        if stripped in ("---", "***", "___"):
            if in_list:
                out.append("</ul>")
                in_list = False
            out.append("<hr>")
            continue

        # Headers
        m = re.match(r'^(#{1,6})\s+(.*)', stripped)
        if m:
            if in_list:
                out.append("</ul>")
                in_list = False
            level = len(m.group(1))
            text = m.group(2)
            slug = re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')
            out.append(f'<h{level} id="{slug}">{inline(text)}</h{level}>')
            continue

        # Unordered list
        if re.match(r'^[-*+]\s', stripped):
            if not in_list:
                out.append("<ul>")
                in_list = True
            out.append(f"<li>{inline(stripped[2:])}</li>")
            continue

        # Numbered list
        m2 = re.match(r'^(\d+)\.\s(.*)', stripped)
        if m2:
            if not in_list:
                out.append("<ul>")
                in_list = True
            out.append(f"<li>{inline(m2.group(2))}</li>")
            continue

        # Paragraph
        if in_list:
            out.append("</ul>")
            in_list = False
        out.append(f"<p>{inline(stripped)}</p>")

    if in_code:
        out.append("</code></pre>")
    if in_table:
        out.append("</table>")
    if in_list:
        out.append("</ul>")

    return "\n".join(out)

def inline(text):
    """Convert inline markdown: bold, italic, code, links, images."""
    # Inline code (must come first to protect content inside backticks)
    text = re.sub(r'`([^`]+)`', r'<code>\1</code>', text)
    # Bold + italic
    text = re.sub(r'\*\*\*(.+?)\*\*\*', r'<strong><em>\1</em></strong>', text)
    # Bold
    text = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
    # Italic
    text = re.sub(r'\*(.+?)\*', r'<em>\1</em>', text)
    # Images
    text = re.sub(r'!\[([^\]]*)\]\(([^)]+)\)', r'<img src="\2" alt="\1">', text)
    # Links
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'<a href="\2">\1</a>', text)
    return text

def extract_toc(md):
    """Extract headers for sidebar navigation."""
    toc = []
    for line in md.split("\n"):
        m = re.match(r'^(#{1,3})\s+(.*)', line.strip())
        if m:
            level = len(m.group(1))
            text = m.group(2)
            slug = re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')
            toc.append((level, text, slug))
    return toc

CSS = """
:root {
    --bg: #1a1b2e;
    --sidebar-bg: #141524;
    --card-bg: #222339;
    --text: #ccd0e0;
    --text-dim: #8888aa;
    --accent: #50a0ff;
    --accent2: #7b68ee;
    --border: #333456;
    --code-bg: #1c1d30;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: var(--bg); color: var(--text);
    display: flex; min-height: 100vh;
}
#sidebar {
    width: 280px; min-width: 280px; background: var(--sidebar-bg);
    border-right: 1px solid var(--border); padding: 20px 0;
    overflow-y: auto; position: fixed; height: 100vh;
}
#sidebar h2 { color: var(--accent); padding: 0 20px 15px; font-size: 18px; }
#sidebar .doc-group { margin-bottom: 8px; }
#sidebar .doc-group-title {
    display: block; padding: 10px 20px; color: var(--text);
    font-weight: 600; font-size: 14px; cursor: pointer;
    border-left: 3px solid transparent; transition: 0.15s;
}
#sidebar .doc-group-title:hover,
#sidebar .doc-group-title.active {
    background: var(--card-bg); border-left-color: var(--accent); color: var(--accent);
}
#sidebar .toc-list { display: none; padding: 0; }
#sidebar .doc-group.open .toc-list { display: block; }
#sidebar .toc-item {
    display: block; padding: 4px 20px 4px 30px; color: var(--text-dim);
    font-size: 13px; text-decoration: none; transition: 0.1s;
}
#sidebar .toc-item:hover { color: var(--accent); background: rgba(80,160,255,0.05); }
#sidebar .toc-item.h3 { padding-left: 45px; font-size: 12px; }
#content {
    margin-left: 280px; flex: 1; padding: 40px 60px; max-width: 900px;
}
.doc-section { display: none; }
.doc-section.active { display: block; }
h1 { color: var(--accent); font-size: 28px; margin: 0 0 20px; border-bottom: 1px solid var(--border); padding-bottom: 10px; }
h2 { color: var(--accent2); font-size: 22px; margin: 35px 0 12px; }
h3 { color: var(--text); font-size: 17px; margin: 25px 0 8px; }
h4, h5, h6 { color: var(--text-dim); margin: 15px 0 5px; }
p { line-height: 1.7; margin: 8px 0; }
a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
pre {
    background: var(--code-bg); border: 1px solid var(--border); border-radius: 6px;
    padding: 14px 18px; overflow-x: auto; margin: 10px 0; font-size: 13px; line-height: 1.5;
}
code { font-family: 'JetBrains Mono', 'Fira Code', 'Consolas', monospace; font-size: 13px; }
p code, li code, td code {
    background: var(--code-bg); padding: 2px 6px; border-radius: 3px;
    border: 1px solid var(--border);
}
table {
    border-collapse: collapse; margin: 12px 0; width: 100%;
}
th, td {
    border: 1px solid var(--border); padding: 8px 12px; text-align: left; font-size: 14px;
}
th { background: var(--card-bg); color: var(--accent); font-weight: 600; }
tr:nth-child(even) { background: rgba(255,255,255,0.02); }
ul { padding-left: 24px; margin: 8px 0; }
li { line-height: 1.7; margin: 2px 0; }
blockquote {
    border-left: 3px solid var(--accent2); padding: 8px 16px; margin: 12px 0;
    background: rgba(123,104,238,0.08); color: var(--text-dim); font-style: italic;
}
hr { border: none; border-top: 1px solid var(--border); margin: 30px 0; }
img { max-width: 100%; }
@media (max-width: 800px) {
    #sidebar { display: none; }
    #content { margin-left: 0; padding: 20px; }
}
"""

JS = """
document.addEventListener('DOMContentLoaded', () => {
    const groups = document.querySelectorAll('.doc-group');
    const sections = document.querySelectorAll('.doc-section');

    function showSection(idx) {
        sections.forEach((s, i) => s.classList.toggle('active', i === idx));
        groups.forEach((g, i) => {
            g.classList.toggle('open', i === idx);
            g.querySelector('.doc-group-title').classList.toggle('active', i === idx);
        });
    }

    groups.forEach((g, i) => {
        g.querySelector('.doc-group-title').addEventListener('click', () => showSection(i));
    });

    // Show first section by default
    showSection(0);
});
"""

def main():
    os.makedirs(os.path.join(REPO_ROOT, "docs"), exist_ok=True)

    sidebar_html = ""
    content_html = ""

    for i, (path, title) in enumerate(DOC_FILES):
        md = read_file(path)
        if md is None:
            continue

        # Sidebar: doc group with TOC
        toc = extract_toc(md)
        toc_items = ""
        for level, text, slug in toc:
            if level > 3:
                continue
            cls = "toc-item" + (" h3" if level == 3 else "")
            toc_items += f'<a class="{cls}" href="#{slug}" onclick="document.querySelectorAll(\'.doc-section\').forEach((s,j)=>s.classList.toggle(\'active\',j==={i}))">{text}</a>\n'

        sidebar_html += f"""
        <div class="doc-group">
            <span class="doc-group-title">{title}</span>
            <div class="toc-list">{toc_items}</div>
        </div>"""

        # Content section
        body = md_to_html(md)
        content_html += f'<div class="doc-section">{body}</div>\n'

    out_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>KonEngine Documentation</title>
<style>{CSS}</style>
</head>
<body>
<nav id="sidebar">
    <h2>KonEngine Docs</h2>
    {sidebar_html}
</nav>
<main id="content">
    {content_html}
</main>
<script>{JS}</script>
</body>
</html>"""

    out_path = os.path.join(REPO_ROOT, "docs", "index.html")
    with open(out_path, "w") as f:
        f.write(out_html)
    print(f"Generated: {out_path}")

if __name__ == "__main__":
    main()
