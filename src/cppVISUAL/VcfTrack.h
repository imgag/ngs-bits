#ifndef VCFTRACK_H
#define VCFTRACK_H

#include "TrackWidget.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "TabixIndexedFile.h"
#include <memory>

struct CPPVISUALSHARED_EXPORT VcfTrackSettings : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double min_qual MEMBER min_qual)
	Q_PROPERTY(bool pass_only MEMBER pass_only)
	Q_PROPERTY(QByteArray sample MEMBER sample)

public:
	double min_qual{};
	bool pass_only{};
	QByteArray sample;
};

//Displays VCF sites and sample genotypes. Indexed VCF.GZ data is loaded per visible region.
class CPPVISUALSHARED_EXPORT VcfTrack : public TrackWidget
{
	Q_OBJECT
public:
	VcfTrack(QWidget* parent, QString file_path, QString name);
	static QByteArray type() { return "VcfTrack"; }
	static VcfTrack* createTrack(QWidget* parent, QString file_path, QString name);
	void reloadTrack() override;
	QSize sizeHint() const override;

protected:
	QList<Parameter> getParameters() const override;
	QHash<QByteArray, QVariant> getParameterDefaults() const override;
	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent*) override;
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	bool event(QEvent*) override;

private:
	struct Genotype
	{
		QByteArray text;
		QVector<int> alleles; //Zero is REF, positive values index ALT, -1 is missing.
	};
	struct Variant
	{
		Chromosome chr;
		int start;
		int end;
		double qual;
		QByteArrayList columns;
		QVector<Genotype> genotypes;
	};
	struct Placement
	{
		int index;
		QRectF rect;
	};
	VcfTrackSettings fast_settings_;
	QByteArrayList samples_;
	int column_count_ = 8;
	QVector<Variant> variants_;
	BedFile positions_;
	std::unique_ptr<ChromosomalIndex<BedFile>> chr_index_;
	std::unique_ptr<TabixIndexedFile> tabix_;
	QVector<Placement> placements_;
	int rows_ = 0;
	int sample_index_ = -1;
	QString load_error_;
	QPoint mouse_press_pos_;

	void loadSource();
	Variant parseVariant(const QByteArray& line) const;
	void setVariants(QVector<Variant> variants);
	QVector<Variant> queryRegion(const TabixIndexedFile& tabix) const;
	void updateLayout();
	//Deleted reference bases, including the replaced part of delins alleles.
	//Allele -1 returns the combined span if all alternate alleles delete reference bases.
	QPair<int, int> deletionSpan(const Variant& variant, int allele = -1) const;
	QString variantText(const Variant& variant) const;
	const Placement* placementAt(const QPoint& pos) const;
	QByteArrayList sampleChoices() const;
};

#endif // VCFTRACK_H
