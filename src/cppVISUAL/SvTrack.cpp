#include "SvTrack.h"
#include "SharedData.h"
#include <QApplication>
#include <QFontMetricsF>
#include <QHelpEvent>
#include <QMessageBox>
#include <QPainter>
#include <QResizeEvent>
#include <QToolTip>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int TICK_HEIGHT = 8;
int midpoint(int start, int end)
{
	return start + (end - start) / 2;
}
void drawArrow(QPainter& painter, QPointF from, QPointF to)
{
	painter.drawLine(QLineF(from, to));
	const QPointF direction = (to - from) / QLineF(from, to).length();
	const QPointF normal(-direction.y(), direction.x());
	painter.drawLine(QLineF(to, to - direction * 5 + normal * 3));
	painter.drawLine(QLineF(to, to - direction * 5 - normal * 3));
}
}

SvTrack::SvTrack(QWidget* parent, QString file_path, QString name)
	: TrackWidget(parent, file_path, name, type())
{
	loadSource();
	initializeSettings(&fast_settings_);
	connect(&settings_, &ParameterList::parameterChanged, this, &SvTrack::updateLayout);
	connect(SharedData::instance(), &SharedData::settingsChanged, this, &SvTrack::updateLayout);
	connect(viewer_, &GenomeVisualizationWidget::regionChanged, this, &SvTrack::updateLayout);
	updateLayout();
}

QList<Parameter> SvTrack::getParameters() const
{
	return {
		Parameter("min QUAL", "Minimum variant quality. Missing or invalid QUAL is included only when the minimum is zero.", ParameterType::DOUBLE, {{ConstraintType::MIN, 0.0}, {ConstraintType::DISPLAY_DIGITS, 2}}),
		Parameter("PASS only", "Show only variants with FILTER=PASS. Excludes missing FILTER and FILTER=.", ParameterType::BOOL, {})};
}

QHash<QByteArray, QVariant> SvTrack::getParameterDefaults() const
{
	return {{"min QUAL", 0.0}, {"PASS only", false}};
}

void SvTrack::loadSource()
{
	BedpeFile variants;
	variants.load(file_path_);
	chr_index_.reset();
	variants_ = std::move(variants);
	qual_index_ = variants_.annotationIndexByName("QUAL", false);
	filter_index_ = variants_.annotationIndexByName("FILTER", false);
	breakpoints_.clear();
	breakpoint_variants_.clear();
	for (int i = 0; i < variants_.count(); ++i)
	{
		const auto& sv = variants_[i];
		//BEDPE may contain an unknown mate (chromosome/position '.').
		for (const BedLine& bp : {BedLine(sv.chr1(), sv.start1(), sv.end1()), BedLine(sv.chr2(), sv.start2(), sv.end2())})
		{
			if (!bp.chr().isValid() || bp.start() < 0 || bp.end() < bp.start()) continue;
			BedLine indexed = bp;
			indexed.annotations() << QByteArray::number(i);
			breakpoints_.append(indexed);
		}
	}
	breakpoints_.sort();
	for (int i = 0; i < breakpoints_.count(); ++i) breakpoint_variants_ << breakpoints_[i].annotations().first().toInt();
	chr_index_ = std::make_unique<ChromosomalIndex<BedFile>>(breakpoints_);
}

void SvTrack::reloadTrack()
{
	try
	{
		loadSource();
		updateLayout();
	}
	catch (const Exception& e)
	{
		QMessageBox::critical(QApplication::activeWindow(), "Error loading BEDPE", e.message());
		emit trackDeleted();
	}
}

