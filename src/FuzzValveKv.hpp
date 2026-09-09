#pragma once
#include <QString>
#include <QVariantMap>
#include <optional>

namespace wekde::fuzz
{

// Public test seam for the Valve KV-format parser embedded in FileHelper.cpp.
// Accepts the raw ACF text and returns either a QVariantMap (success) or
// std::nullopt (syntax error).  The grammar accepted is the strict subset
// Steam emits for appworkshop_<appid>.acf:
//
//   file   := pair*
//   pair   := STRING (STRING | object)
//   object := '{' pair* '}'
//
// STRING is a `"…"` quoted token with support for `\"`, `\\`, `\n`, `\t`.
// Whitespace is any ASCII <= 0x20.  C++-style // comments are skipped.
//
// This function is a fuzz-friendly wrapper: it does NOT read files, has no
// size cap, and returns a clean parse-tree or nullopt — ideal for
// libFuzzer-driven fuzzing.
std::optional<QVariantMap> parseValveKV(const QString& text);

} // namespace wekde::fuzz