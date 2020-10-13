#ifndef JOB_P_H
#define JOB_P_H

#include <QThread>
#include "job.h"

namespace Fm {

class JobThread: public QThread {
    Q_OBJECT
public:
    JobThread(Job* job): job_{job} {
    }

protected:

    void run() override {
        job_->run();
    }

    Job* job_;
};

} // namespace Fm

#endif // JOB_P_H
