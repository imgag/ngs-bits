#ifndef IGVSESSIONMANAGER_H
#define IGVSESSIONMANAGER_H

#include <QString>
#include <QMutex>
#include "LoginManager.h"
#include "Exceptions.h"
#include "Settings.h"
#include "IGVSession.h"

//A manager class to handle multiple instances of IGV from GSvar. Each instance uses its own pool of
//threads, each command runs in a separate thread. It keeps GSvar from freezing and from
//showing "Not responding" messages
class IgvSessionManager
{
public:
    //instantiate a new IGV
    static void create(QWidget *parent, Ui::MainWindow parent_ui, const QString& name, const QString& app, const QString& host, const QString& genome);
    //removes a session by index
    static void remove(const int& session_index);
    //finds a session index by its name
    static const int indexByName(const QString& name);
    //returns a reference to an IGVSession object
    static IGVSession& get(const int& session_index);
    //total number of IGV sessions
    static const int count();
    //for each opened sample IGV has to be reset (since each sample has a different set of files to be passed to IGV)
    static void resetIGVInitialized();
    //checks among all available IGV sessions if there is at least one command running
    static const bool hasAtLeastOneActiveIGV();

protected:
    IgvSessionManager();
    ~IgvSessionManager();
    static IgvSessionManager& instance();
    static int findAvailablePortForIGV();

private:
    QList<QSharedPointer<IGVSession>> session_list_;
    QMutex mutex_;
};

#endif // IGVSESSIONMANAGER_H
