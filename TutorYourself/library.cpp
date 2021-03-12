#include <QtWidgets>
#include <QtXml>
#include "library.h"
#include "stage.h"
#include "probabilityrandom.h"

WordProperties::WordProperties(WordClass wordClass,QString meaning, QString translation, QStringList forms, QStringList contexts)
	:wordClass(wordClass), meaning(meaning), translation(translation), forms(forms), contexts(contexts) {}

bool WordProperties::operator==(WordProperties other)
{
	if (wordClass == other.wordClass &&
		meaning == other.meaning &&
		translation == other.translation &&
		forms == other.forms &&
		contexts == other.contexts)
			return true;
	return false;
}

WordProperties::WordProperties(const WordProperties &properties)
{
	meaning = properties.meaning;
	translation = properties.translation;
	forms = properties.forms;
	contexts = properties.contexts;
	wordClass = properties.wordClass;
}

void Element::defineWord()
{
	if (_isWord)
		return;
	_isWord = true;
	wordProperties.wordClass = Other;
	changeWordsCount(_parent, 1);
}

void Element::undefWord()
{
	if (!_isWord)
		return;
	_isWord = false;
	changeWordsCount(_parent, -1);
}

Element::Element(bool isWord, bool enabled, const QString &name)
{
	this->name = name;
	this->_enabled = enabled;
	_parent = 0;
	_isWord = isWord;
}

Element::Element(const QString &name, bool enabled, WordProperties wordProperties)
	:name(name),_enabled(enabled),wordProperties(wordProperties)
{
	_parent = 0;
	_isWord = true;
}

Element::Element(const QString &name, bool enabled, GroupProperties groupProperties)
{
	this->name = name;
	this->_enabled = enabled;
	this->groupPropeties = groupProperties;
	_parent = 0;
	_isWord = false;
}

Element::~Element()
{
	qDeleteAll(_children);
}

Element * Element::parent() const
{
	return _parent;
}

QList<Element*> Element::children() const
{
	return _children;
}

QList<Element*> Element::setChildren(QList<Element*> &elements)
{
	if (_isWord)
		return QList<Element*>();

	QList<Element*> oldChildren = _children;

	for (int i = 0; i < _children.count(); i++)
		removeChildren(*_children.begin());


	QList<Element*>::iterator iterator = elements.begin();
	while (iterator != elements.end())
	{
		addChildren(*iterator, iterator - elements.begin());
		iterator++;
	}

	return oldChildren;

}

void Element::addChildren(Element * element,int row)
{
	if (_children.contains(element))
		return;
	
	if (element->_parent != 0)
		element->_parent->removeChildren(element);

	element->_parent = this;

	int count = element->_isWord ? 1 : element->groupPropeties.wordsCount;
	changeWordsCount(this, count);

	_children.insert(row,element);
}

void Element::removeChildren(Element * element)
{
	if (!_children.contains(element))
		return;

	int count = element->_isWord ? 1 : element->groupPropeties.wordsCount;
	changeWordsCount(this, -count);

	element->_parent = 0;
	_children.removeOne(element);
	
}

void Element::removeChildren(int row)
{
	if (row >= 0 && row < _children.count())
		removeChildren(_children[row]);
}

void Element::setParent(Element * element)
{
	if (_parent != 0)
		_parent->removeChildren(this);
	if (element != 0)
	{
		element->addChildren(this);
		return;
	}
	_parent = 0;
}

//BEGIN EDIT ON 11.24.2017
void Element::setEnabled(bool enabled)
{
	setParentEnabled(enabled);
	setChildEnabled(enabled);
}

void Element::setChildEnabled(bool enabled)
{
	_enabled = enabled;
	foreach(Element *elem, _children)
	{
		elem->setChildEnabled(enabled);
	}
}

void Element::setParentEnabled(bool enabled)
{
	if (enabled == _enabled)
		return;
	_enabled = enabled;
	if (enabled && _parent != 0)
		_parent->setParentEnabled(enabled);
}
//END EDIT OLD VERDION:
/*
void Element::setEnabled(bool enabled)
{
if (enabled == _enabled)
return;
_enabled = enabled;

if (enabled)
{
if (_parent != 0)
_parent->setEnabled(true);
}
else
{
foreach(Element *elem, _children)
{
elem->setEnabled(false);
}
}
}*/

void Element::changeWordsCount(Element * element, int difference)
{
	Element* parentInc = element;
	while (parentInc != 0)
	{
		parentInc->groupPropeties.wordsCount += difference;
		parentInc = parentInc->_parent;
	}
}

