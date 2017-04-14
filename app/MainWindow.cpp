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

#include <sstream>

#include <QLabel>

#include "Proxy.h"
#include "ProxyFactory.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    proxy(ProxyFactory().create(9999)),
    connections(),
    model(new QStringListModel)
{
    ui->setupUi(this);

    connections << connect(proxy.get(), &Proxy::connectionEstablished, this, &MainWindow::connectionEstablished);
    connections << connect(proxy.get(), &Proxy::requestReceived,       this, &MainWindow::requestReceived);
    connections << connect(proxy.get(), &Proxy::responseReceived,      this, &MainWindow::responseReceived);
    connections << connect(proxy.get(), &Proxy::connectionClosed,      this, &MainWindow::connectionClosed);

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

void MainWindow::connectionEstablished(const std::shared_ptr<Connection> &connection)
{
    QString text = QString("CONN(%1): Established").arg(connection->id());

    model->insertRow(model->rowCount());
    QModelIndex index = model->index(model->rowCount() - 1);
    model->setData(index, text);
}

void MainWindow::requestReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &request)
{
    // TODO(ben): Print hostname and port
    std::stringstream ss;
    ss << "CONN(" << connection->id() << "): >>> " << request.method() << " " << request.uri();

    QString text(ss.str().c_str());

    model->insertRow(model->rowCount());
    QModelIndex index = model->index(model->rowCount() - 1);
    model->setData(index, text);
}

void MainWindow::responseReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &response)
{
    std::stringstream ss;
    ss << "CONN(" << connection->id() << "): <<< " << response.status_code() << " " << response.status_message();

    QString text(ss.str().c_str());

    model->insertRow(model->rowCount());
    QModelIndex index = model->index(model->rowCount() - 1);
    model->setData(index, text);
}

void MainWindow::connectionClosed(const std::shared_ptr<Connection> &connection)
{
    std::stringstream ss;
    ss << "CONN(" << connection->id() << "): Closed";

    QString text(ss.str().c_str());

    model->insertRow(model->rowCount());
    QModelIndex index = model->index(model->rowCount() - 1);
    model->setData(index, text);
}
