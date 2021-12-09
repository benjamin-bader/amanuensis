// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <iostream>
#include <sstream>

#include <QDebug>
#include <QLabel>
#include <QSettings>

#include "core/Proxy.h"
#include "core/ProxyFactory.h"
#include "core/Transaction.h"

#ifdef Q_OS_MAC
#include "mac/MacProxy.h"
#endif

using namespace ama;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    connections(),
    model(new QStringListModel)
{
    ui->setupUi(this);

    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    int port = settings.value("Proxy/port", 9999).toInt();

#ifdef Q_OS_MAC
    proxy = new ama::MacProxy(port, this);

    static_cast<ama::MacProxy*>(proxy)->enable();
    //static_cast<ama::MacProxy*>(proxy.get())->say_hi();
#else
    proxy = ProxyFactory().create(port);
#endif

    connect(proxy, &Proxy::transactionStarted, this, &MainWindow::onNewTransaction);

    ui->listView->setModel(model);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    proxy->init();
}

MainWindow::~MainWindow()
{
    proxy->deinit();

    for (QMetaObject::Connection &connection : connections)
    {
        QObject::disconnect(connection);
    }
    delete ui;
}

void MainWindow::onNewTransaction(ama::Transaction* tx)
{
    qDebug() << "Got a tx! " << tx->id();
    connect(tx, &ama::Transaction::on_transaction_start, this, &MainWindow::transactionStarted, Qt::QueuedConnection);
    connect(tx, &ama::Transaction::on_request_read, this, &MainWindow::requestRead, Qt::QueuedConnection);
    connect(tx, &ama::Transaction::on_response_headers_read, this, &MainWindow::responseHeadersRead, Qt::QueuedConnection);
    connect(tx, &ama::Transaction::on_response_read, this, &MainWindow::responseRead, Qt::QueuedConnection);
    connect(tx, &ama::Transaction::on_transaction_complete, this, &MainWindow::transactionComplete, Qt::QueuedConnection);
    connect(tx, &ama::Transaction::on_transaction_failed, this, &MainWindow::transactionFailed, Qt::QueuedConnection);
}

void MainWindow::transactionStarted(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): started";

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::requestRead(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): " << tx->request().method() << " " << tx->request().uri();

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::responseHeadersRead(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): " << tx->response().status_code() << " " << tx->response().status_message();

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::responseRead(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): " << tx->request().method() << " " << tx->request().uri();

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::transactionComplete(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): complete";

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::transactionFailed(ama::Transaction* tx)
{
    std::stringstream ss;
    ss << "TX(" << tx->id() << "): failed";

    on_message_logged(QString{ss.str().c_str()});
}

void MainWindow::on_message_logged(const QString& message)
{
    int rowCount = model->rowCount();
    model->insertRow(rowCount);
    QModelIndex ix = model->index(rowCount);
    model->setData(ix, message);
}

