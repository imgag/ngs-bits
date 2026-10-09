#include "FilterEditDialog.h"
#include "Helper.h"
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

	ui_.description->setText(filter_->description().join("\n"));
	setupForm();
}

void FilterEditDialog::setupForm()
{
	bool first_parameter = true;
	foreach(const Parameter& p, filter_->parameters())
	{
		const QString parameter_name = QString::fromUtf8(p.name());
		QList<QWidget*> widgets;
		switch (p.type())
		{
			case ParameterType::INT:
			{
				QSpinBox* widget = new QSpinBox(this);
				bool min_set = p.constraints().contains(ConstraintType::MIN);
				if (min_set)
				{
					widget->setMinimum(Helper::toInt(p.constraints().value(ConstraintType::MIN).toString(), parameter_name + " min"));
				}
				else
				{
					widget->setMinimum(-std::numeric_limits<int>::max());
				}
				bool max_set = p.constraints().contains(ConstraintType::MAX);
				if (max_set)
				{
					widget->setMaximum(Helper::toInt(p.constraints().value(ConstraintType::MAX).toString(), parameter_name + " max"));
				}
				else
				{
					widget->setMaximum(std::numeric_limits<int>::max());
				}
				if (min_set && max_set)
				{
					widget->setSingleStep((widget->maximum() - widget->minimum())/100);
				}
				widget->setValue(p.value().toInt());
				widget->setMaximumWidth(100);
				widgets << widget;
				break;
			}
			case ParameterType::DOUBLE:
			{
				auto widget = new QDoubleSpinBox(this);
				bool min_set = p.constraints().contains(ConstraintType::MIN);
				if (min_set)
				{
					widget->setMinimum(Helper::toDouble(p.constraints().value(ConstraintType::MIN).toString(), parameter_name + " min"));
				}
				else
				{
					widget->setMinimum(-std::numeric_limits<double>::max());
				}
				bool max_set = p.constraints().contains(ConstraintType::MAX);
				if (max_set)
				{
					widget->setMaximum(Helper::toDouble(p.constraints().value(ConstraintType::MAX).toString(), parameter_name + " max"));
				}
				else
				{
					widget->setMaximum(std::numeric_limits<double>::max());
				}
				if (min_set && max_set)
				{
					widget->setSingleStep((widget->maximum() - widget->minimum())/100.0);
				}
				widget->setValue(p.value().toDouble());
				widget->setMaximumWidth(100);
				widgets << widget;
				break;
			}
			case ParameterType::BOOL:
			{
				QButtonGroup* group = new QButtonGroup(this);

				QRadioButton* button = new QRadioButton("yes", this);
				group->addButton(button);
				button->setChecked(p.value().toBool());
				widgets << button;

				button = new QRadioButton("no", this);
				group->addButton(button);
				button->setChecked(!p.value().toBool());
				widgets << button;

				break;
			}
			case ParameterType::STRING:
			{
				QString value = p.value().toString();
				if(p.constraints().contains(ConstraintType::ALLOWED_VALUES))
				{
					QButtonGroup* group = new QButtonGroup(this);

					QStringList valid = p.constraints().value(ConstraintType::ALLOWED_VALUES).toString().split('\t');
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
			case ParameterType::STRINGLIST:
			{
				QStringList values;
				for (const QByteArray& entry : p.value().value<QByteArrayList>()) values.append(QString::fromUtf8(entry));

				if(p.constraints().contains(ConstraintType::ALLOWED_VALUES))
				{
					QStringList valid = p.constraints().value(ConstraintType::ALLOWED_VALUES).toString().split('\t');
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
				THROW(ProgrammingException, "Unknown filter type '" + Parameter::toString(p.type()) + "' in FilterEditDialog!");
		}

		//add label and edit widget(s)
		for(int i=0; i<widgets.count(); ++i)
		{
			//prepare label
			QLabel* label = nullptr;
			if (i==0)
			{
				label = new QLabel();
				label->setText(parameter_name + ":");
				label->setToolTip(p.description());
			}

			//add widget
			widgets[i]->setObjectName(parameter_name);
			ui_.form_layout->addRow(label, widgets[i]);

			//set focus to first widget
			if (first_parameter)
			{
				widgets[0]->setFocus();
				first_parameter = false;
			}
		}
	}
}

void FilterEditDialog::done(int r)
{
	if(r == QDialog::Accepted)
	{
		foreach (const Parameter& p, filter_->parameters())
		{
			const QString parameter_name = QString::fromUtf8(p.name());
			try
			{
				switch (p.type())
				{
					case ParameterType::INT:
					{
						filter_->setInteger(parameter_name, getWidget<QSpinBox*>(parameter_name)->value());
						break;
					}
					case ParameterType::DOUBLE:
					{
						filter_->setDouble(parameter_name, getWidget<QDoubleSpinBox*>(parameter_name)->value());
						break;
					}
					case ParameterType::BOOL:
					{
						QList<QRadioButton*> buttons = getWidgets<QRadioButton*>(parameter_name);
						foreach(QRadioButton* button, buttons)
						{
							if (button->text()=="yes")
							{
								filter_->setBool(parameter_name, button->isChecked());
							}
						}
						break;
					}
					case ParameterType::STRING:
					{
						if(p.constraints().contains(ConstraintType::ALLOWED_VALUES))
						{
							QList<QRadioButton*> buttons = getWidgets<QRadioButton*>(parameter_name);
							foreach(QRadioButton* button, buttons)
							{
								if (button->isChecked())
								{
									filter_->setString(parameter_name, button->text());
								}
							}
						}
						else
						{
							filter_->setString(parameter_name, getWidget<QLineEdit*>(parameter_name)->text());
						}
						break;
					}
					case ParameterType::STRINGLIST:
					{
						if(p.constraints().contains(ConstraintType::ALLOWED_VALUES))
						{
							QStringList selected;
							QList<QCheckBox*> boxes = getWidgets<QCheckBox*>(parameter_name);
							foreach(QCheckBox* box, boxes)
							{
								if (box->isChecked())
								{
									selected << box->text();
								}
							}
							filter_->setStringList(parameter_name, selected);
						}
						else
						{
							QStringList entries = getWidget<QPlainTextEdit*>(parameter_name)->toPlainText().split('\n');
							entries.removeAll("");
							filter_->setStringList(parameter_name, entries);
						}
						break;
					}
					default:
						THROW(ProgrammingException, "Unknown filter type '" + Parameter::toString(p.type()) + "' in FilterEditDialog!");
				}
			}
			catch (const Exception& e)
			{
				QMessageBox::warning(this, "Error while setting parameter " + parameter_name, e.message());
				return;
			}
		}
	}

	QDialog::done(r);
}
