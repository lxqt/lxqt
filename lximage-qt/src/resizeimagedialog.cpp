/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * Resize feature inspired by Gwenview's one
 * Copyright 2010 Aurélien Gâteau <agateau@kde.org>
 * adjam refactored
 * Copyright 2020 Andrea Diamantini <adjam@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "resizeimagedialog.h"
#include <QPushButton>
using namespace LxImage;


ResizeImageDialog::ResizeImageDialog(QWidget* parent):
  QDialog(parent) {

  ui.setupUi(this);
  ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  updateFromRatio_ = false;
  updateFromSizeOrPercentage_ = false;
  connect(ui.widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::onWidthChanged);
  connect(ui.heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ResizeImageDialog::onHeightChanged);
  connect(ui.widthPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::onWidthPercentChanged);
  connect(ui.heightPercentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ResizeImageDialog::onHeightPercentChanged);
  connect(ui.keepAspectCheckBox, &QCheckBox::toggled, this, &ResizeImageDialog::onKeepAspectChanged);
}

ResizeImageDialog::~ResizeImageDialog() {

}

void ResizeImageDialog::setOriginalSize(const QSize& size) {
  originalSize_ = size;
  ui.originalWidthLabel->setText(QString::number(size.width()));
  ui.originalHeightLabel->setText(QString::number(size.height()));
  ui.widthSpinBox->setValue(size.width());
  ui.heightSpinBox->setValue(size.height());
}

QSize ResizeImageDialog::scaledSize() const {
  return QSize (ui.widthSpinBox->value(),
                ui.heightSpinBox->value());
}

void ResizeImageDialog::onWidthChanged(int width) {
  // Update width percentage to match width, only if this was a manual adjustment
  if(!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.widthPercentSpinBox->setValue((double(width) / originalSize_.width()) * 100);
    updateFromSizeOrPercentage_ = false;
  }

  if(!ui.keepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Update height to keep ratio, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.heightSpinBox->setValue(originalSize_.height() * width / originalSize_.width());
  updateFromRatio_ = false;
}

void ResizeImageDialog::onHeightChanged(int height) {
  // Update height percentage to match height, only if this was a manual adjustment
  if(!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.heightPercentSpinBox->setValue((double(height) / originalSize_.height()) * 100);
    updateFromSizeOrPercentage_ = false;
  }

  if(!ui.keepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Update width to keep ratio, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.widthSpinBox->setValue(originalSize_.width() * height / originalSize_.height());
  updateFromRatio_ = false;
}

void ResizeImageDialog::onWidthPercentChanged(double widthPercent) {
  // Update width to match width percentage, only if this was a manual adjustment
  if(!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.widthSpinBox->setValue((widthPercent / 100) * originalSize_.width());
    updateFromSizeOrPercentage_ = false;
  }

  if(!ui.keepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.heightPercentSpinBox->setValue(ui.widthPercentSpinBox->value());
  updateFromRatio_ = false;
}

void ResizeImageDialog::onHeightPercentChanged(double heightPercent) {
  // Update height to match height percentage, only if this was a manual adjustment
  if(!updateFromSizeOrPercentage_ && !updateFromRatio_) {
    updateFromSizeOrPercentage_ = true;
    ui.heightSpinBox->setValue((heightPercent / 100) * originalSize_.height());
    updateFromSizeOrPercentage_ = false;
  }

  if(!ui.keepAspectCheckBox->isChecked() || updateFromRatio_) {
    return;
  }

  // Keep height percentage in sync with width percentage, only if ratio locked and this was a manual adjustment
  updateFromRatio_ = true;
  ui.widthPercentSpinBox->setValue(ui.heightPercentSpinBox->value());
  updateFromRatio_ = false;
}

void ResizeImageDialog::onKeepAspectChanged(bool value) {
  if(value) {
    updateFromSizeOrPercentage_ = true;
    onWidthChanged(ui.widthSpinBox->value());
    onWidthPercentChanged(ui.widthPercentSpinBox->value());
    updateFromSizeOrPercentage_ = false;
  }
}
