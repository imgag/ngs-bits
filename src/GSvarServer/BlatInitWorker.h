#ifndef BLATINITWORKER_H
#define BLATINITWORKER_H

#include <QRunnable>
#include <QString>

class BlatInitWorker
    : public QRunnable
{

public:
    explicit  BlatInitWorker(int port, QString server_folder);
    void run() override;

private:
    bool prepareBlatServer();
    int port_;
    QString server_folder_;
};

#endif // BLATINITWORKER_H
