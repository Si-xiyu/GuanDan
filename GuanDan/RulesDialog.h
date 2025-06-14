#ifndef RULESDIALOG_H
#define RULESDIALOG_H

#include <QDialog>

class QLabel;
class QScrollArea;
class QPushButton;

class RulesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit RulesDialog(QWidget* parent = nullptr);
	~RulesDialog();

private:
	void setupUI();
	QString getRulesAsHtml() const; // 获取HTML格式的规则文本

	QLabel* m_contentLabel;
	QScrollArea* m_scrollArea;
	QPushButton* m_closeButton;
};

#endif // RULESDIALOG_H

