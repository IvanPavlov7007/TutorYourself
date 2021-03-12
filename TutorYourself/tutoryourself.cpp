#include <QtWidgets>
#include "tutoryourself.h"

using Trigger = QAbstractItemView::EditTrigger;

TutorYourself::TutorYourself(QWidget* parent)
	:QWidget(parent)
{
	setupUi(this);

	currentItemIcon = new QIcon(":/images/transparent_stage.png");
	currentItemIcon->addPixmap(QPixmap::fromImage(QImage(":/images/transparent_stage.png")), QIcon::Mode::Selected);
	completedItemIcon = new QIcon(":/images/green_stage.png");
	nextItemIcon = new QIcon(":/images/yellow_stage.png");
	failedItemIcon = new QIcon(":/images/red_stage.png");

	QSizePolicy _sizePolicy= aboutPushButton->sizePolicy();
	_sizePolicy.setRetainSizeWhenHidden(true);
	aboutPushButton->setSizePolicy(_sizePolicy);
	nextPushButton->setSizePolicy(_sizePolicy);
	previousPushButton->setSizePolicy(_sizePolicy);
	breakPushButton->setSizePolicy(_sizePolicy);
	aboutPushButton->hide();
	previousPushButton->hide();
	breakPushButton->hide();

	alreadyContains = tr("*Already contains");
	mustIncludeForm = tr("*The string must include one of this word's forms. To declare new form you can select it using @..@");

	widgetsOfContextsList.label = addContext_hint_label;
	widgetsOfContextsList.lineEdit = addContextLineEdit;
	widgetsOfContextsList.listWidget = contextsListWidget;
	widgetsOfContextsList.pushButton = addContextPushButton;
	widgetsOfContextsList.stringParser = &TutorYourself::contextStringParser;

	widgetsOfFormsList.label = addForm_hint_label;
	widgetsOfFormsList.lineEdit = addFormLineEdit;
	widgetsOfFormsList.listWidget = formsListWidget;
	widgetsOfFormsList.pushButton = addFormPushButton;
	widgetsOfFormsList.stringParser = &TutorYourself::formStringParser;

	translationLineEdit->installEventFilter(this);
	meaningLineEdit->installEventFilter(this);
	libraryNameLineEdit->installEventFilter(this);
	elementNameLineEdit->installEventFilter(this);
	addContextLineEdit->installEventFilter(this);
	addFormLineEdit->installEventFilter(this);
	input_lineEdit->installEventFilter(this);

	libraryModel = new LibraryModel(this);

	libraryTreeView->setModel(libraryModel);
	libraryTreeView->setEditTriggers(Trigger::AnyKeyPressed | Trigger::DoubleClicked | Trigger::SelectedClicked );

	proxyModel = new CheckableLibraryProxyModel(this);
	proxyModel->setSourceModel(libraryModel);

	enableElementsTreeView->setModel(proxyModel);
	enableElementsTreeView->setEditTriggers(Trigger::NoEditTriggers);

	readSettings();

	connect(runButton, &QPushButton::clicked, this, &TutorYourself::setRunMode);
	connect(editButton, &QPushButton::clicked, this, &TutorYourself::setEditMode);

	setEditPageConnections();
	setRunPageConnections();

	isLibraryModified = false;

	if (currentFile.isEmpty())
		newLibrary();
	else
		openFile(currentFile);

	chosenStagesCount = 0;
	updateStages();
}

void TutorYourself::setRunMode()
{
	workPlace->setCurrentIndex(0);
	if (!started)
		updateStages();
}

TutorYourself::~TutorYourself()
{
	qDeleteAll(stages);
	delete wordChoosingButtons;
	delete currentItemIcon;
	delete completedItemIcon;
	delete nextItemIcon;
	delete failedItemIcon;
	
	for (int i = 0; i < contextsListWidget->count(); i++)
	{
		deleteRow(contextsListWidget, i);
	}

	for (int i = 0; i < formsListWidget->count(); i++)
	{
		deleteRow(formsListWidget, i);
	}

	clearStagesItems();

}

void TutorYourself::setEditMode()
{
	workPlace->setCurrentIndex(1);
}

void TutorYourself::addGroup()
{
	addElement();
}

void TutorYourself::addWord()
{
	QAbstractItemModel *model = libraryTreeView->model();
	QModelIndex index = addElement();
	if (index.isValid())
	{
		model->setData(index, "New Word", Qt::EditRole);
		model->setData(index, QVariant::fromValue<WordProperties>(WordProperties()), WordRole);
		otherElelmentSelected(index, QModelIndex());
	}
}

void TutorYourself::deleteElement()
{
	QAbstractItemModel *model = libraryTreeView->model();
	QModelIndex index = libraryTreeView->selectionModel()->currentIndex();
	model->removeRow(index.row(), index.parent());
	otherElelmentSelected(libraryTreeView->selectionModel()->currentIndex(), QModelIndex());
	libraryTreeView->setFocus();
}

void TutorYourself::openLibrary()
{
	if (okToContinue())
	{
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open Existing Library"), QDir::currentPath(), tr("XML files(*.xml)"));
		openFile(fileName);
	}
}

