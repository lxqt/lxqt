#ifndef CONNECTSERVERDIALOG_H
#define CONNECTSERVERDIALOG_H

#include <QDialog>
#include <QList>
#include "ui_connect.h"

namespace PCManFM {

class ConnectServerDialog : public QDialog {
  Q_OBJECT

public:
  ConnectServerDialog(QWidget* parent=nullptr);
  virtual ~ConnectServerDialog();

  QString uriText();

private:
  struct ServerType {
    QString name;
    const char* scheme;
    int defaultPort;
    bool canAnonymous;
  };

private Q_SLOTS:
  void onCurrentIndexChanged(int index);
  void checkInput();

private:
  Ui::ConnectServerDialog ui;
  QList<ServerType> serverTypes;
};

} // namespace PCManFM

#endif // CONNECTSERVERDIALOG_H
