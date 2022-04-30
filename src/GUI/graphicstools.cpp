#include "graphicstools.h"

#include <QGraphicsColorizeEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyle>
#include <QProcess>
#include <QApplication>
#include <QSettings>

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

QString GraphicsTools::get_default_tray_color(){
    if (test_if_gnome()) {
        return QStringLiteral("#FFFFFF");
    }
    return QStringLiteral("#000000");
}

QPixmap GraphicsTools::loadColorize(const QString& path, bool loadForTray, bool mainTrayIcon){
    QColor text_color;

    if (mainTrayIcon){
        QSettings settings;
        auto color = settings.value("main/tray_icon_color", GraphicsTools::get_default_tray_color()).toString();
        text_color.setNamedColor(color);
    } else {
//        text_color = QGuiApplication::palette().color(QPalette::WindowText);
        text_color = QColor::fromRgb(0xaa, 0xaa, 0xaa);
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
