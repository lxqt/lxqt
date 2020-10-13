/******************************************************************************
Copyright (c) 2010, Artem Galichkin <doomer3d@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <QEvent>
#include <QKeyEvent>
#include <QIcon>

#include "qkeysequencewidget_p.h"
#include "qkeysequencewidget.h"

/*!
  Creates a QKeySequenceWidget object wuth \a parent and empty \a keySequence
*/
QKeySequenceWidget::QKeySequenceWidget(QWidget *parent) :
    QWidget(parent), d_ptr(new QKeySequenceWidgetPrivate())
{
    Q_D(QKeySequenceWidget);
    d->q_ptr = this;
    d->init(QKeySequence(), QString());

    _connectingSlots();
}

/*!
  Creates a QKeySequenceWidget object wuth \a parent and keysequence \a keySequence
  and string for \a noneString
*/
QKeySequenceWidget::QKeySequenceWidget(QKeySequence const &seq, const QString &noneString, QWidget *parent) :
        QWidget(parent), d_ptr(new QKeySequenceWidgetPrivate())
{
    Q_D(QKeySequenceWidget);
    d->q_ptr = this;
    d->init(seq, noneString);
    _connectingSlots();
}

/*!
  Creates a QKeySequenceWidget object wuth \a parent and keysequence \a keySequence
*/
QKeySequenceWidget::QKeySequenceWidget(const QKeySequence &seq, QWidget *parent) :
        QWidget(parent), d_ptr(new QKeySequenceWidgetPrivate())
{
    Q_D(QKeySequenceWidget);
    d->q_ptr = this;
    d->init(seq, QString());
    _connectingSlots();
}

/*!
  Creates a QKeySequenceWidget object wuth \a parent and string for \a noneString
*/
QKeySequenceWidget::QKeySequenceWidget(const QString &noneString, QWidget *parent) :
        QWidget(parent), d_ptr(new QKeySequenceWidgetPrivate())
{
    Q_D(QKeySequenceWidget);
    d->q_ptr = this;
    d->init(QKeySequence(), noneString);

    _connectingSlots();
}

/*!
  Destroy a QKeySequenceWidget object
*/
QKeySequenceWidget::~QKeySequenceWidget()
{
    delete d_ptr;
}

QSize QKeySequenceWidget::sizeHint() const
{
    return d_ptr->shortcutButton->sizeHint();
}

/*!
  Setting tooltip text to sequence button
  \param tip Text string
*/
void QKeySequenceWidget::setToolTip(const QString &tip)
{
    d_ptr->setToolTip(tip);
}

/*!
  Setting mode of Clear Buttorn display.
  \param show Position of clear button \a ClearButtornShow
  \sa clearButtonShow
*/
void QKeySequenceWidget::setClearButtonShow(QKeySequenceWidget::ClearButtonShow show)
{
    d_ptr->showClearButton = show;
    d_ptr->updateView();
}

/*!
  Return mode of clear button dosplay.
  \param show Display mode of clear button (NoShow, ShowLeft or ShorRight)
  \sa setClearButtonShow
*/
QKeySequenceWidget::ClearButtonShow QKeySequenceWidget::clearButtonShow() const
{
    return static_cast<QKeySequenceWidget::ClearButtonShow>(d_ptr->showClearButton);
}


/*!
    Set the key sequence.
    \param key Key sequence
    \sa clearKeySequence
 */
void QKeySequenceWidget::setKeySequence(const QKeySequence& key)
{
    if (d_ptr->isRecording == false)
    {
        d_ptr->oldSequence = d_ptr->currentSequence;
    }

    d_ptr->currentSequence = key;
    d_ptr->doneRecording();
}

/*!
    Get current key sequence.
    \return Current key sequence
    \sa setKeySequence
    \sa clearKeySequence
 */
QKeySequence QKeySequenceWidget::keySequence() const
{
    return d_ptr->currentSequence;
}

/*!
    Clear key sequence.
    \sa setKeySequence
 */
void QKeySequenceWidget::clearKeySequence()
{
    d_ptr->clearSequence();
}

/*!
 * Start process capturing key sequence.
 * This slot is designed for software process capturing key sequence.
 */
void QKeySequenceWidget::captureKeySequence()
{
    d_ptr->startRecording();
}

/*!
    Set string for display when key sequence is undefined.
    \param text Text string
    \sa noneText
 */
void QKeySequenceWidget::setNoneText(const QString &text)
{
    d_ptr->noneSequenceText = text;
    d_ptr->updateDisplayShortcut();
}

/*!
    Get string for display when key sequence is undefined.
    \return Text string
    \sa setNoneText
 */
QString QKeySequenceWidget::noneText() const
{
    return d_ptr->noneSequenceText;
}

/*!
    Set custom icon for clear buttom.
    \param icon QIcon object
    \sa clearButtonIcon
 */
