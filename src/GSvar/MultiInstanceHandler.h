#ifndef MULTIINSTANCEHANDLER_H
#define MULTIINSTANCEHANDLER_H

#include <QUrl>
#include <QLocalServer>
#include <QSharedPointer>

//Multi-instance handler for Windows
class MultiInstanceHandler : public QObject
{
	Q_OBJECT
	public:
		MultiInstanceHandler(QObject* parent = nullptr);
		bool anotherInstanceExists();
		bool forwardToExistingInstance(const QUrl& url);
		bool startServer();
		void stopServer();
		QString errorString();

	signals:
		void urlReceived(QUrl url);

	private:
		static constexpr auto SERVER_NAME = "GSvar_single_instance";
		QSharedPointer<QLocalServer> server_;
};

#endif // MULTIINSTANCEHANDLER_H
