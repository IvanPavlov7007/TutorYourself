#pragma once
#include <QTextEdit>

class EditableLabel : public QTextEdit
{
	Q_OBJECT
public:
	EditableLabel(QString text = "", QWidget *parent = 0);
	void setIsEditing(bool b);
	void setEditFrameShape(QFrame::Shape shape, QFrame::Shadow shadow, int lineWidth);
signals:
	void editingOver(const QString &changedText);
	void startEditing();
protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
	void focusOutEvent(QFocusEvent *event);
	void keyPressEvent(QKeyEvent *event);
private:
	bool isEditing;
	QFrame::Shape editFrameShape;
};
