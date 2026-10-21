#include "VcfTrack.h"
#include "SharedData.h"
#include "VersatileFile.h"
#include "Helper.h"
#include <QApplication>
#include <QHelpEvent>
#include <QMessageBox>
#include <QPainter>
#include <QFontMetricsF>
#include <QImage>
#include <QResizeEvent>
#include <QToolTip>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int ROW_HEIGHT = 18;
constexpr int MARKER_HEIGHT = 12;
const QBrush& delinsBrush()
{
	static const QBrush brush = []()
	{
		QImage tile(8, 8, QImage::Format_RGB32);
		for (int y = 0; y < tile.height(); ++y)
		{
			for (int x = 0; x < tile.width(); ++x)
			{
				tile.setPixelColor(x, y, (x + y) % 8 < 4 ? QColor(140, 0, 200) : QColor(224, 224, 224));
			}
		}
		return QBrush(tile);
	}();
	return brush;
}
struct AlleleChange
{
	int prefix = 0;
	int removed = 0;
	int inserted = 0;
};
AlleleChange alleleChange(QByteArray ref, QByteArray alt)
{
	ref = ref.toUpper();
	alt = alt.toUpper();
	if (alt.startsWith('<') || alt.contains('[') || alt.contains(']') || alt == "*" || alt == ".") return {};
	int prefix = 0;
	while (prefix < ref.size() && prefix < alt.size() && ref[prefix] == alt[prefix]) ++prefix;
	int suffix = 0;
	while (suffix < ref.size() - prefix && suffix < alt.size() - prefix && ref[ref.size() - 1 - suffix] == alt[alt.size() - 1 - suffix]) ++suffix;
	return {prefix, int(ref.size()) - prefix - suffix, int(alt.size()) - prefix - suffix};
}
bool isDelins(const QByteArray& ref, const QByteArrayList& alts)
{
	return std::any_of(alts.begin(), alts.end(), [&](const QByteArray& alt)
	{
		const auto change = alleleChange(ref, alt);
		return change.removed > 0 && change.inserted > 0 && (change.removed > 1 || change.inserted > 1);
	});
}
bool isSnp(const QByteArray& ref, const QByteArrayList& alts)
{
	return ref.size() == 1 && !alts.isEmpty() && std::all_of(alts.begin(), alts.end(), [](const QByteArray& alt)
	{
		return alt.size() == 1 && QByteArray("ACGTN").contains(alt.toUpper());
	});
}
bool isInsertion(const QByteArray& ref, const QByteArrayList& alts)
{
	return !alts.isEmpty() && std::all_of(alts.begin(), alts.end(), [&](const QByteArray& alt)
	{
		const auto change = alleleChange(ref, alt);
		return alt.toUpper() == "<INS>" || (change.removed == 0 && change.inserted > 0);
	});
}
QColor alleleColor(int allele, const QByteArray& ref, const QByteArrayList& alts)
{
	if (allele < 0) return QColor(190, 190, 190);
	if (allele == 0) return Qt::white;
	const QByteArray alt = alts[allele - 1].toUpper();
	// Match the nucleotide and indel colors used by the BAM tracks.
	if (alt == "<DEL>") return Qt::blue;
	if (alt == "<INS>") return QColor(140, 0, 200);
	if (alt.startsWith('<') || alt.contains('[') || alt.contains(']') || alt == "*") return Qt::gray;
	if (alt.size() < ref.size()) return Qt::blue;
	if (alt.size() > ref.size()) return QColor(140, 0, 200);
	const QByteArray reference = ref.toUpper();
	int position = 0;
	while (position + 1 < alt.size() && alt[position] == reference[position]) ++position;
	switch (alt[position])
	{
		case 'A': return QColor(0, 150, 0);
		case 'C': return Qt::blue;
		case 'G': return QColor(209, 113, 5);
		case 'T': return Qt::red;
		case 'N': return QColor(128, 128, 128);
		default: return Qt::black;
	}
}
QString html(QString text)
{
	return "<qt>" + text.toHtmlEscaped().replace("\n", "<br>") + "</qt>";
}
void drawInsertionLength(QPainter& painter, QRectF area, const QByteArray& ref, const QByteArray& alt)
{
	//Symbolic alleles do not encode the inserted sequence length.
	if (alt.startsWith('<') || alt.size() <= ref.size()) return;
	const QString text = QString::number(alt.size() - ref.size());
	area = area.intersected(painter.clipBoundingRect());
	QFont font = painter.font();
	font.setPixelSize(area.height() < 10 ? 8 : 10);
	const QRectF bounds = QFontMetricsF(font).tightBoundingRect(text);
	if (bounds.width() + 2 > area.width() || bounds.height() > area.height()) return;
	painter.save();
	painter.setClipRect(area, Qt::IntersectClip);
	painter.setFont(font);
	painter.setPen(Qt::white);
	painter.drawText(area.center() - bounds.center(), text);
	painter.restore();
}
}

