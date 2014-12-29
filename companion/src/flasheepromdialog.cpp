#include "flasheepromdialog.h"
#include "ui_flasheepromdialog.h"
#include "eeprominterface.h"
#include "helpers.h"
#include "splashlibrary.h"
#include "firmwareinterface.h"
#include "hexinterface.h"
#include "appdata.h"
#include "progressdialog.h"
#include "radiointerface.h"
#include "converteeprom.h"

FlashEEpromDialog::FlashEEpromDialog(QWidget *parent, const QString &filename):
QDialog(parent),
ui(new Ui::FlashEEpromDialog),
eepromFilename(filename),
radioData(new RadioData())
{
  ui->setupUi(this);
  ui->profileLabel->setText(tr("Current profile: %1").arg(g.profile[g.id()].name()));
  if (!filename.isEmpty()) {
    ui->eepromFilename->hide();
    ui->eepromLoad->hide();
  }
  QString backupPath = g.backupDir();
  if (backupPath.isEmpty() || !QDir(backupPath).exists()) {
    ui->backupBeforeWrite->setEnabled(false);
  }
  updateUI();
}

FlashEEpromDialog::~FlashEEpromDialog()
{
  delete ui;
}

void FlashEEpromDialog::updateUI()
{
  // ui->burnButton->setEnabled(true);
  ui->eepromFilename->setText(eepromFilename);
  if (isValidEEprom(eepromFilename)) {
    ui->profileLabel->show();
    ui->patchCalibration->show();
    ui->patchHardwareSettings->show();
    QString name = g.profile[g.id()].name();
    QString calib = g.profile[g.id()].stickPotCalib();
    QString trainercalib = g.profile[g.id()].trainerCalib();
    QString DisplaySet = g.profile[g.id()].display();
    QString BeeperSet = g.profile[g.id()].beeper();
    QString HapticSet = g.profile[g.id()].haptic();
    QString SpeakerSet = g.profile[g.id()].speaker();
    if (!name.isEmpty()) {
      ui->profileLabel->show();
      ui->patchCalibration->show();
      ui->patchHardwareSettings->show();
      // TODO I hardcode the number of pots here, should be dependant on the board?
      if (!((calib.length()==(NUM_STICKS+3)*12) && (trainercalib.length()==16))) {
        ui->patchCalibration->setDisabled(true);
      }
      if (!((DisplaySet.length()==6) && (BeeperSet.length()==4) && (HapticSet.length()==6) && (SpeakerSet.length()==6))) {
        ui->patchHardwareSettings->setDisabled(true);
      }
    }
    else {
      ui->profileLabel->hide();
    }
    ui->burnButton->setEnabled(true);
  }
  else {
    ui->profileLabel->hide();
    ui->patchCalibration->hide();
    ui->patchHardwareSettings->hide();
  }
  QTimer::singleShot(0, this, SLOT(shrink()));
}

void FlashEEpromDialog::on_eepromLoad_clicked()
{
  QString filename = QFileDialog::getOpenFileName(this, tr("Choose Radio Backup file"), g.eepromDir(), tr(EXTERNAL_EEPROM_FILES_FILTER));
  if (!filename.isEmpty()) {
    eepromFilename = filename;
    updateUI();
  }
}

bool FlashEEpromDialog::patchCalibration()
{
  QString calib = g.profile[g.id()].stickPotCalib();
  QString trainercalib = g.profile[g.id()].trainerCalib();
  int potsnum=GetCurrentFirmware()->getCapability(Pots);
  int8_t vBatCalib=(int8_t) g.profile[g.id()].vBatCalib();
  int8_t currentCalib=(int8_t) g.profile[g.id()].currentCalib();
  int8_t PPM_Multiplier=(int8_t) g.profile[g.id()].ppmMultiplier();

  if ((calib.length()==(NUM_STICKS+potsnum)*12) && (trainercalib.length()==16)) {
    QString Byte;
    int16_t byte16;
    bool ok;
    for (int i=0; i<(NUM_STICKS+potsnum); i++) {
      Byte=calib.mid(i*12,4);
      byte16=(int16_t)Byte.toInt(&ok,16);
      if (ok)
        radioData->generalSettings.calibMid[i]=byte16;
      Byte=calib.mid(4+i*12,4);
      byte16=(int16_t)Byte.toInt(&ok,16);
      if (ok)
        radioData->generalSettings.calibSpanNeg[i]=byte16;
      Byte=calib.mid(8+i*12,4);
      byte16=(int16_t)Byte.toInt(&ok,16);
      if (ok)
        radioData->generalSettings.calibSpanPos[i]=byte16;
    }
    for (int i=0; i<4; i++) {
      Byte = trainercalib.mid(i*4,4);
      byte16 = (int16_t)Byte.toInt(&ok,16);
      if (ok) {
        radioData->generalSettings.trainer.calib[i] = byte16;
      }
    }
    radioData->generalSettings.currentCalib = currentCalib;
    radioData->generalSettings.vBatCalib = vBatCalib;
    radioData->generalSettings.PPM_Multiplier = PPM_Multiplier;
    return true;
  }
  else {
    QMessageBox::critical(this, tr("Warning"), tr("Wrong radio calibration data in profile, Settings not patched"));
    return false;
  }
}

