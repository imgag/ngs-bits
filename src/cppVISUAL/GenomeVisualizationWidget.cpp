#include "ui_GenomeVisualizationWidget.h"
#include "GenomeVisualizationWidget.h"
#include "BedFile.h"
#include "GUIHelper.h"
#include "SharedData.h"
#include "XmlHelper.h"
#include "Settings.h"
#include "ParameterEditor.h"
#include "BookmarkManager.h"
#include "AddBookmarkDialog.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "GenePanel.h"
#include <QApplication>
#include <QAbstractScrollArea>
#include <QToolTip>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSignalBlocker>
#include <QMenu>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QScopedPointer>
#include <cmath>

GenomeVisualizationWidget::GenomeVisualizationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::GenomeVisualizationWidget)
	, timer_(this, true)
{
	ui_->setupUi(this);
	//The UI compiler attaches scroll-area contents only after constructing their children.
	//Create the gene panel once the contents belong to this viewer.
	auto* gene_panel = new GenePanel(ui_->scrollAreaWidgetContents_2);
	gene_panel->setObjectName("gene_panel");
	ui_->horizontalLayout_2->addWidget(gene_panel);
	//QMainWindow defaults to a top-level window even when it has a parent.
	ui_->dock_window->setWindowFlags(Qt::Widget);
	ui_->settings_dock->hide();
	connect(ui_->track_group_manager, &TrackGroupManager::trackSelected, this, &GenomeVisualizationWidget::selectTrack);
	connect(ui_->track_group_manager, &TrackGroupManager::settingsRequested, this, [this](TrackWidget* track)
	{
		selectTrack(track);
		ui_->settings_dock->show();
	});
	connect(ui_->settings_btn, &QToolButton::clicked, this, &GenomeVisualizationWidget::toggleSettingsVisibility);
	connect(ui_->settings_dock, &QDockWidget::visibilityChanged, this, [this](bool visible)
	{
		QSignalBlocker blocker(ui_->settings_btn);
		ui_->settings_btn->setChecked(visible);
	});
	GUIHelper::styleSplitter(ui_->track_group_manager);
	setupDebugButton();
	setupBookmarkMenu();

	//connect signals and slots
	connect(ui_->chr_selector, SIGNAL(currentTextChanged(QString)), this, SLOT(setChromosomeRegion(QString)));
	connect(ui_->search, SIGNAL(editingFinished()), this, SLOT(search()));
	connect(ui_->zoomin_btn, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));
	connect(ui_->zoomout_btn, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));
	connect(SharedData::instance(), SIGNAL(transcriptsChanged()), this, SLOT(updateIndices()));
	connect(this, &GenomeVisualizationWidget::regionChanged, this, &GenomeVisualizationWidget::updateRegion);
	connect(this, &GenomeVisualizationWidget::updateGenomicCoordinate, this, &GenomeVisualizationWidget::updateCoordinateLabel);

	//clean session
	clearSession();

	//Observe child events even when a track consumes them, including newly shown tracks.
	for (QWidget* child : ui_->track_group_manager->findChildren<QWidget*>()) child->setMouseTracking(true);
	qApp->installEventFilter(this);

	//show the current coordinates before the first region change
	updateRegion();
}

void GenomeVisualizationWidget::setupDebugButton()
{
	//show only in QtCreator
	ui_->debug_btn->setVisible(Helper::runningInQtCreator());

	QMenu* debug_menu = new QMenu(ui_->debug_btn);

	//add goto entries
	for (const QString& query : QStringList{"CHEK2", "BRCA1", "BRCA2", "FOXG1", "PMS2"})
	{
		QAction* action = debug_menu->addAction("goto " + query);
		connect(action, &QAction::triggered, this, [this, query]()
		{
			ui_->search->setText(query);
			search();
		});
	}

	ui_->debug_btn->setMenu(debug_menu);
}

