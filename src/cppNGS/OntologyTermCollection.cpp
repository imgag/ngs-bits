#include "OntologyTermCollection.h"
#include "ToolBase.h"
#include "Helper.h"


OntologyTerm::OntologyTerm()
	:id_("")
	,name_("")
	,def_("")
	,parent_ids_()
	,is_obsolete_(false)
{
}

OntologyTerm::OntologyTerm(QByteArray& id, QByteArray& name, QByteArray& def, QList<QByteArray>& is_as, bool is_obsolete)
	:id_(id)
	,name_(name)
	,def_(def)
	,parent_ids_(is_as)
	,is_obsolete_(is_obsolete)
{
}

bool OntologyTerm::isChildOf(const QByteArray& parent_id) const
{
	foreach(const QByteArray& parent_term, parent_ids_)
	{
		if(parent_term.contains(parent_id))
		{
			return true;
		}
	}
	return false;
}


OntologyTermCollection::OntologyTermCollection()
{
}

//read complete obo file
OntologyTermCollection::OntologyTermCollection(const QByteArray &filename,bool skip_obsolete)
{
	QSharedPointer<QFile> fp = Helper::openFileForReading(filename);

	while(!fp->atEnd())
	{
		OntologyTerm temp;
		QByteArray line = fp->readLine().trimmed();

		//A new set of terms always begins with [Term] followed by a set of tag:value pairs
		if(line == "[Term]")
		{
			line = fp->readLine().trimmed();

			do
			{
				if(line.startsWith("id"))
					temp.setId(line.mid(4));
				if(line.startsWith("name"))
					temp.setName(line.mid(6));
				if(line.startsWith("def"))
					temp.setDefinition(line.mid(5));
				if(line.startsWith("is_a"))
					temp.addParentID(line.mid(6));

				if(line.startsWith("is_obsolete"))
				{
					if(line.contains("true"))
						temp.setIsObsolete(true);
					else
						temp.setIsObsolete(false);
				}
				line = fp->readLine().trimmed();

			}while(!line.isEmpty());
			if(temp.isObsolete() && skip_obsolete)
				continue;
			add(temp);
		}
	}
	fp->close();
}

const OntologyTerm& OntologyTermCollection::findByID(const QByteArray& id)
{
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.id() == id)
		{
			return term;
		}
	}
	THROW(ArgumentException, "No term with id "+ id + " found.");
}

bool OntologyTermCollection::containsByID(const QByteArray& id)
{
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.id() == id)
		{
			return true;
		}
	}
	return false;
}

bool OntologyTermCollection::containsByName(const QByteArray& name) const
{
	//some variant files use shortened and altered variant_type names: correct to SO nomenclature
	QByteArray utr_name;
	try
	{
		utr_name = utr_name.replace("'","_prime_variant");
	}
	catch(...)
	{
		utr_name = name;
	}

	QByteArray var_name = name + "_variant";
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.name() == name || term.name() == utr_name || term.name() == var_name)
		{
			return true;
		}
	}

	return false;
}

QList<QByteArray> OntologyTermCollection::childIDs(const QByteArray& term_id, bool recursive)
{
	QList<QByteArray> ids;
	foreach(const OntologyTerm& term, ontology_terms_)
	{
		if(term.isChildOf(term_id))
		{
			ids.append(term.id());
			if(recursive)
			{
				ids.append(childIDs(term.id(), true));
			}
		}

	}
	return ids;
}


