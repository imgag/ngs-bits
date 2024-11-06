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
	//checks the form data of the given field (also stores them in 'errors_')
	void check(QString field_name);
	//check all fields
	void checkAllFields();
	//edit date of QLineEdit corresponding to the sender button
	void editDate();
	//edit processed sample of QLineEdit of corresponding sender button
	void editProcessedSample();

protected:
	//reimplemented show event to update parent dialog button box
	virtual void showEvent(QShowEvent* event) override;

private:
	NGSD db_;
	QString table_;
	int id_;
	QHash<QString, QStringList> errors_;

	//returns if the for data is valid (based on 'errors_')
	bool dataIsValid() const;

	//updates the surrounding dialog ok button
	void updateParentDialogButtonBox();

	//returns a edit widget of the given type and name
	template<typename T>
	inline T getEditWidget(const QString& field_name) const
	{
		T widget = findChild<T>("editor_" + field_name);
		if (widget==nullptr) THROW(ProgrammingException, "Could not find widget with name 'editor_" + field_name + "'!");
		return widget;
	}

};

#endif // DBEDITOR_H
