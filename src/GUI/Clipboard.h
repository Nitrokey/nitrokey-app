//
// Created by sz on 16.01.17.
//

#ifndef NITROKEYAPP_CLIPBOARD_H
#define NITROKEYAPP_CLIPBOARD_H

#include <QObject>
#include <QClipboard>
#include <QString>

class Clipboard : public QObject
{
Q_OBJECT
public:
    Clipboard(QObject *parent);
    void copyToClipboard(QString text, int time=10);

    virtual ~Clipboard();

private:
    QClipboard *clipboard;
    uint lastClipboardTime;
    QString secretInClipboard;
private slots:
    void checkClipboard_Valid(bool force_clear=false);

};


#endif //NITROKEYAPP_CLIPBOARD_H
