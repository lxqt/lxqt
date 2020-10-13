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


#include "sidepane.h"
#include <QEvent>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHeaderView>
#include "placesview.h"
#include "dirtreeview.h"
#include "dirtreemodel.h"
#include "filemenu.h"

namespace Fm {

SidePane::SidePane(QWidget* parent):
    QWidget(parent),
    view_(nullptr),
    combo_(nullptr),
    iconSize_(24, 24),
    mode_(ModeNone),
    showHidden_(false) {

    verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    combo_ = new QComboBox(this);
    combo_->addItem(tr("Lists")); // "Places" is already used in PlacesModel
    combo_->addItem(tr("Directory Tree"));
    connect(combo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SidePane::onComboCurrentIndexChanged);
    verticalLayout->addWidget(combo_);
}

SidePane::~SidePane() {
    // qDebug("delete SidePane");
}

bool SidePane::event(QEvent* event) {
    // when the SidePane's style changes, we should set the text color of
    // PlacesView to its window text color again because the latter may have changed
    if(event->type() == QEvent::StyleChange && mode_ == ModePlaces) {
        if(PlacesView* placesView = static_cast<PlacesView*>(view_)) {
            QPalette p = placesView->palette();
            p.setColor(QPalette::Text, p.color(QPalette::WindowText));
            placesView->setPalette(p);
        }
    }
    return QWidget::event(event);
}

void SidePane::onComboCurrentIndexChanged(int current) {
    if(current != mode_) {
        setMode(Mode(current));
    }
}

void SidePane::setIconSize(QSize size) {
    iconSize_ = size;
    switch(mode_) {
    case ModePlaces:
        static_cast<PlacesView*>(view_)->setIconSize(size);
        /* Falls through. */
    case ModeDirTree:
        static_cast<QTreeView*>(view_)->setIconSize(size);
        break;
    default:
        ;
    }
}

void SidePane::setCurrentPath(Fm::FilePath path) {
    Q_ASSERT(path);
    currentPath_ = std::move(path);
    switch(mode_) {
    case ModePlaces:
        static_cast<PlacesView*>(view_)->setCurrentPath(currentPath_);
        break;
    case ModeDirTree:
        static_cast<DirTreeView*>(view_)->setCurrentPath(currentPath_);
        break;
    default:
        ;
    }
}

SidePane::Mode SidePane::modeByName(const char* str) {
    if(str == nullptr) {
        return ModeNone;
    }
    if(strcmp(str, "places") == 0) {
        return ModePlaces;
    }
    if(strcmp(str, "dirtree") == 0) {
        return ModeDirTree;
    }
    return ModeNone;
}

const char* SidePane::modeName(SidePane::Mode mode) {
    switch(mode) {
    case ModePlaces:
        return "places";
    case ModeDirTree:
        return "dirtree";
    default:
        return nullptr;
    }
}

bool SidePane::setHomeDir(const char* /*home_dir*/) {
    if(view_ == nullptr) {
        return false;
    }
    // TODO: SidePane::setHomeDir

    switch(mode_) {
    case ModePlaces:
        // static_cast<PlacesView*>(view_);
        return true;
    case ModeDirTree:
        // static_cast<PlacesView*>(view_);
        return true;
    default:
        ;
    }
    return true;
}

void SidePane::initDirTree() {
    DirTreeModel* model = new DirTreeModel(view_);
    model->setShowHidden(showHidden_);

    Fm::FilePathList rootPaths;
    rootPaths.emplace_back(Fm::FilePath::homeDir());
    rootPaths.emplace_back(Fm::FilePath::fromLocalPath("/"));
    model->addRoots(std::move(rootPaths));
    static_cast<DirTreeView*>(view_)->setModel(model);
    // wait for the roots to be added and only then set the current path
    connect(model, &DirTreeModel::rootsAdded, view_, [this] {
        if(mode_ == ModeDirTree) {
            DirTreeView* dirTreeView = static_cast<DirTreeView*>(view_);
            dirTreeView->setCurrentPath(currentPath_);
        }
    });
}

void SidePane::setMode(Mode mode) {
    if(mode == mode_) {
        return;
    }

    if(view_) {
        delete view_;
        view_ = nullptr;
        //if(sp->update_popup)
        //  g_signal_handlers_disconnect_by_func(sp->view, on_item_popup, sp);
    }
    mode_ = mode;

    combo_->setCurrentIndex(mode);
    switch(mode) {
    case ModePlaces: {
        PlacesView* placesView = new Fm::PlacesView(this);

        // visually merge it with its surroundings
        placesView->setFrameShape(QFrame::NoFrame);
        QPalette p = placesView->palette();
        p.setColor(QPalette::Base, QColor(Qt::transparent));
        p.setColor(QPalette::Text, p.color(QPalette::WindowText));
        placesView->setPalette(p);
        placesView->viewport()->setAutoFillBackground(false);

        view_ = placesView;
        placesView->restoreHiddenItems(restorableHiddenPlaces_);
        placesView->setIconSize(iconSize_);
        placesView->setCurrentPath(currentPath_);
        connect(placesView, &PlacesView::chdirRequested, this, &SidePane::chdirRequested);
        connect(placesView, &PlacesView::hiddenItemSet, this, &SidePane::hiddenPlaceSet);
        break;
    }
    case ModeDirTree: {
        DirTreeView* dirTreeView = new Fm::DirTreeView(this);
        view_ = dirTreeView;
        initDirTree();
        dirTreeView->setIconSize(iconSize_);
        connect(dirTreeView, &DirTreeView::chdirRequested, this, &SidePane::chdirRequested);
        connect(dirTreeView, &DirTreeView::openFolderInNewWindowRequested,
                this, &SidePane::openFolderInNewWindowRequested);
        connect(dirTreeView, &DirTreeView::openFolderInNewTabRequested,
                this, &SidePane::openFolderInNewTabRequested);
        connect(dirTreeView, &DirTreeView::openFolderInTerminalRequested,
                this, &SidePane::openFolderInTerminalRequested);
        connect(dirTreeView, &DirTreeView::createNewFolderRequested,
                this, &SidePane::createNewFolderRequested);
        connect(dirTreeView, &DirTreeView::prepareFileMenu,
                this, &SidePane::prepareFileMenu);
        break;
    }
    default:
        ;
    }
    if(view_) {
        // if(sp->update_popup)
        //  g_signal_connect(sp->view, "item-popup", G_CALLBACK(on_item_popup), sp);
        verticalLayout->addWidget(view_);
    }
    Q_EMIT modeChanged(mode);
}

void SidePane::setShowHidden(bool show_hidden) {
    if(view_ == nullptr || show_hidden == showHidden_) {
        return;
    }
    showHidden_ = show_hidden;
    if(mode_ == ModeDirTree) {
        DirTreeView* dirTreeView = static_cast<DirTreeView*>(view_);
        DirTreeModel* model = static_cast<DirTreeModel*>(dirTreeView->model());
        if(model) {
            model->setShowHidden(showHidden_);
        }
    }
}

void SidePane::restoreHiddenPlaces(const QSet<QString>& items) {
    if(mode_ == ModePlaces) {
        static_cast<PlacesView*>(view_)->restoreHiddenItems(items);
    }
    else {
        restorableHiddenPlaces_.unite(items);
    }
}

} // namespace Fm
