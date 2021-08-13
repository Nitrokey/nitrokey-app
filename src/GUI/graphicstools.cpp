#include "graphicstools.h"

#include <QGraphicsColorizeEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyle>
#include <QProcess>
#include <QApplication>

bool test_if_gnome(){
#ifdef Q_OS_LINUX
    QProcess proc;
    proc.start("pgrep gnome-session");
    if(!proc.waitForStarted())
        return false;
    QByteArray data;
    while(proc.waitForReadyRead())
        data.append(proc.readAll());
    return data.length() > 0;
#endif
    return false;
}

QPixmap GraphicsTools::loadColorize(const QString& path, bool loadForTray, bool mainTrayIcon){
    QColor text_color;

    if (mainTrayIcon){
        text_color.setRgb(255, 0, 0);
    } else {
        text_color = QGuiApplication::palette().color(QPalette::WindowText);
    }

    auto effect = new QGraphicsColorizeEffect;
    effect->setColor(text_color);
    auto src_image = QImage(path);
    auto colorized_image = applyEffectToImage(src_image, effect);
    return QPixmap::fromImage(colorized_image);
}

QImage GraphicsTools::applyEffectToImage(QImage src_image, QGraphicsEffect *effect)
{
    if(src_image.isNull()) return QImage();
    if(!effect) return src_image;
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(src_image));
    item.setGraphicsEffect(effect);
    scene.addItem(&item);
    QImage res(src_image.size(), QImage::Format_ARGB32);
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(&ptr);
    return res;
}
