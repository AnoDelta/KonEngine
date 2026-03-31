#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QFormLayout>
#include <QLabel>
#include <QString>
#include <QMap>

// A parsed property from a .ks node file
struct NodeProperty {
    QString name;
    QString type;     // F64, I32, Bool, Str, etc.
    QString value;    // default value as string
    bool    mut = true;
};

class Inspector : public QWidget {
    Q_OBJECT
public:
    explicit Inspector(QWidget* parent = nullptr);

signals:
    void propertyChanged(const QString& nodeName, const QString& prop, const QString& value);
    void scriptFileChanged(const QString& path);

public slots:
    void showNode(const QString& name, const QString& type);
    void showNodeFromFile(const QString& name, const QString& type,
                          const QString& scriptPath);
    void updatePosition(const QString& name, float x, float y);
    void patchScriptValue(const QString& prop, const QString& value);
    void writePropertyToFile(const QString& propName, const QString& value);
    void clear() { showNode("", ""); }

private:

    void buildFromScript(const QString& scriptPath);
    void buildFromType(const QString& type);
    void addProperty(const QString& label, const QString& type,
                     const QString& value, bool editable = true);
    void addSection(const QString& label);
    void addVec2Row(const QString& label, double x, double y);
    void clearForm();

    QList<NodeProperty> parseNodeProperties(const QString& scriptPath);

    QLabel*      m_title   = nullptr;
    QLabel*      m_file    = nullptr;
    QFormLayout* m_form    = nullptr;
    QWidget*     m_content = nullptr;

    QString m_currentNode;
    QString m_currentScript;
};
