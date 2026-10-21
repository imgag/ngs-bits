#ifndef MULTIINSTANCEHANDLER_H
#define MULTIINSTANCEHANDLER_H

#include <QUrl>
#include <QLocalServer>
#include <QSharedPointer>

class MultiInstanceHandler : public QObject
{
	Q_OBJECT
	public:
		MultiInstanceHandler(QObject* parent = nullptr);
		bool anotherInstanceExists();
		bool forwardToExistingInstance(const QUrl& url);
		bool startServer();
		QString errorString();

	signals:
		void urlReceived(const QUrl url);

	private:
		static constexpr auto SERVER_NAME = "GSvar_single_instance";
		QSharedPointer<QLocalServer> server_;
};

#endif // MULTIINSTANCEHANDLER_H
