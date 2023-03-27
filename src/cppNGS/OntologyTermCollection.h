#ifndef ONTOLOGYTERMCOLLECTION_H
#define ONTOLOGYTERMCOLLECTION_H

#include "cppNGS_global.h"
#include <QByteArray>
#include <QString>
#include <QList>

///class represents a single Ontology Term
class CPPNGSSHARED_EXPORT OntologyTerm
{
public:
	OntologyTerm();
	OntologyTerm(QByteArray& id, QByteArray& name, QByteArray& definition, QList<QByteArray>& is_as, bool is_obsolete);

	const QByteArray& id() const
	{
		return id_;
	}
	void setId(const QByteArray& id)
	{
		id_ = id;
	}

	const QByteArray& name() const
	{
		return name_;
	}
	void setName(const QByteArray& name)
	{
		name_ = name;
	}

	const QByteArray& definition() const
	{
		return def_;
	}
	void setDefinition(const QByteArray& def)
	{
		def_ = def;
	}

	const QByteArray& type() const
	{
		return type_;
	}
	void setType(const QByteArray& type)
	{
		type_ = type;
	}

	const QByteArrayList& synonyms() const
	{
		return synonyms_;
	}
	void setSynonyms(const QByteArrayList& synonyms)
	{
		synonyms_ = synonyms;
	}
	void addSynonym(const QByteArray& synonym)
	{
		synonyms_ << synonym;
	}

	const QByteArrayList& parentIDs() const
	{
		return parent_ids_;
	}
	void setParentIDs(const QList<QByteArray>& parent_ids)
	{
		parent_ids_ = parent_ids;
	}
	void addParentID(const QByteArray& parent_id)
	{
		parent_ids_.append(parent_id);
	}

	bool isObsolete() const
	{
		return is_obsolete_;
	}
	void setIsObsolete(bool is_obsolete)
	{
		is_obsolete_ = is_obsolete;
	}

	const QByteArray& replacedById() const
	{
		return replaced_by_id_;
	}
	void setReplacedById(const QByteArray& id)
	{
		replaced_by_id_ = id.trimmed();
	}

	///checks whether this term is a child of the passed parent ID
	bool isChildOf(const QByteArray& parent_id) const;

private:
	QByteArray id_;
	QByteArray name_;
	QByteArray def_;
	QByteArray type_;
	QByteArrayList synonyms_;
	QByteArrayList parent_ids_;
	bool is_obsolete_;
	QByteArray replaced_by_id_;
};

///represents a collection of Ontology Terms and provides methods for parsing obo files
class CPPNGSSHARED_EXPORT OntologyTermCollection
{
public:
	///Default constructor
	OntologyTermCollection();
	///Constructor parses given OBO file.
	OntologyTermCollection(QString obo_file, bool skip_obsolete_terms);

	///Adds a term. Throws an exception of a term with the sampe ID is already present.
	void add(const OntologyTerm& term);

	///check whether Collection contains term with given id
	bool containsByID(const QByteArray& id);
	///check whether Collection contains term with given name
	bool containsByName(const QByteArray& name) const;

	///Returns a term by index.
	const OntologyTerm& get(int index);
	///finds obo term by ID in collection, if not found ArgumentException is thrown
	const OntologyTerm& getByID(const QByteArray& id);

	///Returns the child IDs of the given term, if recursive is true all descendants are included.
	QList<QByteArray> childIDs(const QByteArray& term_id, bool recursive);

	///Returns the number of terms
	int size()
	{
		return ontology_terms_.length();
	}

private:
	QList<OntologyTerm> ontology_terms_;
};

#endif // ONTOLOGYTERMCOLLECTION_H
