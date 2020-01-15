#include "DBEditor.h"
#include "DBComboBox.h"
#include "NGSD.h"
#include "GUIHelper.h"

#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDialog>
#include <QMessageBox>

DBEditor::DBEditor(QWidget* parent, QString table, int id)
	: QWidget(parent)
	, table_(table)
	, id_(id)
{
	setMinimumSize(450, 100);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	createGUI();
	fillForm();
}

void DBEditor::createGUI()
{
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


		const TableFieldInfo& field_info = table_info.fieldInfo(field);
		if (field_info.is_primary_key && field!="id")
		{
			THROW(ProgrammingException, "Table contains primary key other than 'id'. Primary key field namy is '" + field + "'!");
		}
		//skip non-editable fields
		if (!isEditable(field_info)) continue;

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
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* edit = new QLineEdit(this);
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));

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
			QComboBox* selector = new QComboBox(this);

			QStringList items = db.getEnum(table_, field);
			if (field_info.is_nullable) items.prepend("");
			selector->addItems(items);

			widget = selector;
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = new QLineEdit(this);
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));
			//TODO date selector?

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* selector = new DBComboBox(this);

			selector->fill(db.createTable(field_info.fk_table, "SELECT id, name FROM " + field_info.fk_table), field_info.is_nullable);

			widget = selector;
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
		}
		widget->setObjectName("editor_" + field);
		if (isReadOnly(table_, field)) widget->setEnabled(false);

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
	//TODO needed for adding new item to the NGSD
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

		//skip non-editable fields
		if (!isEditable(field_info)) continue;

		QVariant value = query.value(field);
		bool is_null = query.isNull(field);

		if (field_info.type==TableFieldInfo::BOOL)
		{
			QCheckBox* box = getEditWidget<QCheckBox*>(field);
			box->setChecked(value.toBool());
		}
		else if (field_info.type==TableFieldInfo::INT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(is_null ? "" : value.toString());
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(is_null ? "" : value.toString());
		}
		else if (field_info.type==TableFieldInfo::TEXT)
		{
			QTextEdit* edit = getEditWidget<QTextEdit*>(field);
			edit->setText(value.toString());
		}
		else if (field_info.type==TableFieldInfo::VARCHAR)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(value.toString());
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QComboBox* edit = getEditWidget<QComboBox*>(field);
			edit->setCurrentText(value.toString());
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(value.isNull() ? "" : value.toDate().toString(Qt::ISODate));
		}
		else if (field_info.type==TableFieldInfo::DATETIME)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(value.toString());
		}
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* edit = getEditWidget<DBComboBox*>(field);

			if (!is_null)
			{
				edit->setCurrentId(value.toString());
			}
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
		}
	}
}

void DBEditor::check()
{
	QStringList errors;

	QString field = sender()->objectName().remove("editor_");
	const TableFieldInfo& field_info = NGSD().tableInfo(table_).fieldInfo(field);
	//qDebug() << __FUNCTION__ << field  << field_info.toString();

	if (field_info.type==TableFieldInfo::INT)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();

		//check null
		if (value.isEmpty() && !field_info.is_nullable)
		{
			errors << "Cannot be empty!";
		}

		//check if numeric
		if (!value.isEmpty())
		{
			bool ok = true;
			int value_numeric = value.toInt(&ok);
			if (!ok)
			{
				errors << "Cannot be converted to a integer number!";
			}
			else if (field_info.is_unsigned && value_numeric<0)
			{
				errors << "Must not be negative!";
			}
		}

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::FLOAT)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();

		//check null
		if (value.isEmpty() && !field_info.is_nullable)
		{
			errors << "Cannot be empty!";
		}

		//check if numeric
		if (!value.isEmpty())
		{
			bool ok = true;
			double value_numeric = value.toDouble(&ok);
			if (!ok)
			{
				errors << "Cannot be converted to a floating-point number!";
			}
			else if (field_info.is_unsigned && value_numeric<0)
			{
				errors << "Must not be negative!";
			}
		}

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::DATE)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();

		//check null
		if (value.isEmpty() && !field_info.is_nullable)
		{
			errors << "Cannot be empty!";
		}

		//check if valid date
		if (!value.isEmpty())
		{
			QDate date = QDate::fromString(value, Qt::ISODate);
			if (!date.isValid())
			{
				errors << "Must not be negative!";
			}
		}

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else
	{
		THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
	}

	//set errors
	errors_[field] = errors;

	//update dialog
	updateParentDialogButtonBox();
}