LibraryModel::LibraryModel(QObject *parent)
	:QAbstractItemModel(parent)
{
	rootElement = 0;
	document = 0;

	verbs = 0;
	adjectives = 0;
	nouns = 0;
	adverbs = 0;
	others = 0;
}

LibraryModel::~LibraryModel()
{
	if (verbs)
	{
		delete verbs;
		delete adjectives;
		delete nouns;
		delete adverbs;
		delete others;
	}

	delete rootElement;
}

void LibraryModel::setDocument(Element *document)
{
	beginResetModel();
	delete rootElement;

	if (verbs)
	{
		delete verbs;
		delete adjectives;
		delete nouns;
		delete adverbs;
		delete others;
	}

	verbs = new QMap<QModelIndex, Element *>;
	adjectives = new QMap<QModelIndex, Element *>;
	nouns = new QMap<QModelIndex, Element *>;
	adverbs = new QMap<QModelIndex, Element *>;
	others = new QMap<QModelIndex, Element *>;

	rootElement = new Element(false,true,"");
	document->setParent(rootElement);
	this->document = document;
	fillLists(document, 0, QModelIndex());
	endResetModel();
}
QModelIndex LibraryModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!rootElement)
		return QModelIndex();
	Element *parentElement = elementFromIndex(parent);
	if (row < parentElement->children().count() && row >= 0)
	{
		Element *element = parentElement->children()[row];
		QModelIndex elementIndex = createIndex(row, column, element);
		if (element->isWord() && areWordPropertiesFull(element->wordProperties))
		{
			QMap<QModelIndex, Element*> *words = getWordClassMap(element);
			if (!words->values().contains(element))
				words->insert(elementIndex, element);
		}
		return elementIndex;
	}
	return QModelIndex();
}

QModelIndex LibraryModel::parent(const QModelIndex & child) const
{
	Element *element = elementFromIndex(child);
	if (!element)
		return QModelIndex();
	Element* parentElement = element->parent();
	if (!parentElement)
		return QModelIndex();
	Element *grandparentElement = parentElement->parent();
	if (!grandparentElement)
		return QModelIndex();
	int row = grandparentElement->children().indexOf(parentElement);
	return createIndex(row, child.column(), parentElement);
}

int LibraryModel::rowCount(const QModelIndex & parent) const
{
	Element *parentElement = elementFromIndex(parent);
	if (!parentElement)
		return 0;
	return parentElement->children().count();
}

int LibraryModel::columnCount(const QModelIndex & parent) const
{
	return 1;
}

QVariant LibraryModel::data(const QModelIndex & index, int role) const
{
	Element *element = elementFromIndex(index);
	if (!element)
		return QVariant();
	switch (role)
	{
	case WordRole:
	{
		if (!element->isWord())
			break;
		QVariant variant = QVariant::fromValue(element->wordProperties);
		return variant;
	}
	case GroupRole:
	{
		if (element->isWord())
			break;
		QVariant variant = QVariant::fromValue(element->groupPropeties);
		return variant;
	}
	case Qt::DisplayRole:
	case Qt::EditRole:
		return element->name;
	}
	return QVariant();
}

QVariant LibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return QVariant();
}

Qt::ItemFlags LibraryModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
}

bool LibraryModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (index.isValid())
	{
		QVector<int> roles;
		roles.append(role);
		Element *element = static_cast<Element*>(index.internalPointer());
		switch (role)
		{
		case Qt::EditRole:
		{
			QString str = value.toString();
			element->name = str;
			break;
		}
		case WordRole:
		{
			if (value.canConvert<WordProperties>())
			{
				WordProperties wordProperties = value.value<WordProperties>();

				if (!element->isWord())
				{
					element->defineWord();
					this->index(index.row(), 0, index.parent());
				}

				if (element->wordProperties.wordClass != wordProperties.wordClass && areWordPropertiesFull(wordProperties))
				{
					QMap<QModelIndex, Element*> *words = getWordClassMap(element);
					if (words->values().contains(element))
						words->remove(index);
					words = getWordClassMap(wordProperties.wordClass);
					words->insert(index, element);
				}
				element->wordProperties = wordProperties;
			}
			break;
		}
		default:
			return false;
		}
		emit dataChanged(index, index, roles);
		return true;
	}
	return false;
}

