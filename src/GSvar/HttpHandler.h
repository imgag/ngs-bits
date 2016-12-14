#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QObject>
#include <QString>
#include <QSslError>

///Helper class for HTTP(S) communication with webserver
class HttpHandler
		: public QObject
{
	Q_OBJECT

public:
	///Constructor
	HttpHandler(QObject* parent=0);
	///Handles request (GET)
	QString getHttpReply(QString url);
	///Handles request (POST)
	QString getHttpReply(QString url, QByteArray data);

private slots:
	///Handles SSL errors (by ignoring them)
	void handleSslErrors(QList<QSslError> errors);
};

#endif // HTTPHANDLER_H