DBEditor::isEditable(const TableFieldInfo& info)
{
	if(info.is_primary_key) return false;
	if (info.type==TableFieldInfo::TIMESTAMP) return false;
	if (info.type==TableFieldInfo::DATETIME) return false;


	return true;
}

DBEditor::isReadOnly(const QString& table, const QString& field)
{
	if (table=="sample" && field=="name") return true;

	return false;
}

bool DBEditor::dataIsValid() const
{
	foreach(const QStringList& errors, errors_)
	{
		if (!errors.isEmpty()) return false;
	}

	return true;
}

void DBEditor::updateParentDialogButtonBox()
{
	QWidget* widget = this;
	while (widget->parentWidget()!=nullptr)
	{
		widget = widget->parentWidget();

		if (qobject_cast<QDialog*>(widget)!=nullptr)
		{
			QDialogButtonBox* box = widget->findChild<QDialogButtonBox*>();
			if (box!=nullptr)
			{
				box->button(QDialogButtonBox::Ok)->setEnabled(dataIsValid());
			}
			return;
		}
	}
}

void DBEditor::store()
{
	if (!dataIsValid()) THROW(ProgrammingException, "Cannot store data that is invalid to the database!");

	NGSD db;
	const TableInfo& table_info = db.tableInfo(table_);

	//create fields/values
	QStringList fields;
	QStringList values;
	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);

		//skip non-editable fields
		if (!isEditable(field_info)) continue;
		if (isReadOnly(table_, field)) continue;

		fields << field;

		if (field_info.type==TableFieldInfo::BOOL)
		{
			QCheckBox* box = getEditWidget<QCheckBox*>(field);
			values << (box->isChecked() ? "1" : "0");
		}
		else if (field_info.type==TableFieldInfo::INT)
		{
			QLineEdit* editor = getEditWidget<QLineEdit*>(field);
			QString value = editor->text().trimmed();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* editor = getEditWidget<QLineEdit*>(field);
			QString value = editor->text().trimmed();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else if (field_info.type==TableFieldInfo::TEXT)
		{
			QTextEdit* editor = getEditWidget<QTextEdit*>(field);
			QString value = editor->toPlainText().trimmed();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else if (field_info.type==TableFieldInfo::VARCHAR)
		{
			QLineEdit* editor = getEditWidget<QLineEdit*>(field);
			QString value = editor->text().trimmed();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QComboBox* editor = getEditWidget<QComboBox*>(field);
			QString value = editor->currentText();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* editor = getEditWidget<QLineEdit*>(field);
			QString value = editor->text();
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : QDate::fromString(value, Qt::ISODate).toString(Qt::ISODate));
		}
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* editor = getEditWidget<DBComboBox*>(field);
			QString value = editor->getCurrentId();
			values << (value.isEmpty() && field_info.is_nullable  ? "NULL" : value);
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "'!");
		}
	}

	//update/insert
	SqlQuery query = db.getQuery();
	try
	{
		if (id_!=-1)
		{
			QString query_str = "UPDATE " + table_ + " SET ";
			for(int i=0; i<fields.count(); ++i)
			{
				query_str += (i!=0 ? ", " : "") + fields[i] + "=?";
			}

			query.prepare(query_str);
			foreach(const QString& value, values)
			{
				query.addBindValue(value=="NULL" ? QVariant() : value);
			}
			query.exec();
		}
		else //insert
		{
			//TODO needed for adding new item to the NGSD
		}
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error storing data", "An error occured while storing data to the database. Please report this to your adminstrator:\n\nERROR:\n" + e.message() + "\n\nQUERY:\n" + query.executedQuery());
	}
}
