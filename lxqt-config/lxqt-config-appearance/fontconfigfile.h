/*
 * Copyright (C) 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * LXQt project: https://lxqt.org/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef FONTCONFIGFILE_H
#define FONTCONFIGFILE_H

#include <QString>
#include <QByteArray>
#include <QObject>

class QTimer;

class FontConfigFile: public QObject
{
    Q_OBJECT
public:
    explicit FontConfigFile(QObject* parent = 0);
    virtual ~FontConfigFile();

    bool antialias() const {
        return mAntialias;
    }
    void setAntialias(bool value);

    bool hinting() const {
        return mHinting;
    }
    void setHinting(bool value);

    QByteArray subpixel() const {
        return mSubpixel;
    }
    void setSubpixel(QByteArray value);

    QByteArray hintStyle() const {
        return mHintStyle;
    }
    void setHintStyle(QByteArray value);

    int dpi() const {
        return mDpi;
    }
    void setDpi(int value);

    bool autohint() const {
        return mAutohint;
    }
    void setAutohint(bool value);


private Q_SLOTS:
    void save();

private:
    void load();
    void queueSave();

private:
    bool mAntialias;
    bool mHinting;
    QByteArray mSubpixel;
    QByteArray mHintStyle;
    int mDpi;
    bool mAutohint;
    QString mDirPath;
    QString mFilePath;
    QTimer* mSaveTimer;
};

#endif // FONTCONFIGFILE_H
