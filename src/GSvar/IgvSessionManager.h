#ifndef IGVSESSIONMANAGER_H
#define IGVSESSIONMANAGER_H

#include <QString>
#include <QMutex>
#include "LoginManager.h"
#include "Exceptions.h"
#include "Settings.h"

struct IGVSession
{
	int port;
	bool is_initialized;
	QString genome;
};

class IgvSessionManager
{
public:
	static int createIGVSession(int port, bool is_initialized, QString genome);
	static void removeIGVSession(int session_index);
	static int findAvailablePortForIGV();
	static int getIGVPort(int session_index);
	static void setIGVPort(int port, int session_index);
	static QString getIGVGenome(int session_index);
	static void setIGVGenome(QString genome, int session_index);
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
