/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mdichild.h"
#include "modelslist.h"
#include "xmlinterface.h"
#include "hexinterface.h"
#include "mainwindow.h"
#include "modeledit/modeledit.h"
#include "generaledit/generaledit.h"
#include "burnconfigdialog.h"
#include "printdialog.h"
#include "flasheepromdialog.h"
#include "helpers.h"
#include "appdata.h"
#include "wizarddialog.h"
#include "flashfirmwaredialog.h"
#include "converteeprom.h"
#include <QFileInfo>

#if defined WIN32 || !defined __GNUC__
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#else
#include <unistd.h>
#endif

MdiChild::MdiChild():
  QMainWindow(),
  firmware(GetCurrentFirmware()),
  isUntitled(true),
  fileChanged(false)
{
  modelsList = new ModelsListWidget(this, &radioData);
  setCentralWidget(modelsList);

  setWindowIcon(CompanionIcon("open.png"));
  setAttribute(Qt::WA_DeleteOnClose);

  eepromInterfaceChanged();

  if (!(this->isMaximized() || this->isMinimized())) {
    adjustSize();
  }

  connect(modelsList, SIGNAL(doubleclick(int)), this, SLOT(onDoubleClick(int)));
  connect(modelsList, SIGNAL(modeledit(int)), this, SLOT(onModelEdit(int)));
  connect(modelsList, SIGNAL(modelwizard(int)), this, SLOT(onModelWizard(int)));
  connect(modelsList, SIGNAL(modelprint(int)), this, SLOT(onModelPrint(int)));
  connect(modelsList, SIGNAL(modelload(int)), this, SLOT(onModelLoad(int)));
  connect(modelsList, SIGNAL(modelsimulate(int)), this, SLOT(onModelSimulate(int)));
  connect(modelsList, SIGNAL(modified()), this, SLOT(setModified()));
  connect(modelsList, SIGNAL(viablemodelselected(bool)), this, SLOT(viableModelSelected(bool)));
}

MdiChild::~MdiChild() 
{
}

void MdiChild::eepromInterfaceChanged()
{
  modelsList->refreshList();
  updateTitle();
}

void MdiChild::cut()
{
  modelsList->cut();
}

void MdiChild::copy()
{
  modelsList->copy();
}

void MdiChild::paste()
{
  modelsList->paste();
}

bool MdiChild::hasPasteData()
{
  return modelsList->hasPasteData();
}

bool MdiChild::hasSelection()
{
    return modelsList->hasSelection();
}

void MdiChild::updateTitle()
{
  QString title = userFriendlyCurrentFile() + "[*]"; //  + " (" + GetCurrentFirmware()->getName() + QString(")");
  if (!IS_SKY9X(GetCurrentFirmware()->getBoard()))
    title += QString(" - %1 ").arg(modelsList->availableSpace()) + tr("free bytes");
  setWindowTitle(title);
}

void MdiChild::setModified()
{
  modelsList->refreshList();
  fileChanged = true;
  updateTitle();
  documentWasModified();
}

void MdiChild::checkAndInitModel(int row)
{
  ModelData &model = radioData.models[row - 1];
  if (model.isempty()) {
    model.setDefaultValues(row - 1, radioData.generalSettings);
    setModified();
  }
}

void MdiChild::generalEdit()
{
  GeneralEdit *t = new GeneralEdit(this, radioData, GetCurrentFirmware()/*firmware*/);
  connect(t, SIGNAL(modified()), this, SLOT(setModified()));
  t->show();
}

void MdiChild::onModelEdit(int row)
{
  if (row == 0){
    generalEdit();
  } 
  else {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    checkAndInitModel( row );
    ModelData &model = radioData.models[row - 1];

    foreach(QDockWidget *dock, findChildren<QDockWidget *>()) {
      removeDockWidget(dock);
    }

    ModelEdit *t = new ModelEdit(this, radioData, (row - 1), GetCurrentFirmware()/*firmware*/);
    setWindowTitle(tr("Model %1: ").arg(row) + model.name);
    connect(t, SIGNAL(modified()), this, SLOT(setModified()));
    QApplication::restoreOverrideCursor();
  }
}

void MdiChild::onModelWizard(int row)
{
  if (row > 0) {
    checkAndInitModel(row);
    WizardDialog * wizard = new WizardDialog(radioData.generalSettings, row, this);
    wizard->exec();
    if (wizard->mix.complete /*TODO rather test the exec() result?*/) {
      radioData.models[row - 1] = wizard->mix;
      setModified();
    }
  }
}

void MdiChild::onDoubleClick(int row)
{
  if (row == 0){
    generalEdit();
  }
  else {
    ModelData &model = radioData.models[row - 1];
    if (model.isempty() && g.useWizard()) {
      onModelWizard(row);
    }
    else {
      onModelEdit(row);
    }
  }
}

void MdiChild::newFile()
{
  static int sequenceNumber = 1;
  isUntitled = true;
  curFile = QString("document%1.eepe").arg(sequenceNumber++);
  updateTitle();
}

bool MdiChild::loadFile(const QString &filename, bool resetCurrentFile)
{
  if (!loadEEprom(filename, radioData)) {
    return false;
  }

  modelsList->refreshList();
  if (resetCurrentFile) setCurrentFile(filename);
  return true;
}

bool MdiChild::save()
{
  if (isUntitled) {
    return saveAs(true);
  }
  else {
    return saveFile(curFile);
  }
}

