#ifndef BLATINITWORKER_H
#define BLATINITWORKER_H

#include <QRunnable>
class BlatInitWorker
    : public QRunnable
{

public:
    explicit  BlatInitWorker(int port);
    void run() override;

private:
    bool prepareBlatServer();
    int port_;
};

#endif // BLATINITWORKER_H
