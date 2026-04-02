#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "cppVISUAL_global.h"

#include <QObject>
#include <QFileDialog>

class CPPVISUALSHARED_EXPORT SessionManager : QObject
{
	Q_OBJECT

public:
	//TO DO:
	QString version();
	static void saveSession(QString file_path);

private:
	static const QString VERSION;
	explicit SessionManager(QObject* parent = nullptr)
		: QObject(parent) {}

	static SessionManager& instance()
	{
		static SessionManager instance;
		return instance;
	}
};


#endif // SESSIONMANAGER_H