bool MdiChild::saveAs(bool isNew)
{
    QString fileName;
    if (IS_SKY9X(GetEepromInterface()->getBoard())) {
      curFile.replace(".eepe", ".bin");
      QFileInfo fi(curFile);
#ifdef __APPLE__
      fileName = QFileDialog::getSaveFileName(this, tr("Save As"), g.eepromDir() + "/" +fi.fileName());
#else
      fileName = QFileDialog::getSaveFileName(this, tr("Save As"), g.eepromDir() + "/" +fi.fileName(), tr(BIN_FILES_FILTER));
#endif      
    }
    else {
      QFileInfo fi(curFile);
#ifdef __APPLE__
      fileName = QFileDialog::getSaveFileName(this, tr("Save As"), g.eepromDir() + "/" +fi.fileName());
#else
      fileName = QFileDialog::getSaveFileName(this, tr("Save As"), g.eepromDir() + "/" +fi.fileName(), tr(EEPROM_FILES_FILTER));
#endif      
    }
    if (fileName.isEmpty())
      return false;
    g.eepromDir( QFileInfo(fileName).dir().absolutePath() );
    if (isNew)
      return saveFile(fileName);
    else 
      return saveFile(fileName,true);
}

bool MdiChild::saveFile(const QString &filename, bool setCurrent)
{
  QString myFile;
  myFile = filename;
  if (IS_SKY9X(GetEepromInterface()->getBoard())) {
    myFile.replace(".eepe", ".bin");
  }
  if (!saveEEprom(myFile, radioData)) {
    return false;
  }

  if (setCurrent) setCurrentFile(myFile);
  return true;
}

QString MdiChild::userFriendlyCurrentFile()
{
  return strippedName(curFile);
}

void MdiChild::closeEvent(QCloseEvent *event)
{
  if (maybeSave()) {
    event->accept();
  }
  else {
    event->ignore();
  }
}

void MdiChild::documentWasModified()
{
  setWindowModified(fileChanged);
}

bool MdiChild::maybeSave()
{
  if (fileChanged) {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Companion"),
        tr("%1 has been modified.\n"
           "Do you want to save your changes?").arg(userFriendlyCurrentFile()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
      return save();
    else if (ret == QMessageBox::Cancel)
      return false;
  }
  return true;
}

void MdiChild::setCurrentFile(const QString &fileName)
{
  curFile = QFileInfo(fileName).canonicalFilePath();
  isUntitled = false;
  fileChanged = false;
  setWindowModified(false);
  updateTitle();
  int MaxRecentFiles = g.historySize();
  QStringList files = g.recentFiles();
  files.removeAll(fileName);
  files.prepend(fileName);
  while (files.size() > MaxRecentFiles)
      files.removeLast();
 
  g.recentFiles( files );
}

QString MdiChild::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void MdiChild::writeEeprom()  // write to Tx
{
  QString tempFile = generateProcessUniqueTempFileName("temp.bin");
  saveFile(tempFile, false);
  if(!QFileInfo(tempFile).exists()) {
    QMessageBox::critical(this, tr("Error"), tr("Cannot write temporary file!"));
    return;
  }

  FlashEEpromDialog *cd = new FlashEEpromDialog(this, tempFile);
  cd->exec();
}

void MdiChild::onModelSimulate(int row)
{
  if (row < 0) row = modelsList->currentRow();

  if (row > 0) {
    startSimulation(this, radioData, row-1);
  }
}

void MdiChild::onModelPrint(int row) // model, QString filename)
{
  if (row < 0) row = modelsList->currentRow();
  
  PrintDialog * pd = NULL;

  if (row > 0) {
    pd = new PrintDialog(this, GetCurrentFirmware()/*firmware*/, &radioData.generalSettings, &radioData.models[row-1]);
  }
    
  if (pd) {
    pd->setAttribute(Qt::WA_DeleteOnClose, true);
    pd->show();
  }
}

void MdiChild::viableModelSelected(bool viable)
{
  emit copyAvailable(viable);
}

void MdiChild::onModelLoad(int row)
{
  if (row < 0) row = modelsList->currentRow();
  
  if (row <= 0) return;

  int index = row-1;

  QString fileName = QFileDialog::getOpenFileName(this, tr("Open backup Models and Settings file"), g.eepromDir(),tr(EEPROM_FILES_FILTER));
  if (fileName.isEmpty())
    return;

  QFile file(fileName);

    if (!file.exists()) {
      QMessageBox::critical(this, tr("Error"), tr("Unable to find file %1!").arg(fileName));
      return;
    }

    int eeprom_size = file.size();
    if (!file.open(QFile::ReadOnly)) {  //reading binary file   - TODO HEX support
        QMessageBox::critical(this, tr("Error"),
                              tr("Error opening file %1:\n%2.")
                              .arg(fileName)
                              .arg(file.errorString()));
        return;
    }
    QByteArray eeprom(eeprom_size, 0);
    long result = file.read((char*)eeprom.data(), eeprom_size);
    file.close();

    if (result != eeprom_size) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Error reading file %1:\n%2.")
                              .arg(fileName)
                              .arg(file.errorString()));

        return;
    }

    if (!::loadBackup(radioData, (uint8_t *)eeprom.data(), eeprom_size, index)) {
      QMessageBox::critical(this, tr("Error"),
          tr("Invalid binary backup File %1")
          .arg(fileName));
      return;
    }

    modelsList->refreshList();
}
