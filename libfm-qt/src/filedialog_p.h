/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_FILEDIALOG_P_H
#define FM_FILEDIALOG_P_H

#include <QWidgetAction>
#include <QLabel>
#include <QSpinBox>
#include <QHBoxLayout>

namespace Fm {

class FileDialogIconSizeAction : public QWidgetAction {
    Q_OBJECT
public:
    FileDialogIconSizeAction(const QString& text, QObject* parent) : QWidgetAction (parent) {
        QWidget* w = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setSpacing(4);
        QLabel* label = new QLabel(text);
        layout->addWidget(label);
        spinBox = new QSpinBox();
        spinBox->setSuffix(tr(" px"));
        spinBox->setSingleStep(2);
        layout->addWidget(spinBox);
        w->setLayout(layout);
        setDefaultWidget(w);

        connect(spinBox, &QAbstractSpinBox::editingFinished, this, &FileDialogIconSizeAction::editingFinished);
    }

    int value() const {
        return spinBox->value();
    }
    void setValue(int val) {
        spinBox->setValue(val);
    }

    void setBounds(int min, int max) {
        spinBox->setMinimum(min);
        spinBox->setMaximum(max);
    }

    void setSingleStep(int val) {
        spinBox->setSingleStep(val);
    }

Q_SIGNALS:
    void editingFinished();

private:
    QSpinBox* spinBox;
};


} // namespace Fm
#endif // FM_FILEDIALOG_P_H
