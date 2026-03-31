#pragma once
// -----------------------------------------------------------------------
// KonScene — simple text format for describing node trees
//
// scene Level1 {
//     RigidBody2D "Player" {
//         position: (100, 200)
//         rotation: 0
//         script: "src/player.ks"
//
//         Sprite2D "Sprite" {
//             texture: "assets/player.png"
//         }
//     }
//     StaticBody2D "Ground" {
//         position: (0, 500)
//     }
// }
// -----------------------------------------------------------------------
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariantMap>

struct SceneNode {
    QString              type;
    QString              name;
    QVariantMap          props;      // position, rotation, scale, texture, etc.
    QList<SceneNode>     children;
};

struct SceneFile {
    QString           sceneName;
    SceneNode         root;
    QString           path;         // on-disk path

    bool isEmpty() const { return sceneName.isEmpty(); }
};

// -----------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------
class SceneParser {
public:
    SceneFile  parse(const QString& src, const QString& path = "");
    QString    serialize(const SceneFile& scene);
    QString    lastError() const { return m_error; }

private:
    struct Token { enum Type { Word, Str, LBrace, RBrace, Colon, LParen, RParen, Comma, Eof } type; QString val; };

    QList<Token> tokenize(const QString& src);
    SceneNode    parseNode();
    QVariantMap  parseProps();
    QVariant     parseValue();

    QList<Token> m_tokens;
    int          m_pos = 0;
    QString      m_error;

    Token&  peek(int off=0) { int i=m_pos+off; return i<m_tokens.size()?m_tokens[i]:m_tokens.last(); }
    Token   advance()       { return m_tokens[m_pos < m_tokens.size()-1 ? m_pos++ : m_pos]; }
    bool    check(Token::Type t) { return peek().type == t; }
    bool    match(Token::Type t) { if(check(t)){advance();return true;}return false; }

    QString serializeNode(const SceneNode& n, int indent);
};

// -----------------------------------------------------------------------
// Script update detector
// -----------------------------------------------------------------------
struct ScriptChange {
    enum Kind { NodeRenamed, NodeDeleted, NodeTypeChanged, ScriptMissing };
    Kind    kind;
    QString oldValue;
    QString newValue;
    QString scriptPath;
    QString description;
};

class ScriptAnalyzer {
public:
    // Detect what needs to change in scripts when scene changes
    QList<ScriptChange> detectChanges(
        const SceneFile& oldScene,
        const SceneFile& newScene,
        const QString&   projectRoot);

    // Apply a change to a script file, return modified content
    QString applyChange(const QString& src, const ScriptChange& change);

    // Generate starter script for a node type
    QString generateScript(const QString& nodeName, const QString& nodeType);
};