VcfTrack::VcfTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name, type())
{
	loadSource();
	initializeSettings(&fast_settings_);
	setMinimumHeight(70);
	connect(&settings_, &ParameterList::parameterChanged, this, &VcfTrack::refreshRegion);
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &VcfTrack::updateLayout);
	connect(SharedData::instance(), &SharedData::regionChanged, this, &VcfTrack::refreshRegion);
	updateLayout();
}

QByteArrayList VcfTrack::sampleChoices() const
{
	return samples_.isEmpty() ? QByteArrayList{"(no samples)"} : samples_;
}

QList<Parameter> VcfTrack::getParameters() const
{
	return {
		Parameter("min QUAL", "Minimum variant QUAL. Missing QUAL is included only when the minimum is zero.", ParameterType::DOUBLE, {{ConstraintType::MIN, 0.0}, {ConstraintType::DISPLAY_DIGITS, 2}}),
		Parameter("PASS only", "Show only records with FILTER=PASS. Excludes unfiltered records (FILTER=.).", ParameterType::BOOL, {}),
		Parameter("max visibility", "Maximum visible region length in bases. Zoom in to show variants in larger regions.", ParameterType::INT, {{ConstraintType::MIN, 1}}),
		Parameter("sample", "Sample used for genotype allele bands: white=REF, BAM base colors=ALT alleles, light gray upper half=heterozygous SNP or insertion, hatched gray=missing.", ParameterType::STRING, {{ConstraintType::ALLOWED_VALUES, sampleChoices().join('\t')}})
	};
}

QHash<QByteArray, QVariant> VcfTrack::getParameterDefaults() const
{
	return {{"min QUAL", 0.0}, {"PASS only", false}, {"sample", sampleChoices().first()}, {"max visibility", 1000000}};
}

VcfTrack::Variant VcfTrack::parseVariant(const QByteArray& line) const
{
	Variant variant;
	variant.columns = line.split('\t');
	const auto& columns = variant.columns;
	if (columns.count() != column_count_) THROW(FileParseException, "VCF record does not match the column header: " + line);
	variant.chr = Chromosome(columns[0]);
	variant.start = Helper::toInt(columns[1], "VCF position");
	const qint64 end = qint64(variant.start) + columns[3].size() - 1;
	if (!variant.chr.isValid() || variant.start < 1 || columns[3].isEmpty() || columns[3] == "." || columns[4].isEmpty() || end > std::numeric_limits<int>::max())
	{
		THROW(FileParseException, "Invalid VCF position or alleles: " + line);
	}
	variant.end = int(end);
	for (const QByteArray& info : columns[7].split(';'))
	{
		if (!info.startsWith("END=")) continue;
		variant.end = Helper::toInt(info.mid(4), "VCF END");
		if (variant.end < variant.start) THROW(FileParseException, "VCF END precedes POS: " + line);
	}
	variant.qual = std::numeric_limits<double>::quiet_NaN();
	if (columns[5] != ".")
	{
		bool ok = false;
		variant.qual = columns[5].toDouble(&ok);
		if (!ok || !std::isfinite(variant.qual) || variant.qual < 0) THROW(FileParseException, "Invalid VCF QUAL: " + columns[5]);
	}
	const int alt_count = columns[4] == "." ? 0 : columns[4].split(',').count();
	const int gt_index = column_count_ > 8 ? columns[8].split(':').indexOf("GT") : -1;
	for (int i = 0; i < samples_.count(); ++i)
	{
		Genotype genotype;
		genotype.text = gt_index < 0 ? QByteArray(".") : columns[9 + i].split(':').value(gt_index, ".");
		if (genotype.text.isEmpty()) genotype.text = ".";
		QByteArray normalized = genotype.text;
		normalized.replace('|', '/');
		for (const QByteArray& allele : normalized.split('/'))
		{
			int value = -1;
			if (allele != ".")
			{
				value = Helper::toInt(allele, "VCF GT allele");
				if (value < 0 || value > alt_count) THROW(FileParseException, "VCF GT allele exceeds ALT count: " + genotype.text);
			}
			genotype.alleles << value;
		}
		variant.genotypes << genotype;
	}
	return variant;
}

