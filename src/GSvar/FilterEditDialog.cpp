#include "FilterEditDialog.h"
#include "Helper.h"
#include <QDebug>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QCheckBox>

FilterEditDialog::FilterEditDialog(QSharedPointer<FilterBase> filter, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, filter_(filter)
{
	ui_.setupUi(this);
	setWindowTitle("Filter '" + filter_->name() + "'");
	connect(ui_.show_doc, SIGNAL(toggled(bool)), this, SLOT(toggleDocumentation(bool)));

	ui_.description->setText(filter_->description().join("\n"));
	setupForm();
}

void FilterEditDialog::toggleDocumentation(bool visible)
{
	QList<QLabel*> labels = findChildren<QLabel*>(QRegularExpression("DOC_.*"));
	foreach(QLabel* label, labels)
	{
		label->setVisible(visible);
	}
}

void FilterEditDialog::setupForm()
{
	bool first_parameter = true;
	foreach(const FilterParameter& p, filter_->parameters())
	{
		QList<QWidget*> widgets;
		switch (p.type)
		{
			case INT:
			{
				QSpinBox* widget = new QSpinBox(this);
				bool min_set = p.constraints.contains("min");
				if (min_set)
				{
					widget->setMinimum(Helper::toInt(p.constraints["min"], p.name + " min"));
				}
				else
				{
					widget->setMinimum(-std::numeric_limits<int>::max());
				}
				bool max_set = p.constraints.contains("max");
				if (max_set)
				{
					widget->setMaximum(Helper::toInt(p.constraints["max"], p.name + " max"));
				}
				else
				{
					widget->setMaximum(std::numeric_limits<int>::max());
				}
				if (min_set && max_set)
				{
					widget->setSingleStep((widget->maximum() - widget->minimum())/100);
				}
				widget->setValue(p.value.toInt());
				widget->setMaximumWidth(100);
				widgets << widget;
				break;
			}
			case DOUBLE:
			{
				auto widget = new QDoubleSpinBox(this);
				bool min_set = p.constraints.contains("min");
				if (min_set)
				{
					widget->setMinimum(Helper::toDouble(p.constraints["min"], p.name + " min"));
				}
				else
				{
					widget->setMinimum(-std::numeric_limits<double>::max());
				}
				bool max_set = p.constraints.contains("max");
				if (max_set)
				{
					widget->setMaximum(Helper::toDouble(p.constraints["max"], p.name + " max"));
				}
				else
				{
					widget->setMaximum(std::numeric_limits<double>::max());
				}
				if (min_set && max_set)
				{
					widget->setSingleStep((widget->maximum() - widget->minimum())/100.0);
				}
				widget->setValue(p.value.toDouble());
				widget->setMaximumWidth(100);
				widgets << widget;
				break;
			}
			case BOOL:
			{
				QButtonGroup* group = new QButtonGroup(this);

				QRadioButton* button = new QRadioButton("yes", this);
				group->addButton(button);
				button->setChecked(p.value.toBool());
				widgets << button;

				button = new QRadioButton("no", this);
				group->addButton(button);
				button->setChecked(!p.value.toBool());
				widgets << button;

				break;
			}
			case STRING:
			{
				QString value = p.value.toString();
				if(p.constraints.contains("valid"))
				{
					QButtonGroup* group = new QButtonGroup(this);

					QStringList valid = p.constraints["valid"].split(',');
					foreach(QString text, valid)
					{
						QRadioButton* button = new QRadioButton(text, this);
						group->addButton(button);
						button->setChecked(value==text);
						widgets << button;
					}
				}
				else
				{
					auto widget = new QLineEdit(this);
					widget->setText(value);
					widgets << widget;
				}
				break;
			}
			case STRINGLIST:
			{
				QStringList values = p.value.toStringList();

				if(p.constraints.contains("valid"))
				{
					QStringList valid = p.constraints["valid"].split(',');
					foreach(QString text, valid)
					{
						QCheckBox* box = new QCheckBox(text, this);
						box->setChecked(values.contains(text));
						widgets << box;
					}
				}
				else
				{
					auto widget = new QPlainTextEdit(this);
					widget->setPlainText(values.join("\n"));
					widget->setToolTip("One entry per line");
					widgets << widget;
				}
				break;
			}
			default:
				THROW(ProgrammingException, "Unknown filter type '" + FilterParameter::typeAsString(p.type) + "' in FilterEditDialog!");
		}

		//add label and edit widget(s)
		for(int i=0; i<widgets.count(); ++i)
		{
			//prepare label
			QLabel* label = nullptr;
			if (i==0)
			{
				label = new QLabel();
				label->setText(p.name + ":");
			}

			//add widget
			widgets[i]->setObjectName(p.name);
			ui_.form_layout->addRow(label, widgets[i]);

			//set focus to first widget
			if (first_parameter)
			{
				widgets[0]->setFocus();
				first_parameter = false;
			}
		}

		//add documentation
		QLabel* doc_label = new QLabel("Description: " + p.description);
		doc_label->setObjectName("DOC_" + p.name);
		doc_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
		ui_.form_layout->addRow(nullptr, doc_label);
		doc_label->setVisible(false);
	}
}

void FilterEditDialog::done(int r)
{
	if(r == QDialog::Accepted)
	{
		foreach (const FilterParameter& p, filter_->parameters())
		{
			try
			{
				switch (p.type)
				{
					case INT:
					{
						filter_->setInteger(p.name, getWidget<QSpinBox*>(p.name)->value());
						break;
					}
					case DOUBLE:
					{
						filter_->setDouble(p.name, getWidget<QDoubleSpinBox*>(p.name)->value());
						break;
					}
					case BOOL:
					{
						QList<QRadioButton*> buttons = getWidgets<QRadioButton*>(p.name);
						foreach(QRadioButton* button, buttons)
						{
							if (button->text()=="yes")
							{
								filter_->setBool(p.name, button->isChecked());
							}
						}
						break;
					}
					case STRING:
					{
						if(p.constraints.contains("valid"))
						{
							QList<QRadioButton*> buttons = getWidgets<QRadioButton*>(p.name);
							foreach(QRadioButton* button, buttons)
							{
								if (button->isChecked())
								{
									filter_->setString(p.name, button->text());
								}
							}
						}
						else
						{
							filter_->setString(p.name, getWidget<QLineEdit*>(p.name)->text());
						}
						break;
					}
					case STRINGLIST:
					{
						if(p.constraints.contains("valid"))
						{
							QStringList selected;
							QList<QCheckBox*> boxes = getWidgets<QCheckBox*>(p.name);
							foreach(QCheckBox* box, boxes)
							{
								if (box->isChecked())
								{
									selected << box->text();
								}
							}
							filter_->setStringList(p.name, selected);
						}
						else
						{
							QStringList entries = getWidget<QPlainTextEdit*>(p.name)->toPlainText().split('\n');
							entries.removeAll("");
							filter_->setStringList(p.name, entries);
						}
						break;
					}
					default:
						THROW(ProgrammingException, "Unknown filter type '" + FilterParameter::typeAsString(p.type) + "' in FilterEditDialog!");
				}
			}
			catch (const Exception& e)
			{
				QMessageBox::warning(this, "Error while setting parameter " + p.name, e.message());
				return;
			}
		}
	}

	QDialog::done(r);
}

