#ifndef DBEDITOR_H
#define DBEDITOR_H

#include "Exceptions.h"
#include "NGSD.h"

#include <QWidget>

//Editor for single DB item
class DBEditor
	: public QWidget
{
	Q_OBJECT
public:
	//Constructor. If 'id' is unset, dafault data is filled into the editor widgets.
	DBEditor(QWidget* parent, QString table, int id=-1);

	//Store the DB item.
	void store();

protected slots:
	//creates the layout and the widgets
	void createGUI();
	//fills the widgets with data from the NGSD
	void fillForm();
	void fillFormWithDefaultData();
	void fillFormWithItemData();
	//checks the form data of the sender widget and shows errors in the GUI (also stores them in 'errors_')
	void check();

private:
	QString table_;
	int id_;
	QHash<QString, QStringList> errors_;

	//returns if a table field is editable
	static isEditable(const TableFieldInfo& info);

	//returns if a table field is read-only
	static isReadOnly(const QString& table, const QString& field);

	//returns if the for data is valid (based on 'errors_')
	bool dataIsValid() const;

	//updates the surrounding dialog ok button
	void updateParentDialogButtonBox();

	//returns a edit widget of the given type and name
	template<typename T>
	inline T getEditWidget(const QString &name) const
	{
		T widget = findChild<T>("editor_" + name);
		if (widget==nullptr) THROW(ProgrammingException, "Could not find widget with name 'editor_" + name + "'!");
		return widget;
	}

};

#endif // DBEDITOR_H
