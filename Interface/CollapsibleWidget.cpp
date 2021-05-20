#include "CollapsibleWidget.h"

#include <QPropertyAnimation>

Spoiler::Spoiler(const QString& title, const int animationDuration, QWidget* parent) : QWidget(parent), m_AnimationDuration(animationDuration)
{
	m_ToggleButton.setStyleSheet("QToolButton { border: none; }");
	m_ToggleButton.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_ToggleButton.setArrowType(Qt::ArrowType::RightArrow);
	m_ToggleButton.setText(title);
	m_ToggleButton.setCheckable(true);
	m_ToggleButton.setChecked(false);

	m_HeaderLine.setFrameShape(QFrame::HLine);
	m_HeaderLine.setFrameShadow(QFrame::Sunken);
	m_HeaderLine.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	m_ContentArea.setStyleSheet("QScrollArea { background-color: white; border: none; }");
	m_ContentArea.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	// start out collapsed
	m_ContentArea.setMaximumHeight(0);
	m_ContentArea.setMinimumHeight(0);
	// let the entire widget grow and shrink with its content
	m_ToggleAnimation.addAnimation(new QPropertyAnimation(this, "minimumHeight"));
	m_ToggleAnimation.addAnimation(new QPropertyAnimation(this, "maximumHeight"));
	m_ToggleAnimation.addAnimation(new QPropertyAnimation(&m_ContentArea, "maximumHeight"));
	// don't waste space
	m_MainLayout.setVerticalSpacing(0);
	m_MainLayout.setContentsMargins(0, 0, 0, 0);
	int row = 0;
	m_MainLayout.addWidget(&m_ToggleButton, row++, 0, 1, 1, Qt::AlignLeft);
	m_MainLayout.addWidget(&m_HeaderLine, row++, 0, 1, 1);
	m_MainLayout.addWidget(&m_ContentArea, row, 0, 1, 1);
	setLayout(&m_MainLayout);
	QObject_connect(&m_ToggleButton, SIGNAL(clicked()), this, SLOT(OnCollapse(bool)));
}

void Spoiler::SetContentLayout(QLayout* contentLayout)
{
	//delete contentArea.layout();
	m_ContentArea.setLayout(contentLayout);
	const auto collapsed_height = sizeHint().height() - m_ContentArea.maximumHeight();
	auto content_height = contentLayout->sizeHint().height();
	for (int i = 0; i < m_ToggleAnimation.animationCount() - 1; ++i)
	{
		QPropertyAnimation* spoiler_animation = static_cast<QPropertyAnimation*>(m_ToggleAnimation.animationAt(i));
		spoiler_animation->setDuration(m_AnimationDuration);
		spoiler_animation->setStartValue(collapsed_height);
		spoiler_animation->setEndValue(collapsed_height + content_height);
	}
	QPropertyAnimation* content_animation = static_cast<QPropertyAnimation*>(m_ToggleAnimation.animationAt(m_ToggleAnimation.animationCount() - 1));
	content_animation->setDuration(m_AnimationDuration);
	content_animation->setStartValue(0);
	content_animation->setEndValue(content_height);
}

void Spoiler::OnCollapse(bool checked)
{
	m_ToggleButton.setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
	m_ToggleAnimation.setDirection(checked ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
	m_ToggleAnimation.start();
}