/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTCPULOAD_H
#define LXQTCPULOAD_H
#include <QLabel>

class ILXQtPanelPlugin;

class LXQtCpuLoad: public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QColor fontColor READ getFontColor WRITE setFontColor)

public:
    /**
      Describes orientation of cpu load bar
     **/
    enum BarOrientation {
        BottomUpBar,    //! Bar begins at bottom and grows up
        TopDownBar,     //! Bar begins at top and grows down
        RightToLeftBar, //! Bar begins at right edge and grows to the left
        LeftToRightBar  //! Bar begins at left edge and grows to the right
    };

    LXQtCpuLoad(ILXQtPanelPlugin *plugin, QWidget* parent = nullptr);
    ~LXQtCpuLoad();


    void settingsChanged();

    void setFontColor(QColor value) { fontColor = value; }
    QColor getFontColor() const { return fontColor; }

protected:
    void virtual timerEvent(QTimerEvent *event);
    void virtual paintEvent ( QPaintEvent * event );
    void virtual resizeEvent(QResizeEvent *);

private:
    double getLoadCpu() const;
    void setSizes();

    ILXQtPanelPlugin *mPlugin;
    QWidget m_stuff;

    //! average load
    int m_avg;

    bool m_showText;
    int m_barWidth;
    BarOrientation m_barOrientation;
    int m_updateInterval;
    int m_timerID;

    QFont m_font;

    QColor fontColor;
};


#endif // LXQTCPULOAD_H
