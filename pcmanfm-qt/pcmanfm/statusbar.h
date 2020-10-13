/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef FM_STATUSBAR_H
#define FM_STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QTimer>

namespace PCManFM {

class Label : public QLabel {
Q_OBJECT

public:
    explicit Label(QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString elidedText_;
    QString lastText_;
    int lastWidth_;
};

class StatusBar : public QStatusBar {
Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = 0);
    ~StatusBar();

public Q_SLOTS:
    void showMessage(const QString &message, int timeout = 0);

protected Q_SLOTS:
    void reallyShowMessage();

private:
    Label* statusLabel_; // for a stable (elided) text
    QTimer* messageTimer_;
    QString lastMessage_;
    int lastTimeOut_;
};

}

#endif // FM_STATUSBAR_H
