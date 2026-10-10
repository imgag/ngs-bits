#include "ui_GenomeVisualizationWidget.h"
#include "GenomeVisualizationWidget.h"
#include "BedFile.h"
#include "GUIHelper.h"
#include "SharedData.h"
#include "XmlHelper.h"
#include "Settings.h"
#include "ParameterEditor.h"
#include <QToolTip>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSignalBlocker>

GenomeVisualizationWidget::GenomeVisualizationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::GenomeVisualizationWidget)
	, timer_(this, true)
{
	ui_->setupUi(this);
	//QMainWindow defaults to a top-level window even when it has a parent.
	ui_->dock_window->setWindowFlags(Qt::Widget);
	ui_->settings_dock->hide();
	connect(ui_->track_group_manager, &TrackGroupManager::trackSelected, this, &GenomeVisualizationWidget::selectTrack);
	connect(ui_->settings_btn, &QToolButton::clicked, this, &GenomeVisualizationWidget::toggleSettingsVisibility);
	connect(ui_->settings_dock, &QDockWidget::visibilityChanged, this, [this](bool visible)
	{
		QSignalBlocker blocker(ui_->settings_btn);
		ui_->settings_btn->setChecked(visible);
	});
	GUIHelper::styleSplitter(ui_->track_group_manager);
	ui_->debug_btn->setVisible(Helper::runningInQtCreator());

	//connect signals and slots
	connect(ui_->chr_selector, SIGNAL(currentTextChanged(QString)), this, SLOT(setChromosomeRegion(QString)));
	connect(ui_->search, SIGNAL(editingFinished()), this, SLOT(search()));
	connect(ui_->zoomin_btn, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));
	connect(ui_->zoomout_btn, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));
	connect(SharedData::instance(), SIGNAL(transcriptsChanged()), this, SLOT(updateIndices()));
	connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	connect(SharedData::instance(), SIGNAL(updateGenomicCoordinate(QString)), this, SLOT(updateCoordinateLabel(QString)));
	connect(ui_->debug_btn, &QPushButton::clicked, this, &GenomeVisualizationWidget::debugMethod);

	//clean session
	clearSession();

	//show the current coordinates before the first region change
	updateRegion();
}

GenomeVisualizationWidget::~GenomeVisualizationWidget()
{
	disconnect(selected_track_destroyed_);
	delete ui_;
}

void GenomeVisualizationWidget::clearParameterEditor()
{
	disconnect(selected_track_destroyed_);
	selected_track_ = nullptr;
	delete parameter_editor_;
	parameter_editor_ = nullptr;
	ui_->settings_placeholder->show();
	ui_->settings_dock->setWindowTitle("Track settings");
}

void GenomeVisualizationWidget::selectTrack(TrackWidget* track)
{
	if (!track || selected_track_ == track) return;
	disconnect(selected_track_destroyed_);
	selected_track_ = track;
	if (parameter_editor_) parameter_editor_->load(track->parameters());
	else
	{
		parameter_editor_ = new ParameterEditor(track->parameters(), ui_->settings_contents);
		ui_->settings_layout->addWidget(parameter_editor_);
	}
	ui_->settings_placeholder->hide();
	ui_->settings_dock->setWindowTitle("Track settings: " + track->displayName());
	selected_track_destroyed_ = connect(track, &QObject::destroyed, this, &GenomeVisualizationWidget::clearParameterEditor);
}

void GenomeVisualizationWidget::delayedInitialization()
{
	//resizing the gene panel works only if the widget is already shown
	ui_->track_group_manager->resizeGenePanel();
}

void GenomeVisualizationWidget::openFileDialog()
{
	QString open_folder = Settings::path("load_store_file_folder", true);
	QStringList files = QFileDialog::getOpenFileNames(this, "Open file(s)", open_folder, "NGS files(*.bam *.cram *.bed *.igv);;All files(*.*)");
	if (!files.isEmpty())
	{
		foreach(QString file, files)
		{
			loadFile(file);
		}
		Settings::setPath("load_store_file_folder", QFileInfo(files[0]).absolutePath());
	}
}

void GenomeVisualizationWidget::openUrlDialog()
{
	QString title = "Open URL";

	QString url = QInputDialog::getText(this, title, "URL");
	url = url.trimmed();
	if (url.isEmpty()) return;

	if (!Helper::isHttpUrl(url))
	{
		QMessageBox::warning(this, title, "This is not a URL:\n" + url);
		return;
	}

	loadFile(url);
}

void GenomeVisualizationWidget::loadFile(QString filename)
{
	ui_->track_group_manager->loadFile(filename);
}

void GenomeVisualizationWidget::reloadTracks()
{
	ui_->track_group_manager->reloadTracks();
}

