#ifndef TESTWORKER_H
#define TESTWORKER_H

#include <QRunnable>
#include "Log.h"
#include "HttpRequestHandler.h"

class TestWorker
    : public QRunnable
{

public:
    explicit TestWorker();
    void run() override;

private:
    int sendGetRequest(QByteArray& reply, QString url, HttpHeaders headers);
    // QMap<QString, UrlEntity> current_storage_;

};



#endif // TESTWORKER_H
