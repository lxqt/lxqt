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


#ifndef ILXQTPANEL_H
#define ILXQTPANEL_H
#include <QRect>
#include "lxqtpanelglobals.h"

class ILXQtPanelPlugin;
class QWidget;

/**
 **/
class LXQT_PANEL_API ILXQtPanel
{
public:
    /**
     * @brief Specifies the position of the panel on screen.
     */
    enum Position{
        PositionBottom, //!< The bottom side of the screen.
        PositionTop,    //!< The top side of the screen.
        PositionLeft,   //!< The left side of the screen.
        PositionRight   //!< The right side of the screen.
    };

    virtual ~ILXQtPanel() { }

    /**
     * @brief Returns the position of the panel. Possible values for the
     * return value are described by the Position enum.
     */
    virtual Position position() const = 0;

    /**
     * @brief Returns the edge length of the icons that are shown on the panel
     * in pixels. The icons are square.
     */
    virtual int iconSize() const = 0;
    /**
     * @brief Returns the number of lines/rows of this panel.
     */
    virtual int lineCount() const = 0;

    /**
     * @brief Helper function for convenient direction/alignment checking.
     * @return True if the panel is on the top or the bottom of the
     * screen; otherwise returns false.
     */
    bool isHorizontal() const { return position() == PositionBottom || position() == PositionTop; }

    /**
     * @brief Helper method that returns the global screen coordinates of the
     * panel, so you do not need to use QWidget::mapToGlobal() by yourself.
     * @return The QRect where the panel is located in global screen
     * coordinates.
     */
    virtual QRect globalGeometry() const = 0;

    /**
     * @brief Helper method for calculating the global screen position of a
     * popup window with size windowSize.
     * @param absolutePos Contains the global screen coordinates where the
     * popup should be appear, i.e. the point where the user has clicked.
     * @param windowSize The size that the window will occupy.
     * @return The global screen position where the popup window can be shown.
     */
    virtual QRect calculatePopupWindowPos(const QPoint &absolutePos, const QSize &windowSize) const = 0;
    /**
     * @brief Helper method for calculating the global screen position of a
     * popup window with size windowSize. The parameter plugin should be a
     * plugin
     * @param plugin Plugin that the popup window will belong to. The position
     * will be calculated according to the position of the plugin in the panel.
     * @param windowSize The size that the window will occupy.
     * @return The global screen position where the popup window can be shown.
     */
    virtual QRect calculatePopupWindowPos(const ILXQtPanelPlugin *plugin, const QSize &windowSize) const = 0;

    /*!
     * \brief By calling this function, a plugin (or any other object) notifies the panel
     * about showing a (standalone) window/menu -> the panel needs this to avoid "hiding" in case any
     * standalone window is shown. The widget/window must be shown later than this notification call because
     * the panel needs to observe its show/hide/close events.
     *
     * \param w the window that will be shown
     *
     */
    virtual void willShowWindow(QWidget * w) = 0;

    /*!
     * \brief By calling this function, a plugin notifies the panel about change of it's "static"
     * configuration
     *
     * \param plugin the changed plugin
     *
     * \sa ILXQtPanelPlugin::isSeparate(), ILXQtPanelPlugin::isExpandable
     */
    virtual void pluginFlagsChanged(const ILXQtPanelPlugin * plugin) = 0;

    /*!
     * \brief Checks if the panel is locked.
     */
    virtual bool isLocked() const = 0;
};

#endif // ILXQTPANEL_H
