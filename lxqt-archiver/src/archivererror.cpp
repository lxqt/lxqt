#include "archivererror.h"
#include <QCoreApplication>

ArchiverError::ArchiverError():
    type_{FR_PROC_ERROR_NONE},
    status_{0},
    domain_{0},
    code_{0} {
}

ArchiverError::ArchiverError(FrProcError* err): ArchiverError{} {
    if(err) {
        type_ = err->type;
        status_ = err->status;
        if(err->gerror) {
            domain_ = err->gerror->domain;
            code_ = err->gerror->code;
            message_ = QString::fromUtf8(err->gerror->message);
        }
        
        if(message_.isEmpty()) {
            switch(type_) {
                case FR_PROC_ERROR_COMMAND_ERROR:
                case FR_PROC_ERROR_SPAWN:
                    message_ = QCoreApplication::translate("ArchiverError", "Failed to execute the command.");
                    break;
                case FR_PROC_ERROR_COMMAND_NOT_FOUND:
                    message_ = QCoreApplication::translate("ArchiverError", "Command is not found.");
                    break;
                case FR_PROC_ERROR_EXITED_ABNORMALLY:
                    message_ = QCoreApplication::translate("ArchiverError", "The command exited abnormally.");
                    break;
                case FR_PROC_ERROR_ASK_PASSWORD:
                    message_ = QCoreApplication::translate("ArchiverError", "Password is required.");
                    break;
                case FR_PROC_ERROR_MISSING_VOLUME:
                    message_ = QCoreApplication::translate("ArchiverError", "Missing volume.");
                    break;
                case FR_PROC_ERROR_BAD_CHARSET:
                    message_ = QCoreApplication::translate("ArchiverError", "Bad charset.");
                    break;
                case FR_PROC_ERROR_UNSUPPORTED_FORMAT:
                    message_ = QCoreApplication::translate("ArchiverError", "Unsupported file format.");
                    break;
                default:
                    message_ = QCoreApplication::translate("ArchiverError", "Unknown errors happened.");
                    break;
            }
        }
    }
}

bool ArchiverError::hasError() const {
    return type_ != FR_PROC_ERROR_NONE;
}

FrProcErrorType ArchiverError::type() const {
    return type_;
}

int ArchiverError::status() const {
    return status_;
}

GQuark ArchiverError::domain() const {
    return domain_;
}

int ArchiverError::code() const {
    return code_;
}

QString ArchiverError::message() const {
    return message_;
}
