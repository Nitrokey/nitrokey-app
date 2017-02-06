#include "ThreadWorker.h"

#include <QMutexLocker>
#include <QDebug>
#include "libnitrokey/include/DeviceCommunicationExceptions.h"


ThreadWorkerNS::Worker::Worker(QObject *parent, const std::function<Data()> &datafunc) :
    QObject(parent),
    datafunc(datafunc) {}

void ThreadWorkerNS::Worker::fetch_data() {
  QMutexLocker lock(&mutex);
  try{
    auto data = datafunc();
    emit finished(data);
  }
  catch (DeviceCommunicationException &e){
    //FIXME to log
    qDebug() << e.what();
  }
  catch (std::exception &e){
    //FIXME other (to log)
    qDebug() << e.what();
  }
  emit finished();
}

ThreadWorker::ThreadWorker(const std::function<Data()> &datafunc, const std::function<void(Data)> &usefunc,
                           QObject *parent) :
    QObject(parent),
    worker(new ThreadWorkerNS::Worker(nullptr, datafunc)),
    worker_thread(new QThread()), //FIXME leak
    usefunc(usefunc) {

  connect(worker, SIGNAL(finished()), this, SLOT(worker_finished()), Qt::QueuedConnection);
  connect(worker_thread, SIGNAL(started()), worker, SLOT(fetch_data()), Qt::QueuedConnection);
  connect(worker, SIGNAL(finished(QMap<QString, QVariant>)),
          this, SLOT(use_data(QMap<QString, QVariant>)), Qt::QueuedConnection);
  worker->moveToThread(worker_thread);
  worker_thread->start();
}

ThreadWorker::~ThreadWorker() {
  stop_thread();
}

void ThreadWorker::worker_finished() {
  qDebug() << "worker finished";
}

void ThreadWorker::use_data(QMap<QString, QVariant> data) {
  QMutexLocker lock(&mutex);
  usefunc(data);
}

void ThreadWorker::stop_thread() {
  worker_thread->quit();
  worker_thread->wait();
}

