// libFuzzer harness for project.json wallpaper property parsing.
// Drives the readWallpaperProperties JSON extraction logic (general.properties
// section) with arbitrary JSON-like byte sequences to discover crashes in the
// QJsonDocument → QJsonObject → QVariantList conversion chain.
//
// Because readWallpaperProperties requires a file path (it calls readFile
// internally with allowlist + canonicalisation), we extract the pure JSON-
// processing logic here: parse, extract general.properties, build the
// descriptor list.  This isolates the JSON path from the filesystem path.
//
// Build:
//   cmake --build ... --target fuzz_ProjectJson
// Run:
//   ./fuzz_ProjectJson -max_len=1048576 -runs=50000

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace
{

// Mirror of the core logic from FileHelper::readWallpaperProperties,
// minus the file I/O and allowlist.  Parses raw bytes as JSON, extracts
// general.properties, and builds a descriptor list.  Any parse error or
// missing section returns an empty list (the production behaviour).
QVariantList parseProjectJsonProperties(const QByteArray& raw) {
    QJsonParseError err {};
    QJsonDocument   doc = QJsonDocument::fromJson(raw, &err);
    // Reject invalid JSON and non-objects
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }

    const QJsonObject root     = doc.object();
    const QJsonValue  generalV = root.value(QStringLiteral("general"));
    if (! generalV.isObject()) return {};

    const QJsonValue propsV = generalV.toObject().value(QStringLiteral("properties"));
    if (! propsV.isObject()) return {};

    const QJsonObject props = propsV.toObject();
    if (props.isEmpty()) return {};

    QVariantList result;
    for (auto it = props.begin(); it != props.end(); ++it) {
        const QJsonObject prop = it.value().toObject();
        const QString     type = prop.value(QStringLiteral("type")).toString();

        // Skip non-interactive types
        if (type.isEmpty() || type == QStringLiteral("text") ||
            type == QStringLiteral("group")) {
            continue;
        }

        QVariantMap desc;
        desc[QStringLiteral("name")] = it.key();
        desc[QStringLiteral("type")] = type;

        const QString rawText = prop.value(QStringLiteral("text")).toString();
        desc[QStringLiteral("text")] =
            rawText.isEmpty() ? it.key() : rawText;

        const QJsonValue defaultValue = prop.value(QStringLiteral("value"));
        desc[QStringLiteral("value")]   = defaultValue.toVariant();
        desc[QStringLiteral("default")] = defaultValue.toVariant();

        // Slider metadata
        if (prop.contains(QStringLiteral("min")))
            desc[QStringLiteral("min")] = prop.value(QStringLiteral("min")).toDouble();
        if (prop.contains(QStringLiteral("max")))
            desc[QStringLiteral("max")] = prop.value(QStringLiteral("max")).toDouble();
        if (prop.contains(QStringLiteral("step")))
            desc[QStringLiteral("step")] =
                prop.value(QStringLiteral("step")).toDouble();

        // Combo options
        if (prop.contains(QStringLiteral("options"))) {
            const QJsonArray opts = prop.value(QStringLiteral("options")).toArray();
            QVariantList     optList;
            for (const QJsonValue& ov : opts) {
                QVariantMap om;
                if (ov.isObject()) {
                    const QJsonObject oo = ov.toObject();
                    om[QStringLiteral("value")] = oo.value(QStringLiteral("value")).toVariant();
                    om[QStringLiteral("label")] =
                        oo.value(QStringLiteral("label")).toString();
                } else {
                    om[QStringLiteral("value")] = ov.toVariant();
                    om[QStringLiteral("label")] = ov.toString();
                }
                optList.append(om);
            }
            desc[QStringLiteral("options")] = optList;
        }

        // File type filter
        if (prop.contains(QStringLiteral("fileType"))) {
            desc[QStringLiteral("fileType")] =
                prop.value(QStringLiteral("fileType")).toString();
        }

        if (prop.contains(QStringLiteral("condition"))) {
            desc[QStringLiteral("condition")] =
                prop.value(QStringLiteral("condition")).toString();
        }

        result.append(desc);
    }

    return result;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Cap at 1 MiB — mirrors FileHelper::kMaxReadSize, and anything larger
    // is more likely to test OOM than the JSON parser.
    if (size > 1024 * 1024) return 0;

    QByteArray raw(reinterpret_cast<const char*>(data), static_cast<int>(size));

    // The parser must never crash, hang, or leak.  parse error => empty list.
    auto result = parseProjectJsonProperties(raw);

    // Quick sanity: the result must NOT contain entries with empty name.
    for (const QVariant& v : result) {
        const QVariantMap m = v.toMap();
        const QString     name = m.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) {
            __builtin_trap(); // bug: entries must have non-empty names
        }
    }

    return 0;
}