bool FlashEEpromDialog::patchHardwareSettings()
{
  QString DisplaySet = g.profile[g.id()].display();
  QString BeeperSet = g.profile[g.id()].beeper();
  QString HapticSet = g.profile[g.id()].haptic();
  QString SpeakerSet = g.profile[g.id()].speaker();
  uint8_t GSStickMode=(uint8_t) g.profile[g.id()].gsStickMode();
  uint8_t vBatWarn=(uint8_t) g.profile[g.id()].vBatWarn();

  if ((DisplaySet.length()==6) && (BeeperSet.length()==4) && (HapticSet.length()==6) && (SpeakerSet.length()==6)) {
    radioData->generalSettings.vBatWarn = vBatWarn;
    radioData->generalSettings.stickMode = GSStickMode;
    uint8_t byte8u;
    int8_t byte8;
    bool ok;
    byte8=(int8_t)DisplaySet.mid(0,2).toInt(&ok,16);
    if (ok) radioData->generalSettings.optrexDisplay=(byte8==1 ? true : false);
    byte8u=(uint8_t)DisplaySet.mid(2,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.contrast=byte8u;
    byte8u=(uint8_t)DisplaySet.mid(4,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.backlightBright=byte8u;
    byte8u=(uint8_t)BeeperSet.mid(0,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.beeperMode=(GeneralSettings::BeeperMode)byte8u;
    byte8=(int8_t)BeeperSet.mid(2,2).toInt(&ok,16);
    if (ok) radioData->generalSettings.beeperLength=byte8;
    byte8u=(uint8_t)HapticSet.mid(0,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.hapticMode=(GeneralSettings::BeeperMode)byte8u;
    byte8u=(uint8_t)HapticSet.mid(2,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.hapticStrength=byte8u;
    byte8=(int8_t)HapticSet.mid(4,2).toInt(&ok,16);
    if (ok) radioData->generalSettings.hapticLength=byte8;
    byte8u=(uint8_t)SpeakerSet.mid(0,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.speakerMode=byte8u;
    byte8u=(uint8_t)SpeakerSet.mid(2,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.speakerPitch=byte8u;
    byte8u=(uint8_t)SpeakerSet.mid(4,2).toUInt(&ok,16);
    if (ok) radioData->generalSettings.speakerVolume=byte8u;
    return true;
  }
  else {
    QMessageBox::critical(this, tr("Warning"), tr("Wrong radio setting data in profile, Settings not patched"));
    return false;
  }
}

void FlashEEpromDialog::on_burnButton_clicked()
{
  // patch the eeprom if needed
  bool patch = false;
  if (ui->patchCalibration->isChecked()) {
    patch |= patchCalibration();
  }
  if (ui->patchHardwareSettings->isChecked()) {
    patch |= patchHardwareSettings();
  }
  QString filename = eepromFilename;
  if (patch) {
    QString filename = generateProcessUniqueTempFileName("temp.bin");
    QFile file(filename);
    uint8_t *eeprom = (uint8_t*)malloc(GetEepromInterface()->getEEpromSize());
    int eeprom_size = GetEepromInterface()->save(eeprom, *radioData, GetCurrentFirmware()->getVariantNumber());
    if (!eeprom_size) {
      QMessageBox::warning(this, tr("Error"), tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
      return;
    }
    if (!file.open(QIODevice::WriteOnly)) {
      QMessageBox::warning(this, tr("Error"), tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
      return;
    }
    QTextStream outputStream(&file);
    long result = file.write((char*)eeprom, eeprom_size);
    if (result != eeprom_size) {
      QMessageBox::warning(this, tr("Error"), tr("Error writing file %1:\n%2.").arg(filename).arg(file.errorString()));
      return;
    }
  }

  close();

  ProgressDialog progressDialog(this, tr("Write Models and Settings to Radio"), CompanionIcon("write_eeprom.png"));

  // backup previous EEPROM if requested
  QString backupFilename;
  if (ui->backupBeforeWrite->isChecked()) {
    backupFilename = g.backupDir() + "/backup-" + QDateTime().currentDateTime().toString("yyyy-MM-dd-HHmmss") + ".bin";
  }
  else if (ui->checkFirmwareCompatibility->isChecked()) {
    backupFilename = generateProcessUniqueTempFileName("eeprom.bin");
  }
  if (!backupFilename.isEmpty()) {
    if (!readEeprom(backupFilename, progressDialog.progress())) {
      return;
    }
  }

  // check EEPROM compatibility if requested
  if (ui->checkFirmwareCompatibility->isChecked()) {
    int eepromVersion = getEEpromVersion(filename);
    QString firmwareFilename = generateProcessUniqueTempFileName("flash.bin");
    if (!readFirmware(firmwareFilename, progressDialog.progress()))
      return;
    QString compatEEprom = generateProcessUniqueTempFileName("compat.bin");
    if (convertEEprom(filename, compatEEprom, firmwareFilename)) {
      int compatVersion = getEEpromVersion(compatEEprom);
      if ((compatVersion / 100) != (eepromVersion / 100)) {
        QMessageBox::warning(this, tr("Warning"), tr("The radio firmware belongs to another product family, check file and preferences!"));
        return;
      }
      else if (compatVersion < eepromVersion) {
        QMessageBox::warning(this, tr("Warning"), tr("The radio firmware is outdated, please upgrade!"));
        return;
      }
      filename = compatEEprom;
    }
    else if (QMessageBox::question(this, "Error", tr("Cannot check Models and Settings compatibility! Continue anyway?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::No) {
      return;
    }
    qunlink(firmwareFilename);
  }

  // and write...
  writeEeprom(filename, progressDialog.progress());

  progressDialog.exec();
}

void FlashEEpromDialog::on_cancelButton_clicked()
{
  close();
}

void FlashEEpromDialog::shrink()
{
  resize(0, 0);
}

