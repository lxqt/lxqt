/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "src/core/regionselect.h"

#include <QApplication>
#include <QScreen>

RegionSelect::RegionSelect(Config *mainconf, QWidget *parent)
    :QWidget(parent)
{
    _currentFit = 0;
    _conf = mainconf;
    sharedInit();

    move(0, 0);
    drawBackGround();

    _processSelection = false;
    _fittedSelection = false;

    show();

    grabKeyboard();
    grabMouse();
}

RegionSelect::RegionSelect(Config* mainconf, const QRect& lastRect, QWidget* parent)
    : QWidget(parent)
{
    _currentFit = 0;
    _conf = mainconf;
    sharedInit();
    _selectRect = lastRect;

    move(0, 0);
    drawBackGround();

    _processSelection = false;
    _fittedSelection = false;

    show();

    grabKeyboard();
    grabMouse();
}

RegionSelect::~RegionSelect()
{
    _conf = nullptr;
    delete _conf;
}

void RegionSelect::sharedInit()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    setWindowState(Qt::WindowFullScreen);
    setCursor(Qt::CrossCursor);

    auto screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr)
        screen = QGuiApplication::screens().at(0);

    _sizeDesktop = screen->virtualSize();
    resize(_sizeDesktop);

    const auto siblings = screen->virtualSiblings();
    if (siblings.size() == 1)
        _desktopPixmapBkg = screen->grabWindow(0); // 0 for the entire screen
    else
    { // consider all siblings
        _desktopPixmapBkg = QPixmap(screen->virtualSize());
        _desktopPixmapBkg.fill(Qt::transparent);
        QPainter painter(&_desktopPixmapBkg);
        for (const auto& sc : siblings)
            painter.drawPixmap(sc->geometry().topLeft(), sc->grabWindow(0));
    }

    _desktopPixmapClr = _desktopPixmapBkg;
}

void RegionSelect::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    painter.drawPixmap(QPoint(0, 0), _desktopPixmapBkg);

    drawRectSelection(painter);
}

void RegionSelect::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton)
        return;

    _selStartPoint = event->pos();
    _processSelection = true;
}

void RegionSelect::mouseReleaseEvent(QMouseEvent* event)
{
    _selEndPoint = event->pos();
    _processSelection = false;
    if (event->button() == Qt::RightButton && !_fittedSelection)
        selectFit();
}

void RegionSelect::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    Q_EMIT processDone(true);
}

void RegionSelect::mouseMoveEvent(QMouseEvent *event)
{
    if (_processSelection)
    {
        _selEndPoint = event->pos();
        if ((_selEndPoint - _selStartPoint).manhattanLength() > 10) // ignore mouse trembles
        {
            _selectRect = QRectF(_selStartPoint, _selEndPoint).normalized(); // relative to the widget
	        _fittedSelection = false;
            update();
        }
    }
}

void RegionSelect::keyPressEvent(QKeyEvent* event)
{
    // canceled select screen area
    if (event->key() == Qt::Key_Escape)
        Q_EMIT processDone(false);
    // canceled select screen area
    else if (event->key() == Qt::Key_Space)
        selectFit();
    else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        Q_EMIT processDone(true);
    else
        event->ignore();
}

void RegionSelect::drawBackGround()
{
    // create painter on  pixelmap of desktop
    QPainter painter(&_desktopPixmapBkg);

    // set painter brush on 85% transparency
    painter.setBrush(QBrush(QColor(0, 0, 0, 85), Qt::SolidPattern));

    auto screens = QGuiApplication::screens();
    const QRect geometry = screens.isEmpty() ? QRect() : screens.at(0)->virtualGeometry();

    // draw rect of desktop size in poainter
    painter.drawRect(QRect(QPoint(0, 0), geometry.size()));

    QRect txtRect = geometry;
    QString txtTip = QApplication::tr("Click and drag to draw a rectangle,\nright click to fit to edges,\ndouble click or press Enter\nto take a screenshot.");

    txtRect.setHeight(qRound((float) (txtRect.height() / 10))); // rounded val of text rect height

    painter.setPen(QPen(Qt::red)); // ste message rect border color
    painter.setBrush(QBrush(QColor(255, 255, 255, 180), Qt::SolidPattern));
    QRect txtBgRect = painter.boundingRect(txtRect, Qt::AlignCenter, txtTip);

    // set height & width of bkg rect
    txtBgRect.setX(txtBgRect.x() - 6);
    txtBgRect.setY(txtBgRect.y() - 4);
    txtBgRect.setWidth(txtBgRect.width() + 12);
    txtBgRect.setHeight(txtBgRect.height() + 8);

    painter.drawRect(txtBgRect);

    // Draw the text
    painter.setPen(QPen(Qt::black)); // black color pen
    painter.drawText(txtBgRect, Qt::AlignCenter, txtTip);

    // set bkg to pallette widget
    QPalette newPalette = palette();
    newPalette.setBrush(QPalette::Window, QBrush(_desktopPixmapBkg));
    setPalette(newPalette);
}

