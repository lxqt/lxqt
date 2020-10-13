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

#ifndef QKEYSEQUENCEWIDGET_H
#define QKEYSEQUENCEWIDGET_H

#include "qkeysequencewidget_p.h"

#include <QWidget>
#include <QIcon>

#if defined IS_SHARED
#define QKSW_EXPORT Q_DECL_EXPORT
#else
#define QKSW_EXPORT Q_DECL_IMPORT
#endif

class QKeySequenceWidgetPrivate;

/*!
  \class QKeySequenceWidget

  \brief The QKeySequenceWidget is a widget to input a QKeySequence.

  This widget lets the user choose a QKeySequence, which is usually used as a
  shortcut key. The recording is initiated by calling captureKeySequence() or
  the user clicking into the widget.

  \code
    // create new QKeySequenceWidget with empty sequence
    QKeySequenceWidget *keyWidget = new QKeySequenceWidget;

    // Set sequence as "Ctrl+Alt+Space"
    keyWidget->setJeySequence(QKeySequence("Ctrl+Alt+Space"));

    // set clear button position is left
    setClearButtonShow(QKeySequenceWidget::ShowLeft);

    // set cutom clear button icon
    setClearButtonIcon(QIcon("/path/to/icon.png"));

    // connecting keySequenceChanged signal to slot
    connect(keyWidget, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(slotKeySequenceChanged(QKeySequence)));
  \endcode
*/
class QKSW_EXPORT QKeySequenceWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QKeySequenceWidget);
    Q_PRIVATE_SLOT(d_func(), void doneRecording())

    Q_PROPERTY(QKeySequence keySequence READ keySequence WRITE setKeySequence)
    Q_PROPERTY(QKeySequenceWidget::ClearButtonShow clearButton READ clearButtonShow WRITE setClearButtonShow)
    Q_PROPERTY(QString noneText READ noneText WRITE setNoneText)
    Q_PROPERTY(QIcon clearButtonIcon READ clearButtonIcon WRITE setClearButtonIcon)

private:
    QKeySequenceWidgetPrivate * const d_ptr;
    void _connectingSlots();

public:
    explicit QKeySequenceWidget(QWidget *parent = 0);
    explicit QKeySequenceWidget(const QKeySequence &seq, QWidget *parent = 0);
    explicit QKeySequenceWidget(const QString &noneString, QWidget *parent = 0);
    explicit QKeySequenceWidget(const QKeySequence &seq, const QString &noneString, QWidget *parent = 0);
    virtual ~QKeySequenceWidget();
    QSize sizeHint() const;
    void setToolTip(const QString &tip);
    QKeySequence keySequence() const;
    QString noneText() const;
    QIcon clearButtonIcon() const;

    /*!
      \brief Modes of sohow ClearButton
    */
    enum ClearButton {
        NoShow      = 0x00, /**< Hide ClearButton */
        ShowLeft    = 0x01, /**< ClearButton isow is left */
        ShowRight   = 0x02  /**< ClearButton isow is left */
    };

    Q_DECLARE_FLAGS(ClearButtonShow, ClearButton);
    Q_FLAGS(ClearButtonShow)

    QKeySequenceWidget::ClearButtonShow clearButtonShow() const;

Q_SIGNALS:
    void keySequenceChanged(const QKeySequence &seq);
    void keySequenceAccepted(const QKeySequence &seq);
    void keySequenceCleared();
    void keyNotSupported();

public Q_SLOTS:
    void setKeySequence(const QKeySequence &key);
    void clearKeySequence();
    void setNoneText(const QString &text);
    void setClearButtonIcon(const QIcon& icon);
    void setClearButtonShow(QKeySequenceWidget::ClearButtonShow show);
    void captureKeySequence();
};

#endif // QKEYSEQUENCEWIDGET_H