void GenomeVisualizationWidget::clearSession()
{
	clearParameterEditor();
	ui_->track_group_manager->removeAll();
	ui_->track_group_manager->addEmptyTrackGroup();
	ui_->track_group_manager->resizeGenePanel();
}

void GenomeVisualizationWidget::updateIndices()
{
	//init chromosome list (ordered correctly)
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->clear();
	foreach(const Chromosome& chr, SharedData::genome().chromosomes())
	{
		valid_chrs_ << chr.str();
	}
	ui_->chr_selector->addItems(valid_chrs_);
	ui_->chr_selector->blockSignals(false);

	//init gene and transcript list
	for(int i=0; i<SharedData::transcripts().size(); ++i)
	{
		const Transcript& trans = SharedData::transcripts()[i];

		if (trans.source()!=Transcript::ENSEMBL) continue;

		gene_to_trans_indices_[trans.gene().toUpper()] << i;
		trans_to_index_[trans.name().toUpper()] = i;
	}
}

void GenomeVisualizationWidget::setChromosomeRegion(QString chr)
{
	Chromosome c(chr);
	if (!c.isValid())
	{
		QMessageBox::warning(this, __FUNCTION__, "Could not convert chromosome string '" + chr + "' to valid chromosome!");
	}

	SharedData::setRegion(chr, 1, SharedData::genome().lengthOf(c));
}

void GenomeVisualizationWidget::debugMethod()
{
	qDebug() << __PRETTY_FUNCTION__ << __LINE__;
	loadFile("https://gsvar.megsap.de/v1/assets/rna.bam");
	qDebug() << __PRETTY_FUNCTION__ << __LINE__;
}

void GenomeVisualizationWidget::toggleSettingsVisibility()
{
	ui_->settings_dock->setVisible(!ui_->settings_dock->isVisible());
}

void GenomeVisualizationWidget::search()
{
	QByteArray text = ui_->search->text().trimmed().toUtf8();

	//chromosome
	if (valid_chrs_.contains(text) || (!text.startsWith("chr") && valid_chrs_.contains("chr"+text)))
	{
		setChromosomeRegion(text);
		return;
	}

	//chromosomal region
	BedLine region = BedLine::fromString(text);
	if (region.isValid())
	{
		SharedData::setRegion(region.chr(), region.start(), region.end());
		return;
	}

	//gene
	text = text.toUpper();
	if (gene_to_trans_indices_.contains(text))
	{
		BedFile roi;
		foreach(int index, gene_to_trans_indices_[text])
		{
			const Transcript& trans = SharedData::transcripts()[index];
			roi.append(BedLine(trans.chr(), trans.start(), trans.end()));
		}
		roi.extend(SharedData::settings().transcript_padding);
		roi.merge();
		if (roi.count()>1)
		{
			QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Gene has several transcript regions, using the first one!\nUse transcript identifiers to select a specific transcript of the gene!" + text);
		}

		SharedData::setRegion(roi[0].chr(), roi[0].start(), roi[0].end());
		return;
	}

	//transcript
	int index = trans_to_index_.value(text, -1);
	if (index!=-1)
	{
		const Transcript& trans = SharedData::transcripts()[index];
		SharedData::setRegion(trans.chr(), trans.start()-SharedData::settings().transcript_padding, trans.end()+SharedData::settings().transcript_padding);
		return;
	}

	QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Could not find locus or feature: " + text);
}

void GenomeVisualizationWidget::zoomIn()
{
	const BedLine& reg = SharedData::region();
	SharedData::setRegion(reg.chr(), reg.start()+reg.length()/4, reg.end()-reg.length()/4);
}

void GenomeVisualizationWidget::zoomOut()
{
	const BedLine& reg = SharedData::region();
	SharedData::setRegion(reg.chr(), reg.start()-reg.length()/2, reg.end()+reg.length()/2);
}

void GenomeVisualizationWidget::zoomIn(int x)
{
	const BedLine& reg = SharedData::region();
	int current_len = reg.length();
	int new_len = current_len / 2;

	double ratio = (current_len > 0) ? (double)(x - reg.start()) / current_len : 0.5;

	int new_start = x - (new_len * ratio);
	int new_end = new_start + new_len;

	SharedData::setRegion(reg.chr(), new_start, new_end);
}

void GenomeVisualizationWidget::zoomOut(int x)
{
	const BedLine& reg = SharedData::region();
	int current_len = reg.length();
	int new_len = current_len * 2;

	double ratio = (current_len > 0) ? (double)(x - reg.start()) / current_len : 0.5;

	int new_start = x - (new_len * ratio);
	int new_end = new_start + new_len;

	SharedData::setRegion(reg.chr(), new_start, new_end);
}

