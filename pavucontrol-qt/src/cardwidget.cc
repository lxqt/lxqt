/***
  This file is part of pavucontrol.

  Copyright 2006-2008 Lennart Poettering
  Copyright 2009 Colin Guthrie

  pavucontrol is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  pavucontrol is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with pavucontrol. If not, see <https://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cardwidget.h"

/*** CardWidget ***/
CardWidget::CardWidget(QWidget* parent) :
    QWidget(parent) {
    setupUi(this);
    connect(profileList, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CardWidget::onProfileChange);
    connect(profileCB, &QAbstractButton::toggled, this, &CardWidget::onProfileCheck);
}


void CardWidget::prepareMenu() {
    int idx = 0;
    const bool off = activeProfile == noInOutProfile;

    profileList->clear();
    /* Fill the ComboBox */
    for (const auto & profile : profiles) {
        QByteArray name = profile.first;
        // skip the "off" profile
        if (name == noInOutProfile)
            continue;
        QString desc = QString::fromUtf8(profile.second);
        profileList->addItem(desc, name);
        if (profile.first == activeProfile
                || (off && profile.first == lastActiveProfile)
                )
        {
            profileList->setCurrentIndex(idx);
            lastActiveProfile = profile.first;
        }
        ++idx;
    }

    profileCB->setChecked(!off);
}

void CardWidget::changeProfile(const QByteArray & name)
{
    pa_operation* o;

    if (!(o = pa_context_set_card_profile_by_index(get_context(), index, name.constData(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_card_profile_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void CardWidget::onProfileChange(int active) {
    if (updating)
        return;

    if (active != -1)
        changeProfile(profileList->itemData(active).toByteArray());
}

void CardWidget::onProfileCheck(bool on)
{
    if (updating)
        return;

    if (on)
        onProfileChange(profileList->currentIndex());
    else
        changeProfile(noInOutProfile);

}
