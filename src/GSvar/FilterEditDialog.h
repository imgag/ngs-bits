#ifndef FilterEditDialog_H
#define FilterEditDialog_H

#include <QDialog>
#include "ui_FilterEditDialog.h"
#include "FilterCascade.h"
#include "Exceptions.h"

//Edit dialog for filters.
class FilterEditDialog
	: public QDialog
{
	Q_OBJECT

public:
	FilterEditDialog(QSharedPointer<FilterBase> filter, QWidget *parent = 0);

private:
	Ui::FilterEditDialog ui_;
	QSharedPointer<FilterBase> filter_;

	//Set up form dynamically for filter parameters
	void setupForm();
	//Override to set filter parameters
	void done(int r) override;

	template <typename T>
	T getWidget(const QString& name) const
	{
		QList<T> widgets = findChildren<T>(name);

		if (widgets.count()==0)
		{
			THROW(ProgrammingException, "FilterEditDialog has no child with name " + name + "!");
		}
		if (widgets.count()>1)
		{
			THROW(ProgrammingException, "FilterEditDialog has more than one child with name " + name + "!");
		}

		return widgets[0];
	}

	template <typename T>
	QList<T> getWidgets(const QString& name) const
	{
		QList<T> widgets = findChildren<T>(name);

		if (widgets.count()==0)
		{
			THROW(ProgrammingException, "FilterEditDialog has no child with name " + name + "!");
		}

		return widgets;
	}
};

#endif // FilterEditDialog_H
