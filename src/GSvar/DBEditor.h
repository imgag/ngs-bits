#ifndef DBEDITOR_H
#define DBEDITOR_H

#include "Exceptions.h"

#include <QWidget>

//Editor for single DB item
class DBEditor
	: public QWidget
{
	Q_OBJECT
public:
	//Constructor. If 'id' is unset, dafault data is filled into the editor widgets.
	DBEditor(QWidget* parent, QString table, int id=-1);

signals:

protected slots:
	//creates the layout and the widgets
	void createGUI();
	//fills the widgets with data from the NGSD
	void fillForm();
	void fillFormWithDefaultData();
	void fillFormWithItemData();

private:
	QString table_;
	int id_;

	template<typename T>
	inline T getEditWidget(const QString &name) const
	{
		T widget = findChild<T>("editor_" + name);
		if (widget==nullptr) THROW(ProgrammingException, "Could not find widget with name 'editor_" + name + "'!");
		return widget;
	}
};

#endif // DBEDITOR_H
