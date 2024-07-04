#include "DBEditor.h"
#include "DBComboBox.h"
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
#include <QCalendarWidget>
#include <QToolButton>
#include <QApplication>
#include <QCryptographicHash>

DBEditor::DBEditor(QWidget* parent, QString table, int id)
	: QWidget(parent)
	, db_()
	, table_(table)
	, id_(id)
{
	setMinimumSize(450, 16);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QApplication::setOverrideCursor(Qt::BusyCursor);
	createGUI();
	fillForm();
	QApplication::restoreOverrideCursor();
}

void DBEditor::createGUI()
{
	//layout
	QGridLayout* layout = new QGridLayout();
	layout->setMargin(0);
	layout->setSpacing(3);
	layout->setColumnStretch(0,0);
	layout->setColumnStretch(1,1);
	layout->setColumnStretch(2,0);
	layout->setSizeConstraint(QLayout::SetMinimumSize);
	setLayout(layout);

	//widgets
	const TableInfo & table_info = db_.tableInfo(table_);

	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);
		if (field_info.is_primary_key && field!="id")
		{
			THROW(ProgrammingException, "Table contains primary key other than 'id'. Primary key field name is '" + field + "'!");
		}

		//skip non-editable fields
		if (field_info.is_hidden) continue;

		//create label
		QLabel* label = new QLabel();
		label->setObjectName("label_" + field);
		label->setText(field_info.label + ":");
		label->setAlignment(Qt::AlignTop|Qt::AlignLeft);

		//create widget
		QWidget* widget = nullptr;
		QWidget* add_widget = nullptr;
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
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
		{
			QLineEdit* edit = new QLineEdit(this);
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));

			widget = edit;
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QComboBox* selector = new QComboBox(this);

			QStringList items = field_info.type_constraints.valid_strings;
			if (field_info.is_nullable) items.prepend("");
			selector->addItems(items);

			widget = selector;
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = new QLineEdit(this);
			connect(edit, SIGNAL(textChanged(QString)), this, SLOT(check()));

			widget = edit;

			//edit button for date
			QToolButton* btn = new QToolButton();
			btn->setIcon(QIcon(":/Icons/date_picker.png"));
			connect(btn, SIGNAL(clicked(bool)), this, SLOT(editDate()));

			add_widget = btn;
		}
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* selector = new DBComboBox(this);

			if (field_info.fk_name_sql=="") THROW(ProgrammingException, "Foreign key name SQL not set for table/field: " + table_ + "/" + field_info.name);

			DBTable entries = db_.createTable(field_info.fk_table, "SELECT id, " + field_info.fk_name_sql + " as display_value FROM " + field_info.fk_table + " ORDER BY display_value");
			selector->fill(entries, field_info.is_nullable);

			//adjusting the size hint to the content can be slow, thus we use the fixed size.
			//the size hint is not used anyway because the layout determines the size.
			selector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
			selector->setMinimumWidth(400);

			//Handle rare case that mandatory FK points to empty table.
			if (entries.rowCount()==0 && !field_info.is_nullable)
			{
				QString error = "Missing entries in table '" + field_info.fk_table + "' - please fill this table first!";
				errors_[field] = QStringList() << error;
				selector->setToolTip(error);
				selector->setStyleSheet("QComboBox {border: 2px solid red;}");
			}

			widget = selector;
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + field_info.typeAsString() + "'!");
		}
		widget->setObjectName("editor_" + field);
		widget->setEnabled(!field_info.is_readonly || id_==-1);

		//tooltip
		if (!field_info.tooltip.isEmpty())
		{
			if (add_widget==nullptr)
			{
				QLabel* info_label = new QLabel();
				info_label->setPixmap(QPixmap(":/Icons/Info.png"));
				info_label->setScaledContents(true);
				info_label->setFixedSize(16, 16);
				add_widget = info_label;
			}

			add_widget->setToolTip(field_info.tooltip);
		}

		//add widgets to layout
		int row = layout->rowCount();
		layout->addWidget(label, row, 0);
		if (add_widget==nullptr)
		{
			layout->addWidget(widget, row, 1, 1, 2);
		}
		else
		{
			layout->addWidget(widget, row, 1);
			layout->addWidget(add_widget, row, 2);
			add_widget->setObjectName("add_" + field);
		}
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

	checkAllFields();
}

void DBEditor::fillFormWithDefaultData()
{
	const TableInfo& table_info = db_.tableInfo(table_);

	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);

		//skip non-editable fields
		if (field_info.is_hidden) continue;

		//skip fields without default value
		if (field_info.default_value=="") continue;

		if (field_info.type==TableFieldInfo::BOOL)
		{
			QCheckBox* box = getEditWidget<QCheckBox*>(field);
			box->setChecked(field_info.default_value=="1");
		}
		else if (field_info.type==TableFieldInfo::INT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::FLOAT)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::TEXT)
		{
			QTextEdit* edit = getEditWidget<QTextEdit*>(field);
			edit->setText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::VARCHAR)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText("");
		}
		else if (field_info.type==TableFieldInfo::ENUM)
		{
			QComboBox* edit = getEditWidget<QComboBox*>(field);
			edit->setCurrentText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::DATE)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(field_info.default_value);
		}
		else if (field_info.type==TableFieldInfo::FK)
		{
			DBComboBox* edit = getEditWidget<DBComboBox*>(field);
			edit->setCurrentId(field_info.default_value);
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + field_info.typeAsString() + "'!");
		}
	}
}

