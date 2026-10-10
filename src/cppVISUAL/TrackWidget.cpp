#include "TrackWidget.h"
#include "SharedData.h"
#include "IgvTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "GenomeVisualizationWidget.h"
#include "QInputDialog"
#include <QMessageBox>
#include <QApplication>
#include <QFileInfo>
#include <QDrag>
#include <QDialog>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>
#include <limits>

TrackWidget::TrackWidget(QWidget* parent, QString file_path, QString display_name, QString type)
	: QWidget(parent)
	, file_path_(file_path)
	, display_name_(display_name)
	, type_(type)
	, settings_(type.toUtf8(), {}, {})
{
}

void TrackWidget::initializeSettings()
{
	settings_.reset(getParameters(), getParameterDefaults());
}

QSize TrackWidget::minimumSizeHint() const
{
	//Allow horizontal shrinking while keeping the label area and track height.
	return QSize(SharedData::settings().label_width + 5, sizeHint().height());
}

void TrackWidget::regionChanged()
{
	updateGeometry();
	update();
}

void TrackWidget::populateContextMenu(QMenu& menu, const QPoint&)
{
	if (menu.actions().count()>0) menu.addSeparator();

	QAction* remove = menu.addAction("Remove Track");
	connect(remove, &QAction::triggered, this, &TrackWidget::trackDeleted);

	QAction* rename = menu.addAction("Rename Track...");
	connect(rename, &QAction::triggered,this, &TrackWidget::handleTrackRename);
}

void TrackWidget::handleTrackRename()
{
	bool ok;
	QString new_name = QInputDialog::getText(this, "Enter Track Name", "", QLineEdit::Normal, display_name_, &ok);

	if (ok && !new_name.isEmpty()) display_name_ = new_name;
}


void TrackWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && event->pos().x() < SharedData::settings().label_width)
	{
		is_dragging_ = true;
		drag_start_pos_ = event->pos();
	}
	else
	{
		is_dragging_ = false;
		event->ignore();
	}
}


void TrackWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!is_dragging_) {
		event->ignore();
		return;
	};
	if (!(event->buttons() & Qt::LeftButton)) return;
	if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) return;

	QDrag* drag = new QDrag(this);
	QMimeData* mime_data = new QMimeData;
	// mime_data->setData("application/track-data", id_.toByteArray());
	mime_data->setData("application/track-name", display_name_.toUtf8());
	drag->setMimeData(mime_data);

	//draw the drag block
	int width = SharedData::settings().label_width;
	QPixmap pixmap(width, height());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);
	QRect rect(0, 0, width, height());
	painter.setPen(QPen(QColor(0, 0, 0), 2));
	painter.setBrush(Qt::NoBrush);

	painter.drawRect(rect);

	drag->setPixmap(pixmap);
	drag->setHotSpot(event->pos());

	drag->exec(Qt::MoveAction);
}

void TrackWidget::drawLabel(QPainter& painter)
{

	int label_width = SharedData::settings().label_width;

	// draw text
	QRectF text_rect(0, 0, label_width-2, height());

	painter.setPen(Qt::black);
	painter.drawText(text_rect, Qt::AlignLeft, display_name_);
}

float Viewport::genomePosToScreen(int genome_pos) const
{
	float scale = (float)total_width / region.length();
	return ((float)(genome_pos - region.start())) * scale + x0;
}

float Viewport::genomeWidthToScreen(int genome_width) const
{
	return ((float)genome_width/region.length()) * total_width;
}

bool Viewport::isOutOfDrawRegion(int x) const
{
	return (x < x0 || x > x0 + total_width);
}

int Viewport::screenXToGenomePos(int x_pos) const
{
	float p = (float)(x_pos - x0) / total_width;
	return region.start() + static_cast<int>(p * region.length());
}

Viewport TrackWidget::getViewport()
{
	int w = width();
	int label_width = SharedData::settings().label_width;
	int total_width = w - label_width - 4;
	const BedLine& region = SharedData::region();
	float pixels_per_base = (float)total_width / region.length();

	return {region, total_width, label_width + 2, pixels_per_base};
}

void TrackWidget::showInfoPopup(QPointF global_pos, QString info)
{
	QDialog* popup = new QDialog(this, Qt::Popup);
	popup->setAttribute(Qt::WA_DeleteOnClose);

	QLabel* label = new QLabel(info.trimmed());
	label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	label->setWordWrap(false);

	QVBoxLayout* layout = new QVBoxLayout(popup);
	layout->setContentsMargins(QMargins(4,4,4,4));
	layout->addWidget(label);

	popup->move(global_pos.x(), global_pos.y());
	popup->show();
}