bool LibraryModel::insertRows(int row, int count, const QModelIndex & parent)
{
	if (count <= 0 || row < 0)
		return false;
	if (!parent.isValid())
		return false;
	Element *element = static_cast<Element*>(parent.internalPointer());
	if (!element || element->isWord())
		return false;

	int pos1 = 0, pos2 = count, parentRowCount = rowCount(parent);
	if (row > 0 && row < parentRowCount)
	{
		pos1 = row;
		pos2 += row;
	}
	else if (row >= parentRowCount)
	{
		pos1 = parentRowCount;
		pos2 += parentRowCount;
	}
	beginInsertRows(parent, pos1, pos2 - 1);
	
	for (int i = pos1; i < pos2; i++)
	{
		element->addChildren(new Element(false, element->enabled(), "New Group"),i);
	}
	endInsertRows();
	emit dataChanged(index(pos1, 0, parent), index(pos2 - 1, 0, parent));
	return true;
}

bool LibraryModel::removeRows(int row, int count, const QModelIndex & parent)
{
	if (count <= 0 || row < 0)
		return false;
	int parenRowCount = rowCount(parent);
	if (row + count - 1 >= parenRowCount)
		return false;
	Element *element = elementFromIndex(parent);
	if (!element || element == rootElement)
		return false;
	beginRemoveRows(parent, row,row + count - 1);
	for (int i = row; i < row + count; i++)
	{
		Element *child = element->children()[row];
		if (child->isWord())
		{
			QMap<QModelIndex, Element*> *words = getWordClassMap(child);
			if (words->values().contains(child))
				words->remove(parent.child(row,0));
		}
		element->removeChildren(row);
	}

	int childrenCount = element->children().count();

	for (int i = row; i < childrenCount; i++)
	{
		Element *child = element->children()[i];
		if (child->isWord() && areWordPropertiesFull(child->wordProperties))
		{
			QMap<QModelIndex, Element*> *words = getWordClassMap(child);
			if (words->values().contains(child))
			{
				words->remove(words->key(child));
				index(i, 0, parent);
			}
		}
	}
	endRemoveRows();
	return true;
}

bool LibraryModel::changeDocumentName(const QString &string)
{
	if (rootElement == 0)
		return false;
	QModelIndex docIndex = documentIndex();
	setData(docIndex, string, Qt::EditRole);
	return true;
}

const QString LibraryModel::documentName()
{
	if(!rootElement)
		return QString();
	return document->name;
}

QModelIndex LibraryModel::documentIndex() const
{
	if (rootElement)
		return index(rootElement->children().indexOf(document), 0, QModelIndex());
	return QModelIndex();
}