void RegionSelect::drawRectSelection(QPainter &painter)
{
    painter.drawPixmap(_selectRect, _desktopPixmapClr, pixmapRect(_selectRect));
    painter.setPen(QPen(QBrush(QColor(0, 0, 0, 255)), 2));
    painter.drawRect(_selectRect);

    QString txtSize = QApplication::tr("%1 x %2 pixels ").arg(_selectRect.width()).arg(_selectRect.height());
    painter.drawText(_selectRect, Qt::AlignBottom | Qt::AlignRight, txtSize);

    if (!_selEndPoint.isNull() && _conf->getZoomAroundMouse())
    {
        const int zoomSide = 200;

        // create magnifier widget coordinates
        QPoint zoomStart = _selEndPoint;
        zoomStart -= QPoint(zoomSide/5, zoomSide/5); // 40, 40

        QPoint zoomEnd = _selEndPoint;
        zoomEnd += QPoint(zoomSide/5, zoomSide/5);

        // creating the widget rect area for magnifier
        QRect zoomRect = QRect(zoomStart, zoomEnd);

        qreal pixelRatio = _desktopPixmapClr.devicePixelRatio();
        int scaledSize = qRound(zoomSide * pixelRatio);
        QPixmap zoomPixmap = _desktopPixmapClr.copy(pixmapRect(zoomRect).toRect()).scaled(QSize(scaledSize, scaledSize), Qt::KeepAspectRatio);

        QPainter zoomPainer(&zoomPixmap); // create painter from the scaled pixmap
        zoomPainer.setPen(QPen(QBrush(QColor(255, 0, 0, 180)), 2));
        QRectF r = widgetRect(zoomPixmap.rect());
        zoomPainer.drawRect(r); // draw
        zoomPainer.drawText(r.center() - QPointF(4, -4), QStringLiteral("+"));

        // position for drawing preview
        QPoint zoomCenter = _selectRect.bottomRight().toPoint();
        if (qRound(zoomCenter.x() * pixelRatio) + zoomPixmap.width() > _desktopPixmapClr.rect().width())
            zoomCenter -= QPoint(qRound(zoomPixmap.width() / pixelRatio), 0);
        if (qRound(zoomCenter.y() * pixelRatio) + zoomPixmap.height() > _desktopPixmapClr.rect().height())
            zoomCenter -= QPoint(0, qRound(zoomPixmap.height() / pixelRatio));
        painter.drawPixmap(zoomCenter, zoomPixmap);
    }
}

void RegionSelect::selectFit()
{
    if (_fittedSelection)
    {
        if (_fitRectangles.isEmpty())
            return;
        _currentFit = (_currentFit + 1) % _fitRectangles.size();
    }
    else
    {
        findFit();
        _currentFit = 1;
        _fittedSelection = true;
    }

    // fitting is done in terms of the pixmap coordinates but we want the widget rectangle
    _selectRect = widgetRect(_fitRectangles[_currentFit]);

    update();
}

void RegionSelect::findFit()
{
    // fitting should be done in terms of the pixmap coordinates
    QRect r = pixmapRect(_selectRect).toRect();

    QRect boundingRect;
    int left = r.left();
    int top = r.top();
    int right = r.right();
    int bottom = r.bottom();

    _fitRectangles.clear();
    _fitRectangles.push_back(r);

    // Set the rectangle in which to search for borders
    if (_conf->getFitInside())
        boundingRect = r;
    else
    {
        boundingRect.setLeft(qMax(left - fitRectExpand, 0));
        boundingRect.setTop(qMax(top - fitRectExpand, 0));
        boundingRect.setRight(qMin(right + fitRectExpand, _sizeDesktop.width() - 1));
        boundingRect.setBottom(qMin(bottom + fitRectExpand, _sizeDesktop.height() - 1));
    }

    // Find borders inside boundingRect
    fitBorder(boundingRect, LEFT, left);
    fitBorder(boundingRect, TOP, top);
    fitBorder(boundingRect, RIGHT, right);
    fitBorder(boundingRect, BOTTOM, bottom);

    const QRect fitRectangle = QRect(QPoint(left, top), QPoint(right, bottom));
    _fitRectangles.push_back(fitRectangle);
}