QVector<VcfTrack::Variant> VcfTrack::queryRegion(const TabixIndexedFile& tabix) const
{
	QVector<Variant> variants;
	const BedLine& region = SharedData::region();
	if (!region.isValid() || region.length() > fast_settings_.max_visibility) return variants;
	for (const QByteArray& line : tabix.getMatchingLines(region.chr(), region.start(), region.end(), true)) variants << parseVariant(line);
	return variants;
}

void VcfTrack::setVariants(QVector<Variant> variants)
{
	std::sort(variants.begin(), variants.end(), [](const Variant& a, const Variant& b)
	{
		if (a.chr != b.chr) return a.chr < b.chr;
		if (a.start != b.start) return a.start < b.start;
		return a.end < b.end;
	});
	chr_index_.reset();
	variants_ = std::move(variants);
	positions_.clear();
	for (const Variant& variant : variants_) positions_.append(BedLine(variant.chr, variant.start, variant.end));
	chr_index_ = std::make_unique<ChromosomalIndex<BedFile>>(positions_);
	load_error_.clear();
}

void VcfTrack::loadSource()
{
	const bool indexed = getDisplayNameFromFilePath(file_path_).endsWith(".vcf.gz", Qt::CaseInsensitive);
	std::unique_ptr<TabixIndexedFile> tabix;
	if (indexed)
	{
		tabix = std::make_unique<TabixIndexedFile>();
		tabix->load(file_path_.toUtf8());
	}
	VersatileFile file(file_path_);
	file.open(QIODevice::ReadOnly);
	QVector<Variant> variants;
	bool header_found = false;
	while (!file.atEnd())
	{
		const QByteArray line = file.readLine(true);
		if (line.isEmpty() || line.startsWith("##")) continue;
		if (line.startsWith("#CHROM\t"))
		{
			if (header_found) THROW(FileParseException, "Duplicate VCF column header.");
			const QByteArrayList columns = line.split('\t');
			const QByteArrayList required{"#CHROM", "POS", "ID", "REF", "ALT", "QUAL", "FILTER", "INFO"};
			if (columns.mid(0, 8) != required || (columns.count() > 8 && columns[8] != "FORMAT")) THROW(FileParseException, "Invalid VCF column header.");
			column_count_ = columns.count();
			samples_ = columns.mid(9);
			if (samples_.count() > 1) THROW(NotImplementedException, "Multi-sample VCF files are not supported.");
			QSet<QByteArray> unique;
			for (const QByteArray& sample : samples_)
			{
				if (sample.isEmpty() || unique.contains(sample)) THROW(FileParseException, "Empty or duplicate VCF sample name.");
				unique.insert(sample);
			}
			header_found = true;
			if (indexed) break;
			continue;
		}
		if (!header_found) THROW(FileParseException, "Missing VCF column header.");
		variants << parseVariant(line);
	}
	if (!header_found) THROW(FileParseException, "Missing VCF column header.");
	if (tabix) variants = queryRegion(*tabix);
	setVariants(std::move(variants));
	loaded_region_ = tabix && SharedData::region().length() <= fast_settings_.max_visibility ? SharedData::region() : BedLine();
	tabix_ = std::move(tabix);
}

