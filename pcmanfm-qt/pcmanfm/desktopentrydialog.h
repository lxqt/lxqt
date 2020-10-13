#ifndef FM_DESKTOPENTRYDIALOG_H
#define FM_DESKTOPENTRYDIALOG_H

#include <QDialog>
#include "ui_desktopentrydialog.h"

namespace PCManFM {

class LIBFM_QT_API DesktopEntryDialog : public QDialog {
    Q_OBJECT
public:
    explicit DesktopEntryDialog(QWidget *parent = nullptr);

    virtual ~DesktopEntryDialog();

    virtual void accept() override;

Q_SIGNALS:
    void desktopEntryCreated(const QString& name);

private Q_SLOTS:
    void onChangingType(int type);
    void onClickingIconButton();
    void onClickingCommandButton();

private:
    Ui::DesktopEntryDialog ui;

};

} // namespace Fm
#endif // FM_DESKTOPENTRYDIALOG_H
