#include "FallbackRenderer.hpp"
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>

namespace wekde
{

FallbackRenderer::FallbackRenderer(QQuickItem* parent): QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

void FallbackRenderer::setFallbackColor(const QColor& c) {
    if (m_color == c) return;
    m_color = c;
    update();
    emit fallbackColorChanged();
}

void FallbackRenderer::setFallbackText(const QString& t) {
    if (m_text == t) return;
    m_text = t;
    update();
    emit fallbackTextChanged();
}

void FallbackRenderer::setShowText(bool v) {
    if (m_showText == v) return;
    m_showText = v;
    update();
    emit showTextChanged();
}

QSGNode* FallbackRenderer::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    // Minimal: just a solid-color rectangle.  No textures, no OpenGL/Vulkan
    // state, no external resources — this must never fail, because it IS the
    // failure path.  A QSGSimpleRectNode is the lightest possible QSG node.
    QSGSimpleRectNode* node = static_cast<QSGSimpleRectNode*>(oldNode);
    if (! node) {
        node = new QSGSimpleRectNode();
    }
    node->setRect(boundingRect());
    node->setColor(m_color);
    return node;
}

} // namespace wekde