void GenomeVisualizationWidget::wheelEvent(QWheelEvent* event)
{
	if (event->modifiers() & Qt::ControlModifier)
	{
		int num_deg = event->angleDelta().y() / 8;
		int num_steps = num_deg / 15;
		if (num_steps != 0)
		{
			int x = ui_->track_group_manager->mapFrom(this, event->position().toPoint()).x();
			int label_width = SharedData::settings().label_width;
			const BedLine& region = SharedData::region();
			int w = (ui_->track_group_manager->width() - label_width - 4);
			if (w <= 0) return;
			float frac = (float)(x - label_width - 2) / w;
			int region_coord = region.start() + frac * region.length();

			if (num_steps > 0) zoomIn(region_coord);
			else zoomOut(region_coord);
		}
		event->accept();
	}
	else
	{
		QWidget::wheelEvent(event);
	}
}

void GenomeVisualizationWidget::updateRegion()
{
	const BedLine& reg = SharedData::region();

	//update region selectors at top
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->setCurrentText(reg.chr().strNormalized(true));
	ui_->chr_selector->blockSignals(false);
	ui_->search->blockSignals(true);
	ui_->search->setText(reg.toString(true));
	ui_->search->blockSignals(false);

	//update size label in toolbar
	ui_->label_region_size->setText(QString::number(reg.length()));
}

void GenomeVisualizationWidget::updateCoordinateLabel(QString text)
{
	ui_->label_coordinate->setText(text);
}

void GenomeVisualizationWidget::saveSession()
{
	QString open_folder = Settings::path("load_store_session_folder", true);
	QString file_path = QFileDialog::getSaveFileName(this, "Open session", open_folder, "Session files (*.xml)");
	if (file_path.isEmpty()) return;

	//check it is a XML file
	if (!file_path.endsWith(".xml"))
	{
		QMessageBox::warning(this, "Error", "Only XML files are supported");
		return;
	}

	//open file
	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(this, "Error", "Failed to open file for writing: " + file.errorString());
		return;
	}

	//store header
	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();
	writer.writeStartElement("GSviewerSession");
	writer.writeAttribute("version", "1");
	writer.writeStartElement("General");

	//store displayed region
	const BedLine& region = SharedData::region();
	writer.writeStartElement("DisplayedRegion");
	writer.writeAttribute("chr", region.chr().strNormalized(true));
	writer.writeAttribute("start", QString::number(region.start()));
	writer.writeAttribute("end", QString::number(region.end()));
	writer.writeEndElement();  // DisplayRegion

	writer.writeEndElement(); // General
	ui_->track_group_manager->writeToXml(writer);
	writer.writeEndElement(); // GSViewerSession
	writer.writeEndDocument();
	file.close();

	//store path
	Settings::setPath("load_store_session_folder", QFileInfo(file_path).absolutePath());
}

void GenomeVisualizationWidget::loadSession()
{
	QString title = "Load session";

	QString open_folder = Settings::path("load_store_session_folder", true);
	QString file_path = QFileDialog::getOpenFileName(this, "Open file(s)", open_folder, "Session files(*.xml);;All files(*.*)");
	if (file_path.isEmpty()) return;

	if (!file_path.endsWith(".xml"))
	{
		QMessageBox::warning(this, title, "Error: Only XML files are supported");
		return;
	}

	QString error = XmlHelper::isValidXml(file_path, ":Resources/GSviewerSession.xsd");
	if (!error.isEmpty())
	{
		QMessageBox::warning(this, title, "XML validation error: " + error);
		return;
	}

	//clear TrackGroups
	clearParameterEditor();
	ui_->track_group_manager->removeAll();

	//load session
	QDomDocument doc = XmlHelper::load(file_path);
	QDomElement root = doc.documentElement(); //GSviewerSession
	QDomElement general = root.elementsByTagName("General").at(0).toElement();
	QDomElement region_info = general.elementsByTagName("DisplayedRegion").at(0).toElement();
	QString chr = region_info.attribute("chr");
	int start = region_info.attribute("start").toInt();
	int end = region_info.attribute("end").toInt();
	SharedData::setRegion(chr, start, end);
	QStringList errors;
	ui_->track_group_manager->loadFromXml(root, errors);
	if (!errors.isEmpty())
	{
		QMessageBox::warning(this, title, "Error(s) while loading session:\n" + errors.join("\n"));
	}

	//store path
	Settings::setPath("load_store_session_folder", QFileInfo(file_path).absolutePath());
}


//TODO Marc:
//- parameters of all tracks: BAM coverage, BED, IGV
//- debug mode drop-down: goto region, goto rene
//- add region bookmarks stored in Settings
//- BAM track: "group by"
//- XSD constraint Paramter Value count based on type attribute