void QKeySequenceWidget::setClearButtonIcon(const QIcon &icon)
{
    d_ptr->clearButton->setIcon(icon);
}

/*!
    Get clear buttom icon.
    \return QIcon object
    \sa setClearButtonIcon
 */
QIcon QKeySequenceWidget::clearButtonIcon() const
{
    return d_ptr->clearButton->icon();
}

// connection internal signals & slots
void QKeySequenceWidget::_connectingSlots()
{
    // connect signals to slots
    connect(d_ptr->clearButton, SIGNAL(clicked()), this,
SLOT(clearKeySequence()));
    connect(&d_ptr->modifierlessTimeout, SIGNAL(timeout()), this, SLOT(doneRecording()));
    connect(d_func()->shortcutButton, SIGNAL(clicked()), this, SLOT(captureKeySequence()));

}

// Private class implementation

QKeySequenceWidgetPrivate::QKeySequenceWidgetPrivate()
    : layout(nullptr), clearButton(nullptr), shortcutButton(nullptr)
{
    Q_Q(QKeySequenceWidget);
    Q_UNUSED(q);
}

QKeySequenceWidgetPrivate::~QKeySequenceWidgetPrivate()
{

}

void QKeySequenceWidgetPrivate::init(const QKeySequence &keySeq, const QString &noneStr)
{
    Q_Q(QKeySequenceWidget);
    Q_UNUSED(q);
    layout = new QHBoxLayout(q_func());
    layout->setMargin(0);
    layout->setSpacing(1);

    clearButton = new QToolButton(q_func());
    clearButton->setText(QStringLiteral("x"));

    layout->addWidget(clearButton);

    shortcutButton = new QShortcutButton(this, q_func());

    if (noneStr.isNull() == true)
    {
        noneSequenceText = QLatin1String("...");
    }
    else
    {
        noneSequenceText = noneStr;
    }

    q_ptr->clearKeySequence();
    currentSequence = keySeq;
    oldSequence = currentSequence;

    shortcutButton->setFocusPolicy(Qt::StrongFocus);

    layout->addWidget(shortcutButton);

    showClearButton = QKeySequenceWidget::ShowRight;

    clearButton->setIcon(QIcon(":/img/delete_32.png"));

    // unfocused clear button  afyer created (small hack)
    clearButton->setFocusPolicy(Qt::NoFocus);

    // update ui
    updateDisplayShortcut();
    updateView();
}

// set tooltip only for seqyence button
void QKeySequenceWidgetPrivate::setToolTip(const QString &tip)
{
    shortcutButton->setToolTip(tip);
    clearButton->setToolTip(QLatin1String(""));
}

// update the location of widgets
void QKeySequenceWidgetPrivate::updateView()
{
    switch(showClearButton)
    {
    case QKeySequenceWidget::ShowLeft:
        clearButton->setVisible(true);
        layout->setDirection(QBoxLayout::LeftToRight);
        break;
    case QKeySequenceWidget::ShowRight:
        clearButton->setVisible(true);
        layout->setDirection(QBoxLayout::RightToLeft);
        break;
    case QKeySequenceWidget::NoShow:
        clearButton->setVisible(false);
        break;
    default:
        layout->setDirection(QBoxLayout::LeftToRight);
    }
}

void QKeySequenceWidgetPrivate::startRecording()
{
    numKey = 0;
    modifierKeys = 0;
    oldSequence = currentSequence;
    currentSequence = QKeySequence();
    isRecording = true;
    shortcutButton->setDown(true);

    shortcutButton->grabKeyboard();

    if (!QWidget::keyboardGrabber())
    {
        qWarning() << "Failed to grab the keyboard! Most likely qt's nograb option is active";
    }

    // update Shortcut display
    updateDisplayShortcut();
}

void QKeySequenceWidgetPrivate::doneRecording()
{
    modifierlessTimeout.stop();

    if (isRecording == true)
    {
	emit q_ptr->keySequenceAccepted(currentSequence);
    }
    else
    {
	if (oldSequence.isEmpty() != true)
	{
	    emit q_ptr->keySequenceChanged(currentSequence);
	}
    }

    isRecording = false;
    shortcutButton->releaseKeyboard();
    shortcutButton->setDown(false);

    // if sequence is not changed
    if (currentSequence == oldSequence)
    {
        // update Shortcut display
        updateDisplayShortcut();

        return;
    }
    // update Shortcut display
    updateDisplayShortcut();
}

inline void QKeySequenceWidgetPrivate::cancelRecording()
{
    currentSequence = oldSequence;
    doneRecording();
}

inline void QKeySequenceWidgetPrivate::clearSequence()
{
    q_ptr->setKeySequence(QKeySequence());
    emit q_ptr->keySequenceCleared();
}