QList<Stage*> LibraryModel::getStages(int runFlags)
{

	bool includeVerbs = runFlags & IncludeVerbs;
	bool includeAdjectives = runFlags & IncludeAdjectives;
	bool includeNouns = runFlags & IncludeNouns;
	bool includeAdverbs = runFlags & IncludeAdverbs;
	bool includeOthers = runFlags & IncludeOthers;
	bool includeTranslations = runFlags & IncludeTranslations;
	bool includeMeanings = runFlags & IncludeMeanings;
	bool includeContexts = runFlags & IncludeContexts;
	bool mixWordClasses = runFlags & MixWordClasses;
	bool askForWord = runFlags & AskForWord;
	bool askForInterpretation = runFlags & AskForInterpretation;
	bool includeInputting = runFlags & IncludeInputting;
	



	QList<Stage*> stages;
	QMap<QModelIndex, Element *> words;
	QMap<QModelIndex, Element *> enabledVerbs = getEnabledFromMap(*verbs);
	QMap<QModelIndex, Element *> enabledAdjectives = getEnabledFromMap(*adjectives);
	QMap<QModelIndex, Element *> enabledNouns = getEnabledFromMap(*nouns);
	QMap<QModelIndex, Element *> enabledAdverbs = getEnabledFromMap(*adverbs);
	QMap<QModelIndex, Element *> enabledOthers = getEnabledFromMap(*others);


	ProbabilityRandom wordClassRand;

	if (includeVerbs)
	{
		words.unite(enabledVerbs);
		wordClassRand.addOccurrence(RunFlag::IncludeVerbs, 1);
	}
	if (includeAdjectives)
	{
		words.unite(enabledAdjectives);
		wordClassRand.addOccurrence(RunFlag::IncludeAdjectives, 1);
	}
	if (includeNouns)
	{
		words.unite(enabledNouns);
		wordClassRand.addOccurrence(RunFlag::IncludeNouns, 1);
	}
	if (includeAdverbs)
	{
		words.unite(enabledAdverbs);
		wordClassRand.addOccurrence(RunFlag::IncludeAdverbs, 1);
	}
	if (includeOthers)
	{
		words.unite(enabledOthers);
		wordClassRand.addOccurrence(RunFlag::IncludeOthers, 1);
	}

	ProbabilityRandom stageTypeRand;
	if (askForWord)
		stageTypeRand.addOccurrence(Word, 3);
	if (askForInterpretation)
		stageTypeRand.addOccurrence(Interpretation, 2);
	if(includeInputting)
		stageTypeRand.addOccurrence(Inputting, 1);

	QMapIterator<QModelIndex, Element*> wordsIterator(words);

	while(wordsIterator.hasNext())
	{

		wordsIterator.next();
		StageType stageType = (StageType) stageTypeRand.Next().toInt();

		ProbabilityRandom stagePropertyRand;
		if (includeMeanings)
			stagePropertyRand.addOccurrence(Meaning, 1);
		if (includeTranslations)
			stagePropertyRand.addOccurrence(Translation, 1);
		if (includeContexts && stageType != Inputting)
			stagePropertyRand.addOccurrence(Context, 1);
		StageProperty stageProperty = (StageProperty) stagePropertyRand.Next().toInt();

		QString header;
		QString hint;
		QStringList options;
		int rightOption;
		StageMapping stageMapping;
		Element *element = wordsIterator.value();


		QMap<QModelIndex, Element*> otherWords;
		if (mixWordClasses)
			otherWords = words;
		else
		{
			switch (element->wordProperties.wordClass)
			{
			case Verb:
				otherWords.unite(enabledVerbs);
				break;
			case Adjective:
				otherWords.unite(enabledAdjectives);
				break;
			case Noun:
				otherWords.unite(enabledNouns);
				break;
			case Adverb:
				otherWords.unite(enabledAdverbs);
				break;
			case Other:
				otherWords.unite(enabledOthers);
				break;
			}
		}
		otherWords.remove(wordsIterator.key());
		if (otherWords.count() < 4)
			continue;

		switch (stageType)
		{
		case Word:
		{
			header = element->name;
			switch (stageProperty)
			{
			case Meaning:
			{
				hint = tr("Choose the word's meaning");
				stageMapping = ExpressionChoosing;
				//delete the & if it doesnt work
				generateChoosingStageOptions(StageMapping::ExpressionChoosing,&LibraryModel::getElementMeaning,element,rightOption,options,otherWords);
				break;
			}
			case Translation:
			{
				hint = tr("Choose the word's translation");
				stageMapping = ExpressionChoosing;
				generateChoosingStageOptions(StageMapping::ExpressionChoosing, &LibraryModel::getElementTranslation, element, rightOption, options, otherWords);
				break;
			}
			case Context:
			{
				hint = tr("Choose the context where the word can be");
				stageMapping = ExpressionChoosing;
				generateChoosingStageOptions(StageMapping::ExpressionChoosing, &LibraryModel::getElementContext, element, rightOption, options, otherWords);
				break;
			}
			}
			break;
		}
		case Interpretation:
		{
			stageMapping = WordChoosing;
			switch (stageProperty)
			{
			case Meaning:
			{
				hint = tr("Choose the word that matches the meaning");
				header = getElementMeaning(element);
				break;
			}
			case Translation:
			{
				hint = tr("Choose the word that matches the translation");
				header = getElementTranslation(element);
				break;
			}
			case Context:
			{
				hint = tr("Choose the word to paste into the sentence");
				header = getElementContext(element);
				break;
			}
			}
			generateChoosingStageOptions(stageMapping, &LibraryModel::getElementName, element, rightOption, options, otherWords);
			break;
		}
		case Inputting:
		{
			stageMapping = StageMapping::InputtingOption;
			header = element->name;
			//this case : rightOption serves count of attempts 
			rightOption = inputAttempts;
			switch (stageProperty)
			{
			case Meaning:
			{
				hint = tr("Enter the word's meaning");
				options.append(element->wordProperties.meaning);
				break;
			}
			case Translation:
			{
				hint = tr("Enter the word's translation");
				options.append(element->wordProperties.translation);
				break;
			}
			}
			//options[1] serves stage's input text
			options.append("");
			break;
		}
		}

		stages.append(new Stage(stageMapping,wordsIterator.key(),header,hint,options,rightOption ));
	}

	return stages;

}

