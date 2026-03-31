#include "SceneFormat.hpp"
#include <QPointF>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QTextStream>

// -----------------------------------------------------------------------
// Tokenizer
// -----------------------------------------------------------------------
QList<SceneParser::Token> SceneParser::tokenize(const QString& src) {
    QList<Token> toks;
    int i = 0;
    auto peek = [&](int off=0) -> QChar {
        return (i+off < src.size()) ? src[i+off] : QChar(0);
    };

    while (i < src.size()) {
        QChar c = src[i];

        // Skip whitespace
        if (c.isSpace()) { i++; continue; }

        // Skip comments
        if (c == '#' || (c == '/' && peek(1) == '/')) {
            while (i < src.size() && src[i] != '\n') i++;
            continue;
        }
        if (c == '/' && peek(1) == '*') {
            i += 2;
            while (i+1 < src.size() && !(src[i]=='*' && src[i+1]=='/')) i++;
            i += 2;
            continue;
        }

        if (c == '{') { toks.append({Token::LBrace, "{"}); i++; continue; }
        if (c == '}') { toks.append({Token::RBrace, "}"}); i++; continue; }
        if (c == ':') { toks.append({Token::Colon,  ":"}); i++; continue; }
        if (c == '(') { toks.append({Token::LParen, "("}); i++; continue; }
        if (c == ')') { toks.append({Token::RParen, ")"}); i++; continue; }
        if (c == ',') { toks.append({Token::Comma,  ","}); i++; continue; }

        // String
        if (c == '"') {
            i++;
            QString s;
            while (i < src.size() && src[i] != '"') {
                if (src[i] == '\\' && i+1 < src.size()) { i++; s += src[i]; }
                else s += src[i];
                i++;
            }
            i++; // closing "
            toks.append({Token::Str, s});
            continue;
        }

        // Word / number / bool
        if (c.isLetterOrNumber() || c == '_' || c == '-' || c == '.') {
            QString w;
            while (i < src.size() && (src[i].isLetterOrNumber() ||
                   src[i]=='_' || src[i]=='.' || src[i]=='-'))
                w += src[i++];
            toks.append({Token::Word, w});
            continue;
        }

        i++; // skip unknown
    }
    toks.append({Token::Eof, ""});
    return toks;
}

// -----------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------
SceneFile SceneParser::parse(const QString& src, const QString& path) {
    m_tokens = tokenize(src);
    m_pos    = 0;
    m_error  = "";
    SceneFile scene;
    scene.path = path;

    // Expect: scene <Name> { ... }
    if (peek().type == Token::Word && peek().val == "scene") {
        advance();
        if (check(Token::Word) || check(Token::Str))
            scene.sceneName = advance().val;
        if (!match(Token::LBrace)) { m_error = "expected '{' after scene name"; return {}; }
        // Parse children of root
        scene.root.type = "Scene";
        scene.root.name = scene.sceneName;
        while (!check(Token::RBrace) && !check(Token::Eof))
            scene.root.children.append(parseNode());
        match(Token::RBrace);
    }
    return scene;
}

SceneNode SceneParser::parseNode() {
    SceneNode node;
    // Type
    if (check(Token::Word)) node.type = advance().val;
    // Name (optional string)
    if (check(Token::Str) || check(Token::Word))
        node.name = advance().val;
    // Props + children
    if (match(Token::LBrace)) {
        while (!check(Token::RBrace) && !check(Token::Eof)) {
            // Could be prop: key: value, or child node: Type "Name" {
            if (peek().type == Token::Word && peek(1).type == Token::Colon) {
                // property
                QString key = advance().val;
                advance(); // colon
                node.props[key] = parseValue();
            } else {
                // child node
                node.children.append(parseNode());
            }
        }
        match(Token::RBrace);
    }
    return node;
}

QVariant SceneParser::parseValue() {
    // Vec2: (x, y)
    if (match(Token::LParen)) {
        double x = 0, y = 0;
        if (check(Token::Word)) x = advance().val.toDouble();
        match(Token::Comma);
        if (check(Token::Word)) y = advance().val.toDouble();
        match(Token::RParen);
        return QVariant(QPointF(x, y));
    }
    // Bool
    if (check(Token::Word) && (peek().val == "true" || peek().val == "false"))
        return advance().val == "true";
    // Number
    if (check(Token::Word)) {
        QString v = advance().val;
        bool ok; double d = v.toDouble(&ok);
        return ok ? QVariant(d) : QVariant(v);
    }
    // String
    if (check(Token::Str)) return advance().val;
    return QVariant();
}

