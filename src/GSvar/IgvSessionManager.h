#ifndef IGVSESSIONMANAGER_H
#define IGVSESSIONMANAGER_H

#include <QString>
#include <QMutex>
#include "LoginManager.h"
#include "Exceptions.h"
#include "Settings.h"
#include "IGVCommandExecutor.h"

struct IGVSession
{
    QString app;
    QString host;
    int port;
	QString genome;
    QSharedPointer<IGVCommandExecutor> command_executor;
    bool is_initialized;
};

class IgvSessionManager
{
public:
    static int create(QString app, QString host, int port, QString genome, bool is_initialized);
    static void remove(int session_index);
	static int findAvailablePortForIGV();
    static QString getHost(int session_index);
    static void setHost(QString host, int session_index);
    static int getPort(int session_index);
    static void setPort(int port, int session_index);
    static QString getGenome(int session_index);
    static void setGenome(QString genome, int session_index);
    static QSharedPointer<IGVCommandExecutor> getCommandExecutor(int session_index);
	static bool isIGVInitialized(int session_index);
	static void setIGVInitialized(bool is_initialized, int session_index);

protected:
	IgvSessionManager();
	~IgvSessionManager();
	static IgvSessionManager& instance();

private:
	QList<IGVSession> session_list_;
    QMutex mutex_;
};

#endif // IGVSESSIONMANAGER_H
