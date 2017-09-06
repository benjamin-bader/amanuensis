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
#include <QSettings>

#include "Proxy.h"
#include "ProxyFactory.h"

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
    proxy = ProxyFactory().create(port);

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
    addRowToListView(connection, "Established");
}

void MainWindow::requestReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &request)
{
    std::stringstream ss;
    ss << ">>> " << request.method() << " " << request.uri();

    addRowToListView(connection, ss.str());
}

void MainWindow::responseReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &response)
{
    std::stringstream ss;
    ss << "<<< " << response.status_code() << " " << response.status_message();

    addRowToListView(connection, ss.str());
}

void MainWindow::connectionClosed(const std::shared_ptr<Connection> &connection)
{
    addRowToListView(connection, "Closed");
}

void MainWindow::addRowToListView(const std::shared_ptr<Connection> &connection, const std::string &message)
{
    std::stringstream ss;
    ss << "CONN(" << connection->id() << "): " << message;
    QString text(ss.str().c_str());

    model->insertRow(model->rowCount());
    QModelIndex index = model->index(model->rowCount() - 1);
    model->setData(index, text);
    ui->listView->setCurrentIndex(index);
}
