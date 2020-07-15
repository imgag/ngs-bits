#ifndef FILTERCASCADE_H
#define FILTERCASCADE_H

#include "VariantList.h"
#include "VcfFileHandler.h"
#include "CnvList.h"
#include "GeneSet.h"
#include "VariantType.h"
#include "BedpeFile.h"

#include <QVariant>
#include <QString>
#include <QList>
#include <QSharedPointer>
#include <QMap>
#include <QBitArray>
#include <QRegularExpression>

//Parameter type
enum FilterParameterType
{
	INT,
	DOUBLE,
	BOOL,
	STRING,
	STRINGLIST
};

//Parameter
struct CPPNGSSHARED_EXPORT FilterParameter
{
	//Convenience constructor
	FilterParameter(QString n, FilterParameterType t, QVariant v, QString d);
	//Returns the string representation of a parameter type.
	static QString typeAsString(FilterParameterType type);

	QString name;
	FilterParameterType type;
	QVariant value;
	QString description;

	QMap<QString, QString> constraints;
};

//Result of a filter cascade or a single filter. Passing variant are flagged 'true', non-passing variant 'false'.
class CPPNGSSHARED_EXPORT FilterResult
{
	public:
		///Default constructor
		FilterResult();
		///Constructor that pre-fills the datastructure.
		FilterResult(int variant_count, bool value=true);

		///Returns if the variant at @p index passed the filter.
		bool passing(int index) const
		{
			return pass[index];
		}

		///Returns the number of passing variants.
		int countPassing() const
		{
			return pass.count(true);
		}

		///Inverts the flags.
		void invert()
		{
			pass = ~pass;
		}

		///Resets the flags to all passing.
		void reset(bool value = true)
		{
			pass.fill(value);
		}

		///Read-write access to flags array.
		QBitArray& flags()
		{
			return pass;
		}

		///Read-only access to flags array.
		const QBitArray& flags() const
		{
			return pass;
		}

		///Remove variants that did not pass the filter (with 'false' flag).
		void removeFlagged(VariantList& variants);
		void removeFlagged( VcfFileHandler& variants);
        void removeFlagged(CnvList& cnvs);
        void removeFlagged(BedpeFile& svs);

		///Tag variants that did not pass the filter using the 'filter' column.
		void tagNonPassing(VariantList& variants, QByteArray tag, QByteArray description);
		void tagNonPassing( VcfFileHandler& variants, QByteArray tag, QString description);

	private:
		QBitArray pass;
};

//Base class for all filters
class CPPNGSSHARED_EXPORT FilterBase
{
	public:
		//Default constructor
		FilterBase();
		// Virtual destructor
		virtual ~FilterBase();

		//Returns the filter name (which is also used by the filter factory).
		const QString& name() const
		{
			return name_;
		}
		//Returns the filter subject.
		VariantType type() const
		{
			return type_;
		}
		//Returns the filter description (and optionally the parameter description).
		QStringList description(bool add_parameter_description = false) const;

		//Returns if the filter step is enabled (default is 'true').
		bool enabled() const
		{
			return enabled_;
		}
		//Toggles the 'enabled' state
		void toggleEnabled()
		{
			enabled_ = !enabled_;
		}

		//Sets a parameter (generic via a string)
		void setGeneric(const QString& name, const QString& value);
		//Sets a integer parameter
		void setInteger(const QString& name, int value);
		//Sets a boolean parameter
		void setBool(const QString& name, bool value);
		//Sets a double parameter
		void setDouble(const QString& name, double value);
		//Sets a byte array parameter
		void setString(const QString& name, const QString& value);
		//Sets a byte array parameter
		void setStringList(const QString& name, const QStringList& value);

		//Returns all parameters
		const QList<FilterParameter>& parameters() const
		{
			return params_;
		}

		//Overrides a constriant of a parameter
		void overrideConstraint(const QString& parameter_name, const QString& constraint_name, const QString& constraint_value);

		//Returns a text representation of the filter
		virtual QString toText() const = 0;

		//Applies the filter to a small variant list
		virtual void apply(const VariantList& variant_list, FilterResult& result) const;
		virtual void apply(const  VcfFileHandler& variants, FilterResult& result) const;

