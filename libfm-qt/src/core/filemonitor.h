#ifndef FM2_FILEMONITOR_H
#define FM2_FILEMONITOR_H

#include "../libfmqtglobals.h"
#include <QObject>
#include "gioptrs.h"
#include "filepath.h"

namespace Fm {

class LIBFM_QT_API FileMonitor: public QObject {
    Q_OBJECT
public:

    explicit FileMonitor();

Q_SIGNALS:


private:
    GFileMonitorPtr monitor_;
};

} // namespace Fm

#endif // FM2_FILEMONITOR_H