void GenomeVisualizationWidget::setupBookmarkMenu()
{
	QMenu* menu = new QMenu(ui_->bookmarks_btn);
	menu->setObjectName("bookmarks_menu");
	ui_->bookmarks_btn->setMenu(menu);
	connect(menu, &QMenu::aboutToShow, this, [this, menu]()
	{
		menu->clear();
		QAction* store = menu->addAction("Store current region");
		store->setEnabled(this->region().isValid());
		connect(store, &QAction::triggered, this, &GenomeVisualizationWidget::storeBookmark);

		connect(menu->addAction("Manage"), &QAction::triggered, this, [this]()
		{
			BookmarkManager dialog(this);
			dialog.exec();
		});

		const QStringList bookmarks = Settings::stringList("bookmark_regions", true);
		if (!bookmarks.isEmpty())
		{
			menu->addSeparator();
			for (int i = 0; i < bookmarks.count(); ++i)
			{
				const QJsonObject bookmark = QJsonDocument::fromJson(bookmarks[i].toUtf8()).object();
				const QString name = bookmark.value("name").toString();
				const BedLine region(Chromosome(bookmark.value("chr").toString()), bookmark.value("start").toInt(), bookmark.value("end").toInt());
				if (name.isEmpty() || !region.isValid() || region.start() < 1) continue;
				const QString label = QString(name).replace("&", "&&");
				QAction* action = menu->addAction(label);
				action->setToolTip(region.toString(true));
				connect(action, &QAction::triggered, this, [this, region]()
				{
					setRegion(region.chr(), region.start(), region.end());
				});
			}
		}
	});
}

void GenomeVisualizationWidget::storeBookmark()
{
	const BedLine region = this->region();
	AddBookmarkDialog dialog(region.toString(true), this);
	if (dialog.exec() != QDialog::Accepted || dialog.name().isEmpty()) return;
	const QJsonObject bookmark{{"name", dialog.name()}, {"chr", QString::fromUtf8(region.chr().strNormalized(true))}, {"start", region.start()}, {"end", region.end()}, {"show_in_chromosome_view", dialog.showInChromosomeTrack()}};
	QStringList bookmarks = Settings::stringList("bookmark_regions", true);
	bookmarks.append(QString::fromUtf8(QJsonDocument(bookmark).toJson(QJsonDocument::Compact)));
	Settings::setStringList("bookmark_regions", bookmarks);
	emit SharedData::instance() -> bookmarksChanged();
}

GenomeVisualizationWidget::~GenomeVisualizationWidget()
{
	qApp->removeEventFilter(this);
	disconnect(selected_track_destroyed_);
	delete ui_;
}

bool GenomeVisualizationWidget::eventFilter(QObject* watched, QEvent* event)
{
	auto* widget = qobject_cast<QWidget*>(watched);
	if (!widget) return QWidget::eventFilter(watched, event);
	QWidget* manager = ui_->track_group_manager;
	const bool chromosome_panel = widget == ui_->chr_panel;
	const bool chromosome_scale = widget == ui_->chr_context_panel;
	if (!chromosome_panel && !chromosome_scale && widget != manager && !manager->isAncestorOf(widget)) return QWidget::eventFilter(watched, event);
	if (widget->window() != window()) return QWidget::eventFilter(watched, event);

	if (event->type() == QEvent::Show)
	{
		widget->setMouseTracking(true);
	}
	else if (event->type() == QEvent::MouseMove)
	{
		const auto* mouse = static_cast<QMouseEvent*>(event);
		const QPoint global_pos = mouse->globalPosition().toPoint();
		if (chromosome_panel)
		{
			const int coordinate = ui_->chr_panel->genomicCoordinateAt(mouse->pos().x());
			updateCoordinateLabel(coordinate > 0 ? QString::fromUtf8(this->region().chr().strNormalized(true)) + ":" + QString::number(coordinate) : QString());
			return QWidget::eventFilter(watched, event);
		}
		if (chromosome_scale)
		{
			mouseMoved(mouse->pos().x(), widget->width());
			return QWidget::eventFilter(watched, event);
		}
		//Use the widget under the cursor also for events propagated to its parents.
		QWidget* hovered = manager->childAt(manager->mapFromGlobal(global_pos));
		QWidget* coordinate_widget = manager;
		for (QWidget* ancestor = hovered; ancestor && ancestor != manager; ancestor = ancestor->parentWidget())
		{
			if (qobject_cast<TrackWidget*>(ancestor) || qobject_cast<GenePanel*>(ancestor))
			{
				coordinate_widget = ancestor;
				break;
			}
			if (auto* scroll_area = qobject_cast<QAbstractScrollArea*>(ancestor))
			{
				coordinate_widget = scroll_area->viewport();
				break;
			}
		}
		const QPoint pos = coordinate_widget->mapFromGlobal(global_pos);
		if (coordinate_widget->rect().contains(pos)) mouseMoved(pos.x(), coordinate_widget->width());
		else updateCoordinateLabel("");
	}
	else if (event->type() == QEvent::Leave && (widget == manager || chromosome_panel || chromosome_scale))
	{
		updateCoordinateLabel("");
	}
	return QWidget::eventFilter(watched, event);
}

