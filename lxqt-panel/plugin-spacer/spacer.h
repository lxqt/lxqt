/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
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


#ifndef SPACER_H
#define SPACER_H

#include "../panel/ilxqtpanelplugin.h"
#include <QFrame>


class SpacerWidget : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QString type READ getType)
    Q_PROPERTY(QString orientation READ getOrientation)

public:
    const QString& getType() const throw () { return mType; }
    void setType(QString const & type);
    const QString& getOrientation() const throw () { return mOrientation; }
    void setOrientation(QString const & orientation);

private:
    QString mType;
    QString mOrientation;
};

class Spacer :  public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT

public:
    Spacer(const ILXQtPanelPluginStartupInfo &startupInfo);

    virtual QWidget *widget() override { return &mSpacer; }
    virtual QString themeId() const override { return QStringLiteral("Spacer"); }

    bool isSeparate() const override { return true; }
    bool isExpandable() const override { return mExpandable; }

    virtual ILXQtPanelPlugin::Flags flags() const override { return HaveConfigDialog; }
    QDialog *configureDialog() override;

    virtual void realign() override;

private slots:
    virtual void settingsChanged() override;

private:
    void setSizes();

private:
    SpacerWidget mSpacer;
    int mSize;
    bool mExpandable;
};

class SpacerPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    // Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const { return new Spacer(startupInfo);}
};

#endif

