#ifndef LIMITEDCOMBOBOX_H
#define LIMITEDCOMBOBOX_H

#include <QComboBox>
#include <QEvent>
#include <QListView>

class LimitedComboBox : public QComboBox
{
public:
	LimitedComboBox(int limit, QWidget *parent = 0)
	  : QComboBox(parent), _limit(limit)
	{
		// accelerate
		setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
		QListView* view = new QListView();
		view->setUniformItemSizes(true);
		view->setLayoutMode(QListView::Batched);
		setView(view);

		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
		setStyleSheet("combobox-popup: 0;");
	}

	QSize sizeHint() const
	{
		return QSize(qMin(_limit, QComboBox::sizeHint().width()),
		  QComboBox::sizeHint().height());
	}
	QSize minimumSizeHint() const
	{
		return QSize(qMin(_limit, QComboBox::minimumSizeHint().width()),
		  QComboBox::minimumSizeHint().height());
	}

	bool event(QEvent *e)
	{
		if (e->type() == QEvent::Polish)
			view()->setMinimumWidth(QComboBox::sizeHint().width());
		return QComboBox::event(e);
	}

private:
	int _limit;
};

#endif // LIMITEDCOMBOBOX_H
