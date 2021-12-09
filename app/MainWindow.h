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

#pragma once

#include <QMainWindow>

#include <memory>

#include <QStringListModel>
#include <QVector>

#include "core/Transaction.h"

namespace ama
{
class Proxy;
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //void addRowToListView(const std::string& message);

public slots:
    void on_message_logged(const QString& message);

private slots:
    void onNewTransaction(ama::Transaction* tx);
    void transactionStarted(ama::Transaction* tx);
    void requestRead(ama::Transaction* tx);
    void responseHeadersRead(ama::Transaction* tx);
    void responseRead(ama::Transaction* tx);
    void transactionComplete(ama::Transaction* tx);
    void transactionFailed(ama::Transaction* tx);

private:
    //void addRowToListView(const std::shared_ptr<ama::Connection> &connection, const std::string &message);


private:
    Ui::MainWindow *ui;
    ama::Proxy* proxy;
    QVector<QMetaObject::Connection> connections;

    QStringListModel *model;

};
