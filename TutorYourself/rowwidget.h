#pragma once
#include "editablelabel.h"
class QListWidgetItem;
class QListWidget;
class QToolButton;
struct WidgetsOfList;

class RowWidget : public QFrame
{
	Q_OBJECT
public:
	RowWidget(const QString &string, WidgetsOfList *widgetsOfList, QListWidgetItem *item, QWidget *parent = 0);
	void setEditing();
	void setText(const QString &string);
signals:
	void askToDestoy(QListWidgetItem *item , WidgetsOfList *widgetsOfList);
	void textChanged(QListWidgetItem *item,const QString &string, WidgetsOfList *widgetsOfList);
private slots:
	void deleteItemWidget();
	void editingOver(const QString &changedText);
	void disableEditButton();
private:

	EditableLabel *editableLabel;
	QToolButton *editButton;
	QToolButton *deleteButton;
	WidgetsOfList *widgetsOfList;
	QListWidgetItem *item;
};