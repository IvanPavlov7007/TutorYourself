#pragma once
#include <QStringList>
#include <QModelIndex>
enum StageMapping 
{
	WordChoosing = 0 ,
	ExpressionChoosing,
	InputtingOption
};
enum StageType
{
	Word = 0,
	Interpretation,
	Inputting
};

enum StageProperty
{
	Meaning = 0,
	Translation,
	Context
};

const int wordChoosingButtonsRows = 2;
const int wordChoosingButtonsColumns = 4;
const int expressionChoosingButtonsCount = 5;
const int inputAttempts = 3;

class Stage
{
public:
	Stage() = default;
	Stage(StageMapping stageType,const QModelIndex wordIndex, const QString &header, const QString &hint, QStringList options,int rightOption)
		:stageMapping(stageType), wordIndex(wordIndex), header(header), hint(hint),
			options(options), rightOption(rightOption){};
	StageMapping stageMapping;
	QString hint;
	QString header;
	QStringList options;
	QVector<int> chosenOptions;
	int rightOption;
	bool finished = false;
	bool completed;
	QModelIndex wordIndex;
};