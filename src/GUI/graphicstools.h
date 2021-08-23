#ifndef GRAPHICSTOOLS_H
#define GRAPHICSTOOLS_H

#include <QPixmap>
#include <QImage>
#include <QGraphicsColorizeEffect>

class GraphicsTools
{
public:
    static QPixmap loadColorize(const QString &path, bool loadForTray = false, bool mainTrayIcon = false);
    static QImage applyEffectToImage(QImage src, QGraphicsEffect *effect);

    static QString get_default_tray_color();
};

#endif // GRAPHICSTOOLS_H
