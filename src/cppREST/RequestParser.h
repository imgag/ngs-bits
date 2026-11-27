#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

#include "cppREST_global.h"
#include <QObject>
#include <QList>
#include <QMap>
#include "HttpRequest.h"

// Kept so that other translation units which may still use them keep compiling.
// RequestParser itself no longer uses them; delete once nothing else references them.
const QString empty_line = "\r\n\r\n";
const char end_of_line = '\n';

class CPPRESTSHARED_EXPORT RequestParser : public QObject
{
	Q_OBJECT

public:
	RequestParser();
	HttpRequest parse(QByteArray *request) const;

private:
	QMap<QString, QString> getVariables(const QByteArray& input) const;
	QByteArray getVariableSequence(const QByteArray& url) const;
	QString getRequestPrefix(const QList<QString>& path_items) const;
	QString getRequestPath(const QList<QString>& path_items) const;
	QList<QString> getRequestPathParams(const QList<QString>& path_items) const;
	RequestMethod inferRequestMethod(const QByteArray& input) const;

	// Multipart handling works on offsets into the original buffer to avoid copying the payload.
	void parseMultipart(const QByteArray& data, int body_start, const QByteArray& boundary, HttpRequest& request) const;
	QList<int> getBoundaryStartPositions(const QByteArray& data, int from, const QByteArray& marker) const;
	QByteArray getPartHeaderParam(const QByteArray& part_headers, const QByteArray& key) const;
};

#endif // REQUESTPARSER_H