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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <memory>

#include <QStringListModel>
#include <QVector>

class Proxy;
class Connection;
class HttpMessage;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void connectionEstablished(const std::shared_ptr<Connection> &connection);
    void requestReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &request);
    void responseReceived(const std::shared_ptr<Connection> &connection, const HttpMessage &response);
    void connectionClosed(const std::shared_ptr<Connection> &connection);

private:
    void addRowToListView(const std::shared_ptr<Connection> &connection, const std::string &message);

private:
    Ui::MainWindow *ui;
    std::shared_ptr<Proxy> proxy;
    QVector<QMetaObject::Connection> connections;

    QStringListModel *model;

};

#endif // MAINWINDOW_H
