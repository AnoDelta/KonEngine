#include "Inspector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFrame>

Inspector::Inspector(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    auto* header = new QLabel("  Inspector");
    header->setStyleSheet("background:#242424;color:#aaa;padding:5px 8px;"
                          "font-size:11px;font-weight:bold;");
    layout->addWidget(header);

    m_title = new QLabel("  Nothing selected");
    m_title->setTextFormat(Qt::RichText);
    m_title->setStyleSheet("color:#888;padding:6px 8px;font-size:11px;");
    layout->addWidget(m_title);

    m_file = new QLabel("");
    m_file->setStyleSheet("color:#555;padding:0 8px 4px;font-size:10px;");
    m_file->setVisible(false);
    layout->addWidget(m_file);

    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#333;");
    layout->addWidget(sep);

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea{border:none;background:#1c1c1c;}");
    m_content = new QWidget();
    m_content->setStyleSheet("background:#1c1c1c;");
    m_form = new QFormLayout(m_content);
    m_form->setContentsMargins(8,8,8,8);
    m_form->setSpacing(5);
    m_form->setLabelAlignment(Qt::AlignRight|Qt::AlignVCenter);
    scroll->setWidget(m_content);
    layout->addWidget(scroll);
}

// ── Parse .ks file for node properties ───────────────────────────────────
QList<NodeProperty> Inspector::parseNodeProperties(const QString& path) {
    QList<NodeProperty> props;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return props;
    QString src = QTextStream(&f).readAll();

    // Match: let [mut] name: Type = value;
    // inside a node { } block
    QRegularExpression re(
        R"(let\s+(mut\s+)?(\w+)\s*:\s*(\w+)\s*=\s*([^;]+);)",
        QRegularExpression::MultilineOption);

    auto it = re.globalMatch(src);
    while (it.hasNext()) {
        auto m = it.next();
        NodeProperty p;
        p.mut   = !m.captured(1).trimmed().isEmpty();
        p.name  = m.captured(2);
        p.type  = m.captured(3);
        p.value = m.captured(4).trimmed();

        // Skip internal engine fields (anim, sounds etc loaded from engine)
        if (p.value.startsWith("this.add") ||
            p.value.startsWith("LoadSound") ||
            p.value.startsWith("LoadTexture"))
            continue;

        props.append(p);
    }
    return props;
}

void Inspector::clearForm() {
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);
}

void Inspector::addSection(const QString& label) {
    auto* l = new QLabel(label.toUpper());
    l->setStyleSheet("color:#555;font-size:10px;font-weight:bold;"
                     "padding-top:6px;letter-spacing:1px;");
    m_form->addRow(l);
}

void Inspector::addVec2Row(const QString& label, double x, double y) {
    auto* w  = new QWidget();
    auto* hl = new QHBoxLayout(w);
    hl->setContentsMargins(0,0,0,0);
    hl->setSpacing(3);

    auto mkLabel = [](const QString& t, const QString& col) {
        auto* l = new QLabel(t);
        l->setStyleSheet("color:"+col+";font-size:10px;font-weight:bold;");
        l->setFixedWidth(10);
        return l;
    };
    auto mkSpin = [](double v) {
        auto* s = new QDoubleSpinBox();
        s->setRange(-99999,99999);
        s->setValue(v);
        s->setDecimals(2);
        s->setSingleStep(1.0);
        s->setStyleSheet("background:#252525;color:#ddd;border:1px solid #3a3a3a;");
        return s;
    };

    auto* sx = mkSpin(x);
    auto* sy = mkSpin(y);
    hl->addWidget(mkLabel("X","#e06c75"));
    hl->addWidget(sx);
    hl->addWidget(mkLabel("Y","#98c379"));
    hl->addWidget(sy);
    connect(sx, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v){ patchScriptValue("x", QString::number(v,'f',3)); });
    connect(sy, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v){ patchScriptValue("y", QString::number(v,'f',3)); });
    m_form->addRow(label+":", w);
}