		//Applies the filter to a CNV list
		virtual void apply(const CnvList& variant_list, FilterResult& result) const;
		//Applies the filter to a SV list
		virtual void apply(const BedpeFile& variant_list, FilterResult& result) const;

	protected:
		FilterBase(const FilterBase& rhs) = delete;
		QString name_;
		VariantType type_;
		QStringList description_;
		QList<FilterParameter> params_;
		bool enabled_;

		//Returns a reference to the parameter, or throws an exception if it does not exist.
		FilterParameter& parameter(const QString& name);
		//Returns a const reference to the parameter, or throws an exception if it does not exist.
		const FilterParameter& parameter(const QString& name) const;

		//Checks that the data of the given parameter has the right type
		void checkParameterType(const QString& name, FilterParameterType type) const;
		//Returns a parameter as a double
		double getDouble(const QString& name, bool check_constraints = true) const;
		//Returns a parameter as an integer
		int getInt(const QString& name, bool check_constraints = true) const;
		//Returns a parameter as a boolean
		double getBool(const QString& name) const;
		//Returns a parameter as a string
		QString getString(const QString& name, bool check_constraints = true) const;
		//Returns a parameter as a string list
		QStringList getStringList(const QString& name, bool check_constraints = true) const;

		//Returns the column index, or throws an exception if the column does not exist
		int annotationColumn(const VariantList& variant_list, const QString& column, bool throw_if_missing=true) const;

		//Checks if the filter is registered
		void checkIsRegistered() const;
};

//Filter cascade that contains polymorphic filters and can apply them
class CPPNGSSHARED_EXPORT FilterCascade
{
	public:
		//Add a filter and takes ownership of the filter.
		void add(QSharedPointer<FilterBase> filter)
		{
			filters_.append(filter);
			errors_.clear();
		}

		//Remove a filter
		void removeAt(int i)
		{
			filters_.removeAt(i);
			errors_.clear();
		}

		//Returns the number of filters
		int count() const
		{
			return filters_.count();
		}

		//Read-only access to a filter
		const QSharedPointer<FilterBase> operator[](int index) const
		{
			return filters_[index];
		}

		//Read-write access to a filter
		QSharedPointer<FilterBase> operator[](int index)
		{
			return filters_[index];
		}

		//Remove all filters;
		void clear()
		{
			filters_.clear();
			errors_.clear();
		}

		//Move filter one position to the front.
		void moveUp(int index);

		//Move filter one position to the back.
		void moveDown(int index);

		//Applies the filter cascade to a small variant list.
		FilterResult apply(const VariantList& variants, bool throw_errors = true, bool debug_time = false) const;

		//Applies the filter cascade to a CNV list.
		FilterResult apply(const CnvList& cnvs, bool throw_errors = true, bool debug_time = false) const;

		//Applies the filter cascade to a SV list (BEDPE file).
		FilterResult apply(const BedpeFile& svs, bool throw_errors = true, bool debug_time = false) const;

		//Returns errors occured during filter application.
		QStringList errors(int index) const;

	private:
		QList<QSharedPointer<FilterBase>> filters_;
		mutable QVector<QStringList> errors_;
};

//Handles loading filters from filter INI files
class CPPNGSSHARED_EXPORT FilterCascadeFile
{
	public:
		//Returns all availiable filter names ("---" is used for separators).
		static QStringList names(QString filename);
		//Resurns a filter cascacde.
		static FilterCascade load(QString filename, QString filter);

	private:
		//Declare constructor away
		FilterCascadeFile() = delete;
};

//Regions filter (not derived from FilterBase!)
class CPPNGSSHARED_EXPORT FilterRegions
{
	public:
		static void apply(const VariantList& variants, const BedFile& regions, FilterResult& result);
		static void apply(const  VcfFileHandler& variants, const BedFile& regions, FilterResult& result);

	protected:
		FilterRegions() = delete;
};


// Factory that registers and creates filters by name
class CPPNGSSHARED_EXPORT FilterFactory
{
	public:
		//Creates a filter by identifier
		static QSharedPointer<FilterBase> create(const QString& name, const QStringList& parameters = QStringList());

