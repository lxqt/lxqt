/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - The Lightweight Desktop Environment
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "lxqtaboutdialog.h"
#include "ui_lxqtaboutdialog.h"
#include "lxqtaboutdialog_p.h"
#include "technicalinfo.h"
#include "translatorsinfo/translatorsinfo.h"
#include <QPushButton>
#include <QDebug>
#include <QDate>
#include <QClipboard>

AboutDialogPrivate::AboutDialogPrivate()
{
    setupUi(this);
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
    QString css=QStringLiteral("<style TYPE='text/css'> "
                               "body { font-family: sans-serif;} "
                               ".name { font-size: 16pt; } "
                               "a { white-space: nowrap ;} "
                               "h2 { font-size: 10pt;} "
                               "li { line-height: 120%;} "
                               ".techInfoKey { white-space: nowrap ; margin: 0 20px 0 16px; } "
                               "</style>")
            ;

    iconLabel->setFixedSize(48, 48);
    iconLabel->setScaledContents(true);
    iconLabel->setPixmap(QPixmap(QStringLiteral(LXQT_SHARE_DIR) + QStringLiteral("/graphics/lxqt_logo.png")));

    nameLabel->setText(css + titleText());

    aboutBrowser->setHtml(css + aboutText());
    aboutBrowser->viewport()->setAutoFillBackground(false);

    autorsBrowser->setHtml(css + authorsText());
    autorsBrowser->viewport()->setAutoFillBackground(false);

    thanksBrowser->setHtml(css + thanksText());
    thanksBrowser->viewport()->setAutoFillBackground(false);

    translationsBrowser->setHtml(css + translationsText());
    translationsBrowser->viewport()->setAutoFillBackground(false);

    TechnicalInfo info;
    techBrowser->setHtml(info.html());
    techBrowser->viewport()->setAutoFillBackground(false);

    connect(techCopyToClipboardButton, SIGNAL(clicked()), this, SLOT(copyToCliboardTechInfo()));
    this->setAttribute(Qt::WA_DeleteOnClose);
    show();

}

QString AboutDialogPrivate::titleText() const
{
    return QStringLiteral("<div class=name>%1</div><div class=ver>%2</div>").arg(QStringLiteral("LXQt"),
                                                                                 tr("Version: %1").arg(QStringLiteral(LXQT_VERSION)));

}

QString AboutDialogPrivate::aboutText() const
{
    return QStringLiteral(
                "<p>%1</p>"
                "<p>%2</p>"
                "<p>%3</p>"
                "<p>%4</p>"
                "<p>%5</p>")
            .arg(
                tr("Advanced, easy-to-use, and fast desktop environment based on Qt technologies.",
                   "About dialog, 'About' tab text"),
                tr("LXQt would not have been possible without the <a %1>Razor-qt</a> project and its many contributors.",
                   "About dialog, 'About' tab text").arg(QStringLiteral("href=\"https://blog.lxde.org/2014/11/21/in-memory-of-razor-qt/\"")),
                tr("Copyright: © %1-%2 %3", "About dialog, 'About' tab text")
                .arg(QStringLiteral("2010"), QDate::currentDate().toString(QStringLiteral("yyyy")), QStringLiteral("LXQt team")),
                tr("Homepage: %1", "About dialog, 'About' tab text")
                .arg(QStringLiteral("<a href=\"https://lxqt.github.io\">https://lxqt.github.io</a>")),
                tr("License: %1", "About dialog, 'About' tab text")
                .arg(QStringLiteral("<a href=\"https://www.gnu.org/licenses/lgpl-2.1.html\">GNU Lesser General Public License version 2.1 or later</a>"
                                    " and partly under the "
                                    "<a href=\"https://www.gnu.org/licenses/gpl-2.0.html\">GNU General Public License version 2</a>"))
                );
}

QString AboutDialogPrivate::authorsText() const
{
    return QStringLiteral("<p>%1</p><p>%2</p>").arg(
                tr("LXQt is developed by the <a %1>LXQt Team and contributors</a>.", "About dialog, 'Authors' tab text")
                .arg(QStringLiteral(" href=\"https://github.com/lxqt/lxqt\"")),
                tr("If you are interested in working with our development team, <a %1>join us</a>.", "About dialog, 'Authors' tab text")
                .arg(QStringLiteral("href=\"https://lxqt.github.io\""))
                );
}


QString AboutDialogPrivate::thanksText() const
{
    return QStringLiteral(
                "%1"
                "<ul>"
                "<li>Alexey Nosov (for the A-MeGo theme)</li>"
                "<li>Alexander Zakher (the Razor-qt name)</li>"
                "<li>Andy Fitzsimon (logo/icon)</li>"
                "<li>Eugene Pivnev (QtDesktop)</li>"
                "<li>Manuel Meier (for ideas)</li>"
                "<li>KDE &lt;<a href=\"https://kde.org/\">https://kde.org/</a>&gt;</li>"
                ).arg(tr("Special thanks to:", "About dialog, 'Thanks' tab text"));
}

QString AboutDialogPrivate::translationsText() const
{
    TranslatorsInfo translatorsInfo;
    return QStringLiteral("%1<p><ul>%2</ul>").arg(
                tr("LXQt is translated into many languages thanks to the work of the translation teams all over the world.", "About dialog, 'Translations' tab text"),
                translatorsInfo.asHtml()
                );
}

AboutDialog::AboutDialog()
{
    d_ptr = new AboutDialogPrivate();
}

void AboutDialogPrivate::copyToCliboardTechInfo()
{
    TechnicalInfo info;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(info.text());
}
