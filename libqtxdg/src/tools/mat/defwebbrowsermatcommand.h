#ifndef DEFWEBBROWSERMATCOMMAND_H
#define DEFWEBBROWSERMATCOMMAND_H

#include "matcommandinterface.h"

class DefWebBrowserMatCommand : public MatCommandInterface {
public:
    explicit DefWebBrowserMatCommand(QCommandLineParser *parser);
    ~DefWebBrowserMatCommand() override;

    int run(const QStringList &arguments) override;
};

#endif // DEFWEBBROWSERMATCOMMAND_H
