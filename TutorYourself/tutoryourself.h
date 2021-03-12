#pragma once
#include <QWidget>
#include "GeneratedFiles\ui_tutoryourself.h"
#include "library.h"
#include "rowwidget.h"
#include "stage.h"


#define CALL_RowStringParser(object,ptrToFunc) ((object).*(ptrToFunc))

class CheckableLibraryProxyModel;
class QSignalMapper;
class QIcon;
class TutorYourself;


enum ColorType{Green,Red,Default};

enum StageItemState{Current,Next,Failed,Completed};

enum StringParsingError{NoError,SameExist,NoFormIncluded};

typedef QString(TutorYourself::* RowStringParser)(const QString &, int , StringParsingError &);

struct WidgetsOfList
{
	QListWidget *listWidget;
	QLineEdit *lineEdit;
	QPushButton *pushButton;
	QLabel *label;
	QStringList *data;
	bool lineEditTextIsCorrect;
	RowStringParser stringParser;
};

class TutorYourself : public QWidget, public Ui::TutorYourself
{
	Q_OBJECT
public:
	TutorYourself(QWidget *parent = 0);
	~TutorYourself();
private slots:
	//Edit mode
	void setRunMode();
	void setEditMode();

	void addGroup();
	void addWord();
	void deleteElement();

	void openLibrary();
	bool saveLibrary();
	void newLibrary();

	void lineEditTextChanged();

	//current index changed
	void otherElelmentSelected(const QModelIndex &current, const QModelIndex &previous);

	//handle widgets signals to change data

	void addRowButtonClicked();

	void currentWordClassChanged();

	void rowAsksToDestroy(QListWidgetItem *item, WidgetsOfList *widgetsOfList);
	void rowTextChanged(QListWidgetItem *item, const QString &string, WidgetsOfList *widgetsOfList);

	//data changed
	void libraryModified(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);


	//Run Mode
	void checkAllButtonClicked();
	void uncheckAllButtonClicked();

	void nextPushButtonClicked();
	void previousPushButtonClicked();
	void aboutPushButtonClicked();
	void breakPushButtonClicked();

	void stagesCountSpinBoxValueChanged(int val);

	void includeTestPartCheckBoxStateChanged(int state);
	void askDestinationRadioButtonClicked();

	void wordChoosingButtonClicked(int index);
	void expressionChoosingButtonClicked(int index);

	void input_lineEditTextChanged(const QString &text);

	void otherStageSelected(QListWidgetItem *current, QListWidgetItem *previous);

protected:
	void resizeEvent(QResizeEvent * event);
	bool eventFilter(QObject *target, QEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	void setEditPageConnections();
	void setRunPageConnections();

	void setGroupPropertiesPage(GroupProperties properties);
	void setWordPropertiesPage(WordProperties properties);

	void setCurrentElementData(QVariant variant, int role);

	void commitLineEdit(QLineEdit *lineEdit,const QString &text);
	void commitAddRowLineEdit(WidgetsOfList *widgetsOfList);

	bool commitAddRow(const QString &text, WidgetsOfList *widgetsOfList);

	void saveCurrentWordProperties();

	QModelIndex getCurrentModelIndex();

	QModelIndex addElement();

	void setList(WidgetsOfList *widgetsOfList);
	void deleteRow(QListWidget *listWidget, int row);
	void addRow(const QString &string, WidgetsOfList* widgetsOfList);

	QString formStringParser(const QString &text, int row, StringParsingError &error);
	QString contextStringParser(const QString &text, int row, StringParsingError &error);
	StringParsingError parseForDuplicates(const QString &text, int row, const QStringList &strList);
	inline QString parseForExistingForm(const QString &string) const;
	QString parseForNewForm(QString &refString, const QString &symbol) const;


	bool parseString(QString &string, WidgetsOfList *widgetsOfList);

	void parseAddRowLineEditText(WidgetsOfList *widgetsOfList);

	void setLibrary(Element *element);

	bool okToContinue();
	bool saveFile(const QString &fileName);
	bool saveAs();
	void openFile(const QString &fileName);

	void setElementEditorEnabled(bool b);

	void writeSettings();
	void readSettings();

	void resizeRowWidget(QListWidget *listWidget);

	void setLabelColor(QLabel *label, ColorType labelColor);
	void setButtonColor(QPushButton *button, ColorType buttonColor);

	void setWidgetEnabled(QWidget *widget, bool enabled);
	void setWidgetShown(QWidget *widget, bool shown);
	inline void updateWidgetStyleSheet(QWidget *widget);

	QString currentFile;
	bool isLibraryModified;
	LibraryModel *libraryModel;
	CheckableLibraryProxyModel *proxyModel;

	WordProperties currentWordProperties;
	WidgetsOfList widgetsOfContextsList;
	WidgetsOfList widgetsOfFormsList;

	QString alreadyContains;
	QString mustIncludeForm;


	//Run mode
	inline void setRunSettingsButtons();


	void loadStage(int index);
	inline void loadNextStage();
	inline void setChoiceLabels();
	inline void setInputLabels();

	void showResults();
	void toStartPage();

	void updateStages();

	void showOptionsButtons(QPushButton **buttons,int buttonsArrayLenght, Stage stage);

	void updateFinishedStagesCountLabel();

	inline void addStageItem(const QString &text);
	inline void setStageItemState(int row, StageItemState state);

	void stageFinished(bool isCompleted);

	void optionChosen(QPushButton** buttons, int index);
	
	void input_lineEditEditingOver();

	void clearStagesItems();

	bool started = false;
	int currentRunFlags;
	QList<Stage*> stages;
	Stage *currentStage;
	int currentStageIndex;
	int MaxStagesCount;
	int chosenStagesCount;
	int completed;
	int failed;

	QIcon *currentItemIcon;
	QIcon *completedItemIcon;
	QIcon *nextItemIcon;
	QIcon *failedItemIcon;

	QPushButton** wordChoosingButtons;
	QPushButton** expressionChoosingButtons;
	QSignalMapper *wordChoosingButtonsMapper;
	QSignalMapper *expressionChoosingButtonsMapper;
};