bool TutorYourself::saveLibrary()
{
	if (currentFile.isEmpty())
		return saveAs();
	return saveFile(currentFile);
}

void TutorYourself::newLibrary()
{
	if (okToContinue())
	{
		currentFile.clear();
		setLibrary(new Element(false, true, "New Library"));
	}
	libraryTreeView->setFocus();
}

void TutorYourself::lineEditTextChanged()
{
	qobject_cast<QLineEdit*>(sender())->setStyleSheet("QLineEdit{color: grey}");
}

void TutorYourself::otherElelmentSelected(const QModelIndex & current, const QModelIndex & previous)
{
	commitLineEdit(elementNameLineEdit, current.data(Qt::DisplayRole).toString());

	QVariant variant = current.data(WordRole);
	if (variant.canConvert<WordProperties>())
	{
		setWordPropertiesPage(variant.value<WordProperties>());
		if (additionalPropertiesEditor->currentIndex() != 1)
			additionalPropertiesEditor->setCurrentIndex(1);
		return;
	}
	variant = current.data(GroupRole);
	if (variant.canConvert<GroupProperties>())
	{
		setGroupPropertiesPage(variant.value<GroupProperties>());
		if (additionalPropertiesEditor->currentIndex() != 0)
			additionalPropertiesEditor->setCurrentIndex(0);
		elementEditor->verticalScrollBar()->setValue(0);
	}

}

void TutorYourself::addRowButtonClicked()
{
	WidgetsOfList *widgetsOfList;
	if (sender() == addContextPushButton)
		widgetsOfList = &widgetsOfContextsList;
	else
		widgetsOfList = &widgetsOfFormsList;
	commitAddRowLineEdit(widgetsOfList);
}

void TutorYourself::currentWordClassChanged()
{
	QObject *obj = sender();
	if (obj == isVerbRadioButton)
		currentWordProperties.wordClass = Verb;
	else if (obj == isNounRadioButton)
		currentWordProperties.wordClass = Noun;
	else if (obj == isAdjectiveRadioButton)
		currentWordProperties.wordClass = Adjective;
	else if (obj == isAdverbRadioButton)
		currentWordProperties.wordClass = Adverb;
	else if (obj == isOtherRadioButton)
		currentWordProperties.wordClass = Other;
	setCurrentElementData(QVariant::fromValue<WordProperties>(currentWordProperties), WordRole);
}

void TutorYourself::rowAsksToDestroy(QListWidgetItem *item, WidgetsOfList *widgetsOfList)
{
	int row = widgetsOfList->listWidget->row(item);
	widgetsOfList->data->removeAt(row);
	deleteRow(widgetsOfList->listWidget, row);
}

void TutorYourself::rowTextChanged(QListWidgetItem *item, const QString &string, WidgetsOfList *widgetsOfList)
{

	RowWidget *rowWidget = qobject_cast<RowWidget*>(sender());
	QStringList *stringList = widgetsOfList->data;
	int row = widgetsOfList->listWidget->row(item);

	if (string.isEmpty())
	{
		rowWidget->setText(stringList->at(row));
		return;
	}

	StringParsingError err;
	QString finalString = CALL_RowStringParser(*this, widgetsOfList->stringParser)(string,row,err);
	if (err != NoError)
	{
		if (err == SameExist)
			widgetsOfList->label->setText(alreadyContains);
		if (err == NoFormIncluded)
			widgetsOfList->label->setText(mustIncludeForm);
		rowWidget->setText(stringList->at(row));
		return;
	}

	if (finalString != string)
		rowWidget->setText(finalString);

	(*stringList)[row] = string;
	saveCurrentWordProperties();
}

void TutorYourself::libraryModified(const QModelIndex & topLeft, const QModelIndex & /*bottomRight*/, const QVector<int>& roles)
{
	isLibraryModified = true;
	if (roles.count() == 0)
		return;

	if (roles[0] == Qt::EditRole)
	{
		if (topLeft == getCurrentModelIndex())
			commitLineEdit(elementNameLineEdit, topLeft.data(Qt::DisplayRole).toString());
		if (topLeft == libraryModel->documentIndex())
			commitLineEdit(libraryNameLineEdit ,topLeft.data(Qt::DisplayRole).toString());
	}
	if (roles[0] == Qt::CheckStateRole)
		updateStages();
}

void TutorYourself::checkAllButtonClicked()
{
	proxyModel->checkAll(true);
}

void TutorYourself::uncheckAllButtonClicked()
{
	proxyModel->checkAll(false);
}

void TutorYourself::nextPushButtonClicked()
{
	if (!started)
	{
		qsrand(QTime::currentTime().second());
		completed = 0;
		failed = 0;
		started = true;
		nextPushButton->setText(tr("Next"));
		breakPushButton->setText(tr("Stop"));

		QList<Stage*> currentStages;
		for (int i = 0; i < chosenStagesCount; i++)
		{
			currentStages.append(stages.takeAt(qrand() % stages.count()));
		}

		qDeleteAll(stages);
		stages.clear();
		stages = currentStages;
		currentStageIndex = -1;
		updateFinishedStagesCountLabel();
		addStageItem(stages[0]->header);
		loadStage(0);
		return;
	}
	loadNextStage();
}

