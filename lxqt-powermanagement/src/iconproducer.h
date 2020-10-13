/*
* Copyright (c) 2014 Christian Surlykke
*
* This file is part of the LXQt project. <https://lxqt.org>
* It is distributed under the LGPL 2.1 or later license.
* Please refer to the LICENSE file for a copy of the license.
*/

#ifndef ICONPRODUCER_H
#define ICONPRODUCER_H
#include <QString>
#include <QMap>
#include <QObject>
#include "../config/powermanagementsettings.h"
#include <Solid/Battery>

class IconProducer : public QObject
{
    Q_OBJECT

public:
    IconProducer(Solid::Battery* battery, QObject *parent = nullptr);
    IconProducer(QObject *parent = nullptr);

    QIcon mIcon;
    QString mIconName;

signals:
    void iconChanged();

public slots:
    void updateChargePercent(int newChargePercent);
    void updateState(int newState);
    void update();
    void themeChanged();

private:

    QIcon &circleIcon();
    QIcon buildCircleIcon(Solid::Battery::ChargeState state, int chargeLevel);


    int mChargePercent;
    Solid::Battery::ChargeState mState;

    PowerManagementSettings mSettings;

    QMap<float, QString> mLevelNameMapCharging;
    QMap<float, QString> mLevelNameMapDischarging;

};

#endif // ICONPRODUCER_H