QMap<QModelIndex, Element*> LibraryModel::getEnabledFromMap(const QMap<QModelIndex, Element*>& map)
{
	QMap<QModelIndex, Element*> outMap;
	QMapIterator<QModelIndex, Element*> wordsIterator(map);
	while (wordsIterator.hasNext())
	{
		wordsIterator.next();
		if (wordsIterator.value()->enabled())
			outMap.insert(wordsIterator.key(), wordsIterator.value());
	}
	return outMap;

}

void LibraryModel::generateChoosingStageOptions(StageMapping mapping, GetElementProperty prop, Element * element, int & rightOption, QStringList & options, QMap<QModelIndex, Element*>& otherWords)
{
	int count;
	switch (mapping)
	{
	case WordChoosing:
		count = wordChoosingButtonsRows * wordChoosingButtonsColumns;
		break;
	case ExpressionChoosing:
		count = expressionChoosingButtonsCount;
	}

	if (otherWords.count() < count)
		count = otherWords.count();
	if(count != 4)
		count = qrand() % (count - 4) + 4;

	rightOption = qrand() % count;
	for (int i = 0; i < count; i++)
	{
		if (i == rightOption)
			options.append(std::invoke(prop, this, element));
		else
			options.append(std::invoke(prop, this, getRandomElement(otherWords)));
	}
	
}

Element * LibraryModel::getRandomElement(QMap<QModelIndex, Element*> &map)
{
	int randomPos = qrand() % map.count();
	Element *element = map.values().at(randomPos);
	map.remove(map.key(element, map.firstKey()));
	return element;

}

QString  LibraryModel::getElementMeaning(Element * element)
{
	return element->wordProperties.meaning;
}

QString LibraryModel::getElementTranslation(Element * element)
{
	return element->wordProperties.translation;
}

QString LibraryModel::getElementContext(Element * element)
{
	QStringList contexts = element->wordProperties.contexts;
	int randomContext = qrand() % contexts.count();
	QString context = contexts.at(randomContext);
	QString longetsForm;
	foreach(QString form, element->wordProperties.forms)
	{
		if (context.contains(form,Qt::CaseInsensitive) && form.length() > longetsForm.length())
			longetsForm = form;
	}
	context.replace(longetsForm, "...", Qt::CaseInsensitive);

	return context;
}

QString LibraryModel::getElementName(Element * element)
{
	return element->name;
}

Element *LibraryModel::elementFromIndex(const QModelIndex &index) const

{
	if (index.isValid())
		return static_cast<Element *>(index.internalPointer());
	else
		return rootElement;
}

QMap<QModelIndex, Element*> *LibraryModel::getWordClassMap(Element * element) const
{
	return getWordClassMap(element->wordProperties.wordClass);
}

QMap<QModelIndex, Element*>* LibraryModel::getWordClassMap(WordClass wordClass) const
{
	QMap<QModelIndex, Element*> *words;
	switch (wordClass)
	{
	case Verb:
		words = verbs;
		break;
	case Adjective:
		words = adjectives;
		break;
	case Noun:
		words = nouns;
		break;
	case Adverb:
		words = adverbs;
		break;
	case Other:
		words = others;
		break;
	}
	return words;
}

void LibraryModel::fillLists(Element *childElement, int row,const QModelIndex &parentIndex)
{
	QModelIndex childIndex = index(row, 0, parentIndex);
	int count = childElement->children().count();
	for (int i = 0; i < count;i++)
	{
		Element *element = childElement->children()[i];
		fillLists(element, i, childIndex);
	}
}

bool LibraryModel::areWordPropertiesFull(const WordProperties &wordProp) const
{
	if (wordProp.contexts.count() > 0 && wordProp.forms.count() > 0
		&& !wordProp.meaning.isEmpty() && !wordProp.translation.isEmpty())
		return true;
	return false;
}

QVariant CheckableLibraryProxyModel::data(const QModelIndex & index, int role) const
{
	if (role != Qt::CheckStateRole)
		return QIdentityProxyModel::data(index, role);
	Element* element = static_cast<Element*>(mapToSource(index).internalPointer());
	if (!element)
		return QVariant();
	return element->enabled() ? Qt::Checked : Qt::Unchecked;
}