void SvTrack::updateLayout()
{
	placements_.clear();
	pressed_variant_ = -1;
	const auto& region = viewer_->region();
	const Viewport viewport = getViewport();
	const QFontMetricsF metrics(font());
	const double label_height = std::ceil(metrics.height());
	double extra_label_space = 0;
	QVector<double> level_ends;
	if (chr_index_ && region.isValid() && viewport.usable_width > 0)
	{
		QSet<int> indices;
		for (int bp : chr_index_->matchingIndices(region.chr(), region.start(), region.end())) indices.insert(breakpoint_variants_[bp]);
		for (int index : indices)
		{
			const auto& sv = variants_[index];
			if (fast_settings_.pass_only && sv.annotations().value(filter_index_) != "PASS") continue;
			bool ok = false;
			const double qual = sv.annotations().value(qual_index_).toDouble(&ok);
			if (fast_settings_.min_qual > 0 && (!ok || !std::isfinite(qual) || qual < fast_settings_.min_qual)) continue;
			const Chromosome chrs[] = {sv.chr1(), sv.chr2()};
			const int starts[] = {sv.start1(), sv.start2()};
			const int ends[] = {sv.end1(), sv.end2()};
			for (int side = 0; side < 2; ++side)
			{
				if (chrs[side] != region.chr() || starts[side] < 0 || ends[side] < starts[side] || starts[side] > region.end() || ends[side] < region.start()) continue;
				//Coincident insertion breakpoints need only one tick and label.
				if (side == 1 && chrs[0] == chrs[1] && midpoint(starts[0], ends[0]) == midpoint(starts[1], ends[1])) continue;
				Placement p{};
				p.index = index;
				p.side = side;
				p.x = viewport.genomePosToScreen(std::clamp(midpoint(starts[side], ends[side]), region.start(), region.end()));
				const bool same_chr = chrs[side] == chrs[1 - side];
				p.mate_visible = same_chr && starts[1 - side] >= 0 && ends[1 - side] >= starts[1 - side] && starts[1 - side] <= region.end() && ends[1 - side] >= region.start();
				const int direction = same_chr && starts[1 - side] >= 0 && midpoint(starts[1 - side], ends[1 - side]) < midpoint(starts[side], ends[side]) ? -1 : 1;
				const double label_width = metrics.horizontalAdvance(StructuralVariantTypeToString(sv.type()));
				const double arrow_length = std::max(32.0, label_width + 8);
				p.arrow_x = std::clamp(p.x + direction * arrow_length, double(viewport.x0), double(viewport.x0 + viewport.usable_width - 1));
				const double label_x = std::max(double(viewport.x0), std::min(direction > 0 ? p.x + 4 : p.x - label_width - 4, viewport.x0 + viewport.usable_width - label_width));
				p.label_rect = QRectF(label_x, 0, label_width, label_height);
				p.bounds = p.label_rect.united(QRectF(std::min(p.x, p.arrow_x) - 3, 0, std::abs(p.arrow_x - p.x) + 6, label_height));
				if (!same_chr || starts[1 - side] < 0)
				{
					p.mate_label = chrs[1 - side].isValid() && starts[1 - side] >= 0 ? QString::fromUtf8(chrs[1 - side].str()) : "unknown mate";
					const double mate_width = metrics.horizontalAdvance(p.mate_label);
					const double mate_x = std::max(double(viewport.x0), std::min(p.arrow_x + 5, viewport.x0 + viewport.usable_width - mate_width));
					p.mate_rect = QRectF(mate_x, 0, mate_width, label_height);
					if (p.mate_rect.intersects(p.label_rect)) extra_label_space = std::ceil(label_height / 2);
					p.bounds = p.bounds.united(p.mate_rect);
				}
				placements_ << p;
			}
		}
		std::sort(placements_.begin(), placements_.end(), [](const Placement& a, const Placement& b)
		{
			if (a.bounds.left() != b.bounds.left()) return a.bounds.left() < b.bounds.left();
			if (a.index != b.index) return a.index < b.index;
			return a.side < b.side;
		});
		QHash<int, int> first_breakpoints;
		for (int i = 0; i < placements_.size(); ++i)
		{
			auto& p = placements_[i];
			const auto mate = first_breakpoints.constFind(p.index);
			if (mate == first_breakpoints.cend()) first_breakpoints.insert(p.index, i);
			else
			{
				p.mate = mate.value();
				auto& first = placements_[p.mate];
				first.mate = i;
				if (p.label_rect.intersects(first.label_rect))
				{
					p.label_rect = QRectF();
					p.bounds = QRectF(p.x - 3, 0, 6, label_height);
				}
			}
		}
		for (int i = 0; i < placements_.size(); ++i)
		{
			auto& p = placements_[i];
			if (p.mate >= 0 && p.mate < i) continue;
			//Reserve the entire connector and keep both endpoints on the same level.
			const QRectF bounds = p.mate >= 0 ? p.bounds.united(placements_[p.mate].bounds) : p.bounds;
			int level = 0;
			while (level < level_ends.size() && bounds.left() <= level_ends[level] + 4) ++level;
			if (level == level_ends.size()) level_ends << bounds.right();
			else level_ends[level] = bounds.right();
			p.level = level;
			if (p.mate >= 0) placements_[p.mate].level = level;
		}
	}
	//All ticks start at the top; overlapping variants extend downward on longer ticks.
	const double level_height = label_height + 10 + extra_label_space;
	tick_top_ = std::max(3.0, label_height / 2 - TICK_HEIGHT + 3);
	content_height_ = int(std::ceil(tick_top_ + TICK_HEIGHT + label_height + 7 + extra_label_space + std::max(0, int(level_ends.size()) - 1) * level_height));
	for (auto& p : placements_)
	{
		p.arrow_y = tick_top_ + TICK_HEIGHT + p.level * level_height;
		p.label_rect.moveTop(p.arrow_y + 3);
		if (!p.mate_label.isEmpty())
		{
			p.mate_rect.moveTop(p.arrow_y - label_height / 2);
			//At the right edge the chromosome label moves left; keep the type label below it.
			if (p.mate_rect.intersects(p.label_rect)) p.label_rect.moveTop(p.mate_rect.bottom() + 3);
		}
		p.bounds = p.label_rect.united(QRectF(std::min(p.x, p.arrow_x) - 3, tick_top_ - 3, std::abs(p.arrow_x - p.x) + 6, p.arrow_y - tick_top_ + 6));
		if (!p.mate_label.isEmpty()) p.bounds = p.bounds.united(p.mate_rect);
	}
	setMinimumHeight(sizeHint().height());
	updateGeometry();
	update();
}