void TutorYourself::previousPushButtonClicked()
{
	stagesListWidget->setCurrentRow(currentStageIndex - 1);
}

void TutorYourself::aboutPushButtonClicked()
{
	libraryTreeView->selectionModel()->setCurrentIndex(currentStage->wordIndex, QItemSelectionModel::SelectionFlag::ClearAndSelect);
	setEditMode();
}

void TutorYourself::breakPushButtonClicked()
{
	toStartPage();
}

void TutorYourself::stagesCountSpinBoxValueChanged(int val)
{
	chosenStagesCount = val;
	if (val > 0 && MaxStagesCount > 0)
		setWidgetEnabled(nextPushButton, true);
	else
		setWidgetEnabled(nextPushButton, false);
}

void TutorYourself::includeTestPartCheckBoxStateChanged(int state)
{
	QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender());
	RunFlag flag;

	if (checkBox == includeVerbsCheckBox)
		flag = IncludeVerbs;
	else if (checkBox == includeAdjectivesCheckBox)
		flag = IncludeAdjectives;
	else if (checkBox == includeNounsCheckBox)
		flag = IncludeNouns;
	else if (checkBox == includeAdverbsCheckBox)
		flag = IncludeAdverbs;
	else if (checkBox == includeOthersCheckBox)
		flag = IncludeOthers;
	else if (checkBox == mixWordClassesCheckBox)
		flag = MixWordClasses;
	else if (checkBox == includeTranslationsCheckBox)
		flag = IncludeTranslations;
	else if (checkBox == includeMeaningsCheckBox)
		flag = IncludeMeanings;
	else if (checkBox == includeContextsCheckBox)
		flag = IncludeContexts;
	else if (checkBox == includeInputCheckBox)
		flag = IncludeInputting;
	currentRunFlags ^= flag;
	updateStages();
}

void TutorYourself::askDestinationRadioButtonClicked()
{
	QRadioButton *radioButton = qobject_cast<QRadioButton*>(sender());
	if (radioButton == wordInterpretationRadioButton)
	{
		currentRunFlags |= AskForWord;
		currentRunFlags &= ~AskForInterpretation;
	}
	else if (radioButton == interpretationWordRadioButton)
	{
		currentRunFlags |= AskForInterpretation;
		currentRunFlags &= ~AskForWord;
	}
	else
		currentRunFlags |= AskForInterpretation | AskForWord;
	updateStages();
}

void TutorYourself::wordChoosingButtonClicked(int index)
{
	optionChosen(wordChoosingButtons ,index);
}

void TutorYourself::expressionChoosingButtonClicked(int index)
{
	optionChosen(expressionChoosingButtons ,index);
}

void TutorYourself::input_lineEditTextChanged(const QString & text)
{
	currentStage->options[1] = text;
}

void TutorYourself::otherStageSelected(QListWidgetItem * current, QListWidgetItem * previous)
{
	if (started)
	{
		loadStage(stagesListWidget->row(current));
	}
}

void TutorYourself::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	resizeRowWidget(contextsListWidget);
	resizeRowWidget(formsListWidget);
}

bool TutorYourself::eventFilter(QObject * target, QEvent * event)
{
	WidgetsOfList *widgetsOfList = 0;
	if (target == addContextLineEdit)
		widgetsOfList = &widgetsOfContextsList;
	else if (target == addFormLineEdit)
		widgetsOfList = &widgetsOfFormsList;
	
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if (keyEvent->key() == Qt::Key_Return)
		{
			if (widgetsOfList)
				commitAddRowLineEdit(widgetsOfList);
			else if (target == input_lineEdit)
			{
				input_lineEditEditingOver();
				return true;
			}
			else if (target == elementNameLineEdit)
			{
				libraryModel->setData(getCurrentModelIndex(), elementNameLineEdit->text(), Qt::EditRole);
				return true;
			}
			else if (target == libraryNameLineEdit)
			{
				libraryModel->setData(libraryModel->documentIndex(), libraryNameLineEdit->text(), Qt::EditRole);
				return true;
			}
			else if (target == meaningLineEdit)
			{
				WordProperties prop = currentWordProperties;
				prop.meaning = meaningLineEdit->text().trimmed();
				if (libraryModel->setData(getCurrentModelIndex(),
					QVariant::fromValue<WordProperties>(prop),
					WordRole))
				{
					commitLineEdit(meaningLineEdit, prop.meaning);
					currentWordProperties = prop;
				}
			}
			else if (target == translationLineEdit)
			{
				WordProperties prop = currentWordProperties;
				prop.translation = translationLineEdit->text().trimmed();
				if (libraryModel->setData(getCurrentModelIndex(),
					QVariant::fromValue<WordProperties>(prop),
					WordRole))
				{
					commitLineEdit(translationLineEdit, prop.translation);
					currentWordProperties = prop;
				}
			}

		}
	}

	return QWidget::eventFilter(target, event);
}

void TutorYourself::closeEvent(QCloseEvent * event)
{
	if (okToContinue())
	{
		writeSettings();
		event->accept();
	}
	else
		event->ignore();
}

