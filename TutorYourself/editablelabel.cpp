#include "editablelabel.h"
#include <QKeyEvent>


//Cause "editingOver" signal when created
EditableLabel::EditableLabel(QString text, QWidget * parent)
	:QTextEdit(parent)
{
	editFrameShape = frameShape();
	setAutoFillBackground(true);
	setText(text);
	setIsEditing(false);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setMinimumHeight(fontMetrics().height() * 3);
}

void EditableLabel::setIsEditing(bool b)
{
	if (isEditing == b)
		return;
	isEditing = b;
	QPalette curPalette = palette();
	curPalette.setColor(QPalette::ColorRole::Base, isEditing ? Qt::white : Qt::transparent);
	setPalette(curPalette);
	setFrameShape(isEditing ? editFrameShape: QFrame::NoFrame);
	setReadOnly(!isEditing);

	if (isEditing)
	{
		emit startEditing();
		setFocus();
	}
	else
	{
		QString text = toPlainText();
		emit editingOver(text);
	}

}

void EditableLabel::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (isEditing)
	{
		QTextEdit::mouseDoubleClickEvent(event);
		return;
	}
	setIsEditing(true);
}

void EditableLabel::focusOutEvent(QFocusEvent * event)
{
	setIsEditing(false);
}

void EditableLabel::keyPressEvent(QKeyEvent * event)
{
	if (hasFocus())
		setIsEditing(true);
	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
	{
		clearFocus();
	}
	else
		QTextEdit::keyPressEvent(event);
}

void EditableLabel::setEditFrameShape(QFrame::Shape shape, QFrame::Shadow shadow, int lineWidth)
{
	editFrameShape = shape;
	if (isEditing)
		setFrameShape(shape);
	setFrameShadow(shadow);
	setLineWidth(lineWidth);
}