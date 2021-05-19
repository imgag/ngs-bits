#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H



#include "cppREST_global.h"
#include <QObject>
#include <QList>
#include <QDebug>
#include "HttpRequest.h"
#include "HttpProcessor.h"
#include "Exceptions.h"
#include "ServerHelper.h"
//#include "HttpResponse.h"

class CPPRESTSHARED_EXPORT RequestPaser : public QObject
{
	Q_OBJECT

public:
	RequestPaser(QByteArray *request, QString client_address);
	HttpRequest getRequest();


private:
	QList<QByteArray> getRawRequestHeaders();
	QByteArray getRequestBody();
	QList<QByteArray> getKeyValuePair(QByteArray input);
	QMap<QString, QString> getVariables(QByteArray input);
	QByteArray getVariableSequence(QByteArray url);
	QString getRequestPrefix(QList<QString> path_items);
	QString getRequestPath(QList<QString> path_items);
	QList<QString> getRequestPathParams(QList<QString> path_items);
	RequestMethod inferRequestMethod(QByteArray input);



	QByteArray *raw_request_;
	QString client_address_;
};


#endif // REQUESTPARSER_H


