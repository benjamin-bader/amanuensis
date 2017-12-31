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

#include "Proxy.h"
#include "ProxyFactory.h"
#include "Transaction.h"

#ifdef Q_OS_MAC
#include "mac/MacProxy.h"
#endif

using namespace ama;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    txListener(std::make_shared<TxListener>()),
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
    proxy = std::make_shared<ama::MacProxy>(port);

    std::error_code ec;
    static_cast<ama::MacProxy*>(proxy.get())->enable(ec);

    if (ec)
    {
        std::cout << ec << std::endl;
        throw std::invalid_argument("wtf");
    }
    else
    {
        static_cast<ama::MacProxy*>(proxy.get())->say_hi();
    }
#else
    proxy = ProxyFactory().create(port);
#endif

    connections << connect(proxy.get(), &Proxy::transactionStarted, [this](std::shared_ptr<Transaction> tx) {
                   qDebug() << "Got a tx! " << tx->id();
                   tx->add_listener(txListener);
    });

    connections << connect(txListener.get(), &TxListener::message_logged, this, &MainWindow::on_message_logged, Qt::QueuedConnection);

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

void TxListener::on_transaction_start(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): started";

    emit message_logged(QString{ss.str().c_str()});
}

void TxListener::on_request_read(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): " << tx.request().method() << " " << tx.request().uri();

    emit message_logged(QString{ss.str().c_str()});
}

void TxListener::on_response_headers_read(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): " << tx.response().status_code() << " " << tx.response().status_message();
    emit message_logged(QString{ss.str().c_str()});
}

void TxListener::on_response_read(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): " << tx.request().method() << " " << tx.request().uri();

    emit message_logged(QString{ss.str().c_str()});
}

void TxListener::on_transaction_complete(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): complete";

    emit message_logged(QString{ss.str().c_str()});
}

void TxListener::on_transaction_failed(Transaction &tx)
{
    std::stringstream ss;
    ss << "TX(" << tx.id() << "): failed";

    emit message_logged(QString{ss.str().c_str()});
}

void MainWindow::on_message_logged(const QString& message)
{
    model->stringList() << QString::fromStdString(message);
}

