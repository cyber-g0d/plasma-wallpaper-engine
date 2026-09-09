// libFuzzer harness for Valve KV-format parsing (Steam ACF files).
// Drives the parseValveKV function with arbitrary byte sequences to
// discover parser crashes, hangs, and memory errors.
//
// Build:
//   cmake --build ... --target fuzz_ValveKV
// Run:
//   ./fuzz_ValveKV -max_len=65536 -runs=100000
//
// The parseValveKV implementation is a standalone copy of the anonymous-
// namespace function from FileHelper.cpp, extracted here so libFuzzer can
// drive it without linking the entire plugin .so.

#include "FuzzValveKv.hpp"
#include <QString>
#include <QVariantMap>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace wekde::fuzz
{

std::optional<QVariantMap> parseValveKV(const QString& text) {
    int       pos = 0;
    const int n   = text.size();

    auto skipWs = [&]() {
        while (pos < n) {
            const QChar c = text.at(pos);
            if (c.unicode() <= 0x20) {
                ++pos;
            } else if (c == QLatin1Char('/') && pos + 1 < n &&
                       text.at(pos + 1) == QLatin1Char('/')) {
                while (pos < n && text.at(pos) != QLatin1Char('\n')) ++pos;
            } else {
                break;
            }
        }
    };

    auto parseString = [&](QString& out) -> bool {
        skipWs();
        if (pos >= n || text.at(pos) != QLatin1Char('"')) return false;
        ++pos;
        out.clear();
        while (pos < n) {
            const QChar c = text.at(pos++);
            if (c == QLatin1Char('"')) return true;
            if (c == QLatin1Char('\\') && pos < n) {
                const QChar esc = text.at(pos++);
                if (esc == QLatin1Char('n'))
                    out.append(QLatin1Char('\n'));
                else if (esc == QLatin1Char('t'))
                    out.append(QLatin1Char('\t'));
                else if (esc == QLatin1Char('r'))
                    out.append(QLatin1Char('\r'));
                else
                    out.append(esc);
            } else {
                out.append(c);
            }
        }
        return false;
    };

    std::function<bool(QVariantMap&)> parsePairs;

    parsePairs = [&](QVariantMap& out) -> bool {
        while (true) {
            skipWs();
            if (pos >= n) return true;
            if (text.at(pos) == QLatin1Char('}')) return true;
            QString key;
            if (! parseString(key)) return false;
            skipWs();
            if (pos >= n) return false;
            const QChar nxt = text.at(pos);
            if (nxt == QLatin1Char('"')) {
                QString val;
                if (! parseString(val)) return false;
                out.insert(key, val);
            } else if (nxt == QLatin1Char('{')) {
                ++pos;
                QVariantMap nested;
                if (! parsePairs(nested)) return false;
                skipWs();
                if (pos >= n || text.at(pos) != QLatin1Char('}')) return false;
                ++pos;
                out.insert(key, nested);
            } else {
                return false;
            }
        }
    };

    QVariantMap root;
    if (! parsePairs(root)) return std::nullopt;
    skipWs();
    if (pos != n) return std::nullopt;
    return root;
}

} // namespace wekde::fuzz

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Feed raw bytes as Latin-1 text to the KV parser.  Real ACF files are
    // plain ASCII, but fuzzing with arbitrary bytes discovers edge cases in
    // escape handling and boundary conditions.
    QString text = QString::fromLatin1(
        reinterpret_cast<const char*>(data), static_cast<int>(size));

    // The parser returns std::nullopt on ANY syntax error — there is no
    // partial-success state.  We just care that it doesn't crash, hang, or
    // corrupt memory.  The return value is discarded; AddressSanitizer /
    // UBSan catch the bugs.
    auto result = wekde::fuzz::parseValveKV(text);

    // Sanity: if the input was empty or whitespace-only, we must get a valid
    // empty object (not a parse error).  Empty means "no pairs", not "garbage".
    // The parser already handles this — this assert just catches regressions.
    if (text.trimmed().isEmpty()) {
        if (!result.has_value()) {
            __builtin_trap(); // bug: empty input must parse as {}
        }
    }

    return 0;
}