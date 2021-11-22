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

class CPPRESTSHARED_EXPORT RequestParser : public QObject
{
	Q_OBJECT

public:
	RequestParser(QByteArray *request, QString client_address);
	HttpRequest getRequest() const;


private:
	QList<QByteArray> getRawRequestHeaders() const;
	QByteArray getRequestBody() const;
	QList<QByteArray> getKeyValuePair(const QByteArray& input) const;
	QMap<QString, QString> getVariables(const QByteArray& input) const;
	QByteArray getVariableSequence(const QByteArray& url) const;
	QString getRequestPrefix(const QList<QString>& path_items) const;
	QString getRequestPath(const QList<QString>& path_items) const;
	QList<QString> getRequestPathParams(const QList<QString>& path_items) const;
	RequestMethod inferRequestMethod(const QByteArray& input) const;

	QByteArray *raw_request_;
	QString client_address_;
};


#endif // REQUESTPARSER_H