QString TrackWidget::getDisplayNameFromFilePath(QString file_path)
{
	QString output;

	file_path = file_path.trimmed();

	if (Helper::isHttpUrl(file_path))
	{
		QString base_url = file_path.split('?')[0]; //remove arguments like tokens
		if (base_url.contains("/"))
		{
			output = base_url.split("/").last();
		}
		output = base_url;
	}
	else //local file
	{
		output = QFileInfo(file_path).fileName();
	}

	return output.isEmpty() ? file_path : output;
}

void TrackWidget::writeToXml(QXmlStreamWriter& writer)
{
	writer.writeStartElement("Track");
	writer.writeAttribute("type", type_);
	writer.writeAttribute("file_name", file_path_);
	writer.writeAttribute("display_name", display_name_);
	for(const Parameter& param: settings_.parameters())
	{
		writer.writeStartElement("Settings");
		writer.writeAttribute("name", QString::fromUtf8(param.name()));
		writer.writeAttribute("type", QString::fromUtf8(Parameter::toString(param.type())));
		if (param.type() == ParameterType::STRINGLIST)
		{
			for (const QByteArray& entry : param.value().value<QByteArrayList>())
			{
				writer.writeTextElement("Value", QString::fromUtf8(entry));
			}
		}
		else
		{
			//Preserve double precision when saving and loading a session.
			QByteArray value = param.value().toByteArray();
			if (param.type() == ParameterType::DOUBLE) value = QByteArray::number(param.value().toDouble(), 'g', std::numeric_limits<double>::max_digits10);
			else if (param.type() == ParameterType::BOOL) value = param.value().toBool() ? "yes" : "no";
			writer.writeTextElement("Value", QString::fromUtf8(value));
		}
		writer.writeEndElement(); // Settings
	}
	writer.writeEndElement(); // Track
}

TrackWidget* TrackWidget::fromXml(const QDomElement& track_element, QWidget* parent, QStringList& errors)
{
	QString type = track_element.attribute("type");
	QString file_path = track_element.attribute("file_name");
	QString display_name = track_element.attribute("display_name");

	//create track
	TrackWidget* track = nullptr;
	if (type == BedTrack::type()) track = BedTrack::createTrack(parent, file_path, display_name);
	else if (type == BamAlignmentTrack::type()) track = BamAlignmentTrack::createTrack(parent, file_path, display_name);
	else if (type == BamCoverageTrack::type()) track = BamCoverageTrack::createTrack(parent, file_path, display_name);
	else if (type == IgvTrack::type()) track = IgvTrack::createTrack(parent, file_path, display_name);
	else errors << ("Unsupported track type '"+type+"' for " + file_path);
	if (!track) return nullptr;

	//load settings
	for (QDomElement item = track_element.firstChildElement("Settings"); !item.isNull(); item = item.nextSiblingElement("Settings"))
	{
		const QByteArray key = item.attribute("name").toUtf8();
		try
		{
			const Parameter& parameter = track->settings_.parameter(key);
			if (item.attribute("type").toUtf8() != Parameter::toString(parameter.type()))
			{
				THROW(ArgumentException, "Parameter type does not match '" + Parameter::toString(parameter.type()) + "'.");
			}
			QByteArrayList values;
			for (QDomElement entry = item.firstChildElement("Value"); !entry.isNull(); entry = entry.nextSiblingElement("Value"))
			{
				values.append(entry.text().toUtf8());
			}
			if (parameter.type() != ParameterType::STRINGLIST && values.size() != 1)
			{
				THROW(ArgumentException, "Expected exactly one Value element.");
			}
			const QString value = values.isEmpty() ? QString() : QString::fromUtf8(values.first());
			bool ok = false;
			switch (parameter.type())
			{
				case ParameterType::INT:
				{
					const int number = value.toInt(&ok);
					if (!ok) THROW(ArgumentException, "Invalid integer '" + value + "'.");
					track->settings_.setInt(key, number);
					break;
				}
				case ParameterType::DOUBLE:
				{
					const double number = value.toDouble(&ok);
					if (!ok) THROW(ArgumentException, "Invalid double '" + value + "'.");
					track->settings_.setDouble(key, number);
					break;
				}
				case ParameterType::BOOL:
				{
					const QString boolean = value.trimmed().toLower();
					if (boolean != "yes" && boolean != "no" && boolean != "true" && boolean != "false" && boolean != "1" && boolean != "0")
					{
						THROW(ArgumentException, "Invalid boolean '" + value + "'.");
					}
					track->settings_.setBool(key, boolean == "yes" || boolean == "true" || boolean == "1");
					break;
				}
				case ParameterType::STRING:
				{
					track->settings_.setString(key, values.first());
					break;
				}
				case ParameterType::STRINGLIST:
				{
					track->settings_.setStringList(key, values);
					break;
				}
				default: THROW(ProgrammingException, "Unhandled parameter type.");
			}
		}
		catch (const Exception& e)
		{
			errors << "Invalid setting '" + QString::fromUtf8(key) + "' for " + file_path + ": " + e.message();
		}
	}

	return track;
}
