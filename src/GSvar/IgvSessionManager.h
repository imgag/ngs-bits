#ifndef IGVSESSIONMANAGER_H
#define IGVSESSIONMANAGER_H

#include <QString>
#include <QMutex>
#include "IGVSession.h"

//A manager class to handle multiple IGV session
class IgvSessionManager
{
public:
	//instantiate a new IGV session
	static IGVSession* create(QWidget* parent, const QString& name, const QString& app, const QString& host, const QString& genome);
	//returns a reference to an IGV session with the given index
    static IGVSession& get(const int& session_index);
    //total number of IGV sessions
    static int count();
	//Clears all instances of IGV. Used e.g. when loading a new GSvar file.
	static void clearAll();

protected:
    IgvSessionManager();
    ~IgvSessionManager();
	static IgvSessionManager& instance();

private:
    QList<QSharedPointer<IGVSession>> session_list_;
    QMutex mutex_;
};

#endif // IGVSESSIONMANAGER_H
