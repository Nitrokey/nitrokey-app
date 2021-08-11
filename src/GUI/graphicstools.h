#ifndef GRAPHICSTOOLS_H
#define GRAPHICSTOOLS_H

#include <QPixmap>
#include <QImage>
#include <QGraphicsColorizeEffect>

class GraphicsTools
{
public:
    static QPixmap loadColorize(QString path, bool loadForTray = false, bool mainTrayIcon = false);
    static QImage applyEffectToImage(QImage src, QGraphicsEffect *effect);
};

#endif // GRAPHICSTOOLS_H
