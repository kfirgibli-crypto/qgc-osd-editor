/****************************************************************************
 *
 * (c) 2026 QGroundControl. https://qgroundcontrol.com
 *
 ****************************************************************************/

#include "APMOSDComponentController.h"

#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QVector>
#include <QtDebug>
#include <QtMath>

// ---------------------------------------------------------------------------
// Element catalogue. Mirrors ELEMENTS in prototype/osd-params.js exactly.
// IMPORTANT: keep these two lists in sync. The JS test
//   `Element catalogue identical` (in test-inline-parity.js) checks
//   prototype consistency; this C++ list is the third copy that the QGC
//   port consumes. If you change one, change all three.
// ---------------------------------------------------------------------------
namespace {

struct ElementDef {
    const char* key;     // matches AP_OSD parameter suffix, e.g. "BAT_VOLT"
    const char* label;   // display name in palette
    int         width;   // visual width hint (cells) for canvas overlap calc
    const char* sample;  // mock value for canvas rendering
};

constexpr ElementDef kElements[] = {
    { "ALTITUDE", "Altitude",            6,  "142m"   },
    { "BAT_VOLT", "Battery voltage",     6,  "16.2V"  },
    { "CURRENT",  "Current",             5,  "8.2A"   },
    { "BATUSED",  "mAh consumed",        7,  "850mAh" },
    { "AVGCELLV", "Avg cell V",          7,  "3.85V"  },
    { "RSSI",     "RSSI",                5,  "98%"    },
    { "SATS",     "GPS sats",            5,  "S14"    },
    { "HDOP",     "HDOP",                6,  "H0.8"   },
    { "FLTMODE",  "Flight mode",         7,  "LOITER" },
    { "GSPEED",   "Ground speed",        6,  "12m/s"  },
    { "ASPEED",   "Airspeed",            6,  "14m/s"  },
    { "VSPEED",   "Climb rate",          6,  "+1.2"   },
    { "HORIZON",  "Artificial horizon", 13,  "-------*-------" },
    { "HOME",     "Home dist+dir",       8,  "^ 142m" },
    { "HEADING",  "Heading",             7,  "147"    },
    { "THROTTLE", "Throttle %",          5,  "64%"    },
    { "COMPASS",  "Compass rose",        9,  "N . E ." },
    { "FLTIME",   "Flight time",         7,  "04:23"  },
    { "DIST",     "Distance flown",      6,  "2.4km"  },
    { "MESSAGE",  "Status messages",    20,  "GPS: 3D Fix"   },
    { "CRSSHAIR", "Crosshair",           3,  "+"      },
    { "CLK",      "Clock",               6,  "14:32"  },
    { "WIND",     "Wind",                6,  "6m/s"   },
    { "STATS",    "Flight stats",       10,  "FT 04:23" },
    { "ARMING",   "Arming status",       6,  "ARMED"  },
};

// Resolution table. Indexed by OSD{n}_TXT_RES value.
struct ResolutionDef {
    int cols;
    int rows;
    const char* name;
};

constexpr ResolutionDef kResolutions[] = {
    /* 0 */ { 30, 16, "SD 30x16"      },
    /* 1 */ { 50, 18, "HD 50x18"      },
    /* 2 */ { 60, 22, "HD 60x22 DP"   },
};
constexpr int kResolutionCount = sizeof(kResolutions) / sizeof(kResolutions[0]);

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
APMOSDComponentController::APMOSDComponentController(void)
    : FactPanelController()
{
    _rebindResolutionFact();
}

// ---------------------------------------------------------------------------
// Active screen
// ---------------------------------------------------------------------------
void APMOSDComponentController::setActiveScreen(int n)
{
    if (n < 1 || n > 4) return;
    if (n == _activeScreen) return;
    _activeScreen = n;
    _rebindResolutionFact();
    emit activeScreenChanged();
    emit resolutionChanged();
}

QStringList APMOSDComponentController::elementKeys() const
{
    QStringList out;
    for (const auto& e : kElements) {
        out << QString::fromLatin1(e.key);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Resolution
// ---------------------------------------------------------------------------
int APMOSDComponentController::maxX() const
{
    Fact* f = const_cast<APMOSDComponentController*>(this)->_lookupFact(
        QString("OSD%1_TXT_RES").arg(_activeScreen));
    int res = f ? f->rawValue().toInt() : 2;
    if (res < 0 || res >= kResolutionCount) res = 2;
    return kResolutions[res].cols - 1;
}

int APMOSDComponentController::maxY() const
{
    Fact* f = const_cast<APMOSDComponentController*>(this)->_lookupFact(
        QString("OSD%1_TXT_RES").arg(_activeScreen));
    int res = f ? f->rawValue().toInt() : 2;
    if (res < 0 || res >= kResolutionCount) res = 2;
    return kResolutions[res].rows - 1;
}

QString APMOSDComponentController::resolutionName() const
{
    Fact* f = const_cast<APMOSDComponentController*>(this)->_lookupFact(
        QString("OSD%1_TXT_RES").arg(_activeScreen));
    int res = f ? f->rawValue().toInt() : 2;
    if (res < 0 || res >= kResolutionCount) res = 2;
    return QString::fromLatin1(kResolutions[res].name);
}

void APMOSDComponentController::_rebindResolutionFact()
{
    if (_boundTxtResFact) {
        disconnect(_boundTxtResFact, &Fact::rawValueChanged,
                   this, &APMOSDComponentController::_onTxtResChanged);
        _boundTxtResFact = nullptr;
    }
    _boundTxtResFact = _lookupFact(QString("OSD%1_TXT_RES").arg(_activeScreen));
    if (_boundTxtResFact) {
        connect(_boundTxtResFact, &Fact::rawValueChanged,
                this, &APMOSDComponentController::_onTxtResChanged);
    }
}

void APMOSDComponentController::_onTxtResChanged()
{
    emit resolutionChanged();
}

// ---------------------------------------------------------------------------
// Fact lookups
// ---------------------------------------------------------------------------
QString APMOSDComponentController::_paramName(const QString& suffix) const
{
    return QString("OSD%1_%2").arg(_activeScreen).arg(suffix);
}

Fact* APMOSDComponentController::_lookupFact(const QString& name)
{
    if (!_vehicle) return nullptr;
    if (!_vehicle->parameterManager()->parameterExists(
            ParameterManager::defaultComponentId, name)) {
        // qCDebug here in real impl. Conditional elements are normal absences.
        return nullptr;
    }
    return _vehicle->parameterManager()->getParameter(
        ParameterManager::defaultComponentId, name);
}

Fact* APMOSDComponentController::elEnabledFact(const QString& key) {
    return _lookupFact(_paramName(key + "_EN"));
}
Fact* APMOSDComponentController::elXFact(const QString& key) {
    return _lookupFact(_paramName(key + "_X"));
}
Fact* APMOSDComponentController::elYFact(const QString& key) {
    return _lookupFact(_paramName(key + "_Y"));
}

Fact* APMOSDComponentController::screenEnabledFact() {
    return _lookupFact(_paramName("ENABLE"));
}
Fact* APMOSDComponentController::screenTxtResFact() {
    return _lookupFact(_paramName("TXT_RES"));
}
Fact* APMOSDComponentController::screenChanMinFact() {
    return _lookupFact(_paramName("CHAN_MIN"));
}
Fact* APMOSDComponentController::screenChanMaxFact() {
    return _lookupFact(_paramName("CHAN_MAX"));
}

QVariantMap APMOSDComponentController::elementInfo(const QString& key) const
{
    QVariantMap m;
    for (const auto& e : kElements) {
        if (key == QString::fromLatin1(e.key)) {
            m["label"]  = QString::fromLatin1(e.label);
            m["width"]  = e.width;
            m["sample"] = QString::fromLatin1(e.sample);
            return m;
        }
    }
    return m;
}

// ---------------------------------------------------------------------------
// importParamText
//
// Mirrors `parseParamFile` + `mergeIntoState` in osd-params.js.
//
// Test mapping (every JS test below must have a Qt Test analogue):
//   - "Parses standard Mission Planner comma format"
//   - "Parses whitespace-separated format"
//   - "Strips inline comments"
//   - "Ignores unknown elements without error"
//   - "Distinguishes per-screen params"
//   - "Handles empty input"
//   - "Handles CRLF line endings"
//   - "Handles multi-underscore element keys like BAT_VOLT"
//   - "Ignores non-OSD params"
// ---------------------------------------------------------------------------
QString APMOSDComponentController::importParamText(const QString& text)
{
    // Build the catalogue lookup once. JS uses Set; QSet is the Qt analogue.
    static const QSet<QString> kElementKeys = []{
        QSet<QString> s;
        for (const auto& e : kElements) s.insert(QString::fromLatin1(e.key));
        return s;
    }();

    // Cache the regexes; QRegularExpression compilation is non-trivial.
    static const QRegularExpression kLineSplit(QStringLiteral("\\r?\\n"));
    static const QRegularExpression kTokenSplit(QStringLiteral("[,\\s]+"));
    static const QRegularExpression kOsdName(QStringLiteral("^OSD([1-4])_(.+)$"));
    static const QRegularExpression kElField(QStringLiteral("^(.+)_(EN|X|Y)$"));

    int applied = 0;
    const QStringList lines = text.split(kLineSplit);
    for (const QString& raw : lines) {
        // Strip inline comments (everything from first '#' onward) and trim.
        const int hash = raw.indexOf(QLatin1Char('#'));
        const QString line = (hash >= 0 ? raw.left(hash) : raw).trimmed();
        if (line.isEmpty()) continue;

        // Mission Planner uses comma; some tools use whitespace. Handle both.
        const QStringList parts = line.split(kTokenSplit, Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        const QString& name = parts.at(0);

        // JS uses Number() which accepts floats; cast to int via `| 0`.
        // We mirror: parse as double, reject non-finite, then truncate to int.
        bool ok = false;
        const double dval = parts.at(1).toDouble(&ok);
        if (!ok || !qIsFinite(dval)) continue;
        const int value = static_cast<int>(dval);

        // Must look like OSD<n>_<rest> with n in 1..4.
        const QRegularExpressionMatch nameMatch = kOsdName.match(name);
        if (!nameMatch.hasMatch()) continue;  // silently skip non-OSD params
        const int screen = nameMatch.captured(1).toInt();
        const QString rest = nameMatch.captured(2);

        // Per-screen meta keys first (exact match).
        static const QSet<QString> kMetaKeys{
            QStringLiteral("ENABLE"),
            QStringLiteral("TXT_RES"),
            QStringLiteral("CHAN_MIN"),
            QStringLiteral("CHAN_MAX"),
        };
        if (kMetaKeys.contains(rest)) {
            Fact* f = _lookupFact(QStringLiteral("OSD%1_%2").arg(screen).arg(rest));
            if (f) { f->setRawValue(value); ++applied; }
            continue;
        }

        // Per-element: <KEY>_<EN|X|Y>. Note `+` is greedy, so for
        // "BAT_VOLT_X" we get key="BAT_VOLT", field="X". Same as JS.
        const QRegularExpressionMatch elMatch = kElField.match(rest);
        if (!elMatch.hasMatch()) continue;
        const QString elKey = elMatch.captured(1);
        const QString field = elMatch.captured(2);

        if (!kElementKeys.contains(elKey)) continue;  // unknown element, silent

        Fact* f = _lookupFact(
            QStringLiteral("OSD%1_%2_%3").arg(screen).arg(elKey, field));
        if (f) { f->setRawValue(value); ++applied; }
    }

    emit importCompleted(applied);
    emit overlapsChanged();
    return tr("Imported %n parameter(s).", "", applied);
}

// ---------------------------------------------------------------------------
// importParamTextFromUrl / exportParamTextToUrl
//
// Thin file-I/O wrappers for QML's FileDialog, which yields QUrl (typically
// file:/// scheme on desktop). Kept on the controller (rather than a separate
// FileIO singleton) because the only callers are import/export and the only
// transforms are read-text and write-text.
// ---------------------------------------------------------------------------
QString APMOSDComponentController::importParamTextFromUrl(const QUrl& fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("Cannot open %1: %2").arg(QFileInfo(path).fileName(), f.errorString());
    }
    QTextStream in(&f);
    const QString text = in.readAll();
    f.close();
    return importParamText(text);  // also emits importCompleted + overlapsChanged
}

bool APMOSDComponentController::exportParamTextToUrl(const QUrl& fileUrl)
{
    const QString path = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "APMOSDComponentController: cannot write" << path << ":" << f.errorString();
        return false;
    }
    QTextStream out(&f);
    out << exportParamText();
    f.close();
    return f.error() == QFile::NoError;
}

// ---------------------------------------------------------------------------
// exportParamText
//
// Mirrors `serializeParamFile` in osd-params.js.
//
// Test mapping:
//   - "Round trips: parse(serialize(state)) ≈ state"
//   - "Custom layout round-trips correctly"
//   - "Real-world fixture: load + round-trip Mission Planner export"
// ---------------------------------------------------------------------------
QString APMOSDComponentController::exportParamText()
{
    QStringList out;
    out << QStringLiteral("# ArduPilot OSD layout - exported by QGroundControl");
    out << QStringLiteral("# Generated: %1").arg(
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    out << QString();

    // Helper: append "<paramName>,<intValue>" if the Fact exists.
    auto emitLine = [this, &out](const QString& paramName) {
        Fact* f = _lookupFact(paramName);
        if (f) out << QStringLiteral("%1,%2").arg(paramName).arg(f->rawValue().toInt());
    };

    static const QStringList kMetaSuffixes{
        QStringLiteral("ENABLE"),
        QStringLiteral("TXT_RES"),
        QStringLiteral("CHAN_MIN"),
        QStringLiteral("CHAN_MAX"),
    };
    static const QStringList kElFields{
        QStringLiteral("EN"),
        QStringLiteral("X"),
        QStringLiteral("Y"),
    };

    for (int n = 1; n <= 4; ++n) {
        out << QStringLiteral("# --- Screen %1 ---").arg(n);
        for (const QString& suffix : kMetaSuffixes) {
            emitLine(QStringLiteral("OSD%1_%2").arg(n).arg(suffix));
        }
        for (const auto& el : kElements) {
            for (const QString& field : kElFields) {
                emitLine(QStringLiteral("OSD%1_%2_%3")
                             .arg(n)
                             .arg(QString::fromLatin1(el.key), field));
            }
        }
        out << QString();
    }

    return out.join(QLatin1Char('\n'));
}

// ---------------------------------------------------------------------------
// detectOverlaps
//
// Mirrors `detectOverlaps` in osd-params.js.
//
// Test mapping:
//   - "Reports overlap for same-row adjacent elements"
//   - "No overlap when on different rows"
//   - "Disabled elements never overlap"
// ---------------------------------------------------------------------------
QVariantList APMOSDComponentController::detectOverlaps()
{
    struct Active {
        QString key;
        int x;
        int y;
        int w;
    };
    QVector<Active> active;
    active.reserve(static_cast<int>(sizeof(kElements) / sizeof(kElements[0])));

    for (const auto& el : kElements) {
        const QString key = QString::fromLatin1(el.key);
        Fact* enFact = elEnabledFact(key);
        if (!enFact || enFact->rawValue().toInt() == 0) continue;
        Fact* xFact = elXFact(key);
        Fact* yFact = elYFact(key);
        if (!xFact || !yFact) continue;  // can't position without coords
        active.append({key,
                       xFact->rawValue().toInt(),
                       yFact->rawValue().toInt(),
                       el.width});
    }

    QVariantList overlaps;
    for (int i = 0; i < active.size(); ++i) {
        for (int j = i + 1; j < active.size(); ++j) {
            const Active& a = active.at(i);
            const Active& b = active.at(j);
            if (a.y != b.y) continue;
            // x-range overlap on the same row: [a.x, a.x+a.w) vs [b.x, b.x+b.w)
            const int aEnd = a.x + a.w;
            const int bEnd = b.x + b.w;
            if (a.x < bEnd && b.x < aEnd) {
                QVariantMap m;
                m[QStringLiteral("a")] = a.key;
                m[QStringLiteral("b")] = b.key;
                m[QStringLiteral("y")] = a.y;
                overlaps.append(m);
            }
        }
    }
    return overlaps;
}

// ---------------------------------------------------------------------------
// clampActiveScreen
//
// Mirrors `clampToResolution` in osd-params.js. Call after the user changes
// TXT_RES so positions become valid in the new bounds.
//
// IMPORTANT (v1 design decision recorded in memory/project_sd_clamp_behavior.md):
// per-element clamping is intentional. After an HD→SD shrink, some elements
// will pile up at the right/bottom edge and overlap. We do NOT try to
// re-resolve those overlaps here. detectOverlaps() will flag them in the UI
// and the user repositions manually. Don't be clever.
// ---------------------------------------------------------------------------
void APMOSDComponentController::clampActiveScreen()
{
    const int xMax = maxX();
    const int yMax = maxY();
    for (const auto& el : kElements) {
        const QString key = QString::fromLatin1(el.key);
        if (Fact* xFact = elXFact(key)) {
            const int v = xFact->rawValue().toInt();
            const int clamped = qBound(0, v, xMax);
            if (clamped != v) xFact->setRawValue(clamped);  // skip no-op writes
        }
        if (Fact* yFact = elYFact(key)) {
            const int v = yFact->rawValue().toInt();
            const int clamped = qBound(0, v, yMax);
            if (clamped != v) yFact->setRawValue(clamped);
        }
    }
    emit overlapsChanged();  // clamping can newly create or resolve overlaps
}
