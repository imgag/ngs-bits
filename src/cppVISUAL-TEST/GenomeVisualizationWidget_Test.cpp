#include "GenomeVisualizationWidget.h"
#include "../GSviewer/TranscriptCache.h"
#include "GffData.h"
#include <QDateTime>
#include "BookmarkManager.h"
#include <QTableWidget>
#include <QPushButton>
#include "TrackGroupManager.h"
#include "ParameterEditor.h"
#include "Settings.h"
#include "SharedData.h"
#include "GenePanel.h"
#include "ChromosomePanel.h"
#include "ChromosomeContextPanel.h"
#include "NGSHelper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "XmlHelper.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QHBoxLayout>
#include "BedTrack.h"
#include "VcfTrack.h"
#include "SvTrack.h"
#include "FileLoader.h"
#include "IgvTrack.h"
#include "SegTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include "BamTrackDataManager.h"
#include "BamWriter.h"
#include <htslib/bgzf.h>
#include <QMessageBox>
#include <QScopeGuard>
#include <QDialog>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QDir>
#include <QUuid>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtTest>
#include <QInputDialog>
#include <QMenu>
#include <QStandardPaths>
#include <QTimer>
#include <QHelpEvent>
#include <QToolTip>

class GenomeVisualizationWidget_Test : public QObject
{
	Q_OBJECT

private:
	QDir data_{QDir::currentPath() + "/test-data-" + QUuid::createUuid().toString(QUuid::WithoutBraces)};

	static void restoreGlobalSettings(const QList<Parameter>& parameters)
	{
		QHash<QByteArray, QVariant> values;
		for (const Parameter& parameter : parameters) values.insert(parameter.name(), parameter.value());
		SharedData::parameters().reset(parameters, values);
	}

	void writeFile(const QString& name, const QByteArray& contents)
	{
		QFile file(data_.filePath(name));
		QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
		QCOMPARE(file.write(contents), contents.size());
	}

	void openEditor(TrackWidget* track)
	{
		emit track->trackSelected(track);
		for (QWidget* parent = track->parentWidget(); parent; parent = parent->parentWidget())
		{
			if (auto* viewer = qobject_cast<GenomeVisualizationWidget*>(parent))
			{
				if (viewer->findChild<QDockWidget*>("settings_dock")->isHidden())
				{
					viewer->findChild<QToolButton*>("settings_btn")->click();
				}
				return;
			}
		}
		QFAIL("Track is not inside a genome visualization widget");
	}

private slots:
	void initTestCase()
	{
		QVERIFY(QDir().mkpath(data_.path()));
		writeFile("reference.fa", ">chr22\n" + QByteArray(100, 'A') + "\n>chr23\nATGATGTAA" + QByteArray(91, 'A') + "\n>chrX\n" + QByteArray(100, 'A') + "\n>chrLong\n" + QByteArray(10000, 'A') + "\n>chrY\n" + QByteArray(200, 'A') + "\n");
		writeFile("reference.fa.fai", "chr22\t100\t7\t100\t101\nchr23\t100\t115\t100\t101\nchrX\t100\t222\t100\t101\nchrLong\t10000\t332\t10000\t10001\nchrY\t200\t10339\t200\t201\n");
		writeFile("settings.ini", "reference_genome=" + data_.filePath("reference.fa").toUtf8() + "\n");
		Settings::setSettingsOverride(data_.filePath("settings.ini"));
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("first.bed", "chr22\t10\t20\n");
		writeFile("second.bed", "chr22\t30\t40\n");
	}

