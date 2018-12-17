#ifndef GBDOFKEDIT_H
#define GBDOFKEDIT_H

#include <QLineEdit>
#include <QCompleter>
#include "NGSD.h"

///Database-aware Foreign-Key edit
class GBDOFKEdit
	: public QLineEdit
{
	Q_OBJECT

public:
	GBDOFKEdit(QString table, QString field, QWidget* parent=0);

	///Sets a valid FK ID.
	void setId(int id);
	///Returns the FK ID, or -1 if unset/invalid.
	int id();

protected slots:
	void search(QString text);

protected:
	QString table_;
	QString field_;
	int id_;
	NGSD db_;
};

#endif // GBDOFKEDIT_H