void VcfTrack::refreshRegion()
{
	if (tabix_)
	{
		const auto& region = SharedData::region();
		if (!region.isValid() || region.length() > fast_settings_.max_visibility)
		{
			setVariants({});
			loaded_region_ = BedLine();
		}
		else if (!(loaded_region_ == region))
		{
			try
			{
				setVariants(queryRegion(*tabix_));
				loaded_region_ = region;
			}
			catch (const Exception& e)
			{
				setVariants({});
				loaded_region_ = BedLine();
				load_error_ = e.message();
			}
		}
	}
	updateLayout();
}

void VcfTrack::reloadTrack()
{
	try
	{
		QHash<QByteArray, QVariant> values;
		for (const Parameter& parameter : settings_.parameters()) values.insert(parameter.name(), parameter.value());
		loadSource();
		if (!sampleChoices().contains(values.value("sample").toByteArray())) values["sample"] = sampleChoices().first();
		settings_.reset(getParameters(), values);
	}
	catch (const Exception& e)
	{
		QMessageBox::critical(QApplication::activeWindow(), "Error loading VCF", e.message());
		emit trackDeleted();
	}
}

QPair<int, int> VcfTrack::deletionSpan(const Variant& variant, int allele) const
{
	const QByteArrayList alts = variant.columns[4].split(',');
	if (allele == -1)
	{
		QPair<int, int> combined{std::numeric_limits<int>::max(), 0};
		for (int i = 1; i <= alts.count(); ++i)
		{
			const auto span = deletionSpan(variant, i);
			if (span.first == 0) return {0, 0};
			combined.first = std::min(combined.first, span.first);
			combined.second = std::max(combined.second, span.second);
		}
		return combined;
	}
	if (allele <= 0) return {0, 0};
	const QByteArray ref = variant.columns[3].toUpper();
	const QByteArray alt = alts[allele - 1].toUpper();
	if (alt == "<DEL>" && variant.end > variant.start) return {variant.start + 1, variant.end};
	const auto change = alleleChange(ref, alt);
	if (change.removed == 0 || (change.removed == 1 && change.inserted == 1)) return {0, 0};
	return {variant.start + change.prefix, variant.start + change.prefix + change.removed - 1};
}

void VcfTrack::updateLayout()
{
	placements_.clear();
	rows_ = 0;
	sample_index_ = samples_.indexOf(fast_settings_.sample);
	const Viewport viewport = getViewport();
	const BedLine& region = SharedData::region();
	if (chr_index_ && region.isValid() && region.length() <= fast_settings_.max_visibility && viewport.usable_width > 0)
	{
		QVector<double> row_ends;
		for (int index : chr_index_->matchingIndices(region.chr(), region.start(), region.end()))
		{
			const Variant& variant = variants_[index];
			if (fast_settings_.pass_only && variant.columns[6] != "PASS") continue;
			if (fast_settings_.min_qual > 0 && (!std::isfinite(variant.qual) || variant.qual < fast_settings_.min_qual)) continue;
			const auto deletion = deletionSpan(variant);
			const int start = std::max(region.start(), deletion.first > 0 ? deletion.first : variant.start);
			const int end = std::min(region.end(), deletion.first > 0 ? deletion.second : variant.end);
			if (end < start) continue;
			double x = viewport.genomePosToScreen(start);
			double width = viewport.genomeWidthToScreen(end - start + 1);
			//Center insertions between the anchor base and the following base.
			if (isInsertion(variant.columns[3], variant.columns[4].split(','))) x += viewport.genomeWidthToScreen(1) / 2;
			const double min_width = 3;
			if (width < min_width) { x -= (min_width - width) / 2; width = min_width; }
			int row = 0;
			while (row < row_ends.size() && x <= row_ends[row] + 2) ++row;
			if (row == row_ends.size()) row_ends << x + width;
			else row_ends[row] = x + width;
			placements_.append({index, QRectF(x, row * ROW_HEIGHT + 2, width, MARKER_HEIGHT)});
		}
		rows_ = row_ends.size();
	}
	//Propagate the required row height to the enclosing scroll area's layout.
	setMinimumHeight(sizeHint().height());
	updateGeometry();
	update();
}

QSize VcfTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200, std::max(70, rows_ * ROW_HEIGHT));
}

