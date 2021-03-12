#include <QtWidgets>
#include "rowwidget.h"

RowWidget::RowWidget(const QString &string, WidgetsOfList *widgetsOfList, QListWidgetItem *item, QWidget *parent)
	:widgetsOfList(widgetsOfList), item(item), QFrame(parent)
{
	setFrameShape(QFrame::Shape::Box);
	setFrameShadow(QFrame::Shadow::Plain);
	setLineWidth(2);

	editableLabel = new EditableLabel(string);
	editableLabel->setEditFrameShape(QFrame::Shape::Panel, QFrame::Shadow::Plain, 2);

	editButton = new QToolButton();
	editButton->setIcon(QIcon(":/images/edit.png"));

	deleteButton = new QToolButton();
	deleteButton->setIcon(QIcon(":/images/delete.png"));

	QSize _maximumSize(30, 30);
	deleteButton->setMaximumSize(_maximumSize);
	editButton->setMaximumSize(_maximumSize);

	QHBoxLayout *mainLayout = new QHBoxLayout();
	mainLayout->addWidget(editableLabel);
	mainLayout->addWidget(editButton);
	mainLayout->addWidget(deleteButton);

	setLayout(mainLayout);

	connect(deleteButton, &QToolButton::clicked, this, &RowWidget::deleteItemWidget);
	connect(editButton, &QToolButton::clicked, this, &RowWidget::setEditing);
	
	connect(editableLabel, &EditableLabel::startEditing, this, &RowWidget::disableEditButton);
	connect(editableLabel, &EditableLabel::editingOver, this, &RowWidget::editingOver);
}

void RowWidget::deleteItemWidget()
{
	emit askToDestoy(item, widgetsOfList);
}

void RowWidget::setEditing()
{
	editableLabel->setIsEditing(true);
}

void RowWidget::setText(const QString &string)
{
	editableLabel->setText(string);
}

void RowWidget::editingOver(const QString & changedText)
{
	editButton->setEnabled(true);
		emit textChanged(item, changedText, widgetsOfList);
}

void RowWidget::disableEditButton()
{
	editButton->setEnabled(false);
}
