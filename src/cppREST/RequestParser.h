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

class CPPRESTSHARED_EXPORT RequestPaser : public QObject
{
	Q_OBJECT

public:
	RequestPaser(QByteArray *request, QString client_address);
	HttpRequest getRequest() const;


private:
	QList<QByteArray> getRawRequestHeaders() const;
	QByteArray getRequestBody() const;
	QList<QByteArray> getKeyValuePair(QByteArray input) const;
	QMap<QString, QString> getVariables(QByteArray input) const;
	QByteArray getVariableSequence(QByteArray url) const;
	QString getRequestPrefix(QList<QString> path_items) const;
	QString getRequestPath(QList<QString> path_items) const;
	QList<QString> getRequestPathParams(QList<QString> path_items) const;
	RequestMethod inferRequestMethod(QByteArray input) const;

	QByteArray *raw_request_;
	QString client_address_;
};


#endif // REQUESTPARSER_H