inline void QKeySequenceWidgetPrivate::controlModifierlessTimout()
{
    if (numKey != 0 && !modifierKeys)
    {
        // No modifier key pressed currently. Start the timout
        modifierlessTimeout.start(600);
    }
    else
    {
        // A modifier is pressed. Stop the timeout
        modifierlessTimeout.stop();
    }
}


inline void QKeySequenceWidgetPrivate::keyNotSupported()
{
    Q_EMIT q_ptr->keyNotSupported();
}

void QKeySequenceWidgetPrivate::updateDisplayShortcut()
{
    // empty string if no non-modifier was pressed
    QString str = currentSequence.toString(QKeySequence::NativeText);
    str.replace('&', QLatin1String("&&"));  // TODO -- check it

    if (isRecording == true)
    {
        if (modifierKeys)
        {
            if (str.isEmpty() == false)
                str.append(",");

            if ((modifierKeys & Qt::META) )
                str += QLatin1String("Meta + ");

            if ((modifierKeys & Qt::CTRL) )
                str += QLatin1String("Ctrl + ");

            if ((modifierKeys & Qt::ALT) )
                str += QLatin1String("Alt + ");

            if ((modifierKeys & Qt::SHIFT) )
                str += QLatin1String("Shift + ");
        }

        // make it clear that input is still going on
        str.append("...");
    }

    // if is noting
    if (str.isEmpty() == true)
    {
        str = noneSequenceText;
    }

    shortcutButton->setText(str);
}


// QKeySequenceButton implementation
QSize QShortcutButton::sizeHint() const
{
    return QPushButton::sizeHint();
}

bool QShortcutButton::event(QEvent *e)
{
    if (d->isRecording == true && e->type() == QEvent::KeyPress)
    {
        keyPressEvent(static_cast<QKeyEvent*>(e));
        return true;
    }

    if (d->isRecording && e->type() == QEvent::ShortcutOverride)
    {
        e->accept();
        return true;
    }

    if (d->isRecording == true && e->type() == QEvent::FocusOut)
    {
        d->cancelRecording();
        return true;
    }

    return QPushButton::event(e);
}

void QShortcutButton::keyPressEvent(QKeyEvent *keyEvent)
{
    int keyQt =  keyEvent->key();

// Qt sometimes returns garbage keycodes, I observed -1,
// if it doesn't know a key.
// We cannot do anything useful with those (several keys have -1,
// indistinguishable)
// and QKeySequence.toString() will also yield a garbage string.
    if (keyQt == -1)
    {
        // keu moy supported in Qt
        d->cancelRecording();
        d->keyNotSupported();

    }

    //get modifiers key
    uint newModifiers = keyEvent->modifiers() & (Qt::SHIFT | Qt::CTRL | Qt::ALT
| Qt::META);

    // block autostart capturing on key_return or key space press
    if (d->isRecording == false && (keyQt == Qt::Key_Return || keyQt == Qt::Key_Space))
    {
        return;
    }

    // We get events even if recording isn't active.
    if (d->isRecording == false)
    {
        return QPushButton::keyPressEvent(keyEvent);
    }

    keyEvent->accept();
    d->modifierKeys = newModifiers;

    // switching key type
    switch(keyQt)
    {
        case Qt::Key_AltGr: //or else we get unicode salad
            return;
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_Meta:
        case Qt::Key_Menu: //unused (yes, but why?)
        // TODO - check it key
            d->controlModifierlessTimout();
            d->updateDisplayShortcut();
            break;
        default:
        {

        }

        // We now have a valid key press.
        if (keyQt)
        {
            if ((keyQt == Qt::Key_Backtab) && (d->modifierKeys & Qt::SHIFT))
            {
                keyQt = Qt::Key_Tab | d->modifierKeys;
            }
            else //if (d->isShiftAsModifierAllowed(keyQt))
            {
                keyQt |= d->modifierKeys;
            }

            if (d->numKey == 0)
            {
                d->currentSequence = QKeySequence(keyQt);
            }

            d->numKey++; // increment nuber of pressed keys

            if (d->numKey >= 4)
            {
                d->doneRecording();
                return;
            }

            d->controlModifierlessTimout();
            d->updateDisplayShortcut();
        }
    }
}

void QShortcutButton::keyReleaseEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == -1)
    {
        // ignore garbage, see keyPressEvent()
        return;
    }

    // if not recording mode
    if (d->isRecording == false)
    {
        return QPushButton::keyReleaseEvent(keyEvent);
    }

    keyEvent->accept();

    uint newModifiers = keyEvent->modifiers() & (Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META);

    // if a modifier that belongs to the shortcut was released...
    if ((newModifiers & d->modifierKeys) < d->modifierKeys)
    {
        d->modifierKeys = newModifiers;
        d->controlModifierlessTimout();
        d->updateDisplayShortcut();
    }
}