void VcfTrack::resizeEvent(QResizeEvent* event)
{
	TrackWidget::resizeEvent(event);
	updateLayout();
}

void VcfTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	if (drawOverviewMessage(painter)) return;
	const Viewport viewport = getViewport();
	painter.setClipRect(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()));
	if (SharedData::region().length() > fast_settings_.max_visibility)
	{
		painter.setPen(Qt::gray);
		painter.drawText(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()), Qt::AlignCenter,
			"Zoom in to show variants (max " + QString::number(fast_settings_.max_visibility) + " bases)");
		return;
	}
	if (!load_error_.isEmpty())
	{
		painter.setPen(Qt::red);
		painter.drawText(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()), Qt::TextWordWrap, load_error_);
		return;
	}
	for (const Placement& placement : placements_)
	{
		const Variant& variant = variants_[placement.index];
		const Genotype missing{".", {-1}};
		const Genotype& gt = sample_index_ < 0 ? missing : variant.genotypes[sample_index_];
		const QByteArrayList alts = variant.columns[4].split(',');
		const bool snp = isSnp(variant.columns[3], alts);
		const bool insertion = isInsertion(variant.columns[3], alts);
		const bool heterozygous = gt.alleles.count() == 2 && gt.alleles[0] >= 0 && gt.alleles[1] >= 0 && gt.alleles[0] != gt.alleles[1];
		if ((snp || insertion) && heterozygous)
		{
			painter.fillRect(placement.rect, QColor(224, 224, 224));
			QVector<int> alternate_alleles;
			for (int allele : gt.alleles) if (allele > 0) alternate_alleles << allele;
			for (int i = 0; i < alternate_alleles.count(); ++i)
			{
				//Keep both alternate colors visible for multi-allelic genotypes such as 1/2.
				const QRectF band(placement.rect.left() + i * placement.rect.width() / alternate_alleles.count(), placement.rect.center().y(), placement.rect.width() / alternate_alleles.count(), placement.rect.height() / 2);
				painter.fillRect(band, alleleColor(alternate_alleles[i], variant.columns[3], alts));
				if (insertion) drawInsertionLength(painter, band, variant.columns[3], alts[alternate_alleles[i] - 1]);
			}
			continue;
		}
		const bool deletion = deletionSpan(variant).first > 0;
		const bool same_insertion = insertion && !gt.alleles.isEmpty() && gt.alleles.first() > 0
			&& std::all_of(gt.alleles.begin(), gt.alleles.end(), [&](int allele) { return allele == gt.alleles.first(); });
		const int deleted_alleles = std::count_if(gt.alleles.begin(), gt.alleles.end(), [](int allele) { return allele > 0; });
		if (deletion)
		{
			if (isDelins(variant.columns[3], alts)) painter.fillRect(placement.rect, delinsBrush());
			else painter.fillRect(placement.rect, QColor(224, 224, 224));
		}
		for (int i = 0; i < gt.alleles.count(); ++i)
		{
			const QRectF band(placement.rect.left(), placement.rect.top() + double(i) * MARKER_HEIGHT / gt.alleles.count(), placement.rect.width(), double(MARKER_HEIGHT) / gt.alleles.count());
			if (deletion)
			{
				if (gt.alleles[i] < 0) painter.fillRect(band, QBrush(QColor(190, 190, 190), Qt::Dense4Pattern));
				const auto span = deletionSpan(variant, gt.alleles[i]);
				if (span.first > 0)
				{
					double x = viewport.genomePosToScreen(span.first);
					double width = viewport.genomeWidthToScreen(span.second - span.first + 1);
					if (width < 3) { x -= (3 - width) / 2; width = 3; }
					const double left = std::max(placement.rect.left(), x);
					const double right = std::min(placement.rect.right(), x + width);
					const double y = deleted_alleles == 1 ? placement.rect.center().y() : band.center().y() - (band.center().y() > placement.rect.center().y() ? 1 : 0);
					painter.save();
					painter.setClipRect(placement.rect, Qt::IntersectClip);
					painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::FlatCap));
					if (right > left) painter.drawLine(QLineF(left, y, right, y));
					painter.restore();
				}
			}
			else
			{
				painter.fillRect(band, QBrush(alleleColor(gt.alleles[i], variant.columns[3], alts), gt.alleles[i] < 0 ? Qt::Dense4Pattern : Qt::SolidPattern));
				if (insertion && !same_insertion && gt.alleles[i] > 0) drawInsertionLength(painter, band, variant.columns[3], alts[gt.alleles[i] - 1]);
			}
		}
		if (same_insertion) drawInsertionLength(painter, placement.rect, variant.columns[3], alts[gt.alleles.first() - 1]);
		const bool homozygous_snp = snp && gt.alleles.count() == 2 && gt.alleles[0] > 0 && gt.alleles[0] == gt.alleles[1];
		if (!deletion && !insertion && !homozygous_snp)
		{
			painter.setBrush(Qt::NoBrush);
			painter.setPen(QPen(Qt::darkGray, 1));
			painter.drawRect(placement.rect);
		}
	}
}