QSize SvTrack::sizeHint() const
{
	return QSize(parentWidget() ? parentWidget()->width() : 200, std::max(70, content_height_));
}

void SvTrack::resizeEvent(QResizeEvent* event)
{
	TrackWidget::resizeEvent(event);
	updateLayout();
}

void SvTrack::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.fillRect(rect(), Qt::white);
	drawLabel(painter);
	if (drawOverviewMessage(painter)) return;
	const Viewport viewport = getViewport();
	painter.setClipRect(QRect(viewport.x0, 0, std::max(0, viewport.usable_width), height()));
	for (int i = 0; i < placements_.size(); ++i)
	{
		const auto& p = placements_[i];
		//Match the gray connecting paired reads in BamAlignmentTrack.
		painter.setPen(QPen(Qt::gray, 1));
		if (p.mate >= 0)
		{
			if (i < p.mate)
			{
				const auto& mate = placements_[p.mate];
				painter.drawLine(QLineF(p.x, p.arrow_y, mate.x, mate.arrow_y));
			}
		}
		else if (!p.mate_visible && std::abs(p.arrow_x - p.x) > 1)
		{
			drawArrow(painter, QPointF(p.x, p.arrow_y), QPointF(p.arrow_x, p.arrow_y));
		}
	}
	for (const auto& p : placements_)
	{
		painter.setPen(Qt::black);
		if (!p.label_rect.isEmpty())
		{
			painter.fillRect(p.label_rect, Qt::white);
			painter.drawText(p.label_rect, Qt::AlignLeft | Qt::AlignVCenter, StructuralVariantTypeToString(variants_[p.index].type()));
		}
		if (!p.mate_label.isEmpty())
		{
			painter.fillRect(p.mate_rect, Qt::white);
			painter.drawText(p.mate_rect, Qt::AlignLeft | Qt::AlignVCenter, p.mate_label);
		}
	}
	//Keep breakpoint ticks visible where a nearby label crosses them.
	painter.setPen(QPen(Qt::gray, 1));
	for (const auto& p : placements_) painter.drawLine(QLineF(p.x, tick_top_, p.x, p.arrow_y));
}
QString SvTrack::variantText(int index) const
{
	const auto& sv = variants_[index];
	QString text = "Type: " + StructuralVariantTypeToString(sv.type()) + "\nBreakpoint 1: " + sv.position1() + "\nBreakpoint 2: " + sv.position2();
	qint64 size = sv.size();
	const int info = variants_.annotationIndexByName("INFO_A", false);
	for (const auto& field : sv.annotations().value(info).split(';'))
	{
		if (!field.startsWith("SVLEN=")) continue;
		bool ok = false;
		const double value = field.mid(6).toDouble(&ok);
		if (ok && std::isfinite(value) && std::abs(value) <= std::numeric_limits<int>::max()) size = qint64(std::abs(value));
	}
	if (size >= 0) text += "\nSize: " + QString::number(size) + " bp";
	for (const QByteArray& name : {QByteArray("ID"), QByteArray("QUAL"), QByteArray("FILTER"), QByteArray("STRAND_A"), QByteArray("STRAND_B"), QByteArray("GENES")})
	{
		const int column = variants_.annotationIndexByName(name, false);
		text += "\n" + QString::fromUtf8(name) + ": " + (column >= 0 ? QString::fromUtf8(sv.annotations().value(column)) : "n/a");
	}
	const int format = variants_.annotationIndexByName("FORMAT", false);
	if (format >= 0)
	{
		const int gt = sv.annotations().value(format).split(':').indexOf("GT");
		const auto& headers = variants_.annotationHeaders();
		for (int sample = format + 1; sample < headers.count(); ++sample)
		{
			//Only the columns following FORMAT up to the first annotation are samples.
			if (sample > format + 1 && !variants_.sampleHeaderInfo().sampleNames().contains(QString::fromUtf8(headers[sample]))) break;
			text += "\n" + QString::fromUtf8(headers[sample]) + " GT: " + QString::fromUtf8(sv.annotations().value(sample).split(':').value(gt, "."));
		}
	}
	return "<qt>" + text.toHtmlEscaped().replace("\n", "<br>") + "</qt>";
}