void DBEditor::fillFormWithItemData()
{
	const TableInfo& table_info = db_.tableInfo(table_);

	SqlQuery query = db_.getQuery();
	query.exec("SELECT * FROM " + table_ + " WHERE id=" + QString::number(id_));
	query.next();
	if (query.size()!=1) THROW(ProgrammingException, "Table '" + table_ + "' contains no row with 'id' " + QString::number(id_) + "!");

	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);

		//skip non-editable fields
		if (field_info.is_hidden) continue;

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
		else if (field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
		{
			QLineEdit* edit = getEditWidget<QLineEdit*>(field);
			edit->setText(NGSD::passwordReplacement());
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
			THROW(ProgrammingException, "Unhandled table field type '" + field_info.typeAsString() + "'!");
		}
	}
}

void DBEditor::check()
{
	QString field = sender()->objectName().remove("editor_");
	check(field);
}

void DBEditor::check(QString field)
{
	QStringList errors;

	const TableFieldInfo& field_info = db_.tableInfo(table_).fieldInfo(field);
	//qDebug() << __FUNCTION__ << __LINE__ << field  << field_info.toString();

	if (field_info.type==TableFieldInfo::INT)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();
		errors = db_.checkValue(table_, field, value, id_==-1);

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::FLOAT)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();
		errors = db_.checkValue(table_, field, value, id_==-1);

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::DATE)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();
		errors = db_.checkValue(table_, field, value, id_==-1);

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::VARCHAR)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();
		errors = db_.checkValue(table_, field, value, id_==-1);

		//special handling of gene columns
		if ((field=="symbol" && table_=="somatic_pathway_gene") || (field=="symbol" && table_=="somatic_gene_role"))
		{
			if (db_.geneId(value.toUtf8())==-1)
			{
				errors << "Gene name '" + value + "' is not a HGNC-approved gene name!";
			}
		}

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else if (field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
	{
		QLineEdit* edit = getEditWidget<QLineEdit*>(field);
		QString value = edit->text().trimmed();
		if (value!=NGSD::passwordReplacement() || id_==-1)
		{
			errors = db_.checkValue(table_, field, value, id_==-1);
		}

		//update GUI
		edit->setToolTip(errors.join("\n"));
		edit->setStyleSheet(errors.isEmpty() ?  "" : "QLineEdit {border: 2px solid red;}");
	}
	else
	{
		//the other fields don't need to be checked
		//qDebug() << "Unhandled table field type '" + QString::number(field_info.type) + "' in check(QString)!";
	}

	//set errors
	errors_[field] = errors;

	//update dialog
	updateParentDialogButtonBox();
}

void DBEditor::checkAllFields()
{
	QList<QWidget*> editors = findChildren<QWidget*>(QRegularExpression("^editor_.*"));
	foreach(QWidget* editor, editors)
	{
		QString field = editor->objectName().replace("editor_", "");
		check(field);
	}
}

void DBEditor::editDate()
{
	QString field = sender()->objectName().replace("add_", "");

	QLineEdit* edit = getEditWidget<QLineEdit*>(field);
	QString value = edit->text().trimmed();

	QCalendarWidget* widget = new QCalendarWidget();
	if (!value.isEmpty())
	{
		widget->setSelectedDate(QDate::fromString(value, Qt::ISODate));
	}

	auto dlg = GUIHelper::createDialog(widget, "Select date", "", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		edit->setText(widget->selectedDate().toString(Qt::ISODate));
	}
}

void DBEditor::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);

	//the widget can be created before the parent dialog, thus we need to update the button box once the dialog is shown
	updateParentDialogButtonBox();
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

	const TableInfo& table_info = db_.tableInfo(table_);

	//create fields/values
	QStringList fields;
	QStringList values;
	QString salt;
	foreach(const QString& field, table_info.fieldNames())
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(field);

		//skip non-editable fields
		if (field_info.is_hidden) continue;
		if (field_info.is_readonly && id_!=-1) continue;

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
		else if (field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
		{
			QLineEdit* editor = getEditWidget<QLineEdit*>(field);
			QString value = editor->text().trimmed();
			if (value==NGSD::passwordReplacement()) //password unchanged > use old password
			{
				values << db_.getValue("SELECT " + field_info.name + " FROM " + table_ + " WHERE id=" + QString::number(id_)).toString();
			}
			else
			{
				salt = Helper::randomString(40);
				values << QCryptographicHash::hash((salt+value).toUtf8(), QCryptographicHash::Sha1).toHex();
			}
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
			values << (value.isEmpty() && field_info.is_nullable ? "NULL" : value);
		}
		else
		{
			THROW(ProgrammingException, "Unhandled table field type '" + field_info.typeAsString() + "'!");
		}
	}

	//special handling of password column with associated salt columnn
	if (!salt.isEmpty())
	{
		fields << "salt";
		values << salt;
	}

	//update/insert
	SqlQuery query = db_.getQuery();
	try
	{
		if (id_!=-1)
		{
			QString query_str = "UPDATE " + table_ + " SET ";
			for(int i=0; i<fields.count(); ++i)
			{
				query_str += (i!=0 ? ", " : "") + fields[i] + "=?";
			}
			query_str += " WHERE id=" + QString::number(id_);

			query.prepare(query_str);
			foreach(const QString& value, values)
			{
				query.addBindValue(value=="NULL" ? QVariant() : value);
			}
			query.exec();
		}
		else //insert
		{
			QString query_str = "INSERT INTO " + table_ + " (" + fields.join(", ") + ") VALUES (";
			for(int i=0; i<fields.count(); ++i)
			{
				query_str +=  (i!=0 ? ", " : "") + QString("?");
			}
			query_str += ")";

			query.prepare(query_str);
			foreach(const QString& value, values)
			{
				query.addBindValue(value=="NULL" ? QVariant() : value);
			}
			query.exec();
		}
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error storing data", "An error occured while storing data to the database.\n\nERROR:\n" + e.message());
	}
}