const VcfTrack::Placement* VcfTrack::placementAt(const QPoint& pos) const
{
	const Viewport viewport = getViewport();
	if (pos.x() < viewport.x0 || pos.x() >= viewport.x0 + viewport.usable_width) return nullptr;
	for (const Placement& placement : placements_) if (placement.rect.contains(pos)) return &placement;
	return nullptr;
}

QString VcfTrack::variantText(const Variant& variant) const
{
	const auto& columns = variant.columns;
	QString text = QString::fromUtf8(variant.chr.str()) + ":" + QString::number(variant.start)
		+ " " + QString::fromUtf8(columns[3]) + ">" + QString::fromUtf8(columns[4]);
	text += "\nQUAL: " + QString::fromUtf8(columns[5]);
	text += "\nFILTER: " + QString::fromUtf8(columns[6]);
	QStringList frequencies;
	if (sample_index_ >= 0)
	{
		text += "\nGT: " + QString::fromUtf8(variant.genotypes[sample_index_].text);
		const auto keys = columns[8].split(':');
		const auto values = columns[9 + sample_index_].split(':');
		const auto af = values.value(keys.indexOf("AF")).split(',');
		const auto ao = values.value(keys.indexOf("AO")).split(',');
		bool depth_ok = false;
		const double depth = values.value(keys.indexOf("DP")).toDouble(&depth_ok);
		for (int allele = 0; allele < columns[4].split(',').count(); ++allele)
		{
			bool ok = false;
			const double frequency = af.value(allele).toDouble(&ok);
			if (ok && std::isfinite(frequency) && frequency >= 0 && frequency <= 1)
			{
				frequencies << QString::fromUtf8(af[allele]);
				continue;
			}
			const double observations = ao.value(allele).toDouble(&ok);
			if (ok && std::isfinite(observations) && observations >= 0 && depth_ok && std::isfinite(depth) && depth > 0 && observations <= depth)
			{
				frequencies << QString::number(observations / depth, 'g', 6);
			}
			else frequencies << ".";
		}
	}
	else text += "\nGT: not available (no samples)";
	text += "\nAF: " + (frequencies.isEmpty() ? QString(".") : frequencies.join(","));
	return text;
}

void VcfTrack::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) mouse_press_pos_ = event->pos();
	TrackWidget::mousePressEvent(event);
}

void VcfTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && (event->pos() - mouse_press_pos_).manhattanLength() < QApplication::startDragDistance())
	{
		if (const Placement* placement = placementAt(event->pos())) showInfoPopup(event->globalPosition(), html(variantText(variants_[placement->index])));
	}
	TrackWidget::mouseReleaseEvent(event);
}

bool VcfTrack::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip && static_cast<QHelpEvent*>(event)->pos().x() >= SharedData::settings().label_width)
	{
		const auto* help = static_cast<QHelpEvent*>(event);
		if (const Placement* placement = placementAt(help->pos())) QToolTip::showText(help->globalPos(), html(variantText(variants_[placement->index])), this, placement->rect.toAlignedRect());
		else { QToolTip::hideText(); event->ignore(); }
		return true;
	}
	return TrackWidget::event(event);
}

VcfTrack* VcfTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
    return new VcfTrack(parent, file_path, name);
}