void GenomeVisualizationWidget::clearParameterEditor()
{
	disconnect(selected_track_destroyed_);
	selected_track_ = nullptr;
	delete parameter_editor_;
	parameter_editor_ = nullptr;
	ui_->settings_placeholder->show();
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
	parameter_editor_->setName(track->displayName());
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
	QStringList files = QFileDialog::getOpenFileNames(this, "Open file(s)", open_folder, "NGS files(*.bam *.cram *.bed *.bed.gz *.bedpe *.bedpe.gz *.vcf *.vcf.gz *.igv *.seg);;All files(*.*)");
	if (!files.isEmpty())
	{
		foreach (QString file, files)
		{
			try
			{
				loadFile(file);
			}
			catch (Exception e)
			{
				QMessageBox::critical(QApplication::activeWindow(), "Could not load file", e.message());
			}
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

	try
	{
		loadFile(url);
	}
	catch (Exception e)
	{
		QMessageBox::warning(this, "Unable to load file", "The URL " + url + " failed to load due to the following error:\n\t" + e.message());
		return;
	}
}

void GenomeVisualizationWidget::loadFile(QString filename)
{
	ui_->track_group_manager->loadFile(filename);
}

void GenomeVisualizationWidget::addBedTrack(QSharedPointer<BedFile> regions, QString file_path, QString name)
{
	QScopedPointer<BedTrack> track(new BedTrack(this, file_path, name));
	track->setBedFile(regions);
	ui_->track_group_manager->addTrack(track.take());
}

void GenomeVisualizationWidget::reloadTracks()
{
	ui_->track_group_manager->reloadTracks();
}

QList<BamAlignmentTrack*> GenomeVisualizationWidget::bamTracks()
{
	return findChildren<BamAlignmentTrack*>();
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
	ui_->chr_selector->addItem("All");
	valid_chrs_.clear();
	foreach (const Chromosome& chr, SharedData::genome().chromosomes())
	{
		valid_chrs_ << chr.str();
	}
	ui_->chr_selector->addItems(valid_chrs_);
	ui_->chr_selector->setCurrentText(this->region().isValid() ? this->region().chr().strNormalized(true) : QByteArray("All"));
	ui_->chr_selector->blockSignals(false);

	//init gene and transcript list
	for (int i = 0; i < SharedData::transcripts().size(); ++i)
	{
		const Transcript& trans = SharedData::transcripts()[i];

		if (trans.source() != Transcript::ENSEMBL) continue;

		gene_to_trans_indices_[trans.gene().toUpper()] << i;
		trans_to_index_[trans.name().toUpper()] = i;
	}
}

void GenomeVisualizationWidget::setChromosomeRegion(QString chr)
{
	if (chr == "All")
	{
		setRegion(Chromosome(), 0, 0);
		return;
	}
	Chromosome c(chr);
	if (!c.isValid())
	{
		QMessageBox::warning(this, __FUNCTION__, "Could not convert chromosome string '" + chr + "' to valid chromosome!");
		return;
	}

	setRegion(chr, 1, SharedData::genome().lengthOf(c));
}

void GenomeVisualizationWidget::toggleSettingsVisibility()
{
	ui_->settings_dock->setVisible(!ui_->settings_dock->isVisible());
}

void GenomeVisualizationWidget::search()
{
	QByteArray text = ui_->search->text().trimmed().toUtf8();

	//chromosome
	if (valid_chrs_.contains(text) || (!text.startsWith("chr") && valid_chrs_.contains("chr" + text)))
	{
		setChromosomeRegion(text);
		return;
	}

	//chromosomal region
	QString region_text = QString::fromUtf8(text).simplified().replace(' ', '\t').replace(':', '\t');
	const QStringList locus_parts = region_text.split('\t');
	if (locus_parts.count() == 2 && !locus_parts[1].contains('-'))
	{
		region_text += "\t" + locus_parts[1]; //Accept a single coordinate as a one-base range.
	}
	BedLine region = BedLine::fromString(region_text);
	if (region.isValid())
	{
		setRegion(region.chr(), region.start(), region.end());
		return;
	}

	//gene
	text = text.toUpper();
	if (gene_to_trans_indices_.contains(text))
	{
		BedFile roi;
		foreach (int index, gene_to_trans_indices_[text])
		{
			const Transcript& trans = SharedData::transcripts()[index];
			roi.append(BedLine(trans.chr(), trans.start(), trans.end()));
		}
		roi.extend(SharedData::settings().transcript_padding);
		roi.merge();
		if (roi.count() > 1)
		{
			QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Gene has several transcript regions, using the first one!\nUse transcript identifiers to select a specific transcript of the gene!" + text);
		}

		setRegion(roi[0].chr(), roi[0].start(), roi[0].end());
		return;
	}

	//transcript
	int index = trans_to_index_.value(text, -1);
	if (index != -1)
	{
		const Transcript& trans = SharedData::transcripts()[index];
		setRegion(trans.chr(), trans.start() - SharedData::settings().transcript_padding, trans.end() + SharedData::settings().transcript_padding);
		return;
	}

	QToolTip::showText(ui_->search->mapToGlobal(QPoint(0, 0)), "Could not find locus or feature: " + text);
}

void GenomeVisualizationWidget::zoomIn()
{
	const BedLine& reg = this->region();
	if (!reg.isValid()) return;
	setRegion(reg.chr(), reg.start() + reg.length() / 4, reg.end() - reg.length() / 4);
}

void GenomeVisualizationWidget::zoomOut()
{
	const BedLine& reg = this->region();
	if (!reg.isValid()) return;
	setRegion(reg.chr(), reg.start() - reg.length() / 2, reg.end() + reg.length() / 2);
}

void GenomeVisualizationWidget::zoomIn(int x)
{
	const BedLine& reg = this->region();
	if (!reg.isValid()) return;
	int current_len = reg.length();
	int new_len = current_len / 2;

	double ratio = (current_len > 0) ? (double)(x - reg.start()) / current_len : 0.5;

	int new_start = x - (new_len * ratio);
	int new_end = new_start + new_len;

	setRegion(reg.chr(), new_start, new_end);
}

void GenomeVisualizationWidget::zoomOut(int x)
{
	const BedLine& reg = this->region();
	if (!reg.isValid()) return;
	int current_len = reg.length();
	int new_len = current_len * 2;

	double ratio = (current_len > 0) ? (double)(x - reg.start()) / current_len : 0.5;

	int new_start = x - (new_len * ratio);
	int new_end = new_start + new_len;

	setRegion(reg.chr(), new_start, new_end);
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
			const BedLine& region = this->region();
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
	const BedLine& reg = this->region();
	//The overview already includes its scale; leave no empty context area below it.
	ui_->chr_context_panel->setVisible(reg.isValid());

	//update region selectors at top
	ui_->chr_selector->blockSignals(true);
	ui_->chr_selector->setCurrentText(reg.isValid() ? reg.chr().strNormalized(true) : QByteArray("All"));
	ui_->chr_selector->blockSignals(false);
	ui_->search->blockSignals(true);
	ui_->search->setText(reg.isValid() ? reg.toString(true) : QString());
	ui_->search->blockSignals(false);

	//update size label in toolbar
	ui_->label_region_size->setText(reg.isValid() ? QString::number(reg.length()) : QString());
	ui_->zoomin_btn->setEnabled(reg.isValid());
	ui_->zoomout_btn->setEnabled(reg.isValid());
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
	writer.writeAttribute("gene_panel_height", QString::number(ui_->scrollArea->height()));

	//store displayed region
	const BedLine& region = this->region();
	writer.writeStartElement("DisplayedRegion");
	writer.writeAttribute("chr", region.chr().strNormalized(true));
	writer.writeAttribute("start", QString::number(region.start()));
	writer.writeAttribute("end", QString::number(region.end()));
	writer.writeEndElement(); // DisplayRegion

	SharedData::writeSettingsToXml(writer);
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

	//load session
	QDomDocument doc = XmlHelper::load(file_path);
	QDomElement root = doc.documentElement(); //GSviewerSession
	QDomElement general = root.elementsByTagName("General").at(0).toElement();
	try
	{
		SharedData::loadSettingsFromXml(general);
	}
	catch (const Exception& e)
	{
		QMessageBox::warning(this, title, "Invalid global settings: " + e.message());
		return;
	}

	//Apply global settings before restoring the region and creating tracks.
	clearParameterEditor();
	ui_->track_group_manager->removeAll();
	QDomElement region_info = general.elementsByTagName("DisplayedRegion").at(0).toElement();
	QString chr = region_info.attribute("chr");
	int start = region_info.attribute("start").toInt();
	int end = region_info.attribute("end").toInt();
	setRegion(chr, start, end);
	QStringList errors;
	ui_->track_group_manager->loadFromXml(root, errors);
	if (!errors.isEmpty())
	{
		QMessageBox::warning(this, title, "Error(s) while loading session:\n" + errors.join("\n"));
	}

	//store path
	Settings::setPath("load_store_session_folder", QFileInfo(file_path).absolutePath());
}

GenomeVisualizationWidget* GenomeVisualizationWidget::fromWidget(QWidget* widget)
{
	for (QWidget* parent = widget; parent; parent = parent->parentWidget())
	{
		if (auto* viewer = qobject_cast<GenomeVisualizationWidget*>(parent)) return viewer;
	}
	THROW(ProgrammingException, "Genome visualization components require a GenomeVisualizationWidget parent.");
}

void GenomeVisualizationWidget::setRegion(const Chromosome& chr, int start, int end)
{
	//An invalid chromosome selects the whole-genome overview.
	if (!chr.isValid())
	{
		if (!region_.isValid()) return;
		region_ = BedLine();
		emit regionChanged(region_);
		return;
	}
	if (end < start) THROW(ArgumentException, "Invalid chromosome range - end before start: "+QString::number(start) + "-" + QString::number(end));

	//extend region to minimal region size
	int size = end-start+1;
	if (size<SharedData::settings().min_window_size)
	{
		int missing = SharedData::settings().min_window_size-size;
		start -= missing/2;
		end += missing/2;
		if (missing%2!=0)
		{
			start -= 1;
			end += 1;
		}
		size = end-start+1;
	}

	//make sure the region is in chromosome boundaries
	if (start<1)
	{
		start = 1;
		end = start + size - 1;
	}
	int genome_end = SharedData::genome().lengthOf(chr);
	if (end>genome_end)
	{
		end = genome_end;
		start = end - size + 1;
		if (start<1) start = 1; //if size is bigger than chromosome, this can happen
	}

	//check new region is different from old
	BedLine new_reg = BedLine(chr, start, end);
	if (new_reg==region_) return;

	region_ = new_reg;
	emit regionChanged(region_);
}

void GenomeVisualizationWidget::mouseMoved(int x, int width)
{
	if (!region().isValid())
	{
		emit updateGenomicCoordinate("");
		return;
	}
	int label_width = SharedData::settings().label_width;
	const BedLine& region = this->region();
	double pixels_per_base_ =  (double)(width-label_width-4) / (double)region.length();

	//show
	if (x>label_width + 2 && x<width - 2)
	{
		int coordinate = region.start() + std::floor((double)(x-label_width - 2) / pixels_per_base_);
		emit updateGenomicCoordinate(region.chr().strNormalized(true) + ":" + QString::number(coordinate));
	}
	else
	{
		emit updateGenomicCoordinate("");
	}
}