	void wholeGenomeOverview()
	{
		const BedLine original = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original.chr(), original.start(), original.end()); });
		QSignalSpy changes(SharedData::instance(), &SharedData::regionChanged);
		SharedData::setRegion(Chromosome(), 0, 0);
		QVERIFY(!SharedData::region().isValid());
		QCOMPARE(changes.count(), 1);
		SharedData::setRegion(Chromosome(), 0, 0);
		QCOMPARE(changes.count(), 1);

		ChromosomePanel panel(nullptr);
		const int x0 = SharedData::settings().label_width + 2;
		panel.resize(x0 + 402, 40);
		QImage image(panel.size(), QImage::Format_ARGB32);
		panel.render(&image);
		//Only chr22, X and Y are canonical in this reference: lengths 100, 100, 200.
		for (int offset : {0, 100, 200, 400})
		{
			QVERIFY(image.pixelColor(x0 + offset, panel.height() - 5).lightness() < 128);
		}
		QVERIFY(image.pixelColor(x0 + 300, panel.height() - 5).lightness() > 128);
		QCOMPARE(panel.genomicCoordinateAt(x0 + 50), 0);
		QTest::mouseClick(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 50, 20));
		QVERIFY(!SharedData::region().isValid());

		GenePanel gene(nullptr);
		writeFile("overview.vcf", "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\nchr22\t10\t.\tA\tG\t.\tPASS\t.\n");
		writeFile("overview.bedpe", "##fileformat=BEDPE\n#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tTYPE\nchr22\t10\t10\tchr22\t20\t20\tDEL\n");
		VcfTrack vcf(nullptr, data_.filePath("overview.vcf"), "VCF");
		SvTrack sv(nullptr, data_.filePath("overview.bedpe"), "SV");
		BamAlignmentTrack alignment(nullptr, "", "Alignments");
		BamCoverageTrack coverage(nullptr, "", "Coverage");
		for (QWidget* widget : QList<QWidget*>{&gene, &vcf, &sv, &alignment, &coverage})
		{
			widget->setFixedSize(900, 80);
			QImage actual(widget->size(), QImage::Format_ARGB32);
			widget->render(&actual);
			QImage expected(widget->size(), QImage::Format_ARGB32);
			expected.fill(Qt::white);
			QPainter painter(&expected);
			painter.setFont(widget->font());
			painter.setPen(Qt::gray);
			const QRect content(x0, 0, 900 - x0 - 2, 80);
			painter.drawText(content, Qt::AlignCenter, "select single chromosome region to show content");
			painter.end();
			QVERIFY2(actual.copy(content) == expected.copy(content), widget->metaObject()->className());
			QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, content.center());
		}

		//Loading BAM data in the overview must avoid querying an invalid chromosome.
		writeFile("overview.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\nread\t0\tchr22\t10\t60\t10M\t*\t0\t0\tAAAAAAAAAA\tIIIIIIIIII\n");
		{
			BamReader reader(data_.filePath("overview.sam"));
			BamWriter writer(data_.filePath("overview.bam"));
			writer.writeHeader(reader);
			BamAlignment read;
			while (reader.getNextAlignment(read)) writer.writeAlignment(read);
		}
		QCOMPARE(sam_index_build(data_.filePath("overview.bam").toUtf8().constData(), 0), 0);
		auto data = BamTrackDataManager::getOrCreate(data_.filePath("overview.bam"));
		QVERIFY(data);
		QVERIFY(data->getAlignments().isEmpty());
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		QCOMPARE(data->getAlignments().count(), 1);
		SharedData::setRegion(Chromosome(), 0, 0);
		QVERIFY(data->getAlignments().isEmpty());

		GenomeVisualizationWidget viewer(nullptr);
		viewer.show();
		QCoreApplication::processEvents();
		auto* context = viewer.findChild<ChromosomeContextPanel*>("chr_context_panel");
		QVERIFY(context->isHidden());
		auto* chromosome = viewer.findChild<ChromosomePanel*>("chr_panel");
		auto* frame = viewer.findChild<QWidget*>("chromosome_frame");
		QVERIFY(frame->height() - chromosome->mapTo(frame, QPoint(0, chromosome->height())).y() <= 2);
		QVERIFY(QMetaObject::invokeMethod(&viewer, "updateIndices", Qt::DirectConnection));
		auto* selector = viewer.findChild<QComboBox*>("chr_selector");
		QCOMPARE(selector->currentText(), QString("All"));
		QVERIFY(!viewer.findChild<QToolButton*>("zoomin_btn")->isEnabled());
		selector->setCurrentText("chr22");
		QVERIFY(SharedData::region().isValid());
		QVERIFY(!context->isHidden());
		QVERIFY(viewer.findChild<QToolButton*>("zoomin_btn")->isEnabled());
		selector->setCurrentText("All");
		QVERIFY(!SharedData::region().isValid());
		QVERIFY(context->isHidden());
	}

	void transcriptCache()
	{
		QDir temporary(QDir::current().filePath("transcript-cache-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces)));
		QVERIFY(QDir().mkpath(temporary.absolutePath()));
		const auto cleanup = qScopeGuard([&]() { temporary.removeRecursively(); });
		const QString source = temporary.filePath("genes.gff3");
		const QString cache_directory = temporary.filePath("cache");
		QFile fixture(QFINDTESTDATA("../cppNGS-TEST/data_in/GffData_loadGffFile_in1.gff3"));
		QVERIFY2(fixture.open(QIODevice::ReadOnly), qPrintable(fixture.errorString()));
		const QByteArray fixture_contents = fixture.readAll();
		auto read = [](const QString& path)
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly)) return QByteArray();
			return file.readAll();
		};
		auto write = [](const QString& path, const QByteArray& contents)
		{
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			QCOMPARE(file.write(contents), qint64(contents.size()));
		};
		auto compare = [](TranscriptList actual, TranscriptList expected)
		{
			actual.sortByPosition();
			expected.sortByPosition();
			QCOMPARE(actual.count(), expected.count());
			for (int i = 0; i < actual.count(); ++i)
			{
				const Transcript& a = actual[i];
				const Transcript& e = expected[i];
				QCOMPARE(a.name(), e.name());
				QCOMPARE(a.gene(), e.gene());
				QCOMPARE(a.geneId(), e.geneId());
				QCOMPARE(a.hgncId(), e.hgncId());
				QCOMPARE(a.version(), e.version());
				QCOMPARE(a.nameCcds(), e.nameCcds());
				QCOMPARE(a.source(), e.source());
				QCOMPARE(a.strand(), e.strand());
				QCOMPARE(a.biotype(), e.biotype());
				QCOMPARE(a.chr().str(), e.chr().str());
				QCOMPARE(a.start(), e.start());
				QCOMPARE(a.end(), e.end());
				QCOMPARE(a.codingStart(), e.codingStart());
				QCOMPARE(a.codingEnd(), e.codingEnd());
				QCOMPARE(a.flags(false), e.flags(false));
				QCOMPARE(a.isGencodeBasicTranscript(), e.isGencodeBasicTranscript());
				QCOMPARE(a.regions().toText(), e.regions().toText());
				QCOMPARE(a.codingRegions().toText(), e.codingRegions().toText());
				QCOMPARE(a.utr5prime().toText(), e.utr5prime().toText());
				QCOMPARE(a.utr3prime().toText(), e.utr3prime().toText());
			}
		};
		GffSettings settings;
		settings.print_to_stdout = false;
		write(source, fixture_contents);
		const TranscriptList expected = GffData::load(source, settings).transcripts;
		compare(TranscriptCache::load(source, cache_directory), expected);
		const QStringList files = QDir(cache_directory).entryList({"*.tsv"}, QDir::Files);
		QCOMPARE(files.count(), 1);
		const QString cache = QDir(cache_directory).filePath(files.first());
		const QByteArray cached = read(cache);
		QVERIFY(cached.startsWith("#GSviewer transcript cache\t"));
		const QByteArray original = read(source);
		const QDateTime modified = QFileInfo(source).lastModified();

		//A hit must avoid parsing the GFF: replace it with invalid data but retain its metadata.
		write(source, QByteArray(original.size(), 'x'));
		{
			QFile file(source);
			QVERIFY(file.open(QIODevice::ReadWrite));
			QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
		}
		compare(TranscriptCache::load(source, cache_directory), expected);
		QCOMPARE(read(cache), cached);

		//A size change invalidates the cache, even with the original modification time.
		write(source, original + "\n#changed\n");
		{
			QFile file(source);
			QVERIFY(file.open(QIODevice::ReadWrite));
			QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
		}
		compare(TranscriptCache::load(source, cache_directory), expected);
		QVERIFY(read(cache) != cached);

		//A timestamp change invalidates an otherwise equally sized source.
		write(source, original);
		compare(TranscriptCache::load(source, cache_directory), expected);
		QByteArray changed = original;
		changed.replace("SEC22B", "TEC22B");
		QCOMPARE(changed.size(), original.size());
		write(source, changed);
		{
			QFile file(source);
			QVERIFY(file.open(QIODevice::ReadWrite));
			QVERIFY(file.setFileTime(modified.addSecs(5), QFileDevice::FileModificationTime));
		}
		compare(TranscriptCache::load(source, cache_directory), GffData::load(source, settings).transcripts);
		QCOMPARE(TranscriptCache::load(source, cache_directory).getTranscript("ENST00000578049").gene(), QByteArray("TEC22B"));

		//Corrupt and incompatible caches are rebuilt rather than preventing startup.
		const QByteArray valid_cache = read(cache);
		write(cache, valid_cache.left(valid_cache.size() / 2));
		compare(TranscriptCache::load(source, cache_directory), GffData::load(source, settings).transcripts);
		QByteArray old_schema = read(cache);
		old_schema.replace("\"version\":1", "\"version\":999");
		write(cache, old_schema);
		compare(TranscriptCache::load(source, cache_directory), GffData::load(source, settings).transcripts);
		QVERIFY(read(cache).contains("\"version\":1"));

		QVERIFY(QDir().mkpath(temporary.filePath("other")));
		const QString other_source = temporary.filePath("other/genes.gff3");
		write(other_source, read(source));
		compare(TranscriptCache::load(other_source, cache_directory), GffData::load(source, settings).transcripts);
		QCOMPARE(QDir(cache_directory).entryList({"*.tsv"}, QDir::Files).count(), 2);
		const QString blocked_directory = temporary.filePath("not_a_directory");
		write(blocked_directory, "file");
		compare(TranscriptCache::load(source, blocked_directory), GffData::load(source, settings).transcripts);
	}

	void globalSettingsParameters()
	{
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		ParameterList& parameters = SharedData::parameters();
		const GlobalSettings& fast = SharedData::settings();
		QCOMPARE(parameters.parameters().size(), 6);
		QCOMPARE(fast.label_width, 165);
		QCOMPARE(fast.min_window_size, 41);
		QCOMPARE(fast.transcript_padding, 2000);
		QCOMPARE(fast.bam_max_visibility_region, 50000);
		QCOMPARE(fast.bam_insert_size_lower_percentile, 0.005);
		QCOMPARE(fast.bam_insert_size_upper_percentile, 0.995);
		QSignalSpy changes(SharedData::instance(), &SharedData::settingsChanged);
		const auto connection = connect(SharedData::instance(), &SharedData::settingsChanged, this, [&]()
		{
			QCOMPARE(fast.label_width, parameters.getInt("label width"));
			QCOMPARE(fast.min_window_size, parameters.getInt("min window size"));
			QCOMPARE(fast.transcript_padding, parameters.getInt("transcript padding"));
			QCOMPARE(fast.bam_max_visibility_region, parameters.getInt("BAM max visibility region"));
			QCOMPARE(fast.bam_insert_size_lower_percentile, parameters.getDouble("BAM insert size lower percentile"));
			QCOMPARE(fast.bam_insert_size_upper_percentile, parameters.getDouble("BAM insert size upper percentile"));
		});
		const auto disconnect = qScopeGuard([&]() { QObject::disconnect(connection); });
		parameters.setInt("label width", 180);
		parameters.setInt("min window size", 21);
		parameters.setInt("transcript padding", 0);
		parameters.setInt("BAM max visibility region", 1000);
		parameters.setDouble("BAM insert size lower percentile", 0.2);
		parameters.setDouble("BAM insert size upper percentile", 0.8);
		QCOMPARE(changes.count(), 6);
		parameters.setInt("label width", 180);
		QCOMPARE(changes.count(), 6);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setInt("label width", -1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setInt("min window size", 0));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setInt("transcript padding", -1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setInt("BAM max visibility region", 0));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setDouble("BAM insert size lower percentile", -0.1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setDouble("BAM insert size upper percentile", 1.1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setDouble("BAM insert size lower percentile", 0.9));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, parameters.setDouble("BAM insert size upper percentile", 0.1));
		QCOMPARE(changes.count(), 6);
		QCOMPARE(fast.bam_insert_size_lower_percentile, 0.2);
		QCOMPARE(fast.bam_insert_size_upper_percentile, 0.8);
		parameters.setDouble("BAM insert size lower percentile", 0.0);
		parameters.setDouble("BAM insert size upper percentile", 1.0);
		QCOMPARE(fast.bam_insert_size_lower_percentile, 0.0);
		QCOMPARE(fast.bam_insert_size_upper_percentile, 1.0);
	}

	void globalSettingsEditor()
	{
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		ParameterEditor editor(SharedData::parameters());
		editor.show();
		QSignalSpy changes(SharedData::instance(), &SharedData::settingsChanged);
		editor.findChild<QLineEdit*>("label width")->setText("180");
		QCOMPARE(SharedData::settings().label_width, 180);
		editor.findChild<QLineEdit*>("label width")->setText("-1");
		QCOMPARE(SharedData::settings().label_width, 180);
		QVERIFY(!editor.store());
		editor.findChild<QLineEdit*>("label width")->setText("180");
		QPointer<QLineEdit> minimum = editor.findChild<QLineEdit*>("BAM insert size lower percentile");
		minimum->selectAll();
		QTest::keyClick(minimum, Qt::Key_0);
		QVERIFY2(minimum, "Changing a bound must not destroy the active input while typing");
		QTest::keyClicks(minimum, ".2");
		QCOMPARE(SharedData::settings().bam_insert_size_lower_percentile, 0.2);
		editor.findChild<QLineEdit*>("BAM insert size upper percentile")->setText("0.1");
		QCOMPARE(SharedData::settings().bam_insert_size_upper_percentile, 0.995);
		QVERIFY(!editor.store());
		editor.findChild<QLineEdit*>("BAM insert size upper percentile")->setText("0.8");
		QVERIFY(editor.store());
		QCOMPARE(SharedData::settings().bam_insert_size_upper_percentile, 0.8);
		QVERIFY(changes.count() >= 3);
	}

	void globalLabelWidthRepaintsWidgets()
	{
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		class PaintCounter : public QObject
		{
		public:
			QHash<QObject*, int> paints;
			bool eventFilter(QObject* watched, QEvent* event) override
			{
				if (event->type() == QEvent::Paint) ++paints[watched];
				return false;
			}
		} counter;
		BedTrack bed(nullptr, "", "BED");
		IgvTrack igv(nullptr, "", "IGV");
		ChromosomePanel chromosome(nullptr);
		ChromosomeContextPanel context(nullptr);
		const QList<QWidget*> widgets{&bed, &igv, &chromosome, &context};
		for (QWidget* widget : widgets)
		{
			widget->installEventFilter(&counter);
			widget->resize(600, 100);
			widget->show();
		}
		QCoreApplication::processEvents();
		for (QWidget* widget : widgets) QVERIFY(counter.paints.value(widget) > 0);
		counter.paints.clear();
		const int label_width = SharedData::settings().label_width + 50;
		SharedData::parameters().setInt("label width", label_width);
		for (QWidget* widget : widgets)
		{
			QTRY_VERIFY_WITH_TIMEOUT(counter.paints.value(widget) > 0, 1000);
			QCOMPARE(widget->size(), QSize(600, 100)); //No resize or region change is needed.
		}
		QCOMPARE(bed.minimumSizeHint().width(), label_width + 5);
		QCOMPARE(igv.minimumSizeHint().width(), label_width + 5);
	}

	void emptyTrackGroupMessage()
	{
		TrackGroup group;
		group.resize(600, 200);
		group.show();
		auto* message = group.findChild<QLabel*>("empty_track_group_message");
		QVERIFY(message);
		QVERIFY(message->isVisible());
		QCOMPARE(message->text(), QString("Empty track group. Drag/drop a track or load a track using the context menu."));
		QVERIFY(message->testAttribute(Qt::WA_TransparentForMouseEvents));
		group.addTrackWidgets({new BedTrack(nullptr, "", "BED")});
		QCOMPARE(group.trackCount(), 1);
		QVERIFY(message->isHidden());
		QVERIFY(QMetaObject::invokeMethod(&group, "clearLayout", Qt::DirectConnection));
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		QCOMPARE(group.trackCount(), 0);
		QVERIFY(message->isVisible());
		group.addTrackWidgets({new BedTrack(nullptr, "", "BED again")});
		QCOMPARE(group.trackCount(), 1);
		QVERIFY(message->isHidden());
	}

	void segRecordsMetadataRenderingAndSession()
	{
		writeFile("coverage.seg", "#type=GENE_EXPRESSION\n"
			"#track graphtype=segment draw_mode=segment name=\"NA12878_58 CNV coverage\" color=0,0,255 altColor=255,0,0 maxHeightPixels=80:80:80 viewLimits=0:2:6 yLineMark=2 yLineOnOff=on\n"
			"ID\tchr\tstart\tend\tvariance\tvalue\n"
			"ignored\tchr22\t50\t60\tannotation text\t4.0\n"
			"ignored\tchr22\t10\t20\t0.1338\t1.0\n");
		const auto file = FileLoader::loadSegFile(data_.filePath("coverage.seg"));
		QVERIFY(file);
		QCOMPARE(file->count(), 2);
		QCOMPARE((*file)[0].start(), 10);
		QCOMPARE((*file)[0].end(), 20);
		QCOMPARE((*file)[0].value(), 1.0);
		QCOMPARE((*file)[0].annotation(), QByteArray("0.1338"));
		QCOMPARE((*file)[1].annotation(), QByteArray("annotation text"));
		ChromosomalIndex<SegFile> index(*file);
		QCOMPARE(index.matchingIndices(Chromosome("chr22"), 10, 10), QVector<int>({0}));
		QVERIFY(index.matchingIndices(Chromosome("chr22"), 21, 49).isEmpty());

		auto tracks = FileLoader::loadTracks(data_.filePath("coverage.seg"));
		QCOMPARE(tracks.size(), 1);
		QScopedPointer<SegTrack> track(qobject_cast<SegTrack*>(tracks[0]));
		QVERIFY(track);
		QCOMPARE(track->displayName(), QString("NA12878_58 CNV coverage"));
		QCOMPARE(track->parameters().getString("draw mode"), QByteArray("ClinCNV"));
		QCOMPARE(track->parameters().getDouble("min"), 0.0);
		QCOMPARE(track->parameters().getDouble("max"), 6.0);
		QCOMPARE(track->parameters().getDouble("reference"), 2.0);
		QCOMPARE(track->sizeHint().height(), 80);
		track->parameters().setString("draw mode", "segments");
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		track->resize(SharedData::settings().label_width + 4 + 160, 80);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		const QPoint low(x0 + 20, 64);
		const QPoint high(x0 + 100, 28);
		QImage image(track->size(), QImage::Format_ARGB32);
		track->render(&image);
		QCOMPARE(image.pixelColor(low), QColor(Qt::red));
		QCOMPARE(image.pixelColor(high), QColor(Qt::blue));
		QTest::mouseClick(track.data(), Qt::LeftButton, Qt::NoModifier, low);
		auto* popup = track->findChild<QDialog*>();
		QVERIFY(popup);
		const QString popup_text = popup->findChild<QLabel*>()->text();
		QVERIFY(popup_text.contains("Region: chr22:10-20"));
		QVERIFY(popup_text.contains("variance: 0.1338"));
		QVERIFY(popup_text.contains("Value: 1"));
		QVERIFY(!popup_text.contains("ignored"));
		delete popup;
		QTest::mouseClick(track.data(), Qt::LeftButton, Qt::NoModifier, low + QPoint(0, -20));
		QVERIFY(!track->findChild<QDialog*>());

		track->parameters().setString("draw mode", "segments");
		QString xml;
		QXmlStreamWriter writer(&xml);
		track->writeToXml(writer);
		QDomDocument document;
		QVERIFY(document.setContent(xml));
		QStringList errors;
		QScopedPointer<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
		QVERIFY(errors.isEmpty());
		QVERIFY(qobject_cast<SegTrack*>(restored.data()));
		QCOMPARE(restored->parameters().getString("draw mode"), QByteArray("segments"));
		QCOMPARE(restored->displayName(), track->displayName());
		QSignalSpy deleted(track.data(), &TrackWidget::trackDeleted);
		track->reloadTrack();
		QCOMPARE(deleted.count(), 0);
	}

	void segWholeGenomeRendering()
	{
		const BedLine original = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original.chr(), original.start(), original.end()); });
		SharedData::setRegion(Chromosome(), 0, 0);
		SegTrack track(nullptr, "", "SEG overview");
		const int x0 = SharedData::settings().label_width + 2;
		track.resize(x0 + 402, 80);
		track.setSegFile(QSharedPointer<SegFile>::create(QVector<SegFileLine>{
			{Chromosome("chr22"), 21, 40, "gain", 3.0},
			{Chromosome("chr22"), 90, 120, "clipped", 4.0},
			{Chromosome("chrX"), 21, 40, "loss", 1.0},
			{Chromosome("chrY"), 21, 40, "gain", 4.0},
			{Chromosome("chrY"), 190, 250, "clipped", 4.0},
			{Chromosome("chr23"), 1, 100, "excluded", 5.0},
			{Chromosome("chrLong"), 1, 10000, "excluded", 5.0}}, QByteArrayList{}, "annotation"));
		QImage image(track.size(), QImage::Format_ARGB32);
		for (const QByteArray& mode : QByteArrayList{"segments", "ClinCNV"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			const QList<QPoint> markers{{x0 + 30, 40}, {x0 + 130, 64}, {x0 + 230, 28}};
			for (int i = 0; i < markers.count(); ++i)
			{
				QCOMPARE(image.pixelColor(markers[i]), QColor(i == 1 ? Qt::red : Qt::blue));
				QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, markers[i]);
				auto* popup = track.findChild<QDialog*>();
				QVERIFY(popup);
				const QString chr = i == 0 ? "chr22" : (i == 1 ? "chrX" : "chrY");
				QVERIFY(popup->findChild<QLabel*>()->text().contains("Region: " + chr + ":21-40"));
				delete popup;
			}
			QCOMPARE(image.pixelColor(x0 + 105, 28), QColor(Qt::white));
			QCOMPARE(image.pixelColor(x0 + 395, 28), QColor(Qt::blue));
			QCOMPARE(image.pixelColor(x0 + 400, 28), QColor(Qt::white));
		}
		SharedData::setRegion(Chromosome("chrX"), 1, 100);
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 120, 64), QColor(Qt::red));
	}

	void segClinCnvColorsAndNegativePopup()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		SegTrack track(nullptr, "", "ClinCNV");
		track.resize(SharedData::settings().label_width + 4 + 800, 128);
		const QVector<double> values{1.8, 2.0, 2.2, 1.4, 2.6, 1.0, 3.0, -1.0};
		QVector<SegFileLine> lines;
		for (int i = 0; i < values.size(); ++i)
		{
			lines.append(SegFileLine(Chromosome("chr22"), 5 + i * 10, 8 + i * 10, "annotation", values[i]));
		}
		track.setSegFile(QSharedPointer<SegFile>::create(lines, QByteArrayList{}, "variance"));
		track.show();
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		const int x0 = SharedData::settings().label_width + 2;
		const QColor reference_over_blue(64, 64, 191);
		const QVector<QColor> colors{Qt::red, reference_over_blue, Qt::blue, Qt::red, Qt::blue, Qt::red, Qt::blue, Qt::black};
		for (int i = 0; i < values.size(); ++i)
		{
			const QPoint position(x0 + 55 + i * 100, qRound(124 - std::max(0.0, values[i]) * 20));
			QCOMPARE(image.pixelColor(position), colors[i]);
		}
		QCOMPARE(image.pixelColor(x0 + 155, 83), QColor(Qt::blue));
		QCOMPARE(image.pixelColor(x0 + 250, 84), QColor(191, 191, 191));
		QCOMPARE(image.pixelColor(x0 + 155, 85), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x0 + 155, 86), QColor(Qt::white));
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 755, 124));
		auto* popup = track.findChild<QDialog*>();
		QVERIFY(popup);
		QVERIFY(popup->findChild<QLabel*>()->text().contains("Value: -1"));
		delete popup;

		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 124, 84), reference_over_blue);
		QCOMPARE(image.pixelColor(x0 + 124, 83), QColor(Qt::blue));
		SharedData::setRegion(Chromosome(), 0, 0);
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 31, 84), reference_over_blue);
		QCOMPARE(image.pixelColor(x0 + 31, 83), QColor(Qt::blue));
		SharedData::setRegion(Chromosome("chr22"), 1, 80);

		track.parameters().setString("draw mode", "segments");
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 355, 96), QColor(Qt::red));
		QCOMPARE(image.pixelColor(x0 + 455, 72), QColor(Qt::blue));
		QCOMPARE(image.pixelColor(x0 + 755, 124), QColor(Qt::red));
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 755, 124));
		popup = track.findChild<QDialog*>();
		QVERIFY(popup);
		QVERIFY(popup->findChild<QLabel*>()->text().contains("Value: -1"));
		delete popup;
	}

	void segOutOfRangeSegmentsAndPopups()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		SegTrack track(nullptr, "", "Clamped segments");
		const int x0 = SharedData::settings().label_width + 2;
		track.resize(x0 + 802, 128);
		track.setSegFile(QSharedPointer<SegFile>::create(QVector<SegFileLine>{
			{Chromosome("chr22"), 10, 20, "low", -1.0},
			{Chromosome("chr22"), 50, 60, "high", 9.0}}, QByteArrayList{}, "annotation"));
		track.parameters().setDouble("min", 1.0);
		track.parameters().setDouble("max", 4.0);
		track.show();
		QImage image(track.size(), QImage::Format_ARGB32);
		for (const QByteArray& mode : QByteArrayList{"segments", "ClinCNV"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			QCOMPARE(image.pixelColor(x0 + 145, 124), QColor(mode == "ClinCNV" ? Qt::black : Qt::red));
			QCOMPARE(image.pixelColor(x0 + 545, 4), QColor(Qt::blue));
			for (const QPoint& point : {QPoint(x0 + 145, 124), QPoint(x0 + 545, 4)})
			{
				QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, point);
				auto* popup = track.findChild<QDialog*>();
				QVERIFY(popup);
				QVERIFY(popup->findChild<QLabel*>()->text().contains(point.y() == 124 ? "Value: -1" : "Value: 9"));
				delete popup;
			}
		}
	}

	void segVarianceStatisticsAndAlpha()
	{
		QByteArray contents = "ID\tchr\tstart\tend\tvariance\tvalue\n";
		QVector<double> variances;
		for (int i = 0; i < 5; ++i) variances << 9.0 << 10.0 << 11.0;
		for (double z : {2.5, 3.25, 4.0, 5.0}) variances.append(10.0 + z * 1.482);
		for (int i = 0; i < variances.size(); ++i)
		{
			contents += "ignored\tchr22\t" + QByteArray::number(1 + i * 4) + "\t" + QByteArray::number(3 + i * 4)
				+ "\t" + QByteArray::number(variances[i], 'g', 15) + "\t3\n";
		}
		writeFile("variance.seg", contents);
		const auto file = FileLoader::loadSegFile(data_.filePath("variance.seg"));
		QVERIFY(file);
		QCOMPARE(file->varianceMedian(), 10.0);
		QCOMPARE(file->varianceMad(), 1.0);
		QCOMPARE(file->varianceStdev(), 1.482);
		QCOMPARE(file->varianceAlpha(0), 1.0);
		QCOMPARE(file->varianceAlpha(10 + 2.5 * 1.482), 1.0);
		QVERIFY(std::abs(file->varianceAlpha(10 + 3.25 * 1.482) - 0.65) < 1e-10);
		QVERIFY(std::abs(file->varianceAlpha(10 + 4.0 * 1.482) - 0.3) < 1e-10);
		QVERIFY(std::abs(file->varianceAlpha(10 + 5.0 * 1.482) - 0.3) < 1e-10);

		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		SegTrack track(nullptr, "", "Variance");
		track.setSegFile(file);
		track.resize(SharedData::settings().label_width + 4 + 800, 128);
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		const int x0 = SharedData::settings().label_width + 2;
		const QVector<int> expected{0, 89, 178, 178};
		for (int i = 0; i < expected.size(); ++i)
		{
			const QColor color = image.pixelColor(x0 + (15 + i) * 40 + 15, 64);
			QVERIFY(std::abs(color.red() - expected[i]) <= 1);
			QCOMPARE(color.red(), color.green());
			QCOMPARE(color.blue(), 255);
		}
		track.parameters().setString("draw mode", "segments");
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 18 * 40 + 15, 64), QColor(Qt::blue));

		SegFile constant({SegFileLine(Chromosome("chr22"), 1, 2, "1", 3)}, {}, "variance");
		QCOMPARE(constant.varianceAlpha(1), 1.0);
		QCOMPARE(constant.varianceAlpha(2), 0.3);
		SegFile missing({SegFileLine(Chromosome("chr22"), 1, 2, "text", 3)}, {}, "variance");
		QCOMPARE(missing.varianceAlpha(missing[0].variance()), 1.0);
	}

	void igvWholeGenomeRendering()
	{
		const BedLine original = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original.chr(), original.start(), original.end()); });
		SharedData::setRegion(Chromosome(), 0, 0);
		IgvTrack track(nullptr, "", "IGV overview");
		const int x0 = SharedData::settings().label_width + 2;
		track.resize(x0 + 402, 100);
		track.setIgvFile(QSharedPointer<IgvFile>::create(QVector<IgvFileLine>{
			{Chromosome("chr22"), 21, 21, 0.75}, {Chromosome("chr22"), 61, 61, 0.75},
			{Chromosome("chrX"), 21, 21, 0.75}, {Chromosome("chrX"), 61, 61, 0.75},
			{Chromosome("chrY"), 21, 21, 0.75}, {Chromosome("chrY"), 161, 161, 0.75},
			{Chromosome("chr23"), 1, 100, 1.0}, {Chromosome("chrLong"), 1, 10000, 1.0}}, QByteArrayList{}));
		QImage image(track.size(), QImage::Format_ARGB32);
		for (const QByteArray& mode : QByteArrayList{"points", "bar chart", "heatmap", "line plot"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			//Canonical chromosome widths are 100, 100, 200 pixels, matching the top panel.
			for (int offset : {20, 120, 220})
			{
				const QColor pixel = image.pixelColor(x0 + offset, 27);
				QVERIFY2(pixel.blue() > pixel.red(), mode.constData());
				QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + offset, 27));
				auto* popup = track.findChild<QDialog*>();
				QVERIFY(popup);
				const QString chr = offset == 20 ? "chr22" : (offset == 120 ? "chrX" : "chrY");
				QVERIFY(popup->findChild<QLabel*>()->text().contains("Region: " + chr + ":21-21"));
				delete popup;
			}
			if (mode == "line plot")
			{
				QCOMPARE(image.pixelColor(x0 + 90, 27), QColor(Qt::white));
				QCOMPARE(image.pixelColor(x0 + 190, 27), QColor(Qt::white));
				QCOMPARE(image.pixelColor(x0 + 40, 27), QColor(Qt::blue));
			}
		}
		//Switch back to the ordinary single-chromosome coordinate mapping.
		SharedData::setRegion(Chromosome("chrX"), 1, 100);
		track.parameters().setString("draw mode", "points");
		track.render(&image);
		QVERIFY(image.pixelColor(x0 + 82, 27).blue() > image.pixelColor(x0 + 82, 27).red());
	}

	void igvNumericRecordsAndMetadata()
	{
		const QByteArray headers = "#track name=numeric graphtype=BAR_CHART viewlimits=-2:3 maxheightpixels=80\nChromosome\tStart\tEnd\tFeature\tSignal\n";
		writeFile("numeric.igv", headers
			+ "chr22\t30\t40\tfeature\t0.123456789012345\n"
			+ "chr22\t20\t25\tfeature\tinvalid\n"
			+ "chr22\t15\t18\tfeature\n"
			+ "chr22\t12\t14\n"
			+ "chr22\t0\t10\tfeature\t-1.5\n"
			+ "chr2\t0\t5\tfeature\t2.5\n");
		auto file = FileLoader::loadIgvFile(data_.filePath("numeric.igv"));
		QVERIFY(file);
		QCOMPARE(file->count(), 3);
		QVERIFY(file->isSorted());
		QCOMPARE(file->headers().count(), 2);
		QCOMPARE(file->headers()[0], headers.split('\n')[0]);
		QCOMPARE((*file)[0].chr(), Chromosome("chr2"));
		QCOMPARE((*file)[1].start(), 1);
		QCOMPARE((*file)[1].end(), 10);
		QCOMPARE((*file)[1].value(), -1.5);
		QCOMPARE((*file)[2].value(), 0.123456789012345);
		ChromosomalIndex<IgvFile> index(*file);
		QCOMPARE(index.matchingIndices(Chromosome("chr22"), 10, 10), QVector<int>({1}));
		QVERIFY(index.matchingIndices(Chromosome("chr22"), 11, 30).isEmpty());
		QCOMPARE(index.matchingIndices(Chromosome("chr22"), 31, 40), QVector<int>({2}));

		QScopedPointer<IgvTrack> track(IgvTrack::createTrack(nullptr, data_.filePath("numeric.igv")));
		QVERIFY(track);
		QCOMPARE(track->parameters().getString("draw mode"), QByteArray("bar chart"));
		QCOMPARE(track->parameters().getDouble("min"), -2.0);
		QCOMPARE(track->parameters().getDouble("max"), 3.0);
		QCOMPARE(track->sizeHint().height(), 80);

		writeFile("empty.igv", headers + "chr22\t0\t10\tfeature\tinvalid\n");
		auto empty = FileLoader::loadIgvFile(data_.filePath("empty.igv"));
		QVERIFY(empty);
		QCOMPARE(empty->count(), 0);
		ChromosomalIndex<IgvFile> empty_index(*empty);
		QVERIFY(empty_index.matchingIndices(Chromosome("chr22"), 1, 10).isEmpty());
	}

	void igvPopupUsesRenderedPositionAndBarHeight()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		IgvTrack track(nullptr, "", "");
		track.resize(SharedData::settings().label_width + 4 + 160, 100);
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 70, 0.5)}, QByteArrayList{}));
		track.show();
		const int x0 = SharedData::settings().label_width + 2;
		const QPoint marker(x0 + 79, 50);
		for (const QByteArray& mode : QByteArrayList{"points", "line plot", "heatmap", "bar chart"})
		{
			track.parameters().setString("draw mode", mode);
			QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 28, 60));
			QVERIFY(!track.findChild<QDialog*>());
			if (mode == "line plot") continue; // A single record has no line segment.
			QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, marker);
			auto* popup = track.findChild<QDialog*>();
			QVERIFY(popup);
			delete popup;
		}
		track.parameters().setString("draw mode", "bar chart");
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 70, 2.0)}, QByteArrayList{}));
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(marker.x(), 1));
		QVERIFY(!track.findChild<QDialog*>());
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(marker.x(), 5));
		QVERIFY(track.findChild<QDialog*>());
		delete track.findChild<QDialog*>();

		QSignalSpy selected(&track, &TrackWidget::trackSelected);
		QTest::mouseClick(&track, Qt::RightButton, Qt::NoModifier, marker);
		QCOMPARE(selected.count(), 1);
		selected.clear();
		QTest::mousePress(&track, Qt::LeftButton, Qt::NoModifier, marker);
		QTest::mouseRelease(&track, Qt::LeftButton, Qt::NoModifier, marker + QPoint(20, 0));
		QCOMPARE(selected.count(), 1);
		QVERIFY(!track.findChild<QDialog*>());
	}

	void igvPlotClipsLinesAndPreservesFractionalCells()
	{
		SharedData::setRegion(Chromosome("chr22"), 20, 59);
		IgvTrack track(nullptr, "", "");
		track.resize(SharedData::settings().label_width + 4 + 60, 100);
		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 10, 10, 0.5), IgvFileLine(Chromosome("chr22"), 70, 70, 0.5)}, QByteArrayList{}));
		track.parameters().setString("draw mode", "line plot");
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		const int x0 = SharedData::settings().label_width + 2;
		QCOMPARE(image.pixelColor(x0 - 1, 50), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x0 + 60, 50), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x0 + 30, 50), QColor(Qt::blue));

		track.setIgvFile(QSharedPointer<IgvFile>::create(
			QVector<IgvFileLine>{IgvFileLine(Chromosome("chr22"), 27, 27, 1.0)}, QByteArrayList{}));
		for (const QByteArray& mode : QByteArrayList{"heatmap", "bar chart"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			// The cell spans x0+10.5 to x0+12: its fractional right edge colors this pixel.
			QColor edge = image.pixelColor(x0 + 12, 30);
			QVERIFY(edge.blue() > edge.red());
		}
	}

	void bamVisibilityRangeIsSharedAndControlsLoading()
	{
		const QList<Parameter> original_settings = SharedData::parameters().parameters();
		const auto restore_settings = qScopeGuard([&]() { restoreGlobalSettings(original_settings); });
		//This fixture uses a 100-base reference, so allow smaller visibility ranges.
		SharedData::parameters().overrideConstraint("BAM max visibility region", ConstraintType::MIN, 1);
		SharedData::parameters().setInt("BAM max visibility region", 40);
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("visibility.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\nread\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n");
		{
			BamReader reader(data_.filePath("visibility.sam"));
			BamWriter writer(data_.filePath("visibility.bam"));
			writer.writeHeader(reader);
			BamAlignment alignment;
			while (reader.getNextAlignment(alignment)) writer.writeAlignment(alignment);
		}
		QCOMPARE(sam_index_build(data_.filePath("visibility.bam").toUtf8().constData(), 0), 0);
		const QString relative_path = QDir::current().relativeFilePath(data_.filePath("visibility.bam"));
		QScopedPointer<BamAlignmentTrack> alignment(BamAlignmentTrack::createTrack(nullptr, relative_path, "alignments"));
		QScopedPointer<BamCoverageTrack> coverage(BamCoverageTrack::createTrack(nullptr, data_.filePath("visibility.bam"), "coverage"));
		QVERIFY(alignment);
		QVERIFY(coverage);
		auto data = BamTrackDataManager::getOrCreate(relative_path);
		QCOMPARE(data, BamTrackDataManager::getOrCreate(data_.filePath("visibility.bam")));
		QCOMPARE(alignment->parameters().getInt("visibility range"), 40);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 40);
		QVERIFY(data->getAlignments().isEmpty());
		QVERIFY_THROWS_EXCEPTION(ArgumentException, alignment->parameters().setInt("visibility range", 0));

		QSignalSpy changes(data.get(), &BamTrackData::visibilityRangeChanged);
		alignment->parameters().setInt("visibility range", 100);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);
		QCOMPARE(data->getAlignments().count(), 1);
		QCOMPARE(changes.count(), 1);
		coverage->parameters().setInt("visibility range", 20);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 20);
		QVERIFY(data->getAlignments().isEmpty());
		coverage->parameters().setInt("visibility range", 100);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 100);
		QCOMPARE(data->getAlignments().count(), 1);
		QImage image(coverage->size(), QImage::Format_ARGB32);
		coverage->render(&image);

		QByteArray xml;
		QXmlStreamWriter writer(&xml);
		alignment->writeToXml(writer);
		QDomDocument doc;
		QVERIFY(doc.setContent(xml));
		coverage->parameters().setInt("visibility range", 20);
		QStringList errors;
		QScopedPointer<TrackWidget> restored(TrackWidget::fromXml(doc.documentElement(), nullptr, errors));
		QVERIFY(restored);
		QVERIFY(errors.isEmpty());
		QCOMPARE(restored->parameters().getInt("visibility range"), 100);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);

		SharedData::parameters().setInt("BAM max visibility region", 200);
		QCOMPARE(alignment->parameters().getInt("visibility range"), 100);
		BamCoverageTrack other(nullptr, "", "other");
		QCOMPARE(other.parameters().getInt("visibility range"), 200);
		other.parameters().setInt("visibility range", 60);
		QCOMPARE(coverage->parameters().getInt("visibility range"), 100);
	}

	void bamPhaseGroupingAndHitTesting()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("phase.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"unassigned\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n"
			"phase2\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"phase1\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"unsupported\t0\tchr22\t40\t60\t10M\t=\t40\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:3\n");
		{
			BamReader reader(data_.filePath("phase.sam"));
			BamWriter writer(data_.filePath("phase.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("phase.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("phase.bam"), ""));
		QVERIFY(track);
		QCOMPARE(track->parameters().getString("group by"), QByteArray("none"));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track->parameters().setString("group by", "other"));
		track->parameters().setString("color by", "none");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		auto checkRead = [&](int row, int pos, const QString& name)
		{
			const QPoint point(x0 + (pos - 1) * 4 + 2, row * 14 + 6);
			QImage image(track->size(), QImage::Format_ARGB32);
			track->render(&image);
			QVERIFY(image.pixelColor(point) != QColor(Qt::white));
			QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, point);
			auto* popup = track->findChild<QDialog*>();
			QVERIFY(popup);
			QVERIFY(popup->findChild<QLabel*>()->text().contains("Name: " + name + "<br/>"));
			delete popup;
		};
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			track->parameters().setString("group by", "phase");
			checkRead(1, 15, "phase1");
			checkRead(4, 15, "phase2");
			checkRead(7, 15, "unassigned");
			checkRead(7, 45, "unsupported");
			QImage grouped(track->size(), QImage::Format_ARGB32);
			track->render(&grouped);
			for (int y : {0, 42, 84}) QCOMPARE(grouped.pixelColor(x0 + 1, y), QColor(Qt::gray)); // Label boxes.
			for (int y : {35, 77})
			{
				int gray = 0, white = 0;
				for (int x = x0 + 90; x < x0 + 310; ++x)
				{
					gray += grouped.pixelColor(x, y) == QColor(Qt::gray);
					white += grouped.pixelColor(x, y) == QColor(Qt::white);
				}
				QVERIFY(gray > 0 && white > 0); // Dashed separators.
			}
			QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 58, 35));
			QVERIFY(!track->findChild<QDialog*>());
			track->parameters().setString("group by", "none");
			QImage ungrouped(track->size(), QImage::Format_ARGB32);
			track->render(&ungrouped);
			QCOMPARE(ungrouped.pixelColor(x0 + 1, 0), QColor(Qt::white));
			checkRead(0, 15, "unassigned");
			checkRead(1, 15, "phase2");
			checkRead(2, 15, "phase1");
		}

		writeFile("phase-pairs.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"assigned-mate\t65\tchr22\t10\t60\t10M\t=\t30\t30\tAAAAAAAAAA\tIIIIIIIIII\n"
			"phase2\t0\tchr22\t10\t60\t10M\t=\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"conflict\t65\tchr22\t10\t60\t10M\t=\t30\t30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"assigned-mate\t129\tchr22\t30\t60\t10M\t=\t10\t-30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"conflict\t129\tchr22\t30\t60\t10M\t=\t10\t-30\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n");
		{
			BamReader reader(data_.filePath("phase-pairs.sam"));
			BamWriter writer(data_.filePath("phase-pairs.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("phase-pairs.bam").toUtf8().constData(), 0), 0);
		track.reset(BamAlignmentTrack::createTrack(nullptr, data_.filePath("phase-pairs.bam"), ""));
		QVERIFY(track);
		track->parameters().setString("color by", "none");
		track->parameters().setString("group by", "phase");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		checkRead(1, 15, "assigned-mate");
		checkRead(1, 35, "assigned-mate");
		checkRead(4, 15, "phase2");
		checkRead(7, 15, "conflict");
		checkRead(7, 35, "conflict");
	}

	void bamMateChromosomeGroupingAndHitTesting()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("mate-groups.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n@SQ\tSN:chr23\tLN:100\n"
			"different\t65\tchr22\t10\t60\t10M\tchr23\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n"
			"same\t65\tchr22\t10\t60\t10M\t=\t30\t30\tAAAAAAAAAA\tIIIIIIIIII\n"
			"different-secondary\t321\tchr22\t10\t60\t10M\tchr23\t10\t0\tAAAAAAAAAA\tIIIIIIIIII\n"
			"same\t129\tchr22\t30\t60\t10M\t=\t10\t-30\tAAAAAAAAAA\tIIIIIIIIII\n"
			"unpaired\t0\tchr22\t50\t60\t10M\t*\t0\t0\tAAAAAAAAAA\tIIIIIIIIII\n");
		const QString bam_path = data_.filePath("mate-groups.bam");
		{
			BamReader reader(data_.filePath("mate-groups.sam"));
			BamWriter writer(bam_path);
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(bam_path.toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, bam_path, ""));
		QVERIFY(track);
		track->parameters().setString("color by", "none");
		track->parameters().setString("group by", "mate chromosome");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			for (bool secondary : {true, false})
			{
				track->parameters().setBool("show secondary alignments", secondary);
				QImage image(track->size(), QImage::Format_ARGB32);
				track->render(&image);
				QCOMPARE(image.pixelColor(x0 + 58, 2 * 14 + 6) != QColor(Qt::white), secondary);
				const int same_row = secondary ? 5 : 4;
				for (int row : {1, same_row})
				{
					const QPoint point(x0 + 58, row * 14 + 6);
					QVERIFY(image.pixelColor(point) != QColor(Qt::white));
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, point);
					auto* popup = track->findChild<QDialog*>();
					QVERIFY(popup);
					QVERIFY(popup->findChild<QLabel*>()->text().contains(row == 1 ? "Name: different<br/>" : "Name: same<br/>"));
					delete popup;
				}
				QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 218, same_row * 14 + 6));
				auto* popup = track->findChild<QDialog*>();
				QVERIFY(popup);
				QVERIFY(popup->findChild<QLabel*>()->text().contains("Name: unpaired<br/>"));
				delete popup;
			}
		}
	}

	void bamPairOrientationGroupingAndHitTesting()
	{
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		SharedData::parameters().setInt("min window size", 21);
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		const QStringList labels{"FR", "RF", "FF", "RR"};
		const QList<int> first_flags{99, 83, 67, 115};
		const QList<int> second_flags{147, 163, 131, 179};
		QByteArray sam = "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n";
		for (int mate = 0; mate < 2; ++mate)
		{
			for (int group = 0; group < 4; ++group)
			{
				sam += labels[group].toUtf8() + "\t" + QByteArray::number(mate == 0 ? first_flags[group] : second_flags[group])
					+ "\tchr22\t" + (mate == 0 ? "10" : "30") + "\t60\t10M\t=\t" + (mate == 0 ? "30" : "10")
					+ "\t" + (mate == 0 ? "30" : "-30") + "\tAAAAAAAAAA\tIIIIIIIIII\n";
			}
		}
		writeFile("orientation.sam", sam);
		const QString bam_path = data_.filePath("orientation.bam");
		{
			BamReader reader(data_.filePath("orientation.sam"));
			BamWriter writer(bam_path);
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(bam_path.toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, bam_path, ""));
		QVERIFY(track);
		track->parameters().setString("color by", "none");
		track->parameters().setString("group by", "pair orientation");
		track->resize(SharedData::settings().label_width + 4 + 320, 240);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			for (int group = 0; group < 4; ++group)
			{
				for (int position : {15, 35})
				{
					const int displayed_group = group == 0 ? 3 : group - 1;
					const QPoint point(x0 + (position - 1) * 4 + 2, (1 + displayed_group * 3) * 14 + 6);
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, point);
					auto* popup = track->findChild<QDialog*>();
					QVERIFY(popup);
					QVERIFY(popup->findChild<QLabel*>()->text().contains("Name: " + labels[group] + "<br/>"));
					delete popup;
				}
			}
		}
		// The mate outside the loaded region still determines the orientation.
		SharedData::setRegion(Chromosome("chr22"), 1, 20);
		auto data = BamTrackDataManager::getOrCreate(bam_path);
		data->reload(); // Discard the larger previously cached region.
		QCOMPARE(data->getAlignments().count(), 4);
		for (int group = 0; group < 4; ++group) QCOMPARE(data->getAlignments()[group].pairOrientation(), group);
	}

	void bamAlignmentPopupMetadataCache()
	{
		const QByteArray cigar("2H2S3M1I2M1D4M1S1H");
		QByteArray sam("@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n");
		const QList<int> flags{0, 256, 2048};
		for (int i = 0; i < flags.size(); ++i)
		{
			sam += "read" + QByteArray::number(i) + "\t" + QByteArray::number(flags[i])
				+ "\tchr22\t" + QByteArray::number(10 + 20 * i) + "\t60\t" + cigar
				+ "\t*\t0\t0\t" + QByteArray(13, 'A') + "\t" + QByteArray(13, 'I') + "\n";
		}
		writeFile("popup-metadata.sam", sam);
		BamReader reader(data_.filePath("popup-metadata.sam"));
		BamAlignment alignment;
		for (int i = 0; i < flags.size(); ++i)
		{
			QVERIFY(reader.getNextAlignment(alignment));
			BamAlignmentWrapper cached(alignment);
			QCOMPARE(cached.cigar(), cigar);
			QCOMPARE(cached.isSecondaryAlignment(), i == 1);
			QCOMPARE(cached.isSupplementaryAlignment(), i == 2);
		}
	}

	void bamSupplementaryAlignmentsAreLoadedAndDisplayed()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("supplementary.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"read\t65\tchr22\t10\t60\t10M\t=\t40\t40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"read\t2113\tchr22\t10\t60\t10M\t=\t40\t40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"read\t129\tchr22\t40\t60\t10M\t=\t10\t-40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n");
		{
			BamReader reader(data_.filePath("supplementary.sam"));
			BamWriter writer(data_.filePath("supplementary.bam"));
			writer.writeHeader(reader);
			BamAlignment alignment;
			while (reader.getNextAlignment(alignment)) writer.writeAlignment(alignment);
		}
		QCOMPARE(sam_index_build(data_.filePath("supplementary.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("supplementary.bam"), ""));
		QVERIFY(track);
		auto data = BamTrackDataManager::getOrCreate(data_.filePath("supplementary.bam"));
		QCOMPARE(data->getAlignments().count(), 3); //Identical primary/supplementary records remain distinct.
		QVERIFY(!data->getAlignments()[0].isSupplementaryAlignment());
		QVERIFY(data->getAlignments()[1].isSupplementaryAlignment());
		track->parameters().setString("color by", "none");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			for (const QByteArray& group : QByteArrayList{"none", "phase"})
			{
				track->parameters().setString("group by", group);
				for (bool secondary : {false, true})
				{
					track->parameters().setBool("show secondary alignments", secondary);
					const int supplementary_y = (group == "phase" ? 4 : 1) * 14 + 6;
					QImage image(track->size(), QImage::Format_ARGB32);
					track->render(&image);
					QVERIFY(image.pixelColor(x0 + 58, supplementary_y) != QColor(Qt::white));
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 58, supplementary_y));
					auto* popup = track->findChild<QDialog*>();
					QVERIFY(popup);
					const QString supplementary = popup->findChild<QLabel*>()->text();
					QVERIFY(supplementary.contains("Alignment type: supplementary<br/>"));
					QVERIFY(supplementary.contains("CIGAR: 10M<br/>"));
					QVERIFY(!supplementary.contains("<hr>"));
					delete popup;
					const int primary_y = (group == "phase" ? 1 : 0) * 14 + 6;
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 178, primary_y));
					popup = track->findChild<QDialog*>();
					QVERIFY(popup);
					const QString primary = popup->findChild<QLabel*>()->text();
					QVERIFY(!primary.contains("Alignment type:"));
					QCOMPARE(primary.contains("<hr>"), pairs);
					delete popup;
				}
			}
		}
		QScopedPointer<BamCoverageTrack> coverage(BamCoverageTrack::createTrack(nullptr, data_.filePath("supplementary.bam"), ""));
		QVERIFY(coverage);
		coverage->resize(SharedData::settings().label_width + 4 + 320, 100);
		coverage->show();
		QTest::mouseClick(coverage.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 58, 5));
		auto* popup = coverage->findChild<QDialog*>();
		QVERIFY(popup);
		QVERIFY(popup->findChild<QLabel*>()->text().contains("Depth: 2"));
		delete popup;
	}

	void bamSecondaryAlignmentVisibility()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("secondary.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"read\t65\tchr22\t10\t60\t10M\t=\t40\t40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"read\t321\tchr22\t10\t60\t10M\t=\t40\t40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:2\n"
			"read\t129\tchr22\t40\t60\t10M\t=\t10\t-40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n");
		{
			BamReader reader(data_.filePath("secondary.sam"));
			BamWriter writer(data_.filePath("secondary.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("secondary.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("secondary.bam"), ""));
		QVERIFY(track);
		QVERIFY(track->parameters().getBool("show secondary alignments"));
		auto data = BamTrackDataManager::getOrCreate(data_.filePath("secondary.bam"));
		QCOMPARE(data->getAlignments().count(), 3); // Same position/name/cigar must not deduplicate primary and secondary.
		QVERIFY(data->getAlignments()[1].isSecondaryAlignment());
		track->parameters().setString("color by", "none");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			for (const QByteArray& group : QByteArrayList{"none", "phase"})
			{
				track->parameters().setString("group by", group);
				const QPoint secondary(x0 + 58, (group == "phase" ? 4 : 1) * 14 + 6);
				for (bool visible : {true, false, true})
				{
					track->parameters().setBool("show secondary alignments", visible);
					QImage image(track->size(), QImage::Format_ARGB32);
					track->render(&image);
					QCOMPARE(image.pixelColor(secondary) != QColor(Qt::white), visible);
					const int primary_y = (group == "phase" ? 14 : 0) + 6;
					QVERIFY(image.pixelColor(x0 + 58, primary_y) != QColor(Qt::white));
					QVERIFY(image.pixelColor(x0 + 178, primary_y) != QColor(Qt::white));
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, secondary);
					auto* popup = track->findChild<QDialog*>();
					QCOMPARE(popup != nullptr, visible);
					if (popup)
					{
						const QString info = popup->findChild<QLabel*>()->text();
						QVERIFY(info.contains("CIGAR: 10M<br/>"));
						QVERIFY(info.contains("Alignment type: secondary<br/>"));
					}
					delete popup;
				}
			}
		}
	}

	void bamPairedPopupShowsClickedReadFirst()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		const QByteArray sequence(25, 'A');
		const QByteArray qualities(25, 'I');
		writeFile("popup-order.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			+ QByteArray("pair\t65\tchr22\t10\t60\t20M5S\t=\t20\t30\t") + sequence + "\t" + qualities + "\n"
			+ "pair\t129\tchr22\t20\t60\t5S20M\t=\t10\t-30\t" + sequence + "\t" + qualities + "\n");
		{
			BamReader reader(data_.filePath("popup-order.sam"));
			BamWriter writer(data_.filePath("popup-order.bam"));
			writer.writeHeader(reader);
			BamAlignment alignment;
			while (reader.getNextAlignment(alignment)) writer.writeAlignment(alignment);
		}
		QCOMPARE(sam_index_build(data_.filePath("popup-order.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("popup-order.bam"), ""));
		QVERIFY(track);
		track->parameters().setBool("view as pairs", true);
		track->resize(SharedData::settings().label_width + 4 + 800, 100);
		track->show();
		for (bool show_soft_clips : {false, true})
		{
			track->parameters().setBool("show soft-clipped bases", show_soft_clips);
			for (int position : {15, 25, 32, 35})
			{
				const int x = SharedData::settings().label_width + 2 + (position - 1) * 10 + 5;
				QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x, 6));
				auto* popup = track->findChild<QDialog*>();
				QVERIFY(popup);
				const QString info = popup->findChild<QLabel*>()->text();
				const int first = info.indexOf("Region: chr22:10-29 @ MQ 60<br/>");
				const int second = info.indexOf("Region: chr22:20-39 @ MQ 60<br/>");
				QVERIFY(first >= 0 && second >= 0);
				QVERIFY(info.contains("CIGAR: 20M5S<br/>"));
				QVERIFY(info.contains("CIGAR: 5S20M<br/>"));
				QVERIFY(!info.contains("Alignment type:"));
				const bool only_second = position == 35 || (position == 32 && !show_soft_clips);
				QCOMPARE(second < first, only_second);
				delete popup;
			}
		}
	}

	void bamInsertionMarkersFitLengthsAndStayCentered()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 41);
		QByteArray sam = "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n";
		const QList<int> lengths{9, 123, 12345};
		for (int length : lengths)
		{
			sam += "ins-" + QByteArray::number(length) + "\t0\tchr22\t10\t60\t10M" + QByteArray::number(length) + "I10M\t*\t0\t0\t"
				+ QByteArray(length + 20, 'C') + "\t" + QByteArray(length + 20, 'I') + "\n";
		}
		writeFile("insertions.sam", sam);
		{
			BamReader reader(data_.filePath("insertions.sam"));
			BamWriter writer(data_.filePath("insertions.bam"));
			writer.writeHeader(reader);
			BamAlignment alignment;
			while (reader.getNextAlignment(alignment)) writer.writeAlignment(alignment);
		}
		QCOMPARE(sam_index_build(data_.filePath("insertions.bam").toUtf8().constData(), 0), 0);
		std::unique_ptr<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("insertions.bam"), "Insertions"));
		QVERIFY(track);
		QFont length_font;
		length_font.setPixelSize(12);
		length_font.setBold(true);
		const QFontMetricsF metrics(length_font);
		const QColor purple(140, 0, 200);
		for (bool pairs : {false, true}) for (bool all_bases : {false, true}) for (int usable_width : {820, 777})
		{
			track->parameters().setBool("view as pairs", pairs);
			track->parameters().setBool("show all bases", all_bases);
			track->resize(SharedData::settings().label_width + 4 + usable_width, 200);
			QImage image(track->size(), QImage::Format_ARGB32);
			track->render(&image);
			//10M at POS=10 puts the insertion between reference bases 19 and 20.
			const double center = SharedData::settings().label_width + 2 + 19 * (float(usable_width) / 41);
			for (int row = 0; row < lengths.size(); ++row)
			{
				const int y = row * 14;
				int left = image.width(), right = -1;
				for (int x = SharedData::settings().label_width + 2; x < image.width() - 2; ++x)
				{
					for (int dy = 0; dy < 12; ++dy) if (image.pixelColor(x, y + dy) == purple)
					{
						left = std::min(left, x);
						right = std::max(right, x);
					}
				}
				QVERIFY(right >= left);
				QVERIFY(right - left + 1 >= metrics.horizontalAdvance(QString::number(lengths[row])) + 3);
				QVERIFY(std::abs((left + right + 1) / 2.0 - center) <= 1.0);
				//Check the complete length remains visible over mismatches and the all-bases display.
				QImage expected(image.size(), QImage::Format_ARGB32);
				expected.fill(Qt::white);
				QPainter painter(&expected);
				const double width = std::ceil(metrics.horizontalAdvance(QString::number(lengths[row]))) + 4;
				const QRectF marker(center - width / 2, y, width, 12);
				painter.fillRect(marker, purple);
				painter.setFont(length_font);
				painter.setPen(Qt::white);
				painter.drawText(marker, Qt::AlignCenter, QString::number(lengths[row]));
				painter.end();
				const QRect interior = marker.toAlignedRect().adjusted(1, 0, -1, 0);
				QCOMPARE(image.copy(interior), expected.copy(interior));
			}
		}
		//An insertion outside the view must not be shifted to the left viewport edge.
		SharedData::setRegion(Chromosome("chr22"), 21, 61);
		QImage image(track->size(), QImage::Format_ARGB32);
		track->render(&image);
		for (int y = 0; y < 40; ++y) for (int x = SharedData::settings().label_width + 2; x < image.width() - 2; ++x)
		{
			QVERIFY(image.pixelColor(x, y) != purple);
		}
	}

	void bamMappingQualityFilter()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("mapq.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"read\t65\tchr22\t10\t0\t10M\t=\t40\t40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n"
			"read\t129\tchr22\t40\t30\t10M\t=\t10\t-40\tAAAAAAAAAA\tIIIIIIIIII\tHP:i:1\n");
		{
			BamReader reader(data_.filePath("mapq.sam"));
			BamWriter writer(data_.filePath("mapq.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("mapq.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("mapq.bam"), ""));
		QVERIFY(track);
		QCOMPARE(track->parameters().getInt("min MQ"), 0);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track->parameters().setInt("min MQ", -1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track->parameters().setInt("min MQ", 256));
		track->parameters().setString("color by", "none");
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool pairs : {false, true})
		{
			track->parameters().setBool("view as pairs", pairs);
			for (const QByteArray& group : QByteArrayList{"none", "phase", "mate chromosome", "pair orientation"})
			{
				track->parameters().setString("group by", group);
				const int y = (group == "none" ? 0 : 14) + 6;
				for (int min_mq : {0, 1, 30, 31, 0})
				{
					track->parameters().setInt("min MQ", min_mq);
					QImage image(track->size(), QImage::Format_ARGB32);
					track->render(&image);
					QCOMPARE(image.pixelColor(x0 + 58, y) != QColor(Qt::white), min_mq == 0);
					QCOMPARE(image.pixelColor(x0 + 178, y) != QColor(Qt::white), min_mq <= 30);
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 58, y));
					auto* popup = track->findChild<QDialog*>();
					QCOMPARE(popup != nullptr, min_mq == 0);
					delete popup;
					QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 178, y));
					popup = track->findChild<QDialog*>();
					QCOMPARE(popup != nullptr, min_mq <= 30);
					if (popup)
					{
						const QString info = popup->findChild<QLabel*>()->text();
						QVERIFY(info.contains("Region: chr22:40-49 @ MQ 30<br/>"));
						QVERIFY(info.contains("Strand: Forward<br/>"));
						QVERIFY(info.contains("@ MQ 30<br/>"));
						QCOMPARE(info.contains("@ MQ 0<br/>"), pairs && min_mq == 0);
					}
					delete popup;
				}
			}
		}
		QCOMPARE(BamTrackDataManager::getOrCreate(data_.filePath("mapq.bam"))->getAlignments().count(), 2);
	}

	void bamCoverageReadFiltersAreShared()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("coverage-filters.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"primary\t0\tchr22\t10\t30\t3S10M2S\t*\t0\t0\tCCCAAAAAAAAAATT\tIIIIIIIIIIIIIII\n"
			"secondary\t272\tchr22\t10\t0\t3S10M2S\t*\t0\t0\tCCCAAAAAAAAAATT\tIIIIIIIIIIIIIII\n");
		const QString bam_path = data_.filePath("coverage-filters.bam");
		{
			BamReader reader(data_.filePath("coverage-filters.sam"));
			BamWriter writer(bam_path);
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(bam_path.toUtf8().constData(), 0), 0);
		QScopedPointer<BamCoverageTrack> coverage(BamCoverageTrack::createTrack(nullptr, bam_path, ""));
		QScopedPointer<BamAlignmentTrack> alignment(BamAlignmentTrack::createTrack(nullptr, QDir::current().relativeFilePath(bam_path), ""));
		QVERIFY(coverage && alignment);
		QVERIFY(!coverage->parameters().getBool("show soft-clipped bases"));
		QVERIFY(coverage->parameters().getBool("show secondary alignments"));
		QCOMPARE(coverage->parameters().getInt("min MQ"), 0);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, coverage->parameters().setInt("min MQ", -1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, coverage->parameters().setInt("min MQ", 256));
		coverage->resize(SharedData::settings().label_width + 4 + 320, 100);
		coverage->show();
		const auto check_depth = [&](int position, int depth)
		{
			const int x0 = SharedData::settings().label_width + 2;
			const int x = x0 + (position - SharedData::region().start()) * 320 / SharedData::region().length() + 2;
			QTest::mouseClick(coverage.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x, 50));
			auto* popup = coverage->findChild<QDialog*>();
			QVERIFY(popup);
			const QString info = popup->findChild<QLabel*>()->text();
			QVERIFY2(info.contains(QString("Depth: %1\n").arg(depth)), qPrintable(info));
			delete popup;
		};
		check_depth(10, 2);
		check_depth(7, 0);
		auto data = BamTrackDataManager::getOrCreate(bam_path);
		QSignalSpy changes(data.get(), &BamTrackData::readFilterChanged);
		coverage->parameters().setInt("min MQ", 1);
		QCOMPARE(alignment->parameters().getInt("min MQ"), 1);
		QCOMPARE(changes.count(), 1);
		check_depth(10, 1);
		alignment->parameters().setInt("min MQ", 31);
		QCOMPARE(coverage->parameters().getInt("min MQ"), 31);
		check_depth(10, 0);
		alignment->parameters().setInt("min MQ", 0);
		check_depth(10, 2);
		alignment->parameters().setBool("show secondary alignments", false);
		QVERIFY(!coverage->parameters().getBool("show secondary alignments"));
		check_depth(10, 1);
		alignment->parameters().setBool("show soft-clipped bases", true);
		QVERIFY(coverage->parameters().getBool("show soft-clipped bases"));
		check_depth(7, 1);
		check_depth(20, 1);
		coverage->parameters().setBool("show secondary alignments", true);
		QVERIFY(alignment->parameters().getBool("show secondary alignments"));
		check_depth(7, 2);
		coverage->parameters().setInt("min MQ", 30);
		check_depth(7, 1);
		// A track attached later inherits the current filters without resetting existing tracks.
		QScopedPointer<BamCoverageTrack> late(BamCoverageTrack::createTrack(nullptr, bam_path, ""));
		QVERIFY(late);
		QVERIFY(late->parameters().getBool("show soft-clipped bases"));
		QCOMPARE(late->parameters().getInt("min MQ"), 30);
		SharedData::setRegion(Chromosome("chr22"), 7, 9); // Only the clipped bases overlap.
		check_depth(7, 1);
		coverage->parameters().setBool("show soft-clipped bases", false);
		QVERIFY(!alignment->parameters().getBool("show soft-clipped bases"));
		QVERIFY(!late->parameters().getBool("show soft-clipped bases"));
		check_depth(7, 0);
		QCOMPARE(changes.count(), 8);
		coverage->parameters().setInt("track height", 80);
		QCOMPARE(changes.count(), 8);
		QCOMPARE(data->getAlignments().count(), 2);
	}

	void bamUnpairedReadsReuseRowsAndShrink()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 20);
		writeFile("unpaired.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
			"dense1\t0\tchr22\t10\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n"
			"dense2\t0\tchr22\t10\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n"
			"dense3\t0\tchr22\t10\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n"
			"sparse1\t0\tchr22\t60\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n"
			"sparse2\t0\tchr22\t75\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n");
		{
			BamReader reader(data_.filePath("unpaired.sam"));
			QVERIFY_EXCEPTION_THROWN(reader.chromosome(-1), ArgumentException);
			QVERIFY_EXCEPTION_THROWN(reader.chromosome(1), ArgumentException);
			QCOMPARE(reader.chromosome(0), Chromosome("chr22"));
			BamWriter writer(data_.filePath("unpaired.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("unpaired.bam").toUtf8().constData(), 0), 0);
		QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, data_.filePath("unpaired.bam"), ""));
		QVERIFY(track);
		QVERIFY(track->parameters().getBool("view as pairs"));
		track->resize(SharedData::settings().label_width + 4 + 320, 200);
		track->show();
		auto data = BamTrackDataManager::getOrCreate(data_.filePath("unpaired.bam"));
		for (int repeat = 0; repeat < 2; ++repeat)
		{
			SharedData::setRegion(Chromosome("chr22"), 1, 20);
			const int dense_height = track->sizeHint().height();
			SharedData::setRegion(Chromosome("chr22"), 55, 85);
			const int sparse_height = track->sizeHint().height();
			QVERIFY(sparse_height < dense_height);
			QCOMPARE(data->getAlignments().count(), 2);
			for (const auto& alignment : data->getAlignments())
			{
				QVERIFY(!alignment.mate_chr.isValid());
				QVERIFY(!alignment.isMateMapped());
			}
			for (int position : {62, 77})
			{
				const BedLine& region = SharedData::region();
				const int x = SharedData::settings().label_width + 2 + (position - region.start()) * (track->width() - SharedData::settings().label_width - 4) / region.length();
				track->parameters().setString("color by", "none");
				QImage neutral(track->size(), QImage::Format_ARGB32);
				track->render(&neutral);
				track->parameters().setString("color by", "insert size");
				QImage insert_size(track->size(), QImage::Format_ARGB32);
				track->render(&insert_size);
				QCOMPARE(insert_size.pixelColor(x, 6), neutral.pixelColor(x, 6));
				QVERIFY(insert_size.pixelColor(x, 6) != QColor(Qt::white));
				QTest::mouseClick(track.get(), Qt::LeftButton, Qt::NoModifier, QPoint(x, 6));
				auto* popup = track->findChild<QDialog*>();
				QVERIFY2(popup, qPrintable(QString("No popup for position %1, repeat %2, width %3").arg(position).arg(repeat).arg(track->width()))); // Both non-overlapping reads must occupy row zero.
				delete popup;
			}
			track->parameters().setString("group by", "none");
			QImage ungrouped(track->size(), QImage::Format_ARGB32);
			track->render(&ungrouped);
			track->parameters().setString("group by", "pair orientation");
			QImage orientation(track->size(), QImage::Format_ARGB32);
			track->render(&orientation);
			QCOMPARE(orientation, ungrouped); // No grouping for single-end files.
			track->parameters().setBool("show secondary alignments", false);
			QCOMPARE(track->sizeHint().height(), sparse_height);
			track->parameters().setBool("show secondary alignments", true);
			QCOMPARE(track->sizeHint().height(), sparse_height);
		}
	}

	void bamLongReadsCacheEntireReference()
	{
		QTest::failOnWarning(QRegularExpression("Reference sequence too short for alignment.*"));
		SharedData::setRegion(Chromosome("chrLong"), 3000, 3020);
		QByteArray bases(5000, 'A');
		bases[0] = 'C';
		bases[4900] = 'C';
		writeFile("long-read.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chrLong\tLN:10000\n"
			"long-read\t0\tchrLong\t1000\t60\t5000M\t*\t0\t0\t" + bases + "\t" + QByteArray(5000, 'I') + "\n");
		const QString bam_path = data_.filePath("long-read.bam");
		{
			BamReader reader(data_.filePath("long-read.sam"));
			BamWriter writer(bam_path);
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(bam_path.toUtf8().constData(), 0), 0);
		auto data = BamTrackDataManager::getOrCreate(bam_path);
		QVERIFY(data);
		QCOMPARE(data->getAlignments().count(), 1);
		QCOMPARE(data->getRefSeqStart(), 1000);
		QCOMPARE(data->getReferenceSeq().length(), 5000);
		const auto& mismatches = data->getAlignments().first().getMismatches();
		QCOMPARE(mismatches.count(), 2);
		QCOMPARE(mismatches[0].genomic_pos, 1000);
		QCOMPARE(mismatches[1].genomic_pos, 5900);
		SharedData::setRegion(Chromosome("chrLong"), 3010, 3030); // Incremental load.
		QCOMPARE(data->getAlignments().count(), 1);
		QCOMPARE(data->getAlignments().first().getMismatches().count(), 2);
		SharedData::setRegion(Chromosome("chrLong"), 5880, 5920); // Pan to the distant mismatch.
		QCOMPARE(data->getAlignments().first().getMismatches().last().genomic_pos, 5900);
	}

	void bamFilePairingControlsUnpairedReadColor()
	{
		SharedData::setRegion(Chromosome("chr22"), 20, 40);
		for (bool paired_end : {false, true})
		{
			const QString base = paired_end ? "paired-color" : "unpaired-color";
			QByteArray sam = "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n"
				"unpaired\t0\tchr22\t30\t60\t5M\t*\t0\t0\tAAAAA\tIIIII\n";
			// Detect pairing from a read outside the displayed region.
			if (paired_end) sam += "paired\t65\tchr22\t80\t60\t5M\t=\t90\t15\tAAAAA\tIIIII\n";
			writeFile(base + ".sam", sam);
			const QString bam_path = data_.filePath(base + ".bam");
			{
				BamReader reader(data_.filePath(base + ".sam"));
				BamWriter writer(bam_path);
				writer.writeHeader(reader);
				BamAlignment al;
				while (reader.getNextAlignment(al)) writer.writeAlignment(al);
			}
			QCOMPARE(sam_index_build(bam_path.toUtf8().constData(), 0), 0);
			QScopedPointer<BamAlignmentTrack> track(BamAlignmentTrack::createTrack(nullptr, bam_path, ""));
			QVERIFY(track);
			auto data = BamTrackDataManager::getOrCreate(bam_path);
			QCOMPARE(data->isPairedEnd(), paired_end);
			QCOMPARE(data->getAlignments().count(), 1);
			QVERIFY(!data->getAlignments().first().isPaired());
			track->resize(SharedData::settings().label_width + 4 + 420, 100);
			track->show();
			const QPoint point(SharedData::settings().label_width + 2 + 12 * 20, 6);
			track->parameters().setString("color by", "none");
			QImage neutral(track->size(), QImage::Format_ARGB32);
			track->render(&neutral);
			track->parameters().setString("color by", "insert size");
			QImage colored(track->size(), QImage::Format_ARGB32);
			track->render(&colored);
			QCOMPARE(colored.pixelColor(point), paired_end ? QColor(Qt::black) : neutral.pixelColor(point));
			QVERIFY(colored.pixelColor(point) != QColor(Qt::white));
			TrackWidget* widget = track.get();
			QMenu menu;
			widget->populateContextMenu(menu, point);
			QCOMPARE(menu.actions().count(), 2);
			QCOMPARE(menu.actions()[0]->text(), QString("Read"));
			QCOMPARE(menu.actions()[1]->text(), QString("Track"));
			QMenu* read_menu = menu.actions()[0]->menu();
			QVERIFY(read_menu);
			QCOMPARE(read_menu->actions()[0]->text(), QString("Go to mate"));
			QVERIFY(!read_menu->actions()[0]->isEnabled()); // Unpaired read.
			QCOMPARE(read_menu->actions()[1]->text(), QString("Select"));
			read_menu->actions()[1]->trigger();
			QMenu selected_menu;
			widget->populateContextMenu(selected_menu, point);
			QMenu* selected_read_menu = selected_menu.actions()[0]->menu();
			QCOMPARE(selected_read_menu->actions()[1]->text(), QString("Deselect"));
			selected_read_menu->actions()[1]->trigger();
			QMenu blank_menu;
			widget->populateContextMenu(blank_menu, QPoint(2, 2));
			QCOMPARE(blank_menu.actions().count(), 1);
			QCOMPARE(blank_menu.actions()[0]->text(), QString("Track"));
			data->reload();
			QCOMPARE(data->isPairedEnd(), paired_end);
		}
	}

	void bamInsertSizePercentileSettings()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		QByteArray sam = "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\n";
		for (int i = 1; i <= 10; ++i)
		{
			sam += "read" + QByteArray::number(i) + "\t0\tchr22\t10\t60\t10M\t=\t10\t" + QByteArray::number(i * 100) + "\tAAAAAAAAAA\tIIIIIIIIII\n";
		}
		writeFile("insert-size.sam", sam);
		{
			BamReader reader(data_.filePath("insert-size.sam"));
			BamWriter writer(data_.filePath("insert-size.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("insert-size.bam").toUtf8().constData(), 0), 0);
		auto data = BamTrackDataManager::getOrCreate(data_.filePath("insert-size.bam"));
		QVERIFY(data);
		QCOMPARE(data->getInsertSizeStats().insert_size_min, 100);
		QCOMPARE(data->getInsertSizeStats().insert_size_max, 1000);
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		SharedData::parameters().setDouble("BAM insert size lower percentile", 0.2);
		SharedData::parameters().setDouble("BAM insert size upper percentile", 0.8);
		QCOMPARE(data->getInsertSizeStats().insert_size_min, 300);
		QCOMPARE(data->getInsertSizeStats().insert_size_max, 900);
		SharedData::parameters().setDouble("BAM insert size lower percentile", 0.0);
		SharedData::parameters().setDouble("BAM insert size upper percentile", 1.0);
		QCOMPARE(data->getInsertSizeStats().insert_size_min, 100);
		QCOMPARE(data->getInsertSizeStats().insert_size_max, 1000);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, SharedData::parameters().setDouble("BAM insert size lower percentile", -0.1));
		SharedData::parameters().setDouble("BAM insert size lower percentile", 0.9);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, SharedData::parameters().setDouble("BAM insert size upper percentile", 0.8));
	}

	void regionBookmarks()
	{
		const QString key = "bookmark_regions";
		const QString original_override = Settings::settingsOverride();
		const QString local_path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		QVERIFY(QDir().mkpath(local_path));
		QSettings local_settings(local_path + "/" + QCoreApplication::applicationName() + "_local.ini", QSettings::IniFormat);
		const bool had_bookmarks = local_settings.contains(key);
		const QVariant original_bookmarks = local_settings.value(key);
		local_settings.setValue(key, QStringList());
		local_settings.sync();
		Settings::setSettingsOverride(local_settings.fileName()); // Read the same file that Settings writes.
		const auto restore_settings = qScopeGuard([&]()
		{
			if (had_bookmarks) local_settings.setValue(key, original_bookmarks);
			else local_settings.remove(key);
			local_settings.sync();
			Settings::setSettingsOverride(original_override);
		});
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		GenomeVisualizationWidget viewer(nullptr);
		viewer.show();
		auto* dropdown = viewer.findChild<QToolButton*>("bookmarks_btn");
		QVERIFY(dropdown);
		const QString coordinates = SharedData::region().toString(true);
		bool default_selected = false;
		for (const QString& name : QStringList{"Z first & bookmark", "A second bookmark", "M third bookmark"})
		{
			bool stored = false;
			QTimer::singleShot(0, &viewer, [&]()
			{
				auto* menu = viewer.findChild<QMenu*>("bookmarks_menu");
				if (!menu) return;
				menu->close();
				QTimer::singleShot(0, &viewer, [&]()
				{
					auto* dialog = viewer.findChild<QDialog*>("AddBookmarkDialog");
					if (!dialog) return;
					auto* name_edit = dialog->findChild<QLineEdit*>("name");
					auto* show_in_track = dialog->findChild<QCheckBox*>("show_in_chromosome_track");
					default_selected = name_edit->selectedText() == coordinates && !show_in_track->isChecked();
					name_edit->setText(name);
					show_in_track->setChecked(name == "M third bookmark");
					dialog->accept();
					stored = true;
				});
				menu->actions().first()->trigger();
			});
			dropdown->showMenu();
			QVERIFY(stored);
			QVERIFY(default_selected);
		}
		QCOMPARE(Settings::stringList(key).count(), 3);
		QVERIFY(!QJsonDocument::fromJson(Settings::stringList(key)[0].toUtf8()).object().value("show_in_chromosome_view").toBool());
		QVERIFY(QJsonDocument::fromJson(Settings::stringList(key)[2].toUtf8()).object().value("show_in_chromosome_view").toBool());
		SharedData::setRegion(Chromosome("chr22"), 40, 100);
		GenomeVisualizationWidget reopened(nullptr); // Bookmarks must survive viewer recreation.
		reopened.show();
		bool order_correct = false;
		QTimer::singleShot(0, &reopened, [&]()
		{
			auto* menu = reopened.findChild<QMenu*>("bookmarks_menu");
			if (!menu) return;
			menu->close();
			order_correct = menu->actions()[3]->text() == "Z first && bookmark" && menu->actions()[4]->text() == "A second bookmark";
			menu->actions()[3]->trigger();
		});
		reopened.findChild<QToolButton*>("bookmarks_btn")->showMenu();
		QVERIFY(order_correct);
		QCOMPARE(SharedData::region().toString(true), coordinates);
		bool deleted = false;
		QTimer::singleShot(0, &reopened, [&]()
		{
			auto* menu = reopened.findChild<QMenu*>("bookmarks_menu");
			if (!menu) return;
			menu->close();
			QTimer::singleShot(0, &reopened, [&]()
			{
				auto* manager = reopened.findChild<BookmarkManager*>();
				if (!manager) return;
				auto* table = manager->findChild<QTableWidget*>("bookmarks_table");
				auto* delete_button = manager->findChild<QPushButton*>("delete_btn");
				if (!table || table->rowCount() != 3 || !delete_button) { manager->reject(); return; }
				const bool initial_state = !delete_button->isEnabled() && table->columnCount() == 3 && table->item(0, 0)->text() == "Z first & bookmark" && table->item(0, 1)->text() == coordinates && table->item(1, 2)->checkState() == Qt::Unchecked;
				ChromosomePanel chromosome(nullptr);
				chromosome.resize(SharedData::settings().label_width + 4 + 480, 80);
				chromosome.show();
				ChromosomeContextPanel context(nullptr);
				context.resize(chromosome.width(), 60);
				context.show();
				auto context_has_bookmark = [&]()
				{
					QImage image(context.size(), QImage::Format_ARGB32);
					context.render(&image);
					return image.pixelColor(SharedData::settings().label_width + 2, context.height() - 2) == QColor(128, 0, 128);
				};
				auto has_purple_line = [&]()
				{
					QImage image(chromosome.size(), QImage::Format_ARGB32);
					chromosome.render(&image);
					for (int x = SharedData::settings().label_width + 2; x < chromosome.width() - 2; ++x)
					{
						for (int y = 23; y < 27; ++y) if (image.pixelColor(x, y) == QColor(128, 0, 128)) return true;
					}
					return false;
				};
				const bool initially_shown = table->item(2, 2)->checkState() == Qt::Checked && has_purple_line() && context_has_bookmark();
				table->item(2, 2)->setCheckState(Qt::Unchecked);
				const bool initially_hidden = initially_shown && !has_purple_line() && !context_has_bookmark();
				QSignalSpy bookmark_changes(SharedData::instance(), &SharedData::bookmarksChanged);
				table->item(1, 2)->setCheckState(Qt::Checked);
				const bool shown = has_purple_line() && context_has_bookmark() && QJsonDocument::fromJson(Settings::stringList(key)[1].toUtf8()).object().value("show_in_chromosome_view").toBool() && bookmark_changes.count() == 1;
				const QPoint marker(SharedData::settings().label_width + 2, 23);
				QHelpEvent tooltip(QEvent::ToolTip, marker, chromosome.mapToGlobal(marker));
				QApplication::sendEvent(&chromosome, &tooltip);
				const bool tooltip_shown = QToolTip::text().contains("Bookmark: A second bookmark");
				QToolTip::hideText();
				const QPoint context_marker(SharedData::settings().label_width + 2, context.height() - 2);
				QHelpEvent context_tooltip(QEvent::ToolTip, context_marker, context.mapToGlobal(context_marker));
				QApplication::sendEvent(&context, &context_tooltip);
				const bool context_tooltip_shown = QToolTip::text().contains("Bookmark: A second bookmark");
				QToolTip::hideText();
				table->item(1, 2)->setCheckState(Qt::Unchecked);
				const bool hidden_again = !has_purple_line() && !context_has_bookmark();
				table->item(1, 2)->setCheckState(Qt::Checked);
				table->item(1, 0)->setCheckState(Qt::Checked);
				bool rename_selected = false;
				QTimer::singleShot(0, manager, [&]()
				{
					auto* menu = manager->findChild<QMenu*>("bookmark_context_menu");
					if (!menu) return;
					menu->close();
					QTimer::singleShot(0, manager, [&]()
					{
						auto* dialog = manager->findChild<QInputDialog*>();
						if (!dialog) return;
						rename_selected = dialog->findChild<QLineEdit*>()->selectedText() == "A second bookmark";
						dialog->setTextValue("Renamed bookmark");
						dialog->accept();
					});
					menu->actions().first()->trigger();
				});
				emit table->customContextMenuRequested(table->visualItemRect(table->item(1, 0)).center());
				const bool renamed = rename_selected && table->item(1, 0)->text() == "Renamed bookmark" && table->item(1, 0)->checkState() == Qt::Checked && table->item(1, 1)->text() == coordinates && Settings::stringList(key)[1].contains("Renamed bookmark");
				table->item(1, 0)->setCheckState(Qt::Unchecked);
				table->item(0, 0)->setCheckState(Qt::Checked);
				table->item(2, 0)->setCheckState(Qt::Checked);
				const bool enabled = delete_button->isEnabled();
				delete_button->click();
				deleted = initial_state && initially_hidden && shown && tooltip_shown && context_tooltip_shown && hidden_again && renamed && enabled && table->rowCount() == 1 && table->item(0, 0)->text() == "Renamed bookmark" && manager->result() == QDialog::Accepted && !manager->isVisible();
			});
			menu->actions()[1]->trigger();
		});
		reopened.findChild<QToolButton*>("bookmarks_btn")->showMenu();
		QVERIFY(deleted);
		QCOMPARE(Settings::stringList(key).count(), 1);
		QVERIFY(Settings::stringList(key).first().contains("Renamed bookmark"));
		QVERIFY(QJsonDocument::fromJson(Settings::stringList(key).first().toUtf8()).object().value("show_in_chromosome_view").toBool());
	}

	void chromosomePanelShowsParOnlyOnX()
	{
		const BedLine original_region = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original_region.chr(), original_region.start(), original_region.end()); });
		SharedData::setRegion(Chromosome("chrX"), 1, 80);
		ChromosomePanel panel(nullptr);
		panel.resize(SharedData::settings().label_width + 4 + 480, 80);
		panel.show();
		QImage image(panel.size(), QImage::Format_ARGB32);
		panel.render(&image);
		int chromosome_end = 0;
		const BedFile& bands = NGSHelper::cytoBands();
		for (int i = 0; i < bands.count(); ++i) if (bands[i].chr().isX()) chromosome_end = std::max(chromosome_end, bands[i].end());
		const int x0 = SharedData::settings().label_width + 2;
		const double scale = 480.0 / chromosome_end;
		const BedFile& par = NGSHelper::pseudoAutosomalRegion();
		for (int i = 0; i < par.count(); ++i)
		{
			if (!par[i].chr().isX()) continue;
			const double x = x0 + (par[i].start() - 1) * scale;
			const double width = std::max(1.0, par[i].length() * scale);
			bool found = false;
			for (int pixel = std::max(x0, (int)std::floor(x)); pixel <= std::min(panel.width() - 3, (int)std::ceil(x + width)); ++pixel)
			{
				if (image.pixelColor(pixel, 3) == QColor(Qt::darkGray)) found = true;
			}
			QVERIFY(found);
			const QPoint marker((int)std::ceil(x), 3);
			QHelpEvent tooltip(QEvent::ToolTip, marker, panel.mapToGlobal(marker));
			QApplication::sendEvent(&panel, &tooltip);
			QVERIFY(QToolTip::text().contains("Pseudoautosomal region:"));
			QVERIFY(QToolTip::text().contains(par[i].toString(true)));
			QToolTip::hideText();
		}
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		panel.render(&image);
		const QPoint marker(x0 + 1, 3);
		QHelpEvent no_par_tooltip(QEvent::ToolTip, marker, panel.mapToGlobal(marker));
		QApplication::sendEvent(&panel, &no_par_tooltip);
		QVERIFY(!no_par_tooltip.isAccepted());
	}

	void genePanelCodingAminoAcids()
	{
		const TranscriptList original_transcripts = SharedData::transcripts();
		const BedLine original_region = SharedData::region();
		const auto restore = qScopeGuard([&]()
		{
			SharedData::setTranscripts(original_transcripts);
			SharedData::setRegion(original_region.chr(), original_region.start(), original_region.end());
		});
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		GenePanel panel(nullptr);
		QFont font = panel.font();
		font.setPixelSize(10);
		panel.setFont(font);
		panel.resize(SharedData::settings().label_width + 4 + 480, 100); //6 pixels per base.
		panel.show();
		const int y = 6 + QFontMetrics(font).height();
		const int x0 = SharedData::settings().label_width + 2;
		for (bool plus : {true, false})
		{
			Transcript transcript;
			transcript.setName("ENSTcoding");
			transcript.setGene("GENE");
			transcript.setSource(Transcript::ENSEMBL);
			transcript.setStrand(plus ? Transcript::PLUS : Transcript::MINUS);
			transcript.setGencodePrimaryTranscript(true);
			BedFile exons;
			exons.append(BedLine(Chromosome("chr22"), 10, 15));
			exons.append(BedLine(Chromosome("chr22"), 30, 36));
			transcript.setRegions(exons, plus ? 11 : 36, plus ? 36 : 11);
			TranscriptList transcripts;
			transcripts.append(transcript);
			SharedData::setTranscripts(transcripts);
			QImage actual(panel.size(), QImage::Format_ARGB32);
			panel.render(&actual);
			//AAA translates to K; its reverse complement TTT translates to F.
			for (const QPair<int, int>& fragment : {qMakePair(11, 13), qMakePair(14, 15), qMakePair(31, 33), qMakePair(34, 36)})
			{
				const int width = (fragment.second - fragment.first + 1) * 6;
				QImage expected(width, 12, QImage::Format_ARGB32);
				const bool darker = plus ? (fragment.first == 14 || fragment.first == 34) : (fragment.first == 11 || fragment.first == 31);
				expected.fill(darker ? QColor("#0C0C78") : QColor("#5C5CA4"));
				QPainter painter(&expected);
				painter.setFont(font);
				painter.setRenderHint(QPainter::Antialiasing, true);
				painter.setPen(Qt::white);
				painter.drawText(QRectF(1, 0, width - 2, 12), Qt::AlignCenter, plus ? "K" : "F");
				painter.end();
				QCOMPARE(actual.copy(x0 + (fragment.first - 1) * 6 + 1, y + 1, width - 2, 10), expected.copy(1, 1, width - 2, 10));
			}
			//The one-base fragment at position 30 is too narrow for either letter.
			for (int x = x0 + 29 * 6 + 1; x < x0 + 30 * 6 - 1; ++x)
			{
				for (int row = y + 1; row < y + 11; ++row) QCOMPARE(actual.pixelColor(x, row).rgba(), (plus ? QColor("#0C0C78") : QColor("#5C5CA4")).rgba());
			}
		}
		panel.resize(SharedData::settings().label_width + 4 + 160, 100); //Even full codons are too narrow.
		QImage zoomed_out(panel.size(), QImage::Format_ARGB32);
		panel.render(&zoomed_out);
		for (int x = x0 + 10 * 2 + 1; x < x0 + 15 * 2 - 1; ++x)
		{
			for (int row = y + 1; row < y + 11; ++row) QCOMPARE(zoomed_out.pixelColor(x, row), QColor("#0000B2"));
		}
		SharedData::setRegion(Chromosome("chr23"), 1, 80);
		panel.resize(SharedData::settings().label_width + 4 + 480, 100);
		Transcript transcript;
		transcript.setName("ENSTstartstop");
		transcript.setGene("STARTSTOP");
		transcript.setSource(Transcript::ENSEMBL);
		transcript.setStrand(Transcript::PLUS);
		transcript.setGencodePrimaryTranscript(true);
		BedFile exon;
		exon.append(BedLine(Chromosome("chr23"), 1, 9));
		transcript.setRegions(exon, 1, 9);
		TranscriptList transcripts;
		transcripts.append(transcript);
		SharedData::setTranscripts(transcripts);
		QImage colors(panel.size(), QImage::Format_ARGB32);
		panel.render(&colors);
		QCOMPARE(colors.pixelColor(x0 + 1, y + 1), QColor(0, 255, 0));
		QCOMPARE(colors.pixelColor(x0 + 19, y + 1).rgba(), QColor("#0C0C78").rgba());
		QCOMPARE(colors.pixelColor(x0 + 37, y + 1), QColor(255, 0, 0));
	}

	void genePanelTranscriptPopups()
	{
		const TranscriptList original_transcripts = SharedData::transcripts();
		const BedLine original_region = SharedData::region();
		const auto restore = qScopeGuard([&]()
		{
			SharedData::setTranscripts(original_transcripts);
			SharedData::setRegion(original_region.chr(), original_region.start(), original_region.end());
		});
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		GenePanel panel(nullptr);
		QFont font = panel.font();
		font.setPixelSize(10);
		panel.setFont(font);
		panel.resize(SharedData::settings().label_width + 4 + 160, 100); //AAs do not fit, but click information must work.
		panel.show();
		const int x0 = SharedData::settings().label_width + 2;
		const int y = 12 + QFontMetrics(font).height();
		auto click_text = [&](int genome_pos)
		{
			QTest::mouseClick(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + (genome_pos - 1) * 2 + 1, y));
			auto* popup = panel.findChild<QDialog*>();
			if (!popup) return QString();
			const QString text = popup->findChild<QLabel*>()->text();
			delete popup;
			return text;
		};
		BedFile exons;
		exons.append(BedLine(Chromosome("chr22"), 10, 15));
		exons.append(BedLine(Chromosome("chr22"), 30, 36));
		for (bool plus : {true, false})
		{
			Transcript trans;
			trans.setName("ENSTpopup");
			trans.setGene("GENE");
			trans.setSource(Transcript::ENSEMBL);
			trans.setGencodePrimaryTranscript(true);
			trans.setStrand(plus ? Transcript::PLUS : Transcript::MINUS);
			trans.setRegions(exons, plus ? 11 : 36, plus ? 36 : 11);
			TranscriptList transcripts;
			transcripts.append(trans);
			SharedData::setTranscripts(transcripts);
			const QString coding = click_text(30); //Codon is split across the two exons.
			QVERIFY(coding.contains("Gene:</b> GENE"));
			QVERIFY(coding.contains("Transcript:</b> ENSTpopup"));
			QVERIFY(coding.contains("<b>Tags:</b> GENCODE primary<br><br><b>Genomic position:</b> chr22:30<br><b>cDNA position:</b>"));
			QVERIFY(coding.contains(plus ? "cDNA position:</b> c.6" : "cDNA position:</b> c.7"));
			QVERIFY(coding.contains(plus ? "AA position:</b> 2" : "AA position:</b> 3"));
			QVERIFY(coding.contains(plus ? "AA (one-letter):</b> K" : "AA (one-letter):</b> F"));
			QVERIFY(coding.contains(plus ? "AA (three-letter):</b> Lys" : "AA (three-letter):</b> Phe"));
			const QString utr = click_text(10);
			QVERIFY(utr.contains(plus ? "cDNA position:</b> c.-1" : "cDNA position:</b> c.*1"));
			QVERIFY(!utr.contains("AA position:"));
			const QString intron = click_text(20);
			QVERIFY(intron.contains("Genomic position:</b> chr22:20"));
			QVERIFY(!intron.contains("cDNA position:"));
			QVERIFY(!intron.contains("AA position:"));
			QTest::mousePress(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 59, y));
			QTest::mouseRelease(&panel, Qt::LeftButton, Qt::NoModifier, QPoint(x0 + 109, y));
			QVERIFY(!panel.findChild<QDialog*>());
			trans.setRegions(exons);
			transcripts.clear();
			transcripts.append(trans);
			SharedData::setTranscripts(transcripts);
			const QString noncoding = click_text(14);
			QVERIFY(noncoding.contains(plus ? "cDNA position:</b> n.5" : "cDNA position:</b> n.9"));
			QVERIFY(!noncoding.contains("AA position:"));
		}
	}

	void genePanelScrollsToAllTranscripts()
	{
		const TranscriptList original_transcripts = SharedData::transcripts();
		const BedLine original_region = SharedData::region();
		const auto restore = qScopeGuard([&]()
		{
			SharedData::setTranscripts(original_transcripts);
			SharedData::setRegion(original_region.chr(), original_region.start(), original_region.end());
		});
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		QScrollArea scroll;
		scroll.setWidgetResizable(true);
		scroll.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		auto* contents = new QWidget();
		auto* layout = new QHBoxLayout(contents);
		layout->setContentsMargins(0, 0, 0, 0);
		auto* panel = new GenePanel(contents);
		layout->addWidget(panel);
		scroll.setWidget(contents);
		scroll.resize(600, 90);
		scroll.show();
		TranscriptList transcripts;
		for (int i = 0; i < 8; ++i)
		{
			Transcript trans;
			trans.setName("ENST" + QByteArray::number(i));
			trans.setGene("GENE" + QByteArray::number(i));
			trans.setSource(Transcript::ENSEMBL);
			trans.setStrand(Transcript::PLUS);
			trans.setGencodePrimaryTranscript(true);
			BedFile exons;
			exons.append(BedLine(Chromosome("chr22"), 10, 20));
			trans.setRegions(exons);
			transcripts.append(trans);
		}
		SharedData::setTranscripts(transcripts);
		QTRY_VERIFY(scroll.verticalScrollBar()->maximum() > 0);
		QVERIFY(panel->minimumHeight() > scroll.viewport()->height());
		scroll.verticalScrollBar()->setValue(scroll.verticalScrollBar()->maximum());
		QCOMPARE(scroll.verticalScrollBar()->value(), scroll.verticalScrollBar()->maximum());
		const int initial_height = panel->minimumHeight();
		QFont larger = panel->font();
		larger.setPointSize(18);
		panel->setFont(larger);
		QTRY_VERIFY(panel->minimumHeight() > initial_height);
		SharedData::setRegion(Chromosome("chr22"), 50, 100);
		QTRY_COMPARE(scroll.verticalScrollBar()->maximum(), 0);
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		QTRY_VERIFY(scroll.verticalScrollBar()->maximum() > 0);
		SharedData::setTranscripts(TranscriptList());
		QTRY_COMPARE(scroll.verticalScrollBar()->maximum(), 0);
	}

	void globalSettingsSessionXml()
	{
		const QList<Parameter> original = SharedData::parameters().parameters();
		const auto restore = qScopeGuard([&]() { restoreGlobalSettings(original); });
		ParameterList& parameters = SharedData::parameters();
		parameters.setInt("label width", 225);
		parameters.setInt("min window size", 61);
		parameters.setInt("transcript padding", 1234);
		parameters.setInt("BAM max visibility region", 12000);
		parameters.setDouble("BAM insert size lower percentile", 0.123456789012345);
		parameters.setDouble("BAM insert size upper percentile", 0.8);
		QByteArray xml;
		QXmlStreamWriter writer(&xml);
		writer.writeStartElement("GSviewerSession");
		writer.writeAttribute("version", "1");
		writer.writeStartElement("General");
		writer.writeAttribute("gene_panel_height", "100");
		writer.writeStartElement("DisplayedRegion");
		writer.writeAttribute("chr", "chr22");
		writer.writeAttribute("start", "1");
		writer.writeAttribute("end", "80");
		writer.writeEndElement();
		SharedData::writeSettingsToXml(writer);
		writer.writeEndElement();
		writer.writeStartElement("TrackGroup");
		writer.writeAttribute("height", "100");
		writer.writeEndElement();
		writer.writeEndElement();
		writeFile("global-settings.xml", xml);
		QCOMPARE(XmlHelper::isValidXml(data_.filePath("global-settings.xml"), ":Resources/GSviewerSession.xsd"), QString());
		QDomDocument doc;
		QVERIFY(doc.setContent(xml));
		const auto general = doc.documentElement().firstChildElement("General");
		QCOMPARE(general.elementsByTagName("Settings").count(), 6);
		parameters.setInt("label width", 165);
		parameters.setInt("min window size", 41);
		parameters.setInt("transcript padding", 2000);
		parameters.setInt("BAM max visibility region", 50000);
		parameters.setDouble("BAM insert size upper percentile", 0.95);
		parameters.setDouble("BAM insert size lower percentile", 0.9);
		QSignalSpy changes(SharedData::instance(), &SharedData::settingsChanged);
		SharedData::loadSettingsFromXml(general);
		QCOMPARE(changes.count(), 1);
		const QList<Parameter> applied = parameters.parameters();
		QCOMPARE(SharedData::settings().label_width, 225);
		QCOMPARE(SharedData::settings().min_window_size, 61);
		QCOMPARE(SharedData::settings().transcript_padding, 1234);
		QCOMPARE(SharedData::settings().bam_max_visibility_region, 12000);
		QCOMPARE(SharedData::settings().bam_insert_size_lower_percentile, 0.123456789012345);
		QCOMPARE(SharedData::settings().bam_insert_size_upper_percentile, 0.8);
		for (int invalid_case = 0; invalid_case < 9; ++invalid_case)
		{
			auto invalid = doc.cloneNode(true).toDocument();
			auto settings = invalid.documentElement().firstChildElement("General");
			auto first = settings.firstChildElement("Settings");
			if (invalid_case == 0) settings.removeChild(first);
			if (invalid_case == 1) settings.appendChild(first.cloneNode(true));
			if (invalid_case == 2) first.setAttribute("name", "unknown");
			if (invalid_case == 3) first.setAttribute("type", "DOUBLE");
			if (invalid_case == 4) first.firstChildElement("Value").firstChild().setNodeValue("invalid");
			if (invalid_case == 5) first.firstChildElement("Value").firstChild().setNodeValue("-1");
			if (invalid_case == 6) first.removeChild(first.firstChildElement("Value"));
			if (invalid_case == 7) first.appendChild(first.firstChildElement("Value").cloneNode(true));
			if (invalid_case == 8)
			{
				for (auto item = first; !item.isNull(); item = item.nextSiblingElement("Settings"))
				{
					if (item.attribute("name") == "BAM insert size lower percentile") item.firstChildElement("Value").firstChild().setNodeValue("0.9");
				}
			}
			QVERIFY_THROWS_EXCEPTION(ArgumentException, SharedData::loadSettingsFromXml(settings));
			QCOMPARE(changes.count(), 1);
			for (const Parameter& parameter : applied) QCOMPARE(parameters.parameter(parameter.name()).value(), parameter.value());
			QCOMPARE(SharedData::settings().label_width, 225);
			QCOMPARE(SharedData::settings().bam_insert_size_lower_percentile, 0.123456789012345);
			QCOMPARE(SharedData::settings().bam_insert_size_upper_percentile, 0.8);
		}
		auto legacy = doc.cloneNode(true).toDocument();
		auto legacy_general = legacy.documentElement().firstChildElement("General");
		while (!legacy_general.firstChildElement("Settings").isNull()) legacy_general.removeChild(legacy_general.firstChildElement("Settings"));
		writeFile("missing-global-settings.xml", legacy.toByteArray());
		QVERIFY(!XmlHelper::isValidXml(data_.filePath("missing-global-settings.xml"), ":Resources/GSviewerSession.xsd").isEmpty());
		QVERIFY_THROWS_EXCEPTION(ArgumentException, SharedData::loadSettingsFromXml(legacy_general));
	}

	void sessionRestoresAndScalesTrackHeights()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		writeFile("heights.sam", "@HD\tVN:1.6\tSO:coordinate\n@SQ\tSN:chr22\tLN:100\nread\t0\tchr22\t10\t60\t10M\t*\t0\t0\tAAAAAAAAAA\tIIIIIIIIII\n");
		{
			BamReader reader(data_.filePath("heights.sam"));
			BamWriter writer(data_.filePath("heights.bam"));
			writer.writeHeader(reader);
			BamAlignment al;
			while (reader.getNextAlignment(al)) writer.writeAlignment(al);
		}
		QCOMPARE(sam_index_build(data_.filePath("heights.bam").toUtf8().constData(), 0), 0);
		GenomeVisualizationWidget source(nullptr);
		source.resize(1100, 800);
		source.show();
		source.loadFile(data_.filePath("heights.bam"));
		source.loadFile(data_.filePath("first.bed"));
		auto* source_manager = source.findChild<TrackGroupManager*>();
		source.findChild<BamAlignmentTrack*>()->setFixedHeight(160);
		source.findChild<BamCoverageTrack*>()->setFixedHeight(60);
		source.findChild<BedTrack*>()->setFixedHeight(70);
		QTest::qWait(60); //Finish delayed initialization before arranging the splitter.
		source_manager->setSizes({100, 320, 110});
		QCoreApplication::processEvents();
		const QList<int> saved_sizes = source_manager->sizes();
		QByteArray xml;
		QXmlStreamWriter writer(&xml);
		writer.writeStartElement("GSviewerSession");
		writer.writeAttribute("version", "1");
		writer.writeStartElement("General");
		writer.writeAttribute("gene_panel_height", QString::number(saved_sizes.last()));
		writer.writeStartElement("DisplayedRegion");
		writer.writeAttribute("chr", "chr22");
		writer.writeAttribute("start", "1");
		writer.writeAttribute("end", "80");
		writer.writeEndElement();
		SharedData::writeSettingsToXml(writer);
		writer.writeEndElement();
		source_manager->writeToXml(writer);
		writer.writeEndElement();
		writeFile("heights.xml", xml);
		QCOMPARE(XmlHelper::isValidXml(data_.filePath("heights.xml"), ":Resources/GSviewerSession.xsd"), QString());
		QDomDocument doc;
		QVERIFY(doc.setContent(xml));
		GenomeVisualizationWidget restored(nullptr);
		restored.resize(source.size());
		restored.show();
		auto* manager = restored.findChild<TrackGroupManager*>();
		manager->removeAll();
		QStringList errors;
		manager->loadFromXml(doc.documentElement(), errors);
		QVERIFY2(errors.isEmpty(), qPrintable(errors.join("\n")));
		QTest::qWait(60);
		QTRY_COMPARE(manager->sizes(), saved_sizes);
		auto* alignment = restored.findChild<BamAlignmentTrack*>();
		auto* coverage = restored.findChild<BamCoverageTrack*>();
		auto* bed = restored.findChild<BedTrack*>();
		QVERIFY(alignment && coverage && bed);
		QCOMPARE(alignment->height(), 160);
		QCOMPARE(coverage->height(), 60);
		QCOMPARE(bed->height(), 70);
		for (int difference : {100, -80, 0})
		{
			restored.resize(source.width(), source.height() + difference);
			QTRY_COMPARE(alignment->height(), 160 + difference);
			QCOMPARE(coverage->height(), 60);
			QCOMPARE(bed->height(), 70);
			QCOMPARE(manager->sizes().last(), saved_sizes.last());
		}
		//Sessions without required height attributes are rejected.
		QDomElement general = doc.documentElement().firstChildElement("General");
		general.removeAttribute("gene_panel_height");
		const QDomNodeList groups = doc.elementsByTagName("TrackGroup");
		for (int i = 0; i < groups.count(); ++i) groups.at(i).toElement().removeAttribute("height");
		const QDomNodeList tracks = doc.elementsByTagName("Track");
		for (int i = 0; i < tracks.count(); ++i) tracks.at(i).toElement().removeAttribute("height");
		writeFile("legacy-heights.xml", doc.toByteArray());
		QVERIFY(!XmlHelper::isValidXml(data_.filePath("legacy-heights.xml"), ":Resources/GSviewerSession.xsd").isEmpty());
	}

	void startupEmbeddedInMainWindow()
	{
		QMainWindow window;
		auto* central = new QWidget(&window);
		auto* layout = new QVBoxLayout(central);
		auto* viewer = new GenomeVisualizationWidget(central);
		layout->addWidget(viewer);
		window.setCentralWidget(central);
		auto* dock_host = viewer->findChild<QMainWindow*>("dock_window");
		QVERIFY(dock_host);
		QVERIFY(!dock_host->isWindow());
		window.show();
		QCoreApplication::processEvents();
		QVERIFY(viewer->isVisible());
		QVERIFY(dock_host->isVisibleTo(viewer));
		QVERIFY(viewer->findChild<QLineEdit*>("search")->isVisibleTo(viewer));
		QVERIFY(viewer->findChild<TrackGroupManager*>()->isVisibleTo(viewer));
		QVERIFY(dock_host->width() > 0);
		QVERIFY(dock_host->height() > 0);
		viewer->findChild<QToolButton*>("settings_btn")->click();
		auto* placeholder = viewer->findChild<QLabel*>("settings_placeholder");
		QVERIFY(placeholder->isVisible());
		QCOMPARE(placeholder->text(), QString("Select a track to show settings"));
		viewer->loadFile(data_.filePath("first.bed"));
		openEditor(viewer->findChild<TrackWidget*>());
		QCoreApplication::processEvents();
		auto* dock = viewer->findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock->isVisibleTo(viewer));
		dock->close();
		QCoreApplication::processEvents();
		QVERIFY(dock->isHidden());
		QVERIFY(viewer->findChild<TrackGroupManager*>()->isVisibleTo(viewer));
	}

	void mouseCoordinatesAcrossTracksAndEmptyGroups()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		GenomeVisualizationWidget viewer(nullptr);
		viewer.resize(1000, 700);
		viewer.show();
		viewer.loadFile(data_.filePath("first.bed"));
		auto* manager = viewer.findChild<TrackGroupManager*>();
		auto* label = viewer.findChild<QLabel*>("label_coordinate");
		QVERIFY(manager && label);
		manager->addEmptyTrackGroup(); // Added after the viewer installed its event filter.
		QCoreApplication::processEvents();
		auto* bed_track = viewer.findChild<BedTrack*>();
		auto* scale_panel = viewer.findChild<ChromosomeContextPanel*>();
		QVERIFY(bed_track && scale_panel);
		QCOMPARE(scale_panel->width(), bed_track->width());
		QCOMPARE(scale_panel->mapTo(&viewer, QPoint()).x(), bed_track->mapTo(&viewer, QPoint()).x());
		QCOMPARE(viewer.findChild<ChromosomePanel*>()->width(), bed_track->width());

		auto check_coordinate = [&](QWidget* widget)
		{
			QVERIFY(widget);
			QVERIFY(widget->hasMouseTracking());
			const int x0 = SharedData::settings().label_width + 2;
			const int x = x0 + (widget->width() - x0 - 2) / 2;
			const QPoint pos(x, widget->height() / 2);
			QMouseEvent move(QEvent::MouseMove, pos, widget->mapToGlobal(pos), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
			QApplication::sendEvent(widget, &move);
			const int coordinate = SharedData::region().start() + int(double(x - x0) * SharedData::region().length() / (widget->width() - x0 - 2));
			QCOMPARE(label->text(), QString("chr22:%1").arg(coordinate));

			const QPoint header(2, pos.y());
			QMouseEvent header_move(QEvent::MouseMove, header, widget->mapToGlobal(header), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
			QApplication::sendEvent(widget, &header_move);
			QVERIFY(label->text().isEmpty());
		};
		check_coordinate(viewer.findChild<BedTrack*>());
		check_coordinate(viewer.findChild<GenePanel*>());
		check_coordinate(viewer.findChild<ChromosomeContextPanel*>());
		bool checked_empty = false;
		for (TrackGroup* group : manager->findChildren<TrackGroup*>())
		{
			if (group->trackCount() != 0) continue;
			check_coordinate(group->viewport());
			checked_empty = true;
		}
		QVERIFY(checked_empty);
		auto* chromosome = viewer.findChild<ChromosomePanel*>();
		QVERIFY(chromosome && chromosome->hasMouseTracking());
		const int x0 = SharedData::settings().label_width + 2;
		QCOMPARE(chromosome->genomicCoordinateAt(x0), 1);
		QCOMPARE(chromosome->genomicCoordinateAt(x0 - 1), 0);
		QCOMPARE(chromosome->genomicCoordinateAt(chromosome->width() - 2), 0);
		const QPoint chromosome_pos(x0 + (chromosome->width() - x0 - 2) / 2, chromosome->height() / 2);
		QMouseEvent chromosome_move(QEvent::MouseMove, chromosome_pos, chromosome->mapToGlobal(chromosome_pos), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
		QApplication::sendEvent(chromosome, &chromosome_move);
		const QString chromosome_coordinate = label->text();
		QVERIFY(chromosome_coordinate.startsWith("chr22:"));
		QVERIFY(chromosome_coordinate.mid(6).toInt() > SharedData::region().end());
		SharedData::setRegion(Chromosome("chr22"), 20, 60);
		QApplication::sendEvent(chromosome, &chromosome_move);
		QCOMPARE(label->text(), chromosome_coordinate); //Overview coordinates do not depend on zoom.
		QEvent leave(QEvent::Leave);
		QApplication::sendEvent(chromosome, &leave);
		QVERIFY(label->text().isEmpty());
		auto* scale = viewer.findChild<ChromosomeContextPanel*>();
		check_coordinate(scale); //Scale coordinates follow the zoomed region.
		QApplication::sendEvent(scale, &leave);
		QVERIFY(label->text().isEmpty());
		QApplication::sendEvent(manager, &leave);
		QVERIFY(label->text().isEmpty());
	}

	void trackSelectionAndCollapse()
	{
		GenomeVisualizationWidget viewer(nullptr);
		viewer.show();
		QCoreApplication::processEvents();
		auto* dock = viewer.findChild<QDockWidget*>("settings_dock");
		QVERIFY(dock);
		QVERIFY(dock->isHidden());
		viewer.loadFile(data_.filePath("first.bed"));
		viewer.loadFile(data_.filePath("second.bed"));
		auto tracks = viewer.findChildren<TrackWidget*>();
		QCOMPARE(tracks.size(), 2);
		auto* first = tracks[0];
		auto* second = tracks[1];
		second->parameters().setString("color", "red");
		QMenu menu;
		first->populateContextMenu(menu, QPoint(2, 2));
		QCOMPARE(menu.actions().first()->text(), QString("Track"));
		QMenu* track_menu = menu.actions().first()->menu();
		QVERIFY(track_menu);
		QCOMPARE(track_menu->actions().first()->text(), QString("Open settings"));
		QVERIFY(track_menu->actions()[1]->isSeparator());
		track_menu->actions().first()->trigger();
		QVERIFY(!dock->isHidden());
		QCOMPARE(dock->windowTitle(), QString("Track settings"));
		auto* editor = dock->findChild<ParameterEditor*>();
		QVERIFY(editor);
		QVERIFY(editor->findChild<QLabel*>("name"));
		QCOMPARE(editor->findChild<QLabel*>("name")->text(), first->displayName());
		editor->findChild<QComboBox*>("color")->setCurrentText("green");
		QVERIFY(editor->store());
		QCOMPARE(first->parameters().getString("color"), QByteArray("green"));
		QCOMPARE(second->parameters().getString("color"), QByteArray("red"));
		editor->findChild<QCheckBox*>("auto_apply")->setChecked(true);
		QTest::mouseClick(second, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
		QCOMPARE(dock->findChild<ParameterEditor*>(), editor);
		QCOMPARE(editor->findChild<QLabel*>("name")->text(), second->displayName());
		QCOMPARE(editor->findChild<QComboBox*>("color")->currentText(), QString("red"));
		QVERIFY(editor->findChild<QCheckBox*>("auto_apply")->isChecked());
		editor->findChild<QComboBox*>("color")->setCurrentText("yellow");
		QCOMPARE(second->parameters().getString("color"), QByteArray("yellow"));
		QCOMPARE(first->parameters().getString("color"), QByteArray("green"));
		dock->close();
		QVERIFY(dock->isHidden());
		track_menu->actions().first()->trigger();
		QVERIFY(!dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->click();
		QVERIFY(dock->isHidden());
		viewer.findChild<QToolButton*>("settings_btn")->click();
		QVERIFY(!dock->isHidden());
	}

	void removedTrackAndClearedSession()
	{
		GenomeVisualizationWidget viewer(nullptr);
		viewer.loadFile(data_.filePath("first.bed"));
		auto* track = viewer.findChild<TrackWidget*>();
		openEditor(track);
		emit track->trackDeleted();
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
		auto* dock = viewer.findChild<QDockWidget*>("settings_dock");
		QVERIFY(!dock->findChild<ParameterEditor*>());
		QVERIFY(!dock->isHidden());
		QVERIFY(!viewer.findChild<QLabel*>("settings_placeholder")->isHidden());
		viewer.loadFile(data_.filePath("second.bed"));
		openEditor(viewer.findChild<TrackWidget*>());
		viewer.clearSession();
		QVERIFY(!dock->findChild<ParameterEditor*>());
		QCOMPARE(dock->windowTitle(), QString("Track settings"));
	}

	void svTrackBreakpointsFilteringAndSession()
	{
		const QByteArray header = "##fileformat=BEDPE\n#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tID\tQUAL\tTYPE\tFILTER\tFORMAT\tS1\n";
		writeFile("variants.BEDPE", header
			+ "chr22\t80\t80\tchr22\t80\t80\tinsert\t.\tINS\tPASS\tGT\t0/1\n"
			+ "chr22\t20\t20\tchr22\t60\t60\tdeletion\t50\tDEL\tPASS\tGT\t1/1\n"
			+ "chr22\t40\t40\tchr23\t50\t50\ttranslocation\t80\tBND\tq10\tGT\t0/1\n"
			+ "chr22\t95\t95\tchr22\t150\t150\tduplication\t20\tDUP\t.\tGT\t0/1\n");
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		auto tracks = FileLoader::loadTracks(data_.filePath("variants.BEDPE"));
		QCOMPARE(tracks.size(), 1);
		std::unique_ptr<TrackWidget> owner(tracks.first());
		auto* track = qobject_cast<SvTrack*>(owner.get());
		QVERIFY(track);
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track->parameters().setDouble("min QUAL", -1));
		auto render = [](SvTrack* target)
		{
			target->resize(800, target->sizeHint().height());
			QImage image(target->size(), QImage::Format_ARGB32);
			target->render(&image);
			return image;
		};
		auto x_for = [](int position)
		{
			return SharedData::settings().label_width + 2 + int((position - SharedData::region().start()) * double(800 - SharedData::settings().label_width - 4) / SharedData::region().length());
		};
		auto has_tick = [&](const QImage& image, int position)
		{
			for (int dx = -1; dx <= 1; ++dx)
				if (image.pixelColor(x_for(position) + dx, 5) == QColor(Qt::gray)) return true;
			return false;
		};
		auto label_point = [&](SvTrack* target, int position, int direction = 1, int level = 0)
		{
			const int label_height = int(std::ceil(QFontMetricsF(target->font()).height()));
			const int arrow_y = 11 + level * (label_height + 10);
			return QPoint(x_for(position) + direction * 12, arrow_y + 3 + label_height / 2);
		};
		auto popup_at = [](SvTrack* target, QPoint point)
		{
			QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, point);
			auto* popup = target->findChild<QDialog*>();
			if (!popup) return QString();
			const QString text = popup->findChild<QLabel*>()->text();
			delete popup;
			return text;
		};
		QImage image = render(track);
		for (int position : {20, 40, 60, 80, 95}) QVERIFY(has_tick(image, position));
		track->parameters().setBool("PASS only", true);
		image = render(track);
		const int arrow_y = 11;
		QVERIFY(image.height() >= 70);
		QCOMPARE(image.pixelColor(790, 69), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x_for(20) + 20, arrow_y), QColor(Qt::gray));
		QCOMPARE(image.pixelColor(x_for(60) - 20, arrow_y), QColor(Qt::gray));
		QCOMPARE(image.pixelColor(x_for(30), arrow_y), QColor(Qt::gray)); //Visible mates are connected.
		QCOMPARE(image.pixelColor(x_for(60) - 3, arrow_y - 2), QColor(Qt::white)); //No arrowhead at the visible mate.
		QCOMPARE(image.pixelColor(x_for(80) + 20, arrow_y), QColor(Qt::white)); //Coincident insertion endpoints need no arrow.
		QVERIFY(popup_at(track, label_point(track, 20)).contains("ID: deletion"));
		QVERIFY(popup_at(track, label_point(track, 60, -1)).contains("ID: deletion"));
		QVERIFY(popup_at(track, QPoint(x_for(20), image.height() - 4)).isEmpty()); //Clicks are on labels, not stems.
		QVERIFY(popup_at(track, QPoint(x_for(5), image.height() - 4)).isEmpty());
		const QPoint label = label_point(track, 20);
		QTest::mouseClick(track, Qt::RightButton, Qt::NoModifier, label);
		QVERIFY(!track->findChild<QDialog*>());
		QTest::mousePress(track, Qt::LeftButton, Qt::NoModifier, label);
		QTest::mouseRelease(track, Qt::LeftButton, Qt::NoModifier, label + QPoint(12, 0));
		QVERIFY(!track->findChild<QDialog*>());
		//The reverse mate is visible even when no first breakpoint is on this chromosome.
		track->parameters().setBool("PASS only", false);
		SharedData::setRegion(Chromosome("chr23"), 1, 100);
		image = render(track);
		QVERIFY(has_tick(image, 50));
		const int bnd_y = 11;
		QCOMPARE(image.pixelColor(x_for(50) + 20, bnd_y), QColor(Qt::gray)); //Other chromosomes also point right.
		QCOMPARE(image.pixelColor(x_for(50), bnd_y - 1), QColor(Qt::gray)); //Tick extends above the arrow.
		QCOMPARE(image.pixelColor(x_for(50), bnd_y - 9), QColor(Qt::white)); //Default tick height is eight pixels.
		const QPoint bnd_label = label_point(track, 50);
		QHelpEvent help(QEvent::ToolTip, bnd_label, track->mapToGlobal(bnd_label));
		QApplication::sendEvent(track, &help);
		QVERIFY(QToolTip::text().contains("Breakpoint 1: chr22:40-40"));
		QVERIFY(QToolTip::text().contains("Breakpoint 2: chr23:50-50"));
		QVERIFY(QToolTip::text().contains("QUAL: 80"));
		QVERIFY(QToolTip::text().contains("FILTER: q10"));
		QVERIFY(QToolTip::text().contains("S1 GT: 0/1"));
		QToolTip::hideText();
		QVERIFY(popup_at(track, bnd_label).contains("ID: translocation"));
		const int bnd_arrow_length = int(std::max(32.0, QFontMetricsF(track->font()).horizontalAdvance("BND") + 8));
		QVERIFY(popup_at(track, QPoint(x_for(50) + bnd_arrow_length + 10, bnd_y)).contains("ID: translocation")); //Chromosome label is clickable too.
		track->parameters().setBool("PASS only", true);
		QVERIFY(!has_tick(render(track), 50));
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		image = render(track);
		QVERIFY(has_tick(image, 80));
		QVERIFY(!has_tick(image, 95));
		track->parameters().setDouble("min QUAL", 30);
		image = render(track);
		QVERIFY(has_tick(image, 20));
		QVERIFY(!has_tick(image, 80));
		SharedData::setRegion(Chromosome("chr22"), 45, 100);
		image = render(track);
		QCOMPARE(image.pixelColor(x_for(60) - 20, arrow_y), QColor(Qt::gray));
		SharedData::setRegion(Chromosome("chr22"), 1, 41);
		image = render(track);
		QCOMPARE(image.pixelColor(x_for(20) + 20, arrow_y), QColor(Qt::gray));
		QString xml;
		QXmlStreamWriter writer(&xml);
		track->writeToXml(writer);
		QDomDocument document;
		QVERIFY(document.setContent(xml));
		QStringList errors;
		std::unique_ptr<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
		QVERIFY(qobject_cast<SvTrack*>(restored.get()));
		QVERIFY(errors.isEmpty());
		QCOMPARE(restored->parameters().getDouble("min QUAL"), 30.0);
		QVERIFY(restored->parameters().getBool("PASS only"));
		track->reloadTrack();
		QCOMPARE(track->parameters().getDouble("min QUAL"), 30.0);
		QVERIFY(track->parameters().getBool("PASS only"));
		QFile plain(data_.filePath("variants.BEDPE"));
		QVERIFY(plain.open(QIODevice::ReadOnly));
		const QByteArray contents = plain.readAll();
		const QString compressed_path = data_.filePath("variants.bedpe.gz");
		BGZF* compressed = bgzf_open(compressed_path.toUtf8().constData(), "w");
		QVERIFY(compressed);
		QCOMPARE(bgzf_write(compressed, contents.constData(), contents.size()), contents.size());
		QCOMPARE(bgzf_close(compressed), 0);
		auto compressed_tracks = FileLoader::loadTracks(compressed_path);
		QCOMPARE(compressed_tracks.size(), 1);
		std::unique_ptr<TrackWidget> compressed_owner(compressed_tracks.first());
		QVERIFY(qobject_cast<SvTrack*>(compressed_owner.get()));
		writeFile("minimal.bedpe", "##fileformat=BEDPE\n#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\tTYPE\n"
			"chr22\t10\t30\tchr22\t90\t90\tINV\n"
			"chr22\t40\t40\t.\t.\t.\tBND\n");
		SvTrack minimal(nullptr, data_.filePath("minimal.bedpe"), "Minimal");
		SharedData::setRegion(Chromosome("chr22"), 25, 80);
		image = render(&minimal);
		QVERIFY(has_tick(image, 25));
		const QPoint minimal_label = label_point(&minimal, 25);
		QHelpEvent minimal_help(QEvent::ToolTip, minimal_label, minimal.mapToGlobal(minimal_label));
		QApplication::sendEvent(&minimal, &minimal_help);
		QVERIFY(QToolTip::text().contains("QUAL: n/a"));
		QToolTip::hideText();
		minimal.parameters().setBool("PASS only", true);
		QVERIFY(!has_tick(render(&minimal), 25));
		//Three variants at one position share a baseline, with progressively longer ticks.
		writeFile("stacked.bedpe", header
			+ "chr22\t20\t20\tchr22\t80\t80\tlow\t50\tDEL\tPASS\tGT\t0/1\n"
			+ "chr22\t20\t20\tchr22\t90\t90\tmiddle\t50\tDUP\tPASS\tGT\t0/1\n"
			+ "chr22\t20\t20\tchr23\t50\t50\thigh\t50\tBND\tPASS\tGT\t0/1\n");
		SharedData::setRegion(Chromosome("chr22"), 1, 41);
		SvTrack stacked(nullptr, data_.filePath("stacked.bedpe"), "Stacked");
		image = render(&stacked);
		const int level_height = int(std::ceil(QFontMetricsF(stacked.font()).height())) + 10;
		for (int level = 0; level < 3; ++level)
		{
			const int y = 11 + level * level_height;
			QCOMPARE(image.pixelColor(x_for(20) + 20, y), QColor(Qt::gray));
			const QString text = popup_at(&stacked, label_point(&stacked, 20, 1, level));
			QVERIFY(text.contains(QStringList{"ID: low", "ID: middle", "ID: high"}[level]));
		}
		//Longer ticks extend down from the same top anchor.
		QCOMPARE(image.pixelColor(x_for(20), 3), QColor(Qt::gray));
		//Close endpoints of one variant keep one label, rather than stacking duplicates.
		writeFile("close.bedpe", header + "chr22\t20\t20\tchr22\t22\t22\tclose\t50\tDEL\tPASS\tGT\t0/1\n");
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		SvTrack close(nullptr, data_.filePath("close.bedpe"), "Close");
		image = render(&close);
		QVERIFY(has_tick(image, 20));
		QVERIFY(has_tick(image, 22));
		QCOMPARE(close.sizeHint().height(), std::max(70, int(std::ceil(QFontMetricsF(close.font()).height())) + 18));
		QVERIFY(popup_at(&close, label_point(&close, 22, -1)).contains("ID: close"));
		const int label_width = int(QFontMetricsF(close.font()).horizontalAdvance("DEL"));
		QPoint hidden_label = label_point(&close, 20);
		hidden_label.setX(x_for(20) + label_width + 2);
		QVERIFY(popup_at(&close, hidden_label).isEmpty());
	}
	void vcfTrackExpandsRowsAndLimitsVisibility()
	{
		QByteArray contents = "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tS1\n";
		for (int i = 0; i < 12; ++i)
		{
			contents += "chrLong\t" + QByteArray::number(5000 + i) + "\t.\tA\tC\t50\tPASS\t.\tGT\t1/1\n";
		}
		writeFile("dense.vcf", contents);
		const QString compressed_path = data_.filePath("dense.vcf.gz");
		BGZF* compressed = bgzf_open(compressed_path.toUtf8().constData(), "w");
		QVERIFY(compressed);
		QCOMPARE(bgzf_write(compressed, contents.constData(), contents.size()), contents.size());
		QCOMPARE(bgzf_close(compressed), 0);
		QCOMPARE(tbx_index_build(compressed_path.toUtf8().constData(), 0, &tbx_conf_vcf), 0);
		for (const QString& path : {data_.filePath("dense.vcf"), compressed_path})
		{
			SharedData::setRegion(Chromosome("chrLong"), 4980, 5020);
			VcfTrack source(nullptr, path, "Dense variants");
			QCOMPARE(source.parameters().getInt("max visibility"), 1000000);
			QVERIFY_THROWS_EXCEPTION(ArgumentException, source.parameters().setInt("max visibility", 0));
			QString xml;
			QXmlStreamWriter writer(&xml);
			source.writeToXml(writer);
			QDomDocument document;
			QVERIFY(document.setContent(xml));
			document.documentElement().setAttribute("height", 36); //An old session with room for only two rows.
			QStringList errors;
			TrackGroup group;
			group.resize(800, 120);
			auto* track = qobject_cast<VcfTrack*>(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
			QVERIFY(track && errors.isEmpty());
			group.addTrackWidgets({track});
			group.show();
			QTRY_COMPARE(track->sizeHint().height(), 70);
			SharedData::setRegion(Chromosome("chrLong"), 1, 10000);
			QTRY_COMPARE(track->sizeHint().height(), 12 * 18);
			QTRY_VERIFY(track->height() >= 12 * 18); //Extra rows expand the scroll area's contents.
			QImage image(track->size(), QImage::Format_ARGB32);
			track->render(&image);
			const int x = SharedData::settings().label_width + 2 + int((5011 - 1) * double(track->width() - SharedData::settings().label_width - 4) / 10000);
			QCOMPARE(image.pixelColor(x, 11 * 18 + 5), QColor(Qt::blue)); //The twelfth variant is drawn.
			track->parameters().setInt("max visibility", 9999);
			QTRY_COMPARE(track->sizeHint().height(), 70);
			QTest::mouseClick(track, Qt::LeftButton, Qt::NoModifier, QPoint(x, 5));
			QVERIFY(!track->findChild<QDialog*>()); //No hidden variants remain clickable.
			track->parameters().setInt("max visibility", 10000);
			QTRY_COMPARE(track->sizeHint().height(), 12 * 18); //Inclusive limit, with indexed data reloaded immediately.
			QTRY_VERIFY(track->height() >= 12 * 18);
			SharedData::setRegion(Chromosome("chrLong"), 4980, 5020);
			QTRY_COMPARE(track->sizeHint().height(), 70);
			QImage minimum_image(track->size(), QImage::Format_ARGB32);
			track->render(&minimum_image);
			QCOMPARE(minimum_image.pixelColor(track->width() - 10, 69), QColor(Qt::white));
			QString limited_xml;
			QXmlStreamWriter limited_writer(&limited_xml);
			track->writeToXml(limited_writer);
			QVERIFY(document.setContent(limited_xml));
			std::unique_ptr<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
			QVERIFY(restored && errors.isEmpty());
			QCOMPARE(restored->parameters().getInt("max visibility"), 10000);
		}
	}

	void vcfTrackRenderingFilteringAndTabix()
	{
		int errors_shown = 0;
		QTimer dismiss_errors;
		connect(&dismiss_errors, &QTimer::timeout, this, [&]()
		{
			if (auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) { ++errors_shown; dialog->accept(); }
		});
		dismiss_errors.start(10);
		const QByteArray multi_header = "##fileformat=VCFv4.2\n##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Depth\">\n##INFO=<ID=END,Number=1,Type=Integer,Description=\"End\">\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT\tS1\tS2\n";
		const QByteArray multi_chr22 =
			"chr22\t10\thet\tA\tG\t20\tPASS\t.\tDP:GT:AF:AO\t10:0/1:0.3:8\t20:1/1:0.9:10\n"
			"chr22\t11\tadjacent\tA\tG\t100\tPASS\t.\tGT\t1/1\t0/1\n"
			"chr22\t20\thom\tA\tT\t40\tq10\t.\tGT:AO:DP\t1/1:8:10\t0/1:5:10\n"
			"chr22\t30\tref\tA\tC\t60\t.\t.\tGT:AO:DP\t0/0:0:0\t1/1:10:10\n"
			"chr22\t40\tmulti\tA\tT,G\t80\tPASS\t.\tGT:AF:AO:DP\t1/2:0.1,.:20,30:100\t2/2:.:0,10:10\n"
			"chr22\t50\tmissing\tA\tT\t.\tPASS\t.\tGT\t./.\t0/1\n"
			"chr22\t60\tpartial\tA\tG\t0\tPASS\t.\tGT\t0/.\t./.\n"
			"chr22\t70\thaploid\tA\tC\t99\tPASS\t.\tGT\t1\t.\n"
			"chr22\t80\tphased\tA\tT\t30\tPASS\t.\tGT\t1|0\t0|1\n"
			"chr22\t90\tdel\tAAAAA\tA\t50\tPASS\t.\tGT\t0/1\t1/1\n";
		const QByteArray multi_structural = "chrLong\t5000\tsv\tN\t<DEL>\t100\tPASS\tEND=5050\tGT\t0/1\t1/1\n"
			"chrLong\t5120\tdelins-longer\tACG\tATTG\t50\tPASS\t.\tGT\t0/1\t1/1\n"
			"chrLong\t5140\tdelins-shorter\tACGT\tATT\t50\tPASS\t.\tGT\t0/1\t1/1\n";
		const QByteArray multi_content = multi_header + multi_chr22 + multi_structural;
		writeFile("multi.vcf", multi_content);
		QVERIFY_THROWS_EXCEPTION(NotImplementedException, VcfTrack(nullptr, data_.filePath("multi.vcf"), "Multi"));
		auto singleSample = [](const QByteArray& content, int sample)
		{
			QByteArray result;
			for (const QByteArray& line : content.split('\n'))
			{
				if (line.isEmpty()) continue;
				if (line.startsWith("##")) { result += line + "\n"; continue; }
				const auto columns = line.split('\t');
				result += columns.mid(0, 9).join('\t') + "\t" + columns[9 + sample] + "\n";
			}
			return result;
		};
		const QByteArray header = singleSample(multi_header, 0);
		const QByteArray chr22 = singleSample(multi_chr22, 0);
		const QByteArray structural = singleSample(multi_structural, 0);
		const QString plain_path = data_.filePath("variants.VCF");
		writeFile("variants.VCF", header + chr22 + structural);
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		auto plain_tracks = FileLoader::loadTracks(plain_path);
		QCOMPARE(plain_tracks.size(), 1);
		std::unique_ptr<TrackWidget> plain_owner(plain_tracks.first());
		auto* plain = qobject_cast<VcfTrack*>(plain_owner.get());
		QVERIFY(plain);
		QCOMPARE(plain->parameters().getString("sample"), QByteArray("S1"));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, plain->parameters().setDouble("min QUAL", -1));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, plain->parameters().setString("sample", "unknown"));
		auto render = [](VcfTrack* track)
		{
			track->resize(600, track->sizeHint().height());
			QImage image(track->size(), QImage::Format_ARGB32);
			track->render(&image);
			return image;
		};
		auto x_for = [](int position)
		{
			return SharedData::settings().label_width + 2 + int((position - SharedData::region().start() + 0.5) * (600 - SharedData::settings().label_width - 4) / SharedData::region().length());
		};
		auto popup_text = [&](VcfTrack* track, int position)
		{
			QTest::mouseClick(track, Qt::LeftButton, Qt::NoModifier, QPoint(x_for(position), 5));
			auto* popup = track->findChild<QDialog*>();
			if (!popup) return QString();
			const QString text = popup->findChild<QLabel*>()->text();
			delete popup;
			return text;
		};
		QImage image = render(plain);
		QCOMPARE(image.pixelColor(x_for(10), 4), QColor(224, 224, 224));
		QCOMPARE(image.pixelColor(x_for(10), 2), QColor(224, 224, 224)); //No heterozygous SNP outline.
		QCOMPARE(image.pixelColor(x_for(10), 11), QColor(209, 113, 5));
		QCOMPARE(image.pixelColor(x_for(20), 4), QColor(Qt::red));
		QCOMPARE(image.pixelColor(x_for(30), 11), QColor(Qt::white));
		QCOMPARE(image.pixelColor(x_for(40), 4), QColor(224, 224, 224));
		QCOMPARE(image.pixelColor(x_for(40), 11), QColor(209, 113, 5));
		QCOMPARE(image.pixelColor(x_for(70), 4), QColor(Qt::blue));
		QCOMPARE(image.pixelColor(x_for(80), 4), QColor(224, 224, 224));
		QCOMPARE(image.pixelColor(x_for(80), 11), QColor(Qt::red)); //1|0 has the same gray upper half as 0/1.
		for (int x = x_for(20) - 1; x <= x_for(20) + 1; ++x) QCOMPARE(image.pixelColor(x, 2), QColor(Qt::red)); //Homozygous alternate SNP has no outline.
		QCOMPARE(image.pixelColor(x_for(90), 8), QColor(Qt::white)); //Anchor base is not drawn.
		QCOMPARE(image.pixelColor(x_for(92), 4), QColor(224, 224, 224));
		QCOMPARE(image.pixelColor(x_for(92), 8), QColor(Qt::black)); //One centered line for 0/1.
		QCOMPARE(image.pixelColor(x_for(92), 11), QColor(224, 224, 224));
		QVERIFY(popup_text(plain, 90).isEmpty());
		QVERIFY(popup_text(plain, 92).contains("chr22:90 AAAAA&gt;A"));
		SharedData::setRegion(Chromosome("chr22"), 90, 90);
		image = render(plain);
		QCOMPARE(image.pixelColor(x_for(90), 8), QColor(Qt::white)); //View contains only the anchor.
		SharedData::setRegion(Chromosome("chr22"), 92, 93);
		image = render(plain);
		QCOMPARE(image.pixelColor(x_for(92), 8), QColor(Qt::black)); //Deletion clipped to view.
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		QVERIFY(popup_text(plain, 10).contains("GT: 0/1")); //GT is not the first FORMAT field.
		QCOMPARE(popup_text(plain, 10), QString("<qt>chr22:10 A&gt;G<br>QUAL: 20<br>FILTER: PASS<br>GT: 0/1<br>AF: 0.3</qt>")); //FORMAT AF takes priority over AO/DP.
		QVERIFY(popup_text(plain, 20).contains("AF: 0.8")); //AO/DP fallback without AF.
		QVERIFY(popup_text(plain, 30).contains("AF: .")); //Zero depth is unavailable.
		QVERIFY(popup_text(plain, 40).contains("AF: 0.1,0.3")); //One frequency per ALT; fallback for a missing value.
		QVERIFY(popup_text(plain, 50).contains("AF: .")); //No frequency fields.
		QVERIFY(popup_text(plain, 50).contains("GT: ./."));
		QVERIFY(popup_text(plain, 60).contains("GT: 0/."));
		writeFile("second-sample.vcf", singleSample(multi_content, 1));
		VcfTrack second_sample(nullptr, data_.filePath("second-sample.vcf"), "Second sample");
		image = render(&second_sample);
		QCOMPARE(image.pixelColor(x_for(10), 4), QColor(209, 113, 5));
		QCOMPARE(image.pixelColor(x_for(40), 4), QColor(209, 113, 5));
		QCOMPARE(image.pixelColor(x_for(92), 5), QColor(Qt::black)); //Two lines for 1/1.
		QCOMPARE(image.pixelColor(x_for(92), 10), QColor(Qt::black));
		QCOMPARE(image.pixelColor(x_for(92), 8), QColor(224, 224, 224));
		QVERIFY(popup_text(&second_sample, 10).contains("GT: 1/1<br>AF: 0.9"));
		SharedData::setRegion(Chromosome("chrLong"), 5110, 5150);
		image = render(plain);
		QCOMPARE(image.pixelColor(x_for(5120), 4), QColor(Qt::white)); //Shared anchor is excluded.
		for (int position : {5121, 5142})
		{
			for (int y : {4, 11}) //Both halves have equal-width stripes.
			{
				int gray = 0, purple = 0;
				for (int x = x_for(position) - 3; x <= x_for(position) + 4; ++x)
				{
					const QColor color = image.pixelColor(x, y);
					gray += color == QColor(224, 224, 224);
					purple += color == QColor(140, 0, 200);
				}
				QCOMPARE(gray, 4);
				QCOMPARE(purple, 4);
			}
		}
		QCOMPARE(image.pixelColor(x_for(5121), 8), QColor(Qt::black));
		QCOMPARE(image.pixelColor(x_for(5142), 8), QColor(Qt::black));
		image = render(&second_sample);
		QCOMPARE(image.pixelColor(x_for(5121), 5), QColor(Qt::black));
		QCOMPARE(image.pixelColor(x_for(5121), 10), QColor(Qt::black));
		SharedData::setRegion(Chromosome("chr22"), 1, 100);
		plain->parameters().setString("sample", "S1");
		plain->parameters().setDouble("min QUAL", 30);
		render(plain);
		QVERIFY(popup_text(plain, 10).isEmpty());
		QVERIFY(popup_text(plain, 50).isEmpty());
		QVERIFY(popup_text(plain, 80).contains("chr22:80 A&gt;T")); //Inclusive QUAL threshold.
		plain->parameters().setBool("PASS only", true);
		render(plain);
		QVERIFY(popup_text(plain, 20).isEmpty());
		QVERIFY(popup_text(plain, 30).isEmpty());
		plain->parameters().setDouble("min QUAL", 0);
		render(plain);
		QVERIFY(popup_text(plain, 50).contains("chr22:50 A&gt;T"));
		plain->parameters().setBool("PASS only", false);

		for (int min_shift : {0, 14})
		{
			const QString path = data_.filePath(QString("variants-%1.VCF.GZ").arg(min_shift));
			//Invalid QUAL is legal to index, but must not be parsed outside the viewed chromosome.
			const QByteArray content = header + chr22 + "chr23\t10\tunused\tA\tG\tinvalid\tPASS\t.\tGT\t0/1\n" + structural;
			BGZF* file = bgzf_open(path.toUtf8().constData(), "w");
			QVERIFY(file);
			const auto written = bgzf_write(file, content.constData(), content.size());
			const int closed = bgzf_close(file);
			QCOMPARE(written, content.size());
			QCOMPARE(closed, 0);
			QCOMPARE(tbx_index_build(path.toUtf8().constData(), min_shift, &tbx_conf_vcf), 0);
			const QString multi_path = data_.filePath(QString("multi-%1.vcf.gz").arg(min_shift));
			BGZF* multi_file = bgzf_open(multi_path.toUtf8().constData(), "w");
			QVERIFY(multi_file);
			QCOMPARE(bgzf_write(multi_file, multi_content.constData(), multi_content.size()), multi_content.size());
			QCOMPARE(bgzf_close(multi_file), 0);
			QCOMPARE(tbx_index_build(multi_path.toUtf8().constData(), min_shift, &tbx_conf_vcf), 0);
			QVERIFY_THROWS_EXCEPTION(NotImplementedException, VcfTrack(nullptr, multi_path, "Multi"));
			auto tracks = FileLoader::loadTracks(path);
			QCOMPARE(tracks.size(), 1);
			std::unique_ptr<TrackWidget> owner(tracks.first());
			auto* track = qobject_cast<VcfTrack*>(owner.get());
			QVERIFY(track);
			//Use the same name when comparing the complete rendered widgets.
			QString xml;
			QXmlStreamWriter writer(&xml);
			track->writeToXml(writer);
			QDomDocument document;
			QVERIFY(document.setContent(xml));
			document.documentElement().setAttribute("display_name", plain->displayName());
			QStringList errors;
			std::unique_ptr<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
			QVERIFY(restored && errors.isEmpty());
			auto* indexed = qobject_cast<VcfTrack*>(restored.get());
			QVERIFY(indexed);
			//The session restored a fixed height; compare automatic row sizing below.
			indexed->setMinimumHeight(70);
			indexed->setMaximumHeight(QWIDGETSIZE_MAX);
			for (const BedLine& region : {BedLine(Chromosome("chr22"), 1, 100), BedLine(Chromosome("chr22"), 40, 80),
				BedLine(Chromosome("chrLong"), 5010, 5050), BedLine(Chromosome("chrLong"), 5110, 5150), BedLine(Chromosome("chrX"), 1, 41), BedLine(Chromosome("chr22"), 1, 100)})
			{
				SharedData::setRegion(region.chr(), region.start(), region.end());
				QCOMPARE(render(indexed), render(plain));
			}
			indexed->parameters().setDouble("min QUAL", 30);
			indexed->parameters().setBool("PASS only", true);
			indexed->parameters().setString("sample", "S1");
			plain->parameters().setDouble("min QUAL", 30);
			plain->parameters().setBool("PASS only", true);
			plain->parameters().setString("sample", "S1");
			QCOMPARE(render(indexed), render(plain));
			QString filtered_xml;
			QXmlStreamWriter filtered_writer(&filtered_xml);
			indexed->writeToXml(filtered_writer);
			QDomDocument filtered_document;
			QVERIFY(filtered_document.setContent(filtered_xml));
			std::unique_ptr<TrackWidget> filtered(TrackWidget::fromXml(filtered_document.documentElement(), nullptr, errors));
			QVERIFY(filtered && errors.isEmpty());
			QCOMPARE(filtered->parameters().getDouble("min QUAL"), 30.0);
			QVERIFY(filtered->parameters().getBool("PASS only"));
			QCOMPARE(filtered->parameters().getString("sample"), QByteArray("S1"));
			const QString session_path = data_.filePath("vcf-session.xml");
			QFile session(session_path);
			QVERIFY(session.open(QIODevice::WriteOnly | QIODevice::Truncate));
			QXmlStreamWriter session_writer(&session);
			session_writer.writeStartElement("GSviewerSession");
			session_writer.writeAttribute("version", "1");
			session_writer.writeStartElement("General");
			session_writer.writeAttribute("gene_panel_height", "80");
			session_writer.writeStartElement("DisplayedRegion");
			session_writer.writeAttribute("chr", "chr22");
			session_writer.writeAttribute("start", "1");
			session_writer.writeAttribute("end", "100");
			session_writer.writeEndElement();
			SharedData::writeSettingsToXml(session_writer);
			session_writer.writeEndElement();
			session_writer.writeStartElement("TrackGroup");
			session_writer.writeAttribute("height", "100");
			indexed->writeToXml(session_writer);
			session_writer.writeEndElement();
			session_writer.writeEndElement();
			session.close();
			QCOMPARE(XmlHelper::isValidXml(session_path, QFINDTESTDATA("../cppVISUAL/Resources/GSviewerSession.xsd")), QString());
			QSignalSpy deleted(indexed, &TrackWidget::trackDeleted);
			indexed->reloadTrack();
			QCOMPARE(deleted.count(), 0);
			QCOMPARE(render(indexed), render(plain));
			SharedData::setRegion(Chromosome("chr23"), 1, 41); //Query errors recover on the next valid view.
			QCOMPARE(indexed->sizeHint().height(), 70);
			SharedData::setRegion(Chromosome("chr22"), 1, 100);
			QCOMPARE(render(indexed), render(plain));
			plain->parameters().setDouble("min QUAL", 0);
			plain->parameters().setBool("PASS only", false);
			plain->parameters().setString("sample", "S1");
		}
		//Reload a changed sample header while preserving quality/filter settings.
		plain->parameters().setString("sample", "S1");
		plain->parameters().setDouble("min QUAL", 25);
		QByteArray renamed = header;
		renamed.replace("\tS1\n", "\tS3\n");
		writeFile("variants.VCF", renamed + chr22 + structural);
		plain->reloadTrack();
		QCOMPARE(plain->parameters().getString("sample"), QByteArray("S3"));
		QCOMPARE(plain->parameters().getDouble("min QUAL"), 25.0);
		writeFile("sites.vcf", "##fileformat=VCFv4.2\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\nchr22\t10\tsite\tA\tG\t.\t.\t.\n");
		std::unique_ptr<VcfTrack> sites(VcfTrack::createTrack(nullptr, data_.filePath("sites.vcf"), "Sites"));
		QVERIFY(sites);
		render(sites.get());
		QVERIFY(popup_text(sites.get(), 10).contains("no samples"));
		sites->parameters().setDouble("min QUAL", 1);
		render(sites.get());
		QVERIFY(popup_text(sites.get(), 10).isEmpty());
		QCOMPARE(errors_shown, 0);
		writeFile("bad-gt.vcf", header + "chr22\t10\tbad\tA\tG\t20\tPASS\t.\tGT\t0/2\n");
		std::unique_ptr<VcfTrack> invalid(VcfTrack::createTrack(nullptr, data_.filePath("bad-gt.vcf"), "Bad GT"));
		QVERIFY(!invalid);
		QCOMPARE(errors_shown, 1);
	}

	void bedTrackWholeGenomeHistogram()
	{
		const BedLine original = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original.chr(), original.start(), original.end()); });
		SharedData::setRegion(Chromosome(), 0, 0);
		BedTrack track(nullptr, "", "BED histogram");
		track.parameters().setString("color", "transcript blue");
		auto data = QSharedPointer<BedFile>::create();
		for (const BedLine& line : {
			BedLine(Chromosome("chr22"), 21, 21), BedLine(Chromosome("chr22"), 22, 22),
			BedLine(Chromosome("chr22"), 21, 40), BedLine(Chromosome("chrX"), 21, 30),
			BedLine(Chromosome("chrY"), 200, 250), BedLine(Chromosome("chr23"), 1, 100),
			BedLine(Chromosome("chrLong"), 1, 10000)}) data->append(line);
		data->sort();
		track.setBedFile(data);
		QCOMPARE(track.sizeHint().height(), 50);
		const int x0 = SharedData::settings().label_width + 2;
		track.resize(x0 + 42, 50); //10 pixels chr22, 10 chrX, 20 chrY.
		QImage image(track.size(), QImage::Format_ARGB32);
		for (const QByteArray& mode : QByteArrayList{"collapsed", "expanded"})
		{
			track.parameters().setString("draw mode", mode);
			track.render(&image);
			QCOMPARE(image.pixelColor(x0 + 2, 30), QColor("#0000B2")); //Three regions under one pixel.
			for (int x : {3, 12, 39})
			{
				QCOMPARE(image.pixelColor(x0 + x, 40), QColor("#0000B2")); //One region: one third height.
				QCOMPARE(image.pixelColor(x0 + x, 30), QColor(Qt::white));
			}
			for (int x : {4, 9, 10, 13, 20, 38, 40})
				QCOMPARE(image.pixelColor(x0 + x, 40), QColor(Qt::white));
		}
		track.resize(x0 + 82, 50);
		image = QImage(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 4, 30), QColor("#0000B2"));
		data = QSharedPointer<BedFile>::create();
		track.setBedFile(data);
		track.render(&image);
		QCOMPARE(image.pixelColor(x0 + 4, 40), QColor(Qt::white));
	}

	void bedTrackHistogramMinimumHeightInLayoutAndSession()
	{
		const BedLine original = SharedData::region();
		const auto restore = qScopeGuard([&]() { SharedData::setRegion(original.chr(), original.start(), original.end()); });
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		TrackGroup group(nullptr);
		group.resize(600, 30);
		auto* track = new BedTrack(nullptr, "", "BED");
		group.addTrackWidgets({track});
		group.show();
		QTRY_COMPARE(track->height(), 25);
		SharedData::setRegion(Chromosome(), 0, 0);
		QTRY_COMPARE(track->height(), 50);
		QCOMPARE(track->minimumHeight(), 50);
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		QTRY_COMPARE(track->height(), 25);

		writeFile("histogram-height.bed", "chr22\t10\t20\n");
		std::unique_ptr<BedTrack> source(BedTrack::createTrack(nullptr, data_.filePath("histogram-height.bed"), "BED"));
		source->setFixedHeight(25);
		QString xml;
		QXmlStreamWriter writer(&xml);
		source->writeToXml(writer);
		QDomDocument document;
		QVERIFY(document.setContent(xml));
		SharedData::setRegion(Chromosome(), 0, 0);
		QStringList errors;
		std::unique_ptr<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, errors));
		QVERIFY(restored);
		QVERIFY(errors.isEmpty());
		QCOMPARE(restored->height(), 50);
		QCOMPARE(restored->minimumHeight(), 50);
	}

	void bedTrackTabixIndexed()
	{
		const QByteArray visible = "chr22\t0\t1\tfirst_base\nchr22\t9\t20\tleft\nchr22\t19\t30\toverlap\t123\t-\nchr22\t40\t41\tedge\nchr22\t80\t100\tfar\n";
		//This zero-length record is not accepted by BedFile. Loading chr22 must never parse it.
		const QByteArray records = visible + "chr23\t50\t50\tunused_invalid\nchrLong\t4999\t5010\tdistant\n";
		auto reference_data = QSharedPointer<BedFile>::create(BedFile::fromText(visible + "chrLong\t4999\t5010\tdistant\n"));
		reference_data->sort();
		int errors = 0;
		QTimer dismiss_errors;
		connect(&dismiss_errors, &QTimer::timeout, this, [&]()
		{
			if (auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
			{
				++errors;
				dialog->accept();
			}
		});
		dismiss_errors.start(10);
		for (int min_shift : {0, 14}) //TBI and CSI.
		{
			const QString path = data_.filePath(QString("indexed-%1.BED.GZ").arg(min_shift));
			BGZF* stream = bgzf_open(path.toUtf8().constData(), "w");
			QVERIFY(stream);
			const auto written = bgzf_write(stream, records.constData(), records.size());
			const int closed = bgzf_close(stream);
			QCOMPARE(written, records.size());
			QCOMPARE(closed, 0);
			QCOMPARE(tbx_index_build(path.toUtf8().constData(), min_shift, &tbx_conf_bed), 0);
			SharedData::setRegion(Chromosome("chr22"), 1, 41);
			auto tracks = FileLoader::loadTracks(path, nullptr);
			QCOMPARE(tracks.count(), 1);
			std::unique_ptr<TrackWidget> owner(tracks.first());
			auto* track = qobject_cast<BedTrack*>(owner.get());
			QVERIFY(track);
			QCOMPARE(errors, 0);
			BedTrack reference(nullptr, "", track->displayName());
			reference.setBedFile(reference_data);
			auto compare_drawing = [&]()
			{
				QCOMPARE(track->sizeHint().height(), reference.sizeHint().height());
				track->resize(600, reference.sizeHint().height());
				reference.resize(track->size());
				QImage actual(track->size(), QImage::Format_ARGB32);
				QImage expected(reference.size(), QImage::Format_ARGB32);
				track->render(&actual);
				reference.render(&expected);
				QCOMPARE(actual, expected);
			};
			for (const BedLine& region : {BedLine(Chromosome("chr22"), 1, 41), BedLine(Chromosome("chr22"), 42, 82),
				BedLine(Chromosome("chr22"), 41, 81), BedLine(Chromosome("chrLong"), 4990, 5030),
				BedLine(Chromosome("chrX"), 1, 41), BedLine(), BedLine(Chromosome("chr22"), 1, 41)})
			{
				SharedData::setRegion(region.chr(), region.start(), region.end());
				compare_drawing();
			}
			track->show();
			QFont font = track->font();
			font.setPointSize(8);
			const int row_height = 10 + QFontMetrics(font).height() + 2;
			const int x = SharedData::settings().label_width + 2 + int(24.5 * (600 - SharedData::settings().label_width - 4) / 41);
			QTest::mouseClick(track, Qt::LeftButton, Qt::NoModifier, QPoint(x, row_height + 5));
			auto* popup = track->findChild<QDialog*>();
			QVERIFY(popup);
			const QString text = popup->findChild<QLabel*>()->text();
			QVERIFY(text.contains("chr22:20-30"));
			QVERIFY(text.contains("overlap"));
			QVERIFY(text.contains("column 5: 123"));
			QVERIFY(text.contains("column 6: -"));
			delete popup;
			track->parameters().setString("draw mode", "collapsed");
			reference.parameters().setString("draw mode", "collapsed");
			compare_drawing();
			QSignalSpy deleted(track, &TrackWidget::trackDeleted);
			track->reloadTrack();
			QCOMPARE(deleted.count(), 0);
			compare_drawing();

			//Errors while panning clear old data and recover when returning to a valid region.
			SharedData::setRegion(Chromosome("chr23"), 30, 70);
			QCOMPARE(track->sizeHint().height(), 25);
			SharedData::setRegion(Chromosome("chr22"), 1, 41);
			compare_drawing();
			QString xml;
			QXmlStreamWriter writer(&xml);
			track->writeToXml(writer);
			QDomDocument document;
			QVERIFY(document.setContent(xml));
			QStringList session_errors;
			std::unique_ptr<TrackWidget> restored(TrackWidget::fromXml(document.documentElement(), nullptr, session_errors));
			QVERIFY(restored);
			QVERIFY(session_errors.isEmpty());
			QCOMPARE(restored->parameters().getString("draw mode"), QByteArray("collapsed"));
			//Explicitly replacing the BED data disables subsequent tabix queries.
			track->setBedFile(QSharedPointer<BedFile>::create(BedFile::fromText("chr22\t4\t6\tgenerated\n")));
			SharedData::setRegion(Chromosome("chr23"), 30, 70);
			auto generated = QSharedPointer<BedFile>::create();
			generated->append(BedLine(Chromosome("chr23"), 40, 50));
			track->setBedFile(generated);
			reference.setBedFile(generated);
			SharedData::setRegion(Chromosome("chr23"), 31, 71);
			compare_drawing();
		}
		QCOMPARE(errors, 0);
		//A compressed track without a tabix index reports an error instead of loading the whole file.
		const QString no_index = data_.filePath("missing-index.bed.gz");
		BGZF* stream = bgzf_open(no_index.toUtf8().constData(), "w");
		QVERIFY(stream);
		const auto written = bgzf_write(stream, visible.constData(), visible.size());
		const int closed = bgzf_close(stream);
		QCOMPARE(written, visible.size());
		QCOMPARE(closed, 0);
		std::unique_ptr<BedTrack> missing(BedTrack::createTrack(nullptr, no_index, "Missing index"));
		QVERIFY(!missing);
		QCOMPARE(errors, 1);
	}

	void bedTrackVisibleRowsAndHitTesting()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		auto bed = QSharedPointer<BedFile>::create();
		bed->append(BedLine(Chromosome("chr22"), 10, 20, {"first"}));
		bed->append(BedLine(Chromosome("chr22"), 20, 30, {"second"}));
		BedTrack track(nullptr, "", "BED");
		track.setBedFile(bed);
		QFont region_font = track.font();
		region_font.setPointSize(8);
		const int row_height = 10 + QFontMetrics(region_font).height() + 2;
		QCOMPARE(track.sizeHint().height(), 2 * row_height);
		track.resize(600, track.sizeHint().height());
		track.show();
		QCoreApplication::processEvents();
		const int x = SharedData::settings().label_width + 2 + int(24.5 * (600 - SharedData::settings().label_width - 4) / 80);
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(x, row_height + 5));
		auto* popup = track.findChild<QDialog*>();
		QVERIFY(popup);
		const QString text = popup->findChild<QLabel*>()->text();
		QVERIFY(text.contains("second"));
		QVERIFY(!text.contains("first"));
		delete popup;
		track.parameters().setBool("show region names", false);
		QCOMPARE(track.sizeHint().height(), 25);
		SharedData::setRegion(Chromosome("chr22"), 50, 100);
		QCOMPARE(track.sizeHint().height(), 25);
		track.parameters().setBool("show region names", true);
		QCOMPARE(track.sizeHint().height(), 25);
	}

	void bedTrackInvalidInputAndEmptyPopup()
	{
		SharedData::setRegion(Chromosome("chr22"), 1, 80);
		BedTrack track(nullptr, "", "BED");
		track.parameters().setString("draw mode", "collapsed");
		track.resize(600, 25);
		track.show();
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(300, 5));
		QVERIFY(!track.findChild<QDialog*>());
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track.setBedFile({}));
		auto bed = QSharedPointer<BedFile>::create();
		bed->append(BedLine(Chromosome("chr22"), 10, 20));
		track.setBedFile(bed);
		auto unsorted = QSharedPointer<BedFile>::create();
		unsorted->append(BedLine(Chromosome("chr22"), 30, 40));
		unsorted->append(BedLine(Chromosome("chr22"), 10, 20));
		QVERIFY_THROWS_EXCEPTION(ArgumentException, track.setBedFile(unsorted));
		//The previous data must remain usable after rejected input.
		QImage image(track.size(), QImage::Format_ARGB32);
		track.render(&image);
		track.setMinimumWidth(0);
		track.resize(SharedData::settings().label_width + 4, 25);
		QTest::mouseClick(&track, Qt::LeftButton, Qt::NoModifier, QPoint(SharedData::settings().label_width + 2, 5));
		QVERIFY(!track.findChild<QDialog*>());
	}
};

QTEST_MAIN(GenomeVisualizationWidget_Test)
#include "GenomeVisualizationWidget_Test.moc"
