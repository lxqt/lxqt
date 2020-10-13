#ifndef ARCHIVERERROR_H
#define ARCHIVERERROR_H

#include <QObject>
#include <QString>
#include "core/fr-archive.h"

class ArchiverError {
public:
    ArchiverError();

    ArchiverError(FrProcError* err);

    bool hasError() const;

    FrProcErrorType type() const;

    int status() const;

    GQuark domain() const;

    int code() const;

    QString message() const;

private:
    FrProcErrorType type_;
    int status_;
    GQuark domain_;
    int code_;
    QString message_;
};


Q_DECLARE_METATYPE(ArchiverError)


#endif // ARCHIVERERROR_H
