#include "connectserverdialog.h"
#include <QMessageBox>
#include <QPushButton>

namespace PCManFM {

ConnectServerDialog::ConnectServerDialog(QWidget *parent): QDialog(parent) {
  serverTypes = QList<ServerType>{
    {tr("SSH"), "sftp", 22, false},
    {tr("FTP"), "ftp", 21, true},
    {tr("WebDav"), "dav", 80, true},
    {tr("Secure WebDav"), "davs", 443, false},
    {tr("HTTP"), "http", 80, true},
    {tr("HTTPS"), "https", 443, true},
  };

  ui.setupUi(this);
  ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  connect(ui.serverType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &ConnectServerDialog::onCurrentIndexChanged);
  connect(ui.host, &QLineEdit::textChanged, this, &ConnectServerDialog::checkInput);
  connect(ui.userName, &QLineEdit::textChanged, this, &ConnectServerDialog::checkInput);
  for(const auto& serverType : const_cast<const QList<ServerType>&>(serverTypes)) {
    ui.serverType->addItem(serverType.name);
  }

  ui.serverType->setCurrentIndex(0);
  onCurrentIndexChanged(0);
}

ConnectServerDialog::~ConnectServerDialog() {
}


QString ConnectServerDialog::uriText() {
  QString uri;
  int serverTypeIdx = ui.serverType->currentIndex();
  const auto& serverType = serverTypes[serverTypeIdx];

  // make an URI from the data
  uri = QString::fromLatin1(serverType.scheme);
  uri += QStringLiteral("://");
  if(ui.loginAsUser->isChecked()) {
    uri += ui.userName->text();
    uri += QLatin1Char('@');
  }

  uri += ui.host->text();
  int port = ui.port->value();
  if(port != serverType.defaultPort) {
    uri += QLatin1Char(':');
    uri += QString::number(port);
  }

  QString path = ui.path->text();
  if(path.isEmpty() || path[0] != QLatin1Char('/')) {
    uri += QLatin1Char('/');
  }
  uri += path;
  return uri;
}

void ConnectServerDialog::onCurrentIndexChanged(int /*index*/) {
  int serverTypeIdx = ui.serverType->currentIndex();
  const auto& serverType = serverTypes[serverTypeIdx];
  ui.port->setValue(serverType.defaultPort);
  ui.ananymousLogin->setEnabled(serverType.canAnonymous);
  if(serverType.canAnonymous)
    ui.ananymousLogin->setChecked(true);
  else
    ui.loginAsUser->setChecked(true);
  ui.host->setFocus();
  checkInput();
}

void ConnectServerDialog::checkInput() {
  bool valid = true;
  if(ui.host->text().isEmpty()) {
    valid = false;
  }
  else if(ui.loginAsUser->isChecked() && ui.userName->text().isEmpty()) {
    valid = false;
  }
  ui.buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

} // namespace PCManFM
