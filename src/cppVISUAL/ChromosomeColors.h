#ifndef CHROMOSOMECOLORS_H
#define CHROMOSOMECOLORS_H

#include "cppVISUAL_global.h"
#include <QHash>
#include <QColor>


struct CPPVISUALSHARED_EXPORT ChromosomeColors
{
public:
	static QColor getColor(QString chr)
	{
		QString base_chr = chr.split("_")[0];
		if (instance()->color_map_.contains(base_chr)) return instance()->color_map_[base_chr];
		return QColor(0, 0, 0);
	}

private:
	QHash<QString, QColor> color_map_;

	static ChromosomeColors* instance()
	{
		static ChromosomeColors s;
		return &s;
	}

	ChromosomeColors(){
		color_map_["chrX"] =  QColor(204, 153, 0);
		color_map_["chrY"] =  QColor(153, 204, 0);
		color_map_["chrUn"] = QColor(150, 150, 150);
		color_map_["chr1"] =  QColor(80, 80, 255);
		color_map_["chrI"] =  QColor(139, 155, 187);
		color_map_["chr2"] =  QColor(206, 61, 50);
		color_map_["chrII"] =  QColor(206, 61, 50);
		color_map_["chr2a"] =  QColor(206, 61, 5);
		color_map_["chr2b"] =  QColor(206, 61, 50);
		color_map_["chr3"] =  QColor(116, 155, 88);
		color_map_["chrIII"] =  QColor(116, 155, 88);
		color_map_["chr4"] =  QColor(240, 230, 133);
		color_map_["chrIV"] =  QColor(240, 230, 133);
		color_map_["chr5"] =  QColor(70, 105, 131);
		color_map_["chr6"] =  QColor(186, 99, 56);
		color_map_["chr7"] =  QColor(93, 177, 221);
		color_map_["chr8"] =  QColor(128, 34, 104);
		color_map_["chr9"] =  QColor(107, 215, 107);
		color_map_["chr10"] =  QColor(213, 149, 167);
		color_map_["chr11"] =  QColor(146, 72, 34);
		color_map_["chr12"] =  QColor(131, 123, 141);
		color_map_["chr13"] =  QColor(199, 81, 39);
		color_map_["chr14"] =  QColor(213, 143, 92);
		color_map_["chr15"] =  QColor(122, 101, 165);
		color_map_["chr16"] =  QColor(228, 175, 105);
		color_map_["chr17"] =  QColor(59, 27, 83);
		color_map_["chr18"] =  QColor(205, 222, 183);
		color_map_["chr19"] =  QColor(97, 42, 121);
		color_map_["chr20"] =  QColor(174, 31, 99);
		color_map_["chr21"] =  QColor(231, 199, 111);
		color_map_["chr22"] =  QColor(90, 101, 94);
		color_map_["chr23"] =  QColor(204, 153, 0);
		color_map_["chr24"] =  QColor(153, 204, 0);
		color_map_["chr25"] =  QColor(51, 204, 0);
		color_map_["chr26"] =  QColor(0, 204, 51);
		color_map_["chr27"] =  QColor(0, 204, 153);
		color_map_["chr28"] =  QColor(0, 153, 204);
		color_map_["chr29"] =  QColor(10, 71, 255);
		color_map_["chr30"] =  QColor(71, 117, 255);
		color_map_["chr31"] =  QColor(255, 194, 10);
		color_map_["chr32"] =  QColor(255, 209, 71);
		color_map_["chr33"] =  QColor(153, 0, 51);
		color_map_["chr34"] =  QColor(153, 26, 0);
		color_map_["chr35"] =  QColor(153, 102, 0);
		color_map_["chr36"] =  QColor(128, 153, 0);
		color_map_["chr37"] =  QColor(51, 153, 0);
		color_map_["chr38"] =  QColor(0, 153, 26);
		color_map_["chr39"] =  QColor(0, 153, 102);
		color_map_["chr40"] =  QColor(0, 128, 153);
		color_map_["chr41"] =  QColor(0, 51, 153);
		color_map_["chr42"] =  QColor(26, 0, 153);
		color_map_["chr43"] =  QColor(102, 0, 153);
		color_map_["chr44"] =  QColor(153, 0, 128);
		color_map_["chr45"] =  QColor(214, 0, 71);
		color_map_["chr46"] =  QColor(255, 20, 99);
		color_map_["chr47"] =  QColor(0, 214, 143);
		color_map_["chr48"] =  QColor(20, 255, 177);
	}
};

#endif // CHROMOSOMECOLORS_H
