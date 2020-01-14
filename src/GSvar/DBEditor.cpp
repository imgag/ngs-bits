#include "DBEditor.h"
#include "DBComboBox.h"
#include "NGSD.h"

#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>

DBEditor::DBEditor(QWidget* parent, QString table, int id)
	: QWidget(parent)
	, table_(table)
	, id_(id)
{
	qDebug() << __FILE__ << __LINE__;
	createGUI();
	qDebug() << __FILE__ << __LINE__;
	fillForm();
	qDebug() << __FILE__ << __LINE__;
}

void DBEditor::createGUI()
{
	qDebug() << __FILE__ << __LINE__;
	//layout
	QFormLayout* layout = new QFormLayout();
	layout->setMargin(0);
	layout->setSpacing(3);
	setLayout(layout);

	//widgets
	NGSD db;
	const TableInfo & table_info = db.tableInfo(table_);

	foreach(const QString& field, table_info.fieldNames())
	{
		//create label
		QLabel* label = new QLabel();
		label->setObjectName("label_" + field);
		label->setText(field + ":");
		//TODO use comments from schema as label->setToolTip()

		//skip primary key
		const TableFieldInfo& field_info = table_info.fieldInfo(field);
		qDebug() << __FILE__ << __LINE__ << field << " type=" << field_info.type << "restriction=" << field_info.type_restiction << " nullable=" << field_info.nullable << " default=" << field_info.default_value << " fk=" << field_info.fk_table << "/" << field_info.fk_field;
		if (field_info.primary_key)
		{
			if (field!="id") THROW(ProgrammingException, "Table contains primary key other than 'id'. Primary key field namy is '" + field + "'!");
			continue;
		}

		//create widget
		QWidget* widget = nullptr;
		if (field_info.type==TableFieldInfo::BOOL)
		{
			QCheckBox* box = new QCheckBox(this);

			widget = box;
		}
		else if (field_info.type==TableFieldInfo::INT)
		{
			QLineEdit* edit = new QLineEdit(this);
			//TODO validator

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* edit = new QLineEdit(this);
			//TODO validator

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::TEXT)
		{
			QTextEdit* edit =  new QTextEdit(this);

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::VARCHAR)
		{
			QLineEdit* edit = new QLineEdit(this);

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QLineEdit* edit = new QLineEdit(this);
			//TODO auto-complete

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = new QLineEdit(this);
			//TODO date selector

			widget = edit;
		}
		//TODO DATETIME, TIMESTAMP
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* selector = new DBComboBox(this);
			//TODO auto-complete

			widget = selector;
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
		}
		widget->setObjectName("editor_" + field);

		//add to layout
		layout->addRow(label, widget);
	}
}

void DBEditor::fillForm()
{
	if (id_==-1)
	{
		fillFormWithDefaultData();
	}
	else
	{
		fillFormWithItemData();
	}
}

void DBEditor::fillFormWithDefaultData()
{
	//TODO
}

void DBEditor::fillFormWithItemData()
{
	NGSD db;
	const TableInfo& table_info = db.tableInfo(table_);

	SqlQuery query = db.getQuery();
	query.exec("SELECT * FROM " + table_ + " WHERE id=" + QString::number(id_));
	query.next();
	if (query.size()!=1) THROW(ProgrammingException, "Table '" + table_ + "' contains no row with 'id' " + QString::number(id_) + "!");

	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);

		//skip primary key
		if (field_info.primary_key) continue;

		if (field_info.type==TableFieldInfo::BOOL)
		{
			QCheckBox* box = getEditWidget<QCheckBox*>(field);
			box->setChecked(query.value(field).toBool());
		}
		else if (field_info.type==TableFieldInfo::INT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		else if (field_info.type==TableFieldInfo::TEXT)
		{
			QTextEdit* edit = getEditWidget<QTextEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		else if (field_info.type==TableFieldInfo::VARCHAR)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(query.value(field).toString());
		}
		//TODO DATETIME, TIMESTAMP
		else if (field_info.type==TableFieldInfo::FK)
		{
			//TODO
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
		}
	}
}
