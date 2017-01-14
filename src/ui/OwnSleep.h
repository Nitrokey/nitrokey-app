#ifndef NITROKEYAPP_OWNSLEEP_H
#define NITROKEYAPP_OWNSLEEP_H

#include <QThread>

class OwnSleep : public QThread {
public:
    static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
    static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
    static void sleep(unsigned long secs) { QThread::sleep(secs); }
};

#endif //NITROKEYAPP_OWNSLEEP_H