void TutorYourself::setEditPageConnections()
{
	connect(addGroupButton, &QPushButton::clicked, this, &TutorYourself::addGroup);
	connect(addWordButton, &QPushButton::clicked, this, &TutorYourself::addWord);
	connect(deleteButton, &QPushButton::clicked, this, &TutorYourself::deleteElement);

	connect(newLibraryButton, &QPushButton::clicked, this, &TutorYourself::newLibrary);
	connect(openLibraryButton, &QPushButton::clicked, this, &TutorYourself::openLibrary);
	connect(saveLibraryButton, &QPushButton::clicked, this, &TutorYourself::saveLibrary);

	connect(libraryTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TutorYourself::otherElelmentSelected);
	connect(libraryModel, &QAbstractItemModel::dataChanged, this, &TutorYourself::libraryModified);
	connect(elementNameLineEdit, &QLineEdit::textChanged, this, &TutorYourself::lineEditTextChanged);
	connect(libraryNameLineEdit, &QLineEdit::textChanged, this, &TutorYourself::lineEditTextChanged);
	connect(proxyModel, &QAbstractItemModel::dataChanged, this, &TutorYourself::libraryModified);

	connect(translationLineEdit,&QLineEdit::textChanged, this, &TutorYourself::lineEditTextChanged);
	connect(meaningLineEdit, &QLineEdit::textChanged, this, &TutorYourself::lineEditTextChanged);

	connect(addContextPushButton, &QPushButton::clicked, this, &TutorYourself::addRowButtonClicked);
	connect(addFormPushButton, &QPushButton::clicked, this, &TutorYourself::addRowButtonClicked);

	connect(isVerbRadioButton, &QRadioButton::clicked, this, &TutorYourself::currentWordClassChanged);
	connect(isNounRadioButton, &QRadioButton::clicked, this, &TutorYourself::currentWordClassChanged);
	connect(isAdverbRadioButton, &QRadioButton::clicked, this, &TutorYourself::currentWordClassChanged);
	connect(isAdjectiveRadioButton, &QRadioButton::clicked, this, &TutorYourself::currentWordClassChanged);
	connect(isOtherRadioButton, &QRadioButton::clicked, this, &TutorYourself::currentWordClassChanged);
}