int SvTrack::variantAt(const QPoint& pos, QRect* hit_rect, bool labels_only) const
{
	const Viewport viewport = getViewport();
	if (pos.x() < viewport.x0 || pos.x() >= viewport.x0 + viewport.usable_width) return -1;
	//Prefer labels over stems of other variants at the same position.
	for (const auto& p : placements_)
	{
		for (const QRectF& label : {p.label_rect, p.mate_rect})
		{
			if (!label.contains(pos)) continue;
			if (hit_rect) *hit_rect = label.toAlignedRect();
			return p.index;
		}
	}
	if (!labels_only)
	{
		for (const auto& p : placements_)
		{
			if (!p.bounds.contains(pos)) continue;
			if (hit_rect) *hit_rect = p.bounds.toAlignedRect();
			return p.index;
		}
	}
	return -1;
}
void SvTrack::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		mouse_press_pos_ = event->pos();
		pressed_variant_ = variantAt(event->pos(), nullptr, true);
	}
	TrackWidget::mousePressEvent(event);
}

void SvTrack::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (pressed_variant_ >= 0 && variantAt(event->pos(), nullptr, true) == pressed_variant_ && (event->pos() - mouse_press_pos_).manhattanLength() < QApplication::startDragDistance())
		{
			showInfoPopup(event->globalPosition(), variantText(pressed_variant_));
		}
		pressed_variant_ = -1;
	}
	TrackWidget::mouseReleaseEvent(event);
}

bool SvTrack::event(QEvent* event)
{
	if (event->type() == QEvent::ToolTip)
	{
		const auto* help = static_cast<QHelpEvent*>(event);
		const Viewport viewport = getViewport();
		if (help->pos().x() < viewport.x0) return TrackWidget::event(event);
		QRect hit_rect;
		const int index = variantAt(help->pos(), &hit_rect);
		if (index >= 0)
		{
			QToolTip::showText(help->globalPos(), variantText(index), this, hit_rect);
			return true;
		}
		QToolTip::hideText();
		event->ignore();
		return true;
	}
	return TrackWidget::event(event);
}

SvTrack* SvTrack::createTrack(QWidget* parent, QString file_path, QString name)
{
	return new SvTrack(parent, file_path, name);
}
