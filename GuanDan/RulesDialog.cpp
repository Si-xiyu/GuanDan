#include "RulesDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>

RulesDialog::RulesDialog(QWidget* parent)
	: QDialog(parent)
{
	// 移除窗口标题栏的问号（帮助）按钮
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	
	setupUI();
}

RulesDialog::~RulesDialog()
{}

void RulesDialog::setupUI()
{
	setWindowTitle(tr("游戏规则介绍"));
	setMinimumSize(800, 600);

	// 主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(15);
	mainLayout->setContentsMargins(20, 20, 20, 20);

	// 滚动区域
	m_scrollArea = new QScrollArea(this);
	m_scrollArea->setWidgetResizable(true);
	m_scrollArea->setFrameShape(QFrame::NoFrame);

	// 内容标签（支持富文本/HTML）
	m_contentLabel = new QLabel(this);
	m_contentLabel->setText(getRulesAsHtml());
	m_contentLabel->setWordWrap(true);
	m_contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许用户复制文本
	m_contentLabel->setStyleSheet("QLabel { font-size: 16px; line-height: 1.8; }");

	m_scrollArea->setWidget(m_contentLabel);

	// 关闭按钮
	m_closeButton = new QPushButton(tr("关闭"), this);
	m_closeButton->setFixedSize(120, 40);
	m_closeButton->setStyleSheet("QPushButton { font-size: 16px; }");
	connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

	// 添加到布局
	mainLayout->addWidget(m_scrollArea);
	mainLayout->addWidget(m_closeButton, 0, Qt::AlignCenter);
}

QString RulesDialog::getRulesAsHtml() const
{
	return R"(
		<html><body style="font-size: 16px;">
		<p>此模式需要两副牌（每一副牌包含2到A各4张，包括红桃、黑桃、红方、梅花四个花色，大小王各1张，共54张），<b>初始级牌为2</b>，具有下列牌型：</p>
		<ol>
			<li><b>单牌：</b>牌的大小顺序为：大王、小王、级牌、A、K、…、2.</li>
			<li><b>对子：</b>大小顺序与单牌相同，最大的对子为对大王。</li>
			<li><b>三张牌：</b>可以是三不带，也可以是三带一对，但不可以三带一。</li>
			<li><b>炸弹：</b>四张及以上是炸弹，张数多则炸弹大。</li>
			<li><b>钢板：</b>连续的两组三张牌。例如555666.与斗地主不同，2 可以出现在钢板中，如AAA222和222333.最大的钢板为KKKAAA.</li>
			<li><b>顺子：</b>连续的<b>五张</b>单牌。与斗地主不同，2可以出现在顺子中，如A2345和23456.最大顺子为10JQKA.花色相同的顺子为<b>同花顺</b>，5炸<同花顺<6炸.</li>
			<li><b>连对：</b>连续的<b>三个</b>对子。与斗地主不同，2可以出现在连对中，如AA2233和223344.最大连对为QQKKAA.</li>
			<li><b>癞子规则：</b><b>红桃</b>级牌为癞子，可以替代除大小王以外的任何牌。</li>
			<li><b>升级规则：</b>一二位升三级，一三位升两级，一四位升一级。最后一个出完牌的玩家向第一个出完牌的玩家进贡；进贡的牌为自己手上最大的牌，吃贡的玩家还一张10以下的牌给进贡者。若两张大王同在输家手中，或输家手中没有大过A的牌则不用进贡。</li>
		</ol>
		
		<h3 style="font-size: 20px;">进贡与出牌</h3>
		<ol>
			<li><b>单下情况：</b>如果上一局游戏的最后一名（下游）玩家所在的团队没有包揽最后两名，则由下游玩家向上游玩家进贡一张自己手中最大的牌。</li>
			<li><b>双下情况：</b>如果上一局游戏的最后两名（三游和下游）玩家属于同一团队（即双下），则双下方的两名玩家都需要向上游方的两名玩家分别进贡一张最大的牌。</li>
			<li><b>抗贡条件：</b>如果下游玩家手中有两张大王，则可以抗贡，即无需进贡。在双下情况下，如果双下方的两名玩家各自有一张大王，或者其中一人有两张大王，也可以抗贡。</li>
		</ol>

		<h3 style="font-size: 20px;">进贡的牌选择</h3>
		<ul>
			<li>进贡的牌必须是进贡玩家手中最大的单张牌，但红桃级牌（逢人配）除外。</li>
			<li>如果进贡玩家手中没有大王，则进贡最大的单张牌。</li>
		</ul>

		<h3 style="font-size: 20px;">还贡规则</h3>
		<ul>
			<li>接受进贡的玩家需要从自己的手牌中选择一张牌还给进贡者。</li>
			<li>还给己方搭档的牌必须是10以下（含10）的牌。</li>
			<li>还给对方的牌可以是任意牌。</li>
		</ul>

		<h3 style="font-size: 20px;">出牌顺序</h3>
		<ul>
			<li>在单下情况下，进贡者先出牌。</li>
			<li>在双下情况下，进贡大者先出牌，若进贡牌点相同，则按顺时针方向确定出牌顺序。</li>
			<li>如果出现抗贡，则由上游玩家先出牌。</li>
		</ul>
		<p style="font-style: italic;">这些规则从第二局开始生效，第一局游戏结束后根据玩家的名次确定是否需要进贡。</p>

		</body></html>
	)";
}

