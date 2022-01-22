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

#include "ProxyFactory.h"
#include "TransactionFile.h"

#include <iostream>
#include <sstream>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QSettings>
#include <QStandardPaths>

#include "core/Proxy.h"
#include "core/Transaction.h"

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

    proxy = ProxyFactory::Create(port, this);
    proxy->enable();

    txModel = new TransactionModel(proxy, this);

    createMenu();

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

void MainWindow::createMenu()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    //QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* saveAction = fileMenu->addAction(tr("&Save"));
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setEnabled(false);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveTransactionFile);
    connect(proxy, &Proxy::transactionStarted, saveAction, &QAction::resetEnabled);

    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setMenuRole(QAction::QuitRole);
    connect(quitAction, &QAction::triggered, QCoreApplication::instance(), &QCoreApplication::quit);

    fileMenu->addAction(quitAction);

    QAction* aboutAction = helpMenu->addAction(tr("&About"));
    aboutAction->setMenuRole(QAction::AboutRole);
    connect(aboutAction, &QAction::triggered, QCoreApplication::instance(), &QCoreApplication::quit);

    helpMenu->addAction(aboutAction);
}


void MainWindow::saveTransactionFile()
{
    QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsDir.isEmpty())
    {
        // wtf
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), documentsDir, tr("TXS files (*.txs)"));
    if (fileName.isEmpty())
    {
        return;
    }

    if (QFile::exists(fileName))
    {
        QFile::remove(fileName);
    }

    TransactionFile file(fileName, this);

    auto rc = txModel->rowCount();
    for (auto ix = 0; ix < rc; ++ix)
    {
        auto tx = txModel->transaction(txModel->index(ix, 0));
        file.addTransaction(tx);
    }
}
