/*
 * Copyright (c) 2012-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

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
                           QObject *parent, std::string name) :
    QObject(parent),
    worker_thread(new QThread(this)),
    usefunc(usefunc) {

  this->name = name;

  worker = std::make_shared<ThreadWorkerNS::Worker>(nullptr, datafunc);

  connect(worker.get(), SIGNAL(finished()), this, SLOT(worker_finished()), Qt::QueuedConnection);
  connect(worker_thread, SIGNAL(started()), worker.get(), SLOT(fetch_data()), Qt::QueuedConnection);
  connect(worker.get(), SIGNAL(finished(QMap<QString, QVariant>)),
          this, SLOT(use_data(QMap<QString, QVariant>)), Qt::QueuedConnection);
  worker->moveToThread(worker_thread);
  worker_thread->start();
}

ThreadWorker::~ThreadWorker() {
  stop_thread();
}

void ThreadWorker::worker_finished() {
//  qDebug() << "worker finished"; //FIXME to log
  stop_thread();
}

void ThreadWorker::use_data(QMap<QString, QVariant> data) {
  QMutexLocker lock(&mutex);
  usefunc(data);
}

void ThreadWorker::stop_thread() {
  if (worker_thread == nullptr) return;
  worker_thread->requestInterruption();
  worker_thread->quit();
  for (int i=0; i<5 && !worker_thread->wait(1000);i++){
    qDebug() << "Worker has not finished yet: " << QString::fromStdString(name) << worker_thread;
  };
}

