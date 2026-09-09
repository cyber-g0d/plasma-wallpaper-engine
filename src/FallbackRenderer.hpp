#pragma once
#include <QQuickItem>
#include <QColor>

namespace wekde
{

// A minimal QQuickItem that fills its bounds with a solid color.  Used as a
// graceful fallback when a wallpaper backend (video / web / scene) cannot
// produce its first frame — instead of crashing plasmashell or presenting a
// black void, the fallback renders a configurable color (defaults to a dark
// neutral grey).  The item has no internal timer, no external dependencies
// (no Vulkan, no mpv, no QtWebEngine), and zero dynamic allocation after
// construction, so it is safe to show even when the wallpaper backend is the
// thing that crashed.
//
// Usage from QML:
//   FallbackRenderer { anchors.fill: parent; fallbackColor: "#2a2a2a" }
//
// The host QML (main.qml or the backend-specific .qml) instantiates this
// once and toggles its `visible` or `opacity` in response to a load failure
// signal.  Because the fallback is a plain QtQuick Rectangle under the hood,
// it never enters the Vulkan/mpv/QtWebEngine code paths that failed.
class FallbackRenderer : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QColor fallbackColor READ fallbackColor WRITE setFallbackColor NOTIFY
                   fallbackColorChanged)
    Q_PROPERTY(QString fallbackText READ fallbackText WRITE setFallbackText NOTIFY
                   fallbackTextChanged)
    Q_PROPERTY(bool showText READ showText WRITE setShowText NOTIFY showTextChanged)

public:
    explicit FallbackRenderer(QQuickItem* parent = nullptr);
    ~FallbackRenderer() override = default;

    QColor  fallbackColor() const { return m_color; }
    QString fallbackText() const { return m_text; }
    bool    showText() const { return m_showText; }

    void setFallbackColor(const QColor& c);
    void setFallbackText(const QString& t);
    void setShowText(bool v);

signals:
    void fallbackColorChanged();
    void fallbackTextChanged();
    void showTextChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    QColor  m_color { "#2a2a2a" }; // dark neutral grey — blank but not black
    QString m_text;
    bool    m_showText { false };
};

} // namespace wekde