void TutorYourself::setRunPageConnections()
{
	connect(checkAllPushButton, &QPushButton::clicked, this,&TutorYourself::checkAllButtonClicked);
	connect(uncheckAllPushButton, &QPushButton::clicked, this, &TutorYourself::uncheckAllButtonClicked);

	connect(nextPushButton, &QPushButton::clicked, this, &TutorYourself::nextPushButtonClicked);
	connect(previousPushButton, &QPushButton::clicked, this, &TutorYourself::previousPushButtonClicked);
	connect(aboutPushButton, &QPushButton::clicked, this, &TutorYourself::aboutPushButtonClicked);
	connect(breakPushButton, &QPushButton::clicked, this, &TutorYourself::breakPushButtonClicked);

	setRunSettingsButtons();

	connect(input_lineEdit, &QLineEdit::textChanged, this, &TutorYourself::input_lineEditTextChanged);

	connect(includeVerbsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeAdjectivesCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeNounsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeAdverbsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeOthersCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(mixWordClassesCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeTranslationsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeMeaningsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeContextsCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);
	connect(includeInputCheckBox, &QCheckBox::stateChanged, this, &TutorYourself::includeTestPartCheckBoxStateChanged);

	connect(wordInterpretationRadioButton, &QRadioButton::clicked, this, &TutorYourself::askDestinationRadioButtonClicked);
	connect(interpretationWordRadioButton, &QRadioButton::clicked, this, &TutorYourself::askDestinationRadioButtonClicked);
	connect(bothRadioButton, &QRadioButton::clicked, this, &TutorYourself::askDestinationRadioButtonClicked);

	connect(stagesCountSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &TutorYourself::stagesCountSpinBoxValueChanged);
	connect(stagesListWidget, &QListWidget::currentItemChanged, this, &TutorYourself::otherStageSelected);

	wordChoosingButtonsMapper = new QSignalMapper(this);
	expressionChoosingButtonsMapper = new QSignalMapper(this);

	QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	policy.setRetainSizeWhenHidden(true);
	QFont buttonFont = font();
	buttonFont.setPixelSize(22);

	wordChoosingButtons = new QPushButton*[wordChoosingButtonsColumns * wordChoosingButtonsRows];
	for (int i = 0; i < wordChoosingButtonsRows; i++)
	{
		for (int j = 0; j < wordChoosingButtonsColumns; j++)
		{
			int num = wordChoosingButtonsColumns * i + j;
			QPushButton *button = new QPushButton(QString::number(num));
			button->setSizePolicy(policy);
			button->setFont(buttonFont);
			connect(button, &QPushButton::clicked, wordChoosingButtonsMapper, static_cast<void(QSignalMapper::*)()> (&QSignalMapper::map));
			wordChoosingButtonsMapper->setMapping(button, num);
			wordChoosingGridLayout->addWidget(button, i, j);
			wordChoosingButtons[num] = button;
		}
	}
	connect(wordChoosingButtonsMapper, static_cast<void(QSignalMapper::*)(int)> (&QSignalMapper::mapped), this, &TutorYourself::wordChoosingButtonClicked);

	expressionChoosingButtons = new QPushButton*[expressionChoosingButtonsCount];
	QLayout *layout = choice_expression->layout();
	for (int i = 0; i < expressionChoosingButtonsCount; i++)
	{
		QPushButton *button = new QPushButton(QString::number(i));
		button->setSizePolicy(policy);
		button->setFont(buttonFont);
		connect(button, &QPushButton::clicked, expressionChoosingButtonsMapper, static_cast<void(QSignalMapper::*)()> (&QSignalMapper::map));
		expressionChoosingButtonsMapper->setMapping(button, i);
		layout->addWidget(button);
		expressionChoosingButtons[i] = button;
	}
	connect(expressionChoosingButtonsMapper, static_cast<void(QSignalMapper::*)(int)> (&QSignalMapper::mapped), this, &TutorYourself::expressionChoosingButtonClicked);

}

void TutorYourself::setWordPropertiesPage(WordProperties properties)
{
	currentWordProperties = properties;
	commitLineEdit(meaningLineEdit, properties.meaning);
	commitLineEdit(translationLineEdit, properties.translation);
	addContext_hint_label->setText("");
	addForm_hint_label->setText("");

	widgetsOfFormsList.data = &currentWordProperties.forms;
	widgetsOfContextsList.data = &currentWordProperties.contexts;
	setList(&widgetsOfFormsList);
	setList(&widgetsOfContextsList);

	switch (properties.wordClass)
	{
	case Other:
		isOtherRadioButton->toggle();
		break;
	case Verb:
		isVerbRadioButton->toggle();
		break;
	case Noun:
		isNounRadioButton->toggle();
		break;
	case Adverb:
		isAdverbRadioButton->toggle();
		break;
	case Adjective:
		isAdjectiveRadioButton->toggle();
	}
}

void TutorYourself::setGroupPropertiesPage(GroupProperties properties)
{
	wordsCountLabel->setText(QString::number(properties.wordsCount));
}

void TutorYourself::setCurrentElementData(QVariant variant, int role)
{
	QModelIndex current = libraryTreeView->selectionModel()->currentIndex();
	QAbstractItemModel *model = libraryTreeView->model();
	if (current.isValid() && model->data(current, role) != variant)
	{
		model->setData(current, variant, role);
	}
}

void TutorYourself::commitLineEdit(QLineEdit * lineEdit, const QString &text)
{
	if (lineEdit->text() != text)
		lineEdit->setText(text);

	lineEdit->setStyleSheet("QLineEdit{color: black}");
}

void TutorYourself::commitAddRowLineEdit(WidgetsOfList * widgetsOfList)
{
	commitAddRow(widgetsOfList->lineEdit->text(), widgetsOfList);
}

bool TutorYourself::commitAddRow(const QString & text, WidgetsOfList * widgetsOfList)
{
	if (text.isEmpty())
		return false;

	StringParsingError err;
	QString string = CALL_RowStringParser(*this, widgetsOfList->stringParser)(text, -1, err);
	if (err == NoError)
	{
		widgetsOfList->data->append(string);
		addRow(string, widgetsOfList);
		widgetsOfList->label->setText("");
		widgetsOfList->lineEdit->setText("");
		saveCurrentWordProperties();
		return true;
	}
	else if (err == SameExist)
		widgetsOfList->label->setText(alreadyContains);
	else if (err == NoFormIncluded)
		widgetsOfList->label->setText(mustIncludeForm);
	return false;
}

void TutorYourself::saveCurrentWordProperties()
{
	libraryModel->setData(getCurrentModelIndex(), QVariant::fromValue<WordProperties>(currentWordProperties), WordRole);
}

QModelIndex TutorYourself::getCurrentModelIndex()
{
	return libraryTreeView->selectionModel()->currentIndex();
}

QModelIndex TutorYourself::addElement()
{
	QModelIndex index;
	QAbstractItemModel *model = libraryTreeView->model();
	QModelIndex parent = libraryTreeView->selectionModel()->currentIndex();
	int row = model->rowCount(parent);
	if (model->insertRow(row, parent))
	{
		index = model->index(row, 0, parent);
		libraryTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
	}
	return index;
	libraryTreeView->setFocus();
}

void TutorYourself::setList(WidgetsOfList *widgetsOfList)
{
	int count = widgetsOfList->listWidget->count();
	for (int i = 0; i < count; i++)
	{
		deleteRow(widgetsOfList->listWidget, 0);
	}
	foreach(QString string, *(widgetsOfList->data))
	{
		addRow(string, widgetsOfList);
	}
}

void TutorYourself::deleteRow(QListWidget *listWidget, int row)
{
	QListWidgetItem *item = listWidget->item(row);
	QWidget *widget = listWidget->itemWidget(item);
	listWidget->removeItemWidget(item);
	delete widget;
	delete listWidget->takeItem(row);
}

void TutorYourself::addRow(const QString & string, WidgetsOfList* widgetsOfList)
{
	QListWidgetItem *item = new QListWidgetItem();
	RowWidget *itemWidet = new RowWidget(string, widgetsOfList, item);
	widgetsOfList->listWidget->addItem(item);
	widgetsOfList->listWidget->setItemWidget(item, itemWidet);
	item->setSizeHint(itemWidet->minimumSizeHint());
	connect(itemWidet, &RowWidget::textChanged, this, &TutorYourself::rowTextChanged);
	connect(itemWidet, &RowWidget::askToDestoy, this, &TutorYourself::rowAsksToDestroy);
}

QString TutorYourself::formStringParser(const QString & text, int row, StringParsingError & error)
{
	QStringList forms = *(widgetsOfFormsList.data);
	QString outString = text.trimmed().toLower();

	error = parseForDuplicates(outString, row, forms);
	return outString;
}

QString TutorYourself::contextStringParser(const QString & text, int row, StringParsingError & error)
{
	QStringList contexts = *(widgetsOfContextsList.data);
	QString outString = text.trimmed();
	error = parseForDuplicates(outString, row, contexts);

	if (error == NoError)
	{
		QString newForm = parseForNewForm(outString, "@");
		if (!newForm.isEmpty())
		{
			commitAddRow(newForm, &widgetsOfFormsList);
		}
		else if (parseForExistingForm(outString).isEmpty())
			error = NoFormIncluded;
	}

	return outString;

}

StringParsingError TutorYourself::parseForDuplicates(const QString & text, int row, const QStringList & strList)
{
	int index = strList.indexOf(text);
	if (index!= -1 && index != row)
		return SameExist;
	else
		return NoError;
}

QString TutorYourself::parseForExistingForm(const QString & string) const
{
	QString longestForm = "";
	foreach(QString form, currentWordProperties.forms)
	{
		if (string.contains(form, Qt::CaseInsensitive) && form.length() > longestForm.length())
			longestForm = form;
	}
	return longestForm;
}

QString TutorYourself::parseForNewForm(QString & refString, const QString & symbol) const
{
	int index = refString.indexOf(symbol);
	index += symbol.length();
	int n = refString.indexOf(symbol, index) - index;
	if (index == -1 || n < 1)
		return QString();
	QString form = refString.mid(index, n);
	refString = refString.remove(symbol, Qt::CaseInsensitive);
	return form;;
}

void TutorYourself::setLibrary(Element * element)
{
	libraryModel->setDocument(element);
	isLibraryModified = false;
	libraryTreeView->selectionModel()->setCurrentIndex(libraryModel->documentIndex(), QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

bool TutorYourself::okToContinue()
{
	if (isLibraryModified)
	{
		int r = QMessageBox::warning(0, "Library", tr("The library has been modified.\nDo you want to save your changes?"), QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (r == QMessageBox::Yes)
			return saveLibrary();
		else if (r == QMessageBox::Cancel)
			return false;
	}
	return true;
}

bool TutorYourself::saveFile(const QString &fileName)
{
	if (LibraryDomParser::saveFile(libraryModel->getDocument(), fileName))
	{
		isLibraryModified = false;
		QString str = QFileInfo(fileName).fileName().split(".", QString::SkipEmptyParts).at(0);
		if (libraryNameLineEdit->text() != str)
			commitLineEdit(libraryNameLineEdit, str);
		currentFile = fileName;
		return true;
	}
	return false;
}

bool TutorYourself::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Library"), QDir::currentPath() + "/" + libraryNameLineEdit->text().remove(QChar(' '), Qt::CaseInsensitive) + ".xml", tr("XML files(*.xml)"));
	if (!fileName.isEmpty())
	{
		return saveFile(fileName);
	}
	return false;
}

void TutorYourself::openFile(const QString & fileName)
{
	if (!fileName.isEmpty())
	{
		Element *element = LibraryDomParser::parseFile(fileName);
		if (element)
		{
			if (currentFile != fileName)
				currentFile = fileName;
			QString str = QFileInfo(fileName).fileName();
			if (libraryNameLineEdit->text() != str)
				commitLineEdit(libraryNameLineEdit,str.split(".", QString::SkipEmptyParts).at(0));
			setLibrary(element);
		}
		else
			setLibrary(new Element(false, true, "New Library"));
	}
}

void TutorYourself::setElementEditorEnabled(bool b)
{
	elementEditorWidgetContents->setEnabled(b);
	QList<QWidget *> testList = elementEditorWidgetContents->findChildren<QWidget*>();
	for (int i = 0; i < testList.count(); i++)
	{
		QWidget* wid = testList.at(i);
		setWidgetEnabled(wid, b);

	}
}

void TutorYourself::writeSettings()
{
	QSettings settings("Ivan's Inc", "TutorYourself");
	settings.setValue("geometry", geometry());
	settings.setValue("currentLibrary", currentFile);
	settings.setValue("currentRunFlags", currentRunFlags);
}

void TutorYourself::readSettings()
{
	QSettings settings("Ivan's Inc", "TutorYourself");
	QRect rect = settings.value("geometry", QRect(QPoint(100, 100), sizeHint())).toRect();
	move(rect.topLeft());
	resize(rect.size());
	currentFile = settings.value("currentLibrary", currentFile).toString();
	currentRunFlags = settings.value("currentRunFlags", IncludeVerbs | IncludeAdjectives |
		IncludeNouns | IncludeAdverbs | IncludeOthers | IncludeTranslations |
		IncludeMeanings | IncludeContexts | MixWordClasses |
		AskForWord | AskForInterpretation
	).toInt();
}

void TutorYourself::resizeRowWidget(QListWidget *listWidget)
{
	QScrollBar *scrollBar = listWidget->verticalScrollBar();
	int _widgth = listWidget->width() - (!scrollBar->visibleRegion().isEmpty()? scrollBar->width() : 0);
	for (int row = 0; row < listWidget->count(); row++)
	{
		QListWidgetItem* item = listWidget->item(row);
		QWidget *widget = listWidget->itemWidget(item);
		widget->resize(_widgth, widget->height());
	}
}

void TutorYourself::setLabelColor(QLabel *label, ColorType labelColor)
{
	QPalette labelPalette = label->palette();
	QColor color;
	switch (labelColor)
	{
	case Red:
		color = QColor(255, 0, 0);
		break;
	case Green:
		color = QColor(144, 238, 144);
		break;
	case Default:
		color = this->palette().text().color();
	}
	labelPalette.setColor(QPalette::WindowText, color);
	label->setPalette(labelPalette);
}

void TutorYourself::setButtonColor(QPushButton * button, ColorType buttonColor)
{
	switch (buttonColor)
	{
	case Red:
		button->setStyleSheet("QPushButton{color: red; border-color: red}");
		break;
	case Green:
		button->setStyleSheet("QPushButton{color:lightgreen; border-color: lightgreen}");
		break;
	case Default:
		button->setStyleSheet("");
	}
	updateWidgetStyleSheet(button);
}

void TutorYourself::setWidgetEnabled(QWidget * widget,bool enabled)
{
	if (widget->isEnabled() != enabled)
	{
		widget->setEnabled(enabled);
		updateWidgetStyleSheet(widget);
	}
}

void TutorYourself::setWidgetShown(QWidget * widget, bool shown)
{
	if (widget->isHidden() && shown)
		widget->show();
	else if (!widget->isHidden() && !shown)
		widget->hide();
}

inline void TutorYourself::updateWidgetStyleSheet(QWidget * widget)
{
	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
}

inline void TutorYourself::setRunSettingsButtons()
{
	includeVerbsCheckBox->setChecked(currentRunFlags & IncludeVerbs);
	includeAdjectivesCheckBox->setChecked(currentRunFlags & IncludeAdjectives);
	includeNounsCheckBox->setChecked(currentRunFlags & IncludeNouns);
	includeAdverbsCheckBox->setChecked(currentRunFlags & IncludeAdverbs);
	includeOthersCheckBox->setChecked(currentRunFlags & IncludeOthers);
	mixWordClassesCheckBox->setChecked(currentRunFlags & MixWordClasses);
	includeTranslationsCheckBox->setChecked(currentRunFlags & IncludeTranslations);
	includeMeaningsCheckBox->setChecked(currentRunFlags & IncludeMeanings);
	includeContextsCheckBox->setChecked(currentRunFlags & IncludeContexts);
	includeInputCheckBox->setChecked(currentRunFlags & IncludeInputting);

	if (currentRunFlags & AskForWord && currentRunFlags & AskForInterpretation)
		bothRadioButton->click();
	else if (currentRunFlags & AskForWord)
		wordInterpretationRadioButton->click();
	else if (currentRunFlags & AskForInterpretation)
		interpretationWordRadioButton->click();

}

void TutorYourself::loadStage(int index)
{
	if (index < chosenStagesCount && index >= 0)
	{
		if (currentStageIndex >= 0 && currentStageIndex < chosenStagesCount)
		{
			StageItemState state;
			if (currentStage->finished)
				if (currentStage->completed)
					state = Completed;
				else
					state = Failed;
			else
					state = Next;
			setStageItemState(currentStageIndex, state);
		}


		currentStageIndex = index;
		currentStage = stages.at(index);

		setStageItemState(index, Current);
		switch (currentStage->stageMapping)
		{
		case WordChoosing:
		{
			showOptionsButtons(wordChoosingButtons,
				wordChoosingButtonsRows * wordChoosingButtonsColumns, *currentStage);
			setChoiceLabels();
			stagesStackedWidget->setCurrentIndex(1);
			choiceTypeStackedWidget->setCurrentIndex(0);
			break;
		}
		case ExpressionChoosing:
		{
			showOptionsButtons(expressionChoosingButtons,
				expressionChoosingButtonsCount,*currentStage);
			setChoiceLabels();
			stagesStackedWidget->setCurrentIndex(1);
			choiceTypeStackedWidget->setCurrentIndex(1);
			break;
		}
		case InputtingOption:
		{
			setInputLabels();
			stagesStackedWidget->setCurrentIndex(2);
			break;
		}
		}
		
		setWidgetShown(nextPushButton, currentStage->finished);
		setWidgetShown(aboutPushButton, currentStage->finished);

		setWidgetShown(previousPushButton, index != 0 ? true:false);

		breakPushButton->show();
		return;
	}
	showResults();
}

inline void TutorYourself::loadNextStage()
{
	if (currentStageIndex + 1 == stages.count())
		showResults();
	else
		stagesListWidget->setCurrentRow(currentStageIndex + 1);
}

inline void TutorYourself::setChoiceLabels()
{
	choice_label->setText(currentStage->header);
	choice_hint_label->setText(currentStage->hint);
}

inline void TutorYourself::setInputLabels()
{
	input_header_label->setText(currentStage->header);
	input_hint_label->setText(currentStage->hint);
	attempts_count_label->setText(QString::number(currentStage->rightOption));
	setWidgetEnabled(input_lineEdit, !currentStage->finished);

	input_lineEdit->setText(currentStage->options[1]);
}

void TutorYourself::showResults()
{
	completedCountLabel->setText(QString::number(completed));
	failedCountLabel->setText(QString::number(failed));
	stagesStackedWidget->setCurrentIndex(3);
	previousPushButton->hide();
	nextPushButton->hide();
	aboutPushButton->hide();
	breakPushButton->setText("Done");
	breakPushButton->show();
}

void TutorYourself::toStartPage()
{
	started = false;
	clearStagesItems();

	breakPushButton->hide();
	previousPushButton->hide();
	aboutPushButton->hide();
	nextPushButton->show();

	updateStages();

	stagesStackedWidget->setCurrentIndex(0);
}

void TutorYourself::updateStages()
{
	qDeleteAll(stages);
	stages = libraryModel->getStages(currentRunFlags);
	MaxStagesCount = stages.count();
	stagesCountSpinBox->setRange(0, MaxStagesCount);
	stagesCountSpinBox->setSuffix("\\" + QString::number(MaxStagesCount));
	if (chosenStagesCount > MaxStagesCount)
		chosenStagesCount = MaxStagesCount;
	finishedStages_label->setText(QString::number(0) + '/' + QString::number(chosenStagesCount));
}

void TutorYourself::showOptionsButtons(QPushButton **buttons, int buttonsArrayLenght, Stage stage)
{
	for(int i = 0; i < buttonsArrayLenght; i++)
	{
		if (i < stage.options.count())
		{
			
			//0-default 1-rightOption 2-failedOption
			int fastState = 0;

			if (stage.chosenOptions.contains(i))
				if (i == stage.rightOption)
					fastState = 1;
				else
					fastState = 2;

			switch (fastState)
			{
			case 1:
				setButtonColor(buttons[i], Green);
				break;
			case 2:
				setButtonColor(buttons[i], Red);
				break;
			case 0:
				setButtonColor(buttons[i], Default);
			}
			buttons[i]->setText(stage.options[i]);
			buttons[i]->show();
		}
		else
			buttons[i]->hide();
	}
}

void TutorYourself::updateFinishedStagesCountLabel()
{
	finishedStages_label->setText(QString::number(completed + failed) + '/' + QString::number(chosenStagesCount));
}

inline void TutorYourself::addStageItem(const QString & text)
{
	QListWidgetItem *item = new QListWidgetItem(text);
	item->setIcon(*nextItemIcon);
	stagesListWidget->addItem(item);
}

inline void TutorYourself::setStageItemState(int row,StageItemState state)
{
	QIcon *icon;
	switch (state)
	{
	case Next:
		icon = nextItemIcon;
		break;
	case Completed:
		icon = completedItemIcon;
		break;
	case Failed:
		icon = failedItemIcon;
		break;
	case Current:
		icon = currentItemIcon;
	}

	stagesListWidget->item(row)->setIcon(*icon);
}

void TutorYourself::stageFinished(bool isCompleted)
{
	currentStage->finished = true;
	currentStage->completed = isCompleted;

	if (currentStage->stageMapping == StageMapping::InputtingOption)
		setWidgetEnabled(input_lineEdit, false);

	if (isCompleted)
		completed++;
	else
		failed++;
	updateFinishedStagesCountLabel();
	if (currentStageIndex + 1 < stages.count())
		addStageItem(stages[currentStageIndex + 1]->header);

	nextPushButton->show();
	aboutPushButton->show();
}

void TutorYourself::optionChosen(QPushButton** buttons ,int index)
{
	if (!currentStage->chosenOptions.contains(index))
	{
		currentStage->chosenOptions.append(index);
		
		bool isCompleted = index == currentStage->rightOption;
		
		setButtonColor(buttons[index], isCompleted ? Green: Red);

		if (!currentStage->finished)
		{
			stageFinished(isCompleted);
		}

	}
}

void TutorYourself::input_lineEditEditingOver()
{
	QString text = input_lineEdit->text().trimmed().toLower();
	if (text == currentStage->options[0])
	{
		stageFinished(true);
		return;
	}

	currentStage->rightOption--;
	attempts_count_label->setText(QString::number(currentStage->rightOption));
	if(currentStage->rightOption == 0)
		stageFinished(false);

	//currentStage->rightOption serves count of attempts here

}

void TutorYourself::clearStagesItems()
{
	for (int i = 0; i < stagesListWidget->count(); i++)
	{
		delete stagesListWidget->takeItem(i);
	}
	stagesListWidget->clear();
}
