#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include "cppREST_global.h"
#include <QObject>
#include <QList>
#include <QDebug>
#include <QHostAddress>
#include <QSslSocket>
#include "HttpRequest.h"
#include "HttpProcessor.h"
#include "Exceptions.h"
#include "WorkerThread.h"

Q_DECLARE_METATYPE(HttpResponse)

class CPPRESTSHARED_EXPORT RequestHandler : public QObject
{
    Q_OBJECT

public:
	RequestHandler(QSslSocket *socket_);	

private slots:
	void dataReceived();

private:
	QSslSocket *socket_;
	RequestMethod inferRequestMethod(QByteArray in);
	void writeResponse(HttpResponse response);
	bool hasEndOfLineCharsOnly(QByteArray line);
	void handleResults(const HttpResponse &response);
	void handleDataChunk(const QByteArray &data);
	QList<QByteArray> getRequestBody();
	QList<QByteArray> getKeyValuePair(QByteArray in);
	QMap<QString, QString> getVariables(QByteArray in);
	QByteArray getVariableSequence(QByteArray url);
	QString getRequestPrefix(QList<QString> path_items);
	QString getRequestPath(QList<QString> path_items);
	QList<QString> getRequestPathParams(QList<QString> path_items);
	void readRequest(QList<QByteArray> body);
	bool end_of_content_found_;
};

#endif // REQUESTHANDLER