// -----------------------------------------------------------------------
// Serializer
// -----------------------------------------------------------------------
QString SceneParser::serialize(const SceneFile& scene) {
    QString out;
    out += "scene " + scene.sceneName + " {\n";
    for (auto& child : scene.root.children)
        out += serializeNode(child, 1);
    out += "}\n";
    return out;
}

QString SceneParser::serializeNode(const SceneNode& n, int indent) {
    QString pad(indent * 4, ' ');
    QString out;
    out += pad + n.type + " \"" + n.name + "\"";
    if (n.props.isEmpty() && n.children.isEmpty()) { out += " {}\n"; return out; }
    out += " {\n";
    for (auto it = n.props.begin(); it != n.props.end(); ++it) {
        out += pad + "    " + it.key() + ": ";
        auto v = it.value();
        if (v.canConvert<QPointF>()) {
            auto p = v.toPointF();
            out += QString("(%1, %2)").arg(p.x()).arg(p.y());
        } else if (v.type() == QVariant::Bool) {
            out += v.toBool() ? "true" : "false";
        } else if (v.type() == QVariant::Double || v.type() == QVariant::Int) {
            out += QString::number(v.toDouble());
        } else {
            out += "\"" + v.toString() + "\"";
        }
        out += "\n";
    }
    for (auto& child : n.children)
        out += serializeNode(child, indent+1);
    out += pad + "}\n";
    return out;
}

// -----------------------------------------------------------------------
// Script analyzer — detects what needs updating
// -----------------------------------------------------------------------
static void collectNodes(const SceneNode& node, QMap<QString,QString>& out) {
    // map name -> type
    if (!node.name.isEmpty() && node.name != node.type)
        out[node.name] = node.type;
    for (auto& c : node.children) collectNodes(c, out);
}

QList<ScriptChange> ScriptAnalyzer::detectChanges(
    const SceneFile& oldS, const SceneFile& newS, const QString& projectRoot)
{
    QList<ScriptChange> changes;

    QMap<QString,QString> oldNodes, newNodes;
    collectNodes(oldS.root, oldNodes);
    collectNodes(newS.root, newNodes);

    // Renamed nodes
    for (auto it = oldNodes.begin(); it != oldNodes.end(); ++it) {
        QString oldName = it.key();
        QString oldType = it.value();
        if (!newNodes.contains(oldName)) {
            // Check if same type exists with new name (rename detection)
            for (auto jt = newNodes.begin(); jt != newNodes.end(); ++jt) {
                if (jt.value() == oldType && !oldNodes.contains(jt.key())) {
                    // Scan scripts for GetNode("oldName")
                    QDirIterator di(projectRoot + "/src",
                        {"*.ks"}, QDir::Files, QDirIterator::Subdirectories);
                    while (di.hasNext()) {
                        QString f = di.next();
                        QFile file(f); file.open(QIODevice::ReadOnly);
                        QString src = QTextStream(&file).readAll();
                        if (src.contains("\"" + oldName + "\"") ||
                            src.contains("GetNode(\"" + oldName + "\")")) {
                            ScriptChange c;
                            c.kind       = ScriptChange::NodeRenamed;
                            c.oldValue   = oldName;
                            c.newValue   = jt.key();
                            c.scriptPath = f;
                            c.description = QString("Node \"%1\" was renamed to \"%2\"\n"
                                "Script references to \"%1\" should be updated.")
                                .arg(oldName, jt.key());
                            changes.append(c);
                        }
                    }
                    break;
                }
            }
            // Deleted node
            if (changes.isEmpty() || changes.last().kind != ScriptChange::NodeRenamed) {
                QDirIterator di(projectRoot + "/src",
                    {"*.ks"}, QDir::Files, QDirIterator::Subdirectories);
                while (di.hasNext()) {
                    QString f = di.next();
                    QFile file(f); file.open(QIODevice::ReadOnly);
                    QString src = QTextStream(&file).readAll();
                    if (src.contains("\"" + oldName + "\"")) {
                        ScriptChange c;
                        c.kind        = ScriptChange::NodeDeleted;
                        c.oldValue    = oldName;
                        c.scriptPath  = f;
                        c.description = QString("Node \"%1\" was deleted.\n"
                            "Script still references \"%1\" — consider removing.")
                            .arg(oldName);
                        changes.append(c);
                    }
                }
            }
        }
        // Type changed
        else if (newNodes.contains(oldName) && newNodes[oldName] != oldType) {
            ScriptChange c;
            c.kind        = ScriptChange::NodeTypeChanged;
            c.oldValue    = oldType;
            c.newValue    = newNodes[oldName];
            c.scriptPath  = "";
            c.description = QString("Node \"%1\" changed type from %2 to %3.")
                .arg(oldName, oldType, newNodes[oldName]);
            changes.append(c);
        }
    }

    return changes;
}

