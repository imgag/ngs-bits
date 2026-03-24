#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>

class ErrorHandler : public QObject
{
	Q_OBJECT

public:
	static ErrorHandler* instance();
	static void displayError(QString);

signals:
	void displayErrorReq(QString);

protected:
	explicit ErrorHandler(QObject* parent = nullptr);
	Q_DISABLE_COPY(ErrorHandler);
};


#endif // ERRORHANDLER_H
