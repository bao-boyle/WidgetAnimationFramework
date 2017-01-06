/*
 * Copyright (C) 2015  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */

#include "SlideAnimator.h"
#include "SlideForegroundDecorator.h"

#include <QEvent>
#include <QPropertyAnimation>
#include <QWidget>

using WAF::SlideAnimator;
using WAF::SlideForegroundDecorator;


SlideAnimator::SlideAnimator(QWidget* _widgetForSlide) :
	AbstractAnimator(_widgetForSlide),
	m_direction(WAF::FromLeftToRight),
	m_isFixBackground(true),
    m_isFixStartSize(false),
	m_animation(new QPropertyAnimation(_widgetForSlide, "maximumSize")),
	m_decorator(new SlideForegroundDecorator(_widgetForSlide))
{
	Q_ASSERT(_widgetForSlide);
	_widgetForSlide->installEventFilter(this);

	m_animation->setDuration(300);

	m_decorator->hide();

	connect(m_animation, &QPropertyAnimation::valueChanged, [=] {
		widgetForSlide()->setMinimumSize(widgetForSlide()->maximumSize());
	});
	//
	// Корректировки размера по завершению
	//
	connect(m_animation, &QPropertyAnimation::finished, [=] {
		setAnimatedStopped();
		m_decorator->hide();

		//
		// Отменяем фиксацию размера в направлении, которое может изменяться
		//
		if (isAnimatedBackward()) {
			widgetForSlide()->setMinimumSize(m_startMinSize);
			if (m_direction == WAF::FromLeftToRight
				|| m_direction == WAF::FromRightToLeft) {
				widgetForSlide()->setMinimumHeight(m_startMinSize.height());
				widgetForSlide()->setMaximumHeight(m_startMaxSize.height());
			} else {
				widgetForSlide()->setMinimumWidth(m_startMinSize.width());
				widgetForSlide()->setMaximumWidth(m_startMaxSize.width());
			}
		} else {
			widgetForSlide()->setMaximumSize(m_startMaxSize);
			if (m_direction == WAF::FromLeftToRight
				|| m_direction == WAF::FromRightToLeft) {
				widgetForSlide()->setMinimumHeight(m_startMinSize.height());
			} else {
				widgetForSlide()->setMinimumWidth(m_startMinSize.width());
			}
		}
	});
}

void SlideAnimator::setAnimationDirection(WAF::AnimationDirection _direction)
{
	if (m_direction != _direction) {
		m_direction = _direction;
	}
}

void SlideAnimator::setFixBackground(bool _fix)
{
	if (m_isFixBackground != _fix) {
		m_isFixBackground = _fix;
    }
}

void SlideAnimator::setFixStartSize(bool _fix)
{
    if (m_isFixStartSize != _fix) {
        m_isFixStartSize = _fix;
    }
}

int SlideAnimator::animationDuration() const
{
	return m_animation->duration();
}

void SlideAnimator::animateForward()
{
	slideIn();
}

