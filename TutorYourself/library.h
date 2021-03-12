#pragma once
#include <QAbstractItemModel>
#include <QIdentityProxyModel>
#include <qdom.h>
class QLineEdit;
class LibraryModel;
class QTreeView;
class LibraryModel;
class CheckableLibraryProxyModel;
class Element;
class LbraryDomParser;
class Stage;
struct WordProperties;

enum StageMapping;

enum 
{
	WordRole = Qt::UserRole,
	GroupRole
};

enum WordClass
{
	Other,Verb,Adjective,Noun,Adverb
};

enum RunFlag
{
	IncludeVerbs = 0x01,
	IncludeAdjectives = 0x02,
	IncludeNouns = 0x04,
	IncludeAdverbs = 0x08,
	IncludeOthers = 0x10,
	IncludeTranslations = 0x20,
	IncludeMeanings = 0x40,
	IncludeContexts = 0x80,
	MixWordClasses = 0x100,
	AskForWord = 0x200,
	AskForInterpretation = 0x400,
	IncludeInputting = 0x800
};

typedef QString(LibraryModel::* GetElementProperty)(Element *);

class LibraryModel : public QAbstractItemModel
{
public:
	LibraryModel(QObject *parent = 0);
	~LibraryModel();
	void setDocument(Element *document);
	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
	bool removeRows(int row, int count, const QModelIndex &parent);
	bool changeDocumentName(const QString &string);
	const QString documentName();
	Element *getDocument() const {return document;}
	QModelIndex documentIndex() const;
	Element *elementFromIndex(const QModelIndex &index) const;

	QMap<QModelIndex, Element*> *getWordClassMap(Element* element) const;
	QMap<QModelIndex, Element*> *getWordClassMap(WordClass wordClass) const;

	void fillLists(Element *childElement, int row, const QModelIndex &parentIndex);
	bool areWordPropertiesFull(const WordProperties &wordProp) const;

	QList<Stage*> getStages(int runFlags);
	QMap<QModelIndex, Element*> getEnabledFromMap(const QMap<QModelIndex, Element*> &map);

	void generateChoosingStageOptions(StageMapping mapping,GetElementProperty prop, Element * element,int &rightOption, QStringList &options, QMap<QModelIndex,Element*> &otherWords);
	Element *getRandomElement(QMap<QModelIndex, Element*> &map);

	QString getElementMeaning(Element* element);
	QString getElementTranslation(Element *element);
	QString getElementContext(Element* element);
	QString getElementName(Element* element);
private:
	Element *rootElement;
	Element *document;
	QMap<QModelIndex ,Element*> *verbs;
	QMap<QModelIndex,Element*> *adjectives;
	QMap<QModelIndex, Element*> *nouns;
	QMap<QModelIndex, Element*> *adverbs;
	QMap<QModelIndex, Element*> *others;

};

class LibraryDomParser
{
public:
	static Element *parseFile(const QString &fileName);
	static bool saveFile(Element* rootElement, const QString &fileName);
private:
	static Element* parseNext(QDomElement *dElement);
	static QDomElement getNext(Element *element, QDomDocument &doc);
};

class CheckableLibraryProxyModel : public QIdentityProxyModel
{
public:
	CheckableLibraryProxyModel(QObject *parent = 0) : QIdentityProxyModel(parent) {};
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	void checkAll(bool check);
	void checkChildren(QModelIndex index);
};

struct WordProperties
{
	WordClass wordClass;
	QString meaning;
	QString translation;
	QStringList forms;
	QStringList contexts;

	WordProperties(const WordProperties &properies);
	WordProperties() = default;
	WordProperties(WordClass wordClass, QString meaning, QString translation, QStringList forms, QStringList contexts);
	bool operator==(WordProperties other);
};

struct GroupProperties
{
	int wordsCount = 0;
};

class Element
{
public:
	bool isWord() { return _isWord; };
	void defineWord();
	void undefWord();
	Element(bool isWord, bool enabled, const QString &name = "");
	Element(const QString &name, bool enabled, WordProperties wordProperies);
	Element(const QString &name, bool enabled, GroupProperties groupProperties);
	~Element();
	QString name;
	Element* parent() const;
	QList<Element*> children() const;
	WordProperties wordProperties;
	GroupProperties groupPropeties;
	QList<Element*> setChildren(QList<Element*> &elements);
	void addChildren(Element* element, int row = -1);
	void removeChildren(Element *element);
	void removeChildren(int row);
	void setParent(Element* element);
	bool enabled() { return _enabled; }
	void setEnabled(bool enabled);
	void setChildEnabled(bool enabled);
	void setParentEnabled(bool enabled);
private:
	static void changeWordsCount(Element* element,int difference);
	bool _isWord;
	QList<Element*> _children;
	Element* _parent;
	bool _enabled;
};

Q_DECLARE_METATYPE(WordProperties)
Q_DECLARE_METATYPE(GroupProperties)