bool CheckableLibraryProxyModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (role != Qt::CheckStateRole)
		return QIdentityProxyModel::setData(index, value, role);
	Element* element = static_cast<Element*>(mapToSource(index).internalPointer());
	if (!element)
		return false;
	element->setEnabled( value.toBool());
	emit dataChanged(index, QModelIndex(), { role });
	return true;
}

Qt::ItemFlags CheckableLibraryProxyModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return QIdentityProxyModel::flags(index) ^ Qt::ItemIsSelectable ^ Qt::ItemIsEditable;
}

void CheckableLibraryProxyModel::checkAll(bool check)
{
		setData(index(0, 0), check, Qt::CheckStateRole);
}

void CheckableLibraryProxyModel::checkChildren(QModelIndex index)
{
	setData(index, true, Qt::CheckStateRole);
	QList<Element*> elementChildren = ((LibraryModel)sourceModel()).elementFromIndex(mapToSource(index))->children();
	for (int i = 0; i < elementChildren.count(); i++)
		checkChildren(index.child(i, 0));
}

Element * LibraryDomParser::parseFile(const QString & fileName)
{
	QFile file(fileName);
	QDomDocument library;
	QString errorStr;
	int errorLine;
	int errorColumn;
	if (!library.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
	{
		QMessageBox::warning(0, QObject::tr("Library reader"), QObject::tr("Recent open library was removed or deleted"));
		return 0;
	}
	return parseNext(&library.documentElement());
}

Element* LibraryDomParser::parseNext(QDomElement *dElement)
{
	QString name = dElement->attribute("Name");
	if (name.isEmpty())
		return 0;
	bool isWord = dElement->tagName() == "Word";
	bool enabled = dElement->attribute("Enabled") == "true";
	QString meaning;
	QString translation;
	WordClass wordClass;
	QStringList forms;
	QStringList contexts;

	QList<Element*> children;

	if (isWord)
	{
		translation = dElement->attribute("Translation");
		meaning = dElement->attribute("Meaning");
		wordClass = (WordClass) dElement->attribute("WordClass").toInt();
	}

	QDomElement node = dElement->firstChildElement();
	while (!node.isNull())
	{
		QString tagName = node.tagName();
		if (tagName == "Group" || tagName == "Word")
		{
			children.append(parseNext(&node));
		}
		else if (isWord)
		{
			if (tagName == "Form")
				forms.append(node.text());
			else if (tagName == "Context")
				contexts.append(node.text());
		}
		node = node.nextSiblingElement();
	}
	if (isWord)
	{
		return new Element(name, enabled, {wordClass,meaning,translation,forms,contexts });
	}
	else
	{
		Element *element = new Element(false, enabled, name);
		element->setChildren(children);
		return element;
	}
		
}

QDomElement LibraryDomParser::getNext(Element * element, QDomDocument & doc)
{
	if (element == 0)
		return QDomElement();

	bool isWord = element->isWord();
	QDomElement dElement = doc.createElement(isWord ? "Word" : "Group");

	dElement.setAttribute("Name", element->name);
	dElement.setAttribute("Enabled", element->enabled() ? "true" : "false");
	if (isWord)
	{
		WordProperties wordProperties = element->wordProperties;

		if (!wordProperties.translation.isEmpty())
			dElement.setAttribute("Translation", wordProperties.translation);
		if (!wordProperties.meaning.isEmpty())
			dElement.setAttribute("Meaning", wordProperties.meaning);
		dElement.setAttribute("WordClass", wordProperties.wordClass);
		foreach(QString string, wordProperties.forms)
		{
			QDomElement form = doc.createElement("Form");
			QDomText text = doc.createTextNode(string);
			form.appendChild(text);
			dElement.appendChild(form);
		}
		foreach(QString string, wordProperties.contexts)
		{
			QDomElement context = doc.createElement("Context");
			QDomText text = doc.createTextNode(string);
			context.appendChild(text);
			dElement.appendChild(context);
		}
	}
	else
	{
		QList<Element*> children = element->children();
		int count = children.count();
		if (count > 0)
		{
			for (int i = 0; i < count; i++)
			{
				dElement.appendChild(getNext(children[i], doc));
			}
		}
	}
	return dElement;
}

bool LibraryDomParser::saveFile(Element * rootElement, const QString & fileName)
{
	QDomDocument library;
	if (!rootElement)
		return false;

	library.appendChild(getNext(rootElement,library));

	QFile file(fileName);
	if(!file.open(QIODevice::WriteOnly))
		return false;
	library.save(QTextStream(&file), 4);
	return true;

}