void SlideAnimator::slideIn()
{
    if (!m_startMinSize.isValid()) {
        m_startMinSize = widgetForSlide()->minimumSize();
    }
    if (!m_startMaxSize.isValid()) {
        m_startMaxSize = widgetForSlide()->maximumSize();
    }
    if (!m_startSize.isValid()) {
        m_startSize = widgetForSlide()->sizeHint();
    }

	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedForward()) return;
	setAnimatedForward();

	//
	// Определим размер неизменяемой стороны выкатываемого виджета
	//
	switch (m_direction) {
		case WAF::FromLeftToRight:
		case WAF::FromRightToLeft: {
			widgetForSlide()->setMaximumWidth(0);
			break;
		}

		case WAF::FromTopToBottom:
		case WAF::FromBottomToTop: {
			widgetForSlide()->setMaximumHeight(0);
			break;
		}
	}
	widgetForSlide()->show();
	const QSize currentSize = widgetForSlide()->size();

	//
	// Определим финальный размер выкатываемого виджета
	//
	QSize finalSize = currentSize;
	switch (m_direction) {
		case WAF::FromLeftToRight:
		case WAF::FromRightToLeft: {
			finalSize.setWidth(m_startSize.width());
			break;
		}

		case WAF::FromTopToBottom:
		case WAF::FromBottomToTop: {
			finalSize.setHeight(m_startSize.height());
			break;
		}
	}

	//
	// Сформируем изображение выкатываемого виджета
	//
	// NOTE: т.к. у виджета меняется свойство maximumSize, его невозможно корректно отрисовать,
	//		 если высота или ширина равны нулю, поэтому применяем небольшую хитрость
	//
	widgetForSlide()->hide();
	widgetForSlide()->setFixedSize(finalSize);
	m_decorator->grabParent(finalSize);
	widgetForSlide()->setFixedSize(currentSize);
	widgetForSlide()->show();

	if (m_isFixBackground) {
		//
		// Позиционируем декоратор
		//
		m_decorator->move(0, 0);
		m_decorator->show();
		m_decorator->raise();
	}

	//
	// Выкатываем виджет
	//
	if (m_animation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_animation->pause();
		m_animation->setDirection(QPropertyAnimation::Backward);
		m_animation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_animation->setEasingCurve(QEasingCurve::OutQuart);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(widgetForSlide()->size());
		m_animation->setEndValue(finalSize);
		m_animation->start();
	}
}

void SlideAnimator::animateBackward()
{
	slideOut();
}

void SlideAnimator::slideOut()
{
    if (!m_startMinSize.isValid()) {
        m_startMinSize = widgetForSlide()->minimumSize();
    }
    if (!m_startMaxSize.isValid()) {
        m_startMaxSize = widgetForSlide()->maximumSize();
    }
    if (!m_startSize.isValid() || !m_isFixStartSize) {
        m_startSize = widgetForSlide()->size();
    }

	//
	// Прерываем выполнение, если клиент хочет повторить его
	//
	if (isAnimated() && isAnimatedBackward()) return;
	setAnimatedBackward();

	//
	// Определяем результирующий размер
	//
	QSize finalSize = widgetForSlide()->size();
	switch (m_direction) {
		case WAF::FromLeftToRight:
		case WAF::FromRightToLeft: {
			finalSize.setWidth(0);
			break;
		}

		case WAF::FromTopToBottom:
		case WAF::FromBottomToTop: {
			finalSize.setHeight(0);
			break;
		}
	}

	//
	// Сформируем изображение выкатываемого виджета
	//
	m_decorator->grabParent(widgetForSlide()->size());

	if (m_isFixBackground) {
		//
		// Позиционируем декоратор
		//
		m_decorator->move(0, 0);
		m_decorator->show();
		m_decorator->raise();
	}

	//
	// Закатываем виджет
	//
	if (m_animation->state() == QPropertyAnimation::Running) {
		//
		// ... если ещё не закончилась предыдущая анимация реверсируем её
		//
		m_animation->pause();
		m_animation->setDirection(QPropertyAnimation::Backward);
		m_animation->resume();
	} else {
		//
		// ... если предыдущая анимация закончилась, запускаем новую анимацию
		//
		m_animation->setEasingCurve(QEasingCurve::InQuart);
		m_animation->setDirection(QPropertyAnimation::Forward);
		m_animation->setStartValue(widgetForSlide()->size());
		m_animation->setEndValue(finalSize);
		m_animation->start();
	}
}

bool SlideAnimator::eventFilter(QObject* _object, QEvent* _event)
{
	if (_object == widgetForSlide()
		&& _event->type() == QEvent::Resize
		&& m_decorator->isVisible()) {
		switch (m_direction) {
			case WAF::FromLeftToRight: {
				m_decorator->move(widgetForSlide()->width() - m_decorator->width(), 0);
				break;
			}

			case WAF::FromTopToBottom: {
				m_decorator->move(0, widgetForSlide()->height() - m_decorator->height());
				break;
			}

			case WAF::FromRightToLeft:
			case WAF::FromBottomToTop: {
				//
				// Ничего не делает, позиционирование в точке (0, 0) делает всю работу
				//
				break;
			}
		}
	}

	return QObject::eventFilter(_object, _event);
}

QWidget* SlideAnimator::widgetForSlide() const
{
	return qobject_cast<QWidget*>(parent());
}