void Inspector::addProperty(const QString& label, const QString& type,
                             const QString& value, bool editable) {
    QWidget* w = nullptr;

    if (type == "F64" || type == "F32") {
        auto* s = new QDoubleSpinBox();
        s->setRange(-99999,99999);
        s->setValue(value.toDouble());
        s->setDecimals(3);
        s->setSingleStep(1.0);
        s->setEnabled(editable);
        s->setStyleSheet("background:#252525;color:#ddd;border:1px solid #3a3a3a;");
        QString lbl = label;
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [this, lbl](double v){
                    QString val = QString::number(v, 'f', 3);
                    emit propertyChanged(m_currentNode, lbl, val);
                    patchScriptValue(lbl, val);
                });
        w = s;
    } else if (type == "I32" || type == "I64" || type == "I8") {
        auto* s = new QSpinBox();
        s->setRange(-99999,99999);
        s->setValue(value.toInt());
        s->setEnabled(editable);
        s->setStyleSheet("background:#252525;color:#ddd;border:1px solid #3a3a3a;");
        w = s;
    } else if (type == "Bool") {
        auto* cb = new QCheckBox();
        cb->setChecked(value == "true");
        cb->setEnabled(editable);
        {
            QString lbl2 = label;
            connect(cb, &QCheckBox::toggled, [this, lbl2](bool v){
                writePropertyToFile(lbl2, v ? "true" : "false");
            });
        }
        w = cb;
    } else {
        // String / unknown
        auto* e = new QLineEdit(value);
        e->setEnabled(editable);
        e->setStyleSheet("background:#252525;color:#ddd;border:1px solid #3a3a3a;");
        w = e;
    }

    if (w) m_form->addRow(label+":", w);
}

void Inspector::showNode(const QString& name, const QString& type) {
    clearForm();
    m_currentNode = name;

    if (name.isEmpty()) {
        m_title->setText("  Nothing selected");
        m_file->setVisible(false);
        return;
    }

    m_title->setText(QString("  <b>%1</b> <span style='color:#555;font-size:10px;'>%2</span>")
                     .arg(name, type));
    buildFromType(type);
}

void Inspector::showNodeFromFile(const QString& name, const QString& type,
                                  const QString& scriptPath) {
    clearForm();
    m_currentNode   = name;
    m_currentScript = scriptPath;

    m_title->setText(QString("  <b>%1</b> <span style='color:#555;font-size:10px;'>%2</span>")
                     .arg(name, type));

    if (!scriptPath.isEmpty() && QFile::exists(scriptPath)) {
        m_file->setText("  📝 " + QFileInfo(scriptPath).fileName());
        m_file->setVisible(true);
        buildFromScript(scriptPath);
    } else {
        m_file->setVisible(false);
        buildFromType(type);
    }
}

void Inspector::buildFromScript(const QString& path) {
    auto props = parseNodeProperties(path);

    // Always show transform first
    addSection("Transform");
    addVec2Row("position", 0, 0);
    addProperty("rotation", "F64", "0.0");
    addVec2Row("scale", 1, 1);

    if (!props.isEmpty()) {
        addSection("Script Properties");
        for (auto& p : props) {
            // Skip transform fields — already shown above
            if (p.name == "x" || p.name == "y" ||
                p.name == "scaleX" || p.name == "scaleY" ||
                p.name == "rotation") continue;
            addProperty(p.name, p.type, p.value, p.mut);
        }
    }

    // Script attachment row
    addSection("Script");
    auto* row = new QHBoxLayout();
    auto* edit = new QLineEdit(QFileInfo(path).fileName());
    edit->setReadOnly(true);
    edit->setStyleSheet("background:#252525;color:#777;border:1px solid #3a3a3a;");
    auto* openBtn = new QPushButton("Open");
    openBtn->setFixedWidth(45);
    openBtn->setFixedHeight(22);
    QString p = path;
    connect(openBtn, &QPushButton::clicked, [p, this]{
        emit propertyChanged(m_currentNode, "__openScript__", p);
    });
    row->addWidget(edit);
    row->addWidget(openBtn);
    auto* rw = new QWidget(); rw->setLayout(row);
    m_form->addRow("file:", rw);
}

