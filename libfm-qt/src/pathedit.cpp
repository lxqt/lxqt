/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "pathedit.h"
#include "pathedit_p.h"
#include <QCompleter>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QStringBuilder>
#include <QThread>
#include <QDebug>
#include <QKeyEvent>
#include <QDir>
#include <QTimer>

namespace Fm {

void PathEditJob::runJob() {
    GError* err = nullptr;
    GFileEnumerator* enu = g_file_enumerate_children(dirName,
                           // G_FILE_ATTRIBUTE_STANDARD_NAME","
                           G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME","
                           G_FILE_ATTRIBUTE_STANDARD_TYPE,
                           G_FILE_QUERY_INFO_NONE, cancellable,
                           &err);
    if(enu) {
        while(!g_cancellable_is_cancelled(cancellable)) {
            GFileInfo* inf = g_file_enumerator_next_file(enu, cancellable, &err);
            if(inf) {
                GFileType type = g_file_info_get_file_type(inf);
                if(type == G_FILE_TYPE_DIRECTORY) {
                    const char* name = g_file_info_get_display_name(inf);
                    // FIXME: encoding conversion here?
                    subDirs.append(QString::fromUtf8(name) + QLatin1String("/"));
                }
                g_object_unref(inf);
            }
            else {
                if(err) {
                    g_error_free(err);
                    err = nullptr;
                }
                else { /* EOF */
                    break;
                }
            }
        }
        g_file_enumerator_close(enu, cancellable, nullptr);
        g_object_unref(enu);
    }
    subDirs.sort(Qt::CaseInsensitive);
    // finished! let's update the UI in the main thread
    Q_EMIT finished();
    QThread::currentThread()->quit();
}


PathEdit::PathEdit(QWidget* parent):
    QLineEdit(parent),
    completer_(new QCompleter()),
    model_(new QStringListModel()),
    cancellable_(nullptr) {
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    // we sorted the subdir list case-insensitively, so we do the same thing
    // here for performance improvements (see Qt doc)
    completer_->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    setCompleter(completer_);
    completer_->setModel(model_);
    connect(this, &PathEdit::textChanged, this, &PathEdit::onTextChanged);
    connect(this, &PathEdit::textEdited, this, &PathEdit::onTextEdited);
}

PathEdit::~PathEdit() {
    delete completer_;
    if(model_) {
        delete model_;
    }
    if(cancellable_) {
        g_cancellable_cancel(cancellable_);
        g_object_unref(cancellable_);
    }
}

void PathEdit::focusInEvent(QFocusEvent* e) {
    QLineEdit::focusInEvent(e);
    // build the completion list only when we have the keyboard focus
    reloadCompleter(true);
}

void PathEdit::focusOutEvent(QFocusEvent* e) {
    QLineEdit::focusOutEvent(e);
    // free the completion list since we don't need it anymore
    freeCompleter();
}

bool PathEdit::event(QEvent* e) {
    if(e->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        int key = keyEvent->key();
        // Stop Qt from moving the keyboard focus to the next widget when "Tab" is pressed.
        // Instead, we need to do auto-completion in this case.
        if((key == Qt::Key_Tab && keyEvent->modifiers() == Qt::NoModifier)
           || key == Qt::Key_Backtab) {
            e->accept();
            // pressing of "Tab" should be filtered out first; hence the single-shot timer
            QTimer::singleShot(0, completer_, [this, key] {
                if(!completer_->popup()->isVisible()) {
                    // NOTE: To open the popup, we should insert the current text because the text
                    // may not be typed by the user but set by selecting a row from a previous popup,
                    // in which case, QCompleter::complete() would show only the previous popup.

                    lastTypedText_ = text(); // as if text is typed; matches will be updated
                    selectAll();
                    setModified(false); // remove the undo history
                    insert(lastTypedText_);
                }
                else {
                    selectNextCompletionRow(key != Qt::Key_Backtab);
                }
            });
            return true;
        }
        // When the completer popup is visible and "Escape" is pressed, restore the last typed text
        // in addition to closing the popup
        if(key == Qt::Key_Escape && completer_->popup()->isVisible() && lastTypedText_!= text()) {
            e->accept();
            QTimer::singleShot(0, completer_, [this] {
                completer_->popup()->hide();
                setText(lastTypedText_);
            });
            return true;
        }
    }
    return QLineEdit::event(e);
}

void PathEdit::selectNextCompletionRow(bool downward) {
    int rows = completer_->completionCount(); // it is fast because of sorting
    auto popup = completer_->popup();
    if(!popup->selectionModel()->hasSelection()) {
        if(completer_->setCurrentRow(downward ? 0 : rows - 1)) {
            popup->setCurrentIndex(completer_->currentIndex());
            // If there is no ambiguity, insert the text to trigger the next popup.
            // TODO: Just close the popup if users prefer that instead.
            if(rows == 1) {
                lastTypedText_ = text();
                selectAll();
                setModified(false);
                insert(lastTypedText_);
            }
        }
    }
    else {
        int selectedRow = popup->selectionModel()->selectedRows().at(0).row();
        completer_->setCurrentRow(selectedRow); // it can be selected by user
        if(downward) {
            if(!completer_->setCurrentRow(completer_->currentRow() + 1)) {
                completer_->setCurrentRow(0);
            }
        }
        else if(!completer_->setCurrentRow(completer_->currentRow() - 1)) {
            completer_->setCurrentRow(rows - 1);
        }
        popup->setCurrentIndex(completer_->currentIndex());
    }
}

void PathEdit::onTextEdited(const QString& text) {
    // just replace start tilde with home path if text is changed by user
    if(text == QLatin1String("~") || text.startsWith(QLatin1String("~/"))) {
        QString txt(text);
        txt.replace(0, 1, QDir::homePath());
        lastTypedText_ = txt;
        setText(txt); // emits textChanged()
        return;
    }
    lastTypedText_ = text;
}

void PathEdit::onTextChanged(const QString& text) {
    if(text == QLatin1String("~") || text.startsWith(QLatin1String("~/"))) {
        // do nothing with a start tilde because neither Fm::FilePath nor autocompletion
        // understands it; instead, wait until textChanged() is emitted again without it
        // WARNING: replacing tilde may not be safe here
        return;
    }
    // If the text is typed, the last slash means searching child directories.
    // But, since a slash is appended to the name of each directory match, if the text is
    // changed by auto-completion, we should ignore the last slash to remain in the parent directory.
    int pos = text.lastIndexOf(QLatin1Char('/'), lastTypedText_ == text ? -1 : -2);
    if(pos >= 0) {
        ++pos;
    }
    else {
        pos = text.length();
    }
    QString newPrefix = text.left(pos);
    if(currentPrefix_ != newPrefix) {
        currentPrefix_ = newPrefix;
        // only build the completion list if we have the keyboard focus
        // if we don't have the focus now, then we'll rebuild the completion list
        // when focusInEvent happens. this avoid unnecessary dir loading.
        if(hasFocus()) {
            reloadCompleter(false);
        }
    }
}

void PathEdit::reloadCompleter(bool triggeredByFocusInEvent) {
    // parent dir has been changed, reload dir list
    // if(currentPrefix_[0] == "~") { // special case for home dir
    // cancel running dir-listing jobs, if there's any
    if(cancellable_) {
        g_cancellable_cancel(cancellable_);
        g_object_unref(cancellable_);
    }

    // create a new job to do dir listing
    PathEditJob* job = new PathEditJob();
    job->edit = this;
    job->triggeredByFocusInEvent = triggeredByFocusInEvent;
    // need to use fm_file_new_for_commandline_arg() rather than g_file_new_for_commandline_arg().
    // otherwise, our own vfs, such as menu://, won't be loaded.
    job->dirName = g_file_new_for_commandline_arg(currentPrefix_.toLocal8Bit().constData());
    // qDebug("load: %s", g_file_get_uri(data->dirName));
    cancellable_ = g_cancellable_new();
    job->cancellable = (GCancellable*)g_object_ref(cancellable_);

    // launch a new worker thread to handle the job
    QThread* thread = new QThread();
    job->moveToThread(thread);
    connect(job, &PathEditJob::finished, this, &PathEdit::onJobFinished, Qt::BlockingQueuedConnection);
    // connect(job, &PathEditJob::finished, thread, &QThread::quit);
    connect(thread, &QThread::started, job, &PathEditJob::runJob);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, job, &QObject::deleteLater);
    thread->start(QThread::LowPriority);
}

void PathEdit::freeCompleter() {
    if(cancellable_) {
        g_cancellable_cancel(cancellable_);
        g_object_unref(cancellable_);
        cancellable_ = nullptr;
    }
    model_->setStringList(QStringList());
}

// This slot is called from main thread so it's safe to access the GUI
void PathEdit::onJobFinished() {
    PathEditJob* data = static_cast<PathEditJob*>(sender());
    if(!g_cancellable_is_cancelled(data->cancellable)) {
        // update the completer only if the job is not cancelled
        QStringList::iterator it;
        for(it = data->subDirs.begin(); it != data->subDirs.end(); ++it) {
            // qDebug("%s", it->toUtf8().constData());
            *it = (currentPrefix_ % *it);
        }
        model_->setStringList(data->subDirs);
        // trigger completion manually
        if(hasFocus() && !data->triggeredByFocusInEvent) {
            completer_->complete();
        }
    }
    else {
        model_->setStringList(QStringList());
    }
    if(cancellable_) {
        g_object_unref(cancellable_);
        cancellable_ = nullptr;
    }
}

} // namespace Fm
