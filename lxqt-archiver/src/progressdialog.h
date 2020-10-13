#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <memory>

#include "archiver.h"

namespace Ui {
class ProgressDialog;
}


class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget* parent = 0);

    ~ProgressDialog();

    void setArchiver(Archiver* archiver);

    FrAction finishAction() const;

    void setOperation(const QString& operation);

    void setMessage(const QString& msg);

    void reject() override;

private Q_SLOTS:
    void onProgress(double fraction);

    void onFinished(FrAction action, ArchiverError error);

    void onMessage(QString msg);

    void onStoppableChanged(bool value);

    void onWorkingArchive(QString filename);

private:
    std::unique_ptr<Ui::ProgressDialog> ui_;
    Archiver* archiver_;
};

#endif // PROGRESSDIALOG_H
