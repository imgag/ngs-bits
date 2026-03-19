#include "Panel.h"
#include "BedTrack.h"
#include "BedFile.h"

#include <QMenu>
#include <QPainter>
#include <QFileInfo>
#include <QFileDialog>

Panel::Panel(QWidget* parent)
	:QScrollArea(parent), layout_(new QVBoxLayout(this)), content_widget_(new QWidget(this))
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setContextMenuPolicy(Qt::CustomContextMenu);

	content_widget_->setLayout(layout_);

	setWidget(content_widget_);

	setWidgetResizable(true);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

	layout_->addStretch(1);
	layout_->setContentsMargins(0, 0, 0, height());
}

void Panel::trackAdded(Track tr)
{
	auto panel = new BedTrack(this, tr);
	connect(panel, SIGNAL(trackDeleted()), this, SLOT(trackDeleted()));
	layout_->insertWidget(layout_->count() - 1, panel);
	layout_->update();
}

void Panel::trackDeleted()
{
	QWidget* senderPanel = qobject_cast<QWidget*>(sender());
	if (senderPanel) {
		layout_->removeWidget(senderPanel);
		senderPanel->deleteLater();
		layout_->update();
	}
}

void Panel::contextMenu(QPoint pos)
{
	QMenu menu(this);
	QAction* opt1 = menu.addAction("Load file");
	QAction* action = menu.exec(mapToGlobal(pos));

	if (action == opt1)
	{
		QString file_path = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Bed Files(*.bed);;Text Files (*.txt);;All Files (*)")); //

		if (file_path.isEmpty()) return;

		BedFile track;
		const QFileInfo info(file_path);
		if (info.isFile())
		{
			track.load(file_path);
			track.sort();

			/*
			 * TODO: send an error somehow
			 */
			if (track.chromosomes().count() != 1) return; // discard

			Track tr = {/*file path*/ file_path,
						/*filename*/  info.fileName(),
						/*BedFile*/   track};
			trackAdded(tr);
		}
	}


}

void Panel::clearLayout()
{
	if (!layout_) return;
	while (QLayoutItem* item = layout_->takeAt(0))
	{
		if (QWidget* widget = item->widget()) widget->deleteLater();
		delete item;
	}
}



