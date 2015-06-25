#ifndef WORKERBASE_H
#define WORKERBASE_H

#include "cppCORE_global.h"
#include <QObject>
#include <QThread>

///Base class for workers that do calculations in a background thread.
class CPPCORESHARED_EXPORT WorkerBase
		: public QObject
{
	Q_OBJECT

public:
	///Constructor. The name is used for logging the processing time.
	WorkerBase(QString name);
	///Processing function that has to be implemented in derived classes. Set the error_message_ string in this method to make the processing as unsuccessful.
	virtual void process() = 0;
	///Returns the processing error message, or an empty string if the processing was successful.
	QString errorMessage() const;

signals:
	///Signal that is emitted when the processing is finished.
	void finished(bool successful);

public slots:
	///Starts the processing.
	void start();
	///Deletes this object and the background thread.
	void deleteLater();

protected slots:
	void processInternal();

protected:
	QThread* thread_;
	QString error_message_;
};

#endif // WORKERBASE_H