void Inspector::buildFromType(const QString& type) {
    // Node2D base
    if (type != "Node") {
        addSection("Transform");
        addVec2Row("position", 0, 0);
        addProperty("rotation", "F64", "0.0");
        addVec2Row("scale", 1, 1);
    }

    addSection("Node");
    addProperty("visible", "Bool", "true");
    addProperty("active",  "Bool", "true");

    if (type == "RigidBody2D") {
        addSection("Physics");
        addProperty("mass",        "F64", "1.0");
        addProperty("gravityScale","F64", "1.0");
        addProperty("linearDamp",  "F64", "0.0");
        addProperty("angularDamp", "F64", "0.0");
        addProperty("freezed",     "Bool","false");
    } else if (type == "KinematicBody2D") {
        addSection("Physics");
        addProperty("floorAngle",    "F64", "45.0");
        addProperty("slideOnCeiling","Bool","true");
    } else if (type == "CollisionShape2D") {
        addSection("Shape");
        addProperty("width",  "F64","32.0");
        addProperty("height", "F64","32.0");
        addProperty("radius", "F64","16.0");
    } else if (type == "Sprite2D" || type == "AnimatedSprite2D") {
        addSection("Sprite");
        addProperty("texture","Str","");
        addVec2Row("offset", 0, 0);
        addProperty("flipH","Bool","false");
        addProperty("flipV","Bool","false");
        if (type == "AnimatedSprite2D") {
            addSection("Animation");
            addProperty("animation","Str","default");
            addProperty("speed",    "F64","5.0");
            addProperty("autoplay", "Bool","false");
        }
    } else if (type == "Camera2D") {
        addSection("Camera");
        addProperty("zoom",        "F64","1.0");
        addProperty("current",     "Bool","false");
        addProperty("smoothing",   "Bool","false");
        addProperty("smoothSpeed", "F64","5.0");
    } else if (type == "Label") {
        addSection("Label");
        addProperty("text",    "Str","");
        addProperty("fontSize","F64","14.0");
    } else if (type == "Timer") {
        addSection("Timer");
        addProperty("waitTime","F64","1.0");
        addProperty("autostart","Bool","false");
        addProperty("oneShot",  "Bool","true");
    } else if (type == "AudioPlayer") {
        addSection("Audio");
        addProperty("stream",  "Str","");
        addProperty("volume",  "F64","1.0");
        addProperty("autoplay","Bool","false");
        addProperty("loop",    "Bool","false");
    }

    // No script attached yet
    addSection("Script");
    auto* row = new QHBoxLayout();
    auto* edit = new QLineEdit("No script");
    edit->setReadOnly(true);
    edit->setStyleSheet("background:#252525;color:#555;border:1px solid #3a3a3a;");
    auto* attachBtn = new QPushButton("Attach");
    attachBtn->setFixedWidth(50);
    attachBtn->setFixedHeight(22);
    connect(attachBtn, &QPushButton::clicked, [this]{
        emit propertyChanged(m_currentNode, "__attachScript__", "");
    });
    row->addWidget(edit);
    row->addWidget(attachBtn);
    auto* rw = new QWidget(); rw->setLayout(row);
    m_form->addRow("script:", rw);
}

void Inspector::writePropertyToFile(const QString& propName, const QString& value) {
    if (m_currentScript.isEmpty() || propName.isEmpty() || value.isEmpty()) return;
    QFile f(m_currentScript);
    if (!f.open(QIODevice::ReadOnly)) return;
    QString src = QTextStream(&f).readAll();
    f.close();

    // Find Ready() block
    QRegularExpression reReady(R"(func\s+Ready\s*\(\s*\)\s*\{)",
        QRegularExpression::NoPatternOption);
    auto rm = reReady.match(src);
    if (!rm.hasMatch()) return;

    // Find matching closing brace
    int start = rm.capturedEnd();
    int depth = 1, pos = start;
    while (pos < src.size() && depth > 0) {
        if (src[pos] == QLatin1Char('{')) depth++;
        else if (src[pos] == QLatin1Char('}')) depth--;
        if (depth > 0) pos++;
    }
    QString body = src.mid(start, pos - start);

    // Remove ALL existing assignments to propName
    QRegularExpression reAssign(
        QString(R"(\n?[ \t]*\b%1\s*=[^;]+;)")
            .arg(QRegularExpression::escape(propName)));
    body.remove(reAssign);

    // Prepend single clean assignment
    body.prepend(QString("\n        %1 = %2;").arg(propName, value));

    src.replace(start, pos - start, body);

    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream(&f) << src;
        f.close();
        emit scriptFileChanged(m_currentScript);
    }
}

void Inspector::patchScriptValue(const QString& prop, const QString& value) {
    // Delegate to writePropertyToFile which handles let mut matching
    writePropertyToFile(prop, value);
}

void Inspector::updatePosition(const QString&, float x, float y) {
    auto spins = m_content->findChildren<QDoubleSpinBox*>();
    if (spins.size() >= 2) {
        spins[0]->blockSignals(true);
        spins[1]->blockSignals(true);
        spins[0]->setValue(x);
        spins[1]->setValue(y);
        spins[0]->blockSignals(false);
        spins[1]->blockSignals(false);
    }
}