void RegionSelect::fitBorder(const QRect &boundRect, enum Side side, int &border)
{
    if (!boundRect.isValid())
        return;
    const QImage boundImage = _desktopPixmapClr.copy(boundRect).toImage();

    // Set the relative coordinates of a box vertex and a vector along the box side
    QPoint startPoint;
    QPoint directionVector;
    switch(side){
    case TOP:
        startPoint = QPoint(0,0);
        directionVector = QPoint(1,0);
        break;
    case RIGHT:
        startPoint = boundRect.topRight() - boundRect.topLeft();
        directionVector = QPoint(0,1);
        break;
    case BOTTOM:
        startPoint = boundRect.bottomRight() - boundRect.topLeft();
        directionVector = QPoint(-1,0);
        break;
    case LEFT:
        startPoint = boundRect.bottomLeft() - boundRect.topLeft();
        directionVector = QPoint(0,-1);
        break;
    default:
        return;
    }

    // Set vector normal to the box side
    QPoint normalVector = QPoint(-directionVector.y(), directionVector.x());

    // Setbox dimensions relative to the box side
    int directionLength;
    int normalDepth;
    if (directionVector.x() == 0)
    {
        directionLength = boundRect.height() - 1;
        normalDepth = boundRect.width() - 1;
    }
    else
    {
        directionLength = boundRect.width() - 1;
        normalDepth = boundRect.height() - 1;
    }

    // Set how deep in the boundingRect to search for the border
    if (_conf->getFitInside())
        normalDepth = qMin(normalDepth/2 - 1, fitRectDepth);
    else
        normalDepth = qMin(normalDepth/2 - 1, fitRectDepth + fitRectExpand);

    QVector<int> gradient = QVector<int>(normalDepth, 0);
    QVector<int> preR = QVector<int>(normalDepth + 1, 0);
    QVector<int> preG = QVector<int>(normalDepth + 1, 0);
    QVector<int> preB = QVector<int>(normalDepth + 1, 0);

    // Compute pixel Sobel normal gradients and add their absolute values parallel to the box side
    for (int i = 1; i < directionLength; i++)
    {
        for (int j = 0; j <= normalDepth; j++)
        {
            QPoint point = startPoint + i*directionVector + j*normalVector;
            QRgb pixelL = boundImage.pixel(point - directionVector);
            QRgb pixelC = boundImage.pixel(point);
            QRgb pixelR = boundImage.pixel(point + directionVector);
            preR[j] = qRed(pixelL) + 2*qRed(pixelC) + qRed(pixelR);
            preG[j] = qGreen(pixelL) + 2*qGreen(pixelC) + qGreen(pixelR);
            preB[j] = qBlue(pixelL) + 2*qBlue(pixelC) + qBlue(pixelR);
        }
        for (int j = 1; j < normalDepth; j++)
            gradient[j] += qAbs(preR[j-1] - preR[j+1]) + qAbs(preG[j-1] - preG[j+1]) + qAbs(preB[j-1] - preB[j+1]);
    }

    int maxGradient = 0;
    int positionMax = 0;
    for (int j = 1; j < normalDepth; j++)
    {
        // Scale pixel normal gradients and break if drop detected
        gradient[j] = 1 + gradient[j]/(60*directionLength);
        if (_conf->getFitInside() && gradient[j] <= maxGradient/2)
            break;

        // Keep searching for the maximum normal gradient
        if (gradient[j] > maxGradient)
        {
            maxGradient = gradient[j];
            positionMax = j;
        }
    }
    // If all normal gradients small, keep the original user selected border
    if (maxGradient <= 1)
        return;

    // Transform computed border back to original coordinates
    startPoint = boundRect.topLeft() + startPoint + (positionMax + 1)*normalVector;
    if (normalVector.x() == 0)
        border = startPoint.y();
    else
        border = startPoint.x();
}

QPixmap RegionSelect::getSelection()
{
    QPixmap sel = _desktopPixmapClr.copy(pixmapRect(_selectRect).toRect());
    return sel;
}

QPoint RegionSelect::getSelectionStartPos()
{
    return _selectRect.topLeft().toPoint();
}

// Transform a widget rectangle (calculated in terms of the widget coordinates)
// to a pixmap rectangle (calculated in terms of the pixmap coordinates):
QRectF RegionSelect::pixmapRect(const QRectF &widgetRect) const
{
    qreal pixelRatio = _desktopPixmapClr.devicePixelRatio();
    QRectF r;
    r.setTopLeft(widgetRect.topLeft() * pixelRatio);
    r.setBottomRight(widgetRect.bottomRight() * pixelRatio);
    return r;
}

// Transform a pixmap rectangle to a widget rectangle:
QRectF RegionSelect::widgetRect(const QRectF &pixmapRect) const
{
    qreal pixelRatio = _desktopPixmapClr.devicePixelRatio();
    QRectF r;
    r.setTopLeft(pixmapRect.topLeft() / pixelRatio);
    r.setBottomRight(pixmapRect.bottomRight() / pixelRatio);
    return r;
}