QString ScriptAnalyzer::applyChange(const QString& src, const ScriptChange& change) {
    QString out = src;
    if (change.kind == ScriptChange::NodeRenamed) {
        out.replace("\"" + change.oldValue + "\"", "\"" + change.newValue + "\"");
        out.replace("GetNode(\"" + change.oldValue + "\")",
                    "GetNode(\"" + change.newValue + "\")");
    }
    return out;
}

QString ScriptAnalyzer::generateScript(const QString& nodeName, const QString& nodeType) {
    QString s;
    s += "# " + nodeName + ".ks\n";
    s += "#include <engine>\n\n";
    s += "node " + nodeName + " : " + nodeType + " {\n";

    if (nodeType == "KinematicBody2D") {
        s += "    let mut speed:    F64 = 200.0;\n";
        s += "    let mut velY:     F64 = 0.0;\n";
        s += "    let mut gravity:  F64 = 900.0;\n";
        s += "    let mut grounded: Bool = false;\n";
        s += "    let mut width:    F64 = 32.0;\n";
        s += "    let mut height:   F64 = 48.0;\n\n";
        s += "    func Ready() {\n";
        s += "        x = 0.0;\n";
        s += "        y = 0.0;\n";
        s += "        scaleX = 1.0;\n";
        s += "        scaleY = 1.0;\n";
        s += "    }\n\n";
        s += "    func Update(dt: F64) {\n";
        s += "        if KeyDown(Key.D) { x += speed * dt; }\n";
        s += "        if KeyDown(Key.A) { x -= speed * dt; }\n";
        s += "        velY += gravity * dt;\n";
        s += "        y    += velY * dt;\n";
        s += "    }\n\n";
        s += "    func OnCollisionEnter(other: Collider2D) {\n";
        s += "    }\n";
    } else if (nodeType == "RigidBody2D") {
        s += "    let mut mass: F64 = 1.0;\n\n";
        s += "    func Ready() {\n";
        s += "    }\n\n";
        s += "    func Update(dt: F64) {\n";
        s += "    }\n\n";
        s += "    func OnCollisionEnter(other: Collider2D) {\n";
        s += "    }\n";
    } else if (nodeType == "StaticBody2D") {
        s += "    func Ready() {\n";
        s += "    }\n";
    } else if (nodeType == "Area2D") {
        s += "    func Ready() {\n";
        s += "    }\n\n";
        s += "    func OnBodyEntered(body: Collider2D) {\n";
        s += "        Print(\"Body entered\");\n";
        s += "    }\n\n";
        s += "    func OnBodyExited(body: Collider2D) {\n";
        s += "    }\n";
    } else if (nodeType == "Camera2D") {
        s += "    let mut zoom: F64 = 1.0;\n\n";
        s += "    func Ready() {\n";
        s += "    }\n\n";
        s += "    func Update(dt: F64) {\n";
        s += "    }\n";
    } else if (nodeType == "Timer") {
        s += "    let mut waitTime: F64 = 1.0;\n\n";
        s += "    func Ready() {\n";
        s += "    }\n\n";
        s += "    func OnTimeout() {\n";
        s += "        Print(\"Timer fired!\");\n";
        s += "    }\n";
    } else {
        s += "    func Ready() {\n";
        s += "        x = 0.0;\n";
        s += "        y = 0.0;\n";
        s += "    }\n\n";
        s += "    func Update(dt: F64) {\n";
        s += "    }\n";
    }

    s += "}\n";
    return s;
}
