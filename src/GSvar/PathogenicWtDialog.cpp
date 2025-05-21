#include "PathogenicWtDialog.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include <QMenu>

PathogenicWtDialog::PathogenicWtDialog(QWidget* parent, QString bam, bool is_long_read)
	: QDialog(parent)
	, ui_()
	, bam_(bam)
	, is_long_read_(is_long_read)
	, delayed_init_timer_(this, true)
{
	ui_.setupUi(this);
	connect(ui_.variants, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(variantsContextMenu(QPoint)));
}

void PathogenicWtDialog::delayedInitialization()
{
	BamReader reader(bam_);
	for (int row=0; row<ui_.variants->rowCount(); ++row)
	{
		Variant variant = Variant::fromString(ui_.variants->item(row, 0)->text());


		Pileup pileup = reader.getPileup(variant.chr(), variant.start(), -1, 1, is_long_read_);

		//determine AF
		long long depth = pileup.countOf(variant.obs()[0]) + pileup.countOf(variant.ref()[0]);
		long long alt = pileup.countOf(variant.obs()[0]);
		double af = (double)alt/(double)depth;

		//determine genotype
		QString geno = "n/a";
		if (depth>10)
		{
			if (af>0.9) geno = "hom";
			else if (af>0.1) geno = "het";
			else geno = "WT";
			geno += " (af=" + QString::number(af, 'f', 4)+")";
		}

		//create table items
		ui_.variants->setItem(row, 3, GUIHelper::createTableItem(QString::number(depth), Qt::AlignLeft|Qt::AlignVCenter));

		auto item = GUIHelper::createTableItem(geno, Qt::AlignLeft|Qt::AlignVCenter);
        if (geno.startsWith("WT")) item->setBackground(QBrush(QColor(QColor(255,0, 0, 128))));
		ui_.variants->setItem(row, 4, item);
	}

	GUIHelper::resizeTableCellWidths(ui_.variants);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.variants);
}

void PathogenicWtDialog::variantsContextMenu(QPoint pos)
{
	if (ui_.variants->selectedRanges().count()!=1) return;

	QMenu menu;
	menu.addAction(QIcon(":/Icons/NGSD_variant.png"), "Open variant tab");

	QAction* action = menu.exec(ui_.variants->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	int row = ui_.variants->selectedRanges()[0].topRow();
	Variant variant = Variant::fromString(ui_.variants->item(row, 0)->text());
	GlobalServiceProvider::openVariantTab(variant);
}
