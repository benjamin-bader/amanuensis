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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

    txModel = new TransactionModel(proxy, this);

    ui->tableView->setModel(txModel);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->horizontalHeader()->setVisible(true);

    connect(txModel, &TransactionModel::layoutChanged, ui->tableView, &QTableView::resizeColumnsToContents);

    proxy->init();
}

MainWindow::~MainWindow()
{
    proxy->deinit();

    delete ui;
}


