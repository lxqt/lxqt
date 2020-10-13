/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2012  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef LXQTCONFIGDIALOG_H
#define LXQTCONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QScopedPointer>
#include "lxqtglobals.h"

namespace Ui
{
   class ConfigDialog;
}

namespace LXQt
{
   class Settings;
   class ConfigDialogPrivate;

   class LXQT_API ConfigDialog : public QDialog
   {
      Q_OBJECT
      Q_DECLARE_PRIVATE(ConfigDialog)

   public:
      explicit ConfigDialog(const QString &title, Settings *settings, QWidget *parent = nullptr);

      ~ConfigDialog() override;

      /*!
     * Sets buttons in button bar
     */
      void setButtons(QDialogButtonBox::StandardButtons buttons);

      /*!
     * Enable/disable a standard button if it exists
     */
      void enableButton(QDialogButtonBox::StandardButton which, bool enable);

      /*!
     * Add a page to the configure dialog
     */
      void addPage(QWidget *page, const QString &name, const QString &iconName = QLatin1String("application-x-executable"));

      /*!
     * Add a page to the configure dialog, attempting several alternative icons to find one in the theme
     */
      void addPage(QWidget *page, const QString &name, const QStringList &iconNames);

      /*!
     * Show page containing the widget in parameter
     */
      void showPage(QWidget *page);

      /*!
     * \brief Shows the given page
     * \param name The page to be shown.
     */
      void showPage(const QString &name);

   Q_SIGNALS:
      /*!
     * This signal is emitted when the user pressed the "Reset" button.
     * Settings should be re-read and the widgets should be set accordingly.
     */
      void reset();

      /*!
     * This is emitted whenever the window is closed and settings need to be saved.
     * It is only necessary if additional actions need to be performed - Settings are handled automatically.
     */
      void save();

      /*!
     * This is emitted when some button in the buttonbar is clicked.
     */
      void clicked(QDialogButtonBox::StandardButton);

   protected:
      Settings *mSettings;
      bool event(QEvent *event) override;
      void closeEvent(QCloseEvent *event) override;

   private:
      Q_DISABLE_COPY(ConfigDialog)
      QScopedPointer<ConfigDialogPrivate> const d_ptr;
   };

} // namespace LXQt
#endif // LXQTCONFIGDIALOG_H
