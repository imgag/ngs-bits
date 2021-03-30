#ifndef REFGENDOWNLOADDIALOG_H
#define REFGENDOWNLOADDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QDebug>
#include <QKeyEvent>
#include "ui_RefGenDownloadDialog.h"
#include "Settings.h"
#include "HttpHandler.h"
#include "HttpRequestHandler.h"
#include "GSvarHelper.h"

class RefGenDownloadDialog
        : public QDialog
{
        Q_OBJECT

public:
        RefGenDownloadDialog(QWidget *parent = 0);

public slots:
        void startDownload();
        void cancelDownload();

private:
        Ui::RefGenDownloadDialog ui_;
        bool is_interrupted_;
        void closeEvent(QCloseEvent *bar);
        void keyPressEvent(QKeyEvent *e);
};

#endif // REFGENDOWNLOADDIALOG_H