		//Returns a complete list of supported filter names
		static QStringList filterNames();

		//Returns a complete list of supported filter names for a given subject
		static QStringList filterNames(VariantType subject);

	private:
		FilterFactory() = delete;
		static const QMap<QString, FilterBase*(*)()>& getRegistry();
};

//Allele-frequency filter
class CPPNGSSHARED_EXPORT FilterAlleleFrequency
	: public FilterBase
{
	public:
		FilterAlleleFrequency();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Allele-frequency filter of sub-populations
class CPPNGSSHARED_EXPORT FilterSubpopulationAlleleFrequency
	: public FilterBase
{
	public:
		FilterSubpopulationAlleleFrequency();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Genes filter
class CPPNGSSHARED_EXPORT FilterGenes
	: public FilterBase
{
	public:
		FilterGenes();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Filter column empty filter
class CPPNGSSHARED_EXPORT FilterFilterColumnEmpty
	: public FilterBase
{
	public:
		FilterFilterColumnEmpty();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
		void apply(const  VcfFileHandler& variants, FilterResult& result) const override;
};

//Filter column filter
class CPPNGSSHARED_EXPORT FilterFilterColumn
	: public FilterBase
{
	public:
		FilterFilterColumn();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//Returns if there is a filter column match
		bool match(const Variant& v) const;

		mutable QByteArrayList entries;
};

//Filter for SNPs
class CPPNGSSHARED_EXPORT FilterVariantIsSNP
	: public FilterBase
{
	public:
		FilterVariantIsSNP();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
		void apply(const  VcfFileHandler& variants, FilterResult& result) const override;
};

//Variant impact filter
class CPPNGSSHARED_EXPORT FilterVariantImpact
	: public FilterBase
{
	public:
		FilterVariantImpact();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};


//Variant type filter
class CPPNGSSHARED_EXPORT FilterVariantType
	: public FilterBase
{
	public:
		FilterVariantType();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};


//NGSD variant count filter
class CPPNGSSHARED_EXPORT FilterVariantCountNGSD
	: public FilterBase
{
	public:
		FilterVariantCountNGSD();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//NGSD classification filter
class CPPNGSSHARED_EXPORT FilterClassificationNGSD
	: public FilterBase
{
	public:
		FilterClassificationNGSD();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

protected:
		//Returns if there is a class column match
		bool match(const Variant& v) const;

		mutable QStringList classes;
		mutable int i_class;
};

//Gene inheritance filter
class CPPNGSSHARED_EXPORT FilterGeneInheritance
	: public FilterBase
{
	public:
		FilterGeneInheritance();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};


//Gene coinstraint filter
class CPPNGSSHARED_EXPORT FilterGeneConstraint
	: public FilterBase
{
	public:
		FilterGeneConstraint();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Genotype filter (control)
class CPPNGSSHARED_EXPORT FilterGenotypeControl
	: public FilterBase
{
	public:
		FilterGenotypeControl();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//Checks that all samples have the same genotype and returns it, or "" otherwise.
		QByteArray checkSameGenotype(const QList<int>& geno_indices, const Variant& v) const;
};

//Genotype filter (affected)
class CPPNGSSHARED_EXPORT FilterGenotypeAffected
	: public FilterBase
{
	public:
		FilterGenotypeAffected();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//Checks that all samples have the same genotype and returns it, or "" otherwise.
		QByteArray checkSameGenotype(const QList<int>& geno_indices, const Variant& v) const;
};

//Column empty filter
class CPPNGSSHARED_EXPORT FilterColumnMatchRegexp
	: public FilterBase
{
	public:
		FilterColumnMatchRegexp();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		bool match(const Variant& v) const;

		mutable int index;
		mutable QRegularExpression regexp;
};

//Pathogenic annotation filter
class CPPNGSSHARED_EXPORT FilterAnnotationPathogenic
	: public FilterBase
{
	public:
		FilterAnnotationPathogenic();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//Returns of the variant is annotated to be pathogenic
		bool annotatedPathogenic(const Variant& v) const;

		mutable bool also_likely_pathogenic;
		mutable int i_clinvar;
		mutable int i_hgmd;
};

//Pathogenic prediction filter
class CPPNGSSHARED_EXPORT FilterPredictionPathogenic
	: public FilterBase
{
	public:
		FilterPredictionPathogenic();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//Counts the number of pathogenic predictions
		bool predictedPathogenic(const Variant& v) const;

		mutable int min;
		mutable int i_phylop;
		mutable int i_sift;
		mutable int i_polyphen;
		mutable int i_fathmm;
		mutable int i_cadd;
		mutable int i_revel;
};

//Annotation text filter
class CPPNGSSHARED_EXPORT FilterAnnotationText
	: public FilterBase
{
	public:
		FilterAnnotationText();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		bool match(const Variant& v) const;

		mutable QByteArray term;
};

//Variant QC filter
class CPPNGSSHARED_EXPORT FilterVariantQC
	: public FilterBase
{
	public:
		FilterVariantQC();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Trio filter
class CPPNGSSHARED_EXPORT FilterTrio
	: public FilterBase
{
	public:
		FilterTrio();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;

	protected:
		//returns genotypes corrected by allele frequency
		void correctedGenotypes(const Variant& v, QByteArray& geno_c, QByteArray& geno_f, QByteArray& geno_m) const;

		mutable int i_quality;
		mutable int i_c;
		mutable int i_f;
		mutable int i_m;
		mutable int i_af_c;
		mutable int i_af_f;
		mutable int i_af_m;
};

//OMIM filter
class CPPNGSSHARED_EXPORT FilterOMIM
	: public FilterBase
{
	public:
		FilterOMIM();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Conservedness filter
class CPPNGSSHARED_EXPORT FilterConservedness
	: public FilterBase
{
	public:
		FilterConservedness();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

//Filter for reglatory variants
class CPPNGSSHARED_EXPORT FilterRegulatory
	: public FilterBase
{
	public:
		FilterRegulatory();
		QString toText() const override;
		void apply(const VariantList& variants, FilterResult& result) const override;
};

/*************************************************** filters for CNVs ***************************************************/
//Filter CNV size
class CPPNGSSHARED_EXPORT FilterCnvSize
	: public FilterBase
{
	public:
		FilterCnvSize();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

//Filter CNV regions
class CPPNGSSHARED_EXPORT FilterCnvRegions
	: public FilterBase
{
	public:
		FilterCnvRegions();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

//Filter CNV copy-number
class CPPNGSSHARED_EXPORT FilterCnvCopyNumber
	: public FilterBase
{
	public:
		FilterCnvCopyNumber();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

//Filter CNV allele frequency
class CPPNGSSHARED_EXPORT FilterCnvAlleleFrequency
	: public FilterBase
{
	public:
		FilterCnvAlleleFrequency();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};


//Filter CNV z-score
class CPPNGSSHARED_EXPORT FilterCnvZscore
	: public FilterBase
{
	public:
		FilterCnvZscore();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};


//Filter CNV log-likelihood
class CPPNGSSHARED_EXPORT FilterCnvLoglikelihood
	: public FilterBase
{
	public:
		FilterCnvLoglikelihood();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

//Filter CNV q-value
class CPPNGSSHARED_EXPORT FilterCnvQvalue
	: public FilterBase
{
	public:
		FilterCnvQvalue();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};


//Filter CNV compound-heterozygous
class CPPNGSSHARED_EXPORT FilterCnvCompHet
	: public FilterBase
{
	public:
		FilterCnvCompHet();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;

		//Returns the list of genes that have one heterzygous hit in the small variant list.
		const GeneSet& hetHitGenes() const
		{
			return het_hit_genes_;
		}

		//Sets the list of genes that have one heterzygous hit in the small variant list.
		void setHetHitGenes(const GeneSet& het_var_genes) const
		{
			het_hit_genes_ = het_var_genes;
		}

	protected:
		mutable GeneSet het_hit_genes_;
};


//Filter CNVs for OMIM annotation
class CPPNGSSHARED_EXPORT FilterCnvOMIM
	: public FilterBase
{
	public:
		FilterCnvOMIM();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};


//Filter CNVs for overlap with CNP regions
class CPPNGSSHARED_EXPORT FilterCnvCnpOverlap
	: public FilterBase
{
	public:
		FilterCnvCnpOverlap();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

//Filter CNVs for gene constraint
class CPPNGSSHARED_EXPORT FilterCnvGeneConstraint
	: public FilterBase
{
	public:
		FilterCnvGeneConstraint();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};


//Filter CNVs for gene overlap region
class CPPNGSSHARED_EXPORT FilterCnvGeneOverlap
	: public FilterBase
{
	public:
		FilterCnvGeneOverlap();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;

		QByteArrayList selectedOptions() const;
};

//Filter CNVs for overlaps with pathogenic CNVs
class CPPNGSSHARED_EXPORT FilterCnvPathogenicCnvOverlap
	: public FilterBase
{
	public:
		FilterCnvPathogenicCnvOverlap();
		QString toText() const override;
		void apply(const CnvList& cnvs, FilterResult& result) const override;
};

/*************************************************** filters for SVs ***************************************************/

// Filter SVs for SV type
class CPPNGSSHARED_EXPORT FilterSvType
	: public FilterBase
{
	public:
		FilterSvType();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for special chromosomes
class CPPNGSSHARED_EXPORT FilterSvRemoveChromosomeType
	: public FilterBase
{
	public:
		FilterSvRemoveChromosomeType();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for genotype
class CPPNGSSHARED_EXPORT FilterSvGenotype
	: public FilterBase
{
	public:
		FilterSvGenotype();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for quality
class CPPNGSSHARED_EXPORT FilterSvQuality
	: public FilterBase
{
	public:
		FilterSvQuality();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for filter column
class CPPNGSSHARED_EXPORT FilterSvFilterColumn
	: public FilterBase
{
	public:
		FilterSvFilterColumn();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for paired read AF
class CPPNGSSHARED_EXPORT FilterSvPairedReadAF
	: public FilterBase
{
	public:
		FilterSvPairedReadAF();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for split read AF
class CPPNGSSHARED_EXPORT FilterSvSplitReadAF
	: public FilterBase
{
	public:
		FilterSvSplitReadAF();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for paired end read depth
class CPPNGSSHARED_EXPORT FilterSvPeReadDepth
	: public FilterBase
{
	public:
		FilterSvPeReadDepth();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for Somaticscore
class CPPNGSSHARED_EXPORT FilterSvSomaticscore
	: public FilterBase
{
	public:
		FilterSvSomaticscore();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for gnomad oe lof score
class CPPNGSSHARED_EXPORT FilterSvGeneConstraint
	: public FilterBase
{
	public:
		FilterSvGeneConstraint();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

// Filter SVs for gene overlap
class CPPNGSSHARED_EXPORT FilterSvGeneOverlap
	: public FilterBase
{
	public:
		FilterSvGeneOverlap();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;

		QByteArrayList selectedOptions() const;
};

//Filter SV size
class CPPNGSSHARED_EXPORT FilterSvSize
	: public FilterBase
{
	public:
		FilterSvSize();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

//Filter SVs for OMIM annotation
class CPPNGSSHARED_EXPORT FilterSvOMIM
	: public FilterBase
{
	public:
		FilterSvOMIM();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

//Filter SV compound-heterozygous
class CPPNGSSHARED_EXPORT FilterSvCompHet
	: public FilterBase
{
	public:
		FilterSvCompHet();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;

		//Returns the list of genes that have one heterzygous hit in the small variant list.
		const GeneSet& hetHitGenes() const
		{
			return het_hit_genes_;
		}

		//Sets the list of genes that have one heterzygous hit in the small variant list.
		void setHetHitGenes(const GeneSet& het_var_genes) const
		{
			het_hit_genes_ = het_var_genes;
		}

	protected:
		mutable GeneSet het_hit_genes_;
};

//NGSD SV count filter
class CPPNGSSHARED_EXPORT FilterSvCountNGSD
	: public FilterBase
{
	public:
		FilterSvCountNGSD();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

//NGSD SV count filter
class CPPNGSSHARED_EXPORT FilterSvAfNGSD
	: public FilterBase
{
	public:
		FilterSvAfNGSD();
		QString toText() const override;
		void apply(const BedpeFile& svs, FilterResult& result) const override;
};

#endif // FILTERCASCADE_H
