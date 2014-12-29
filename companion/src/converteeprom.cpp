#include "converteeprom.h"
#include "eeprominterface.h"
#include "firmwareinterface.h"
#include "hexinterface.h"
#include <QFile>
#include <QMessageBox>

bool loadEEprom(const QString &filename, RadioData &radioData)
{
  if (filename.isEmpty()) {
    return false;
  }

  QFile file(filename);
  if (!file.exists()) {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Unable to find file %1!").arg(filename));
    return false;
  }

  int fileType = getFileType(filename);
#if 0
  if (fileType==FILE_TYPE_XML) {
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  //reading HEX TEXT file
      QMessageBox::warning(this, QObject::tr("Error"), QObject::tr("Error opening file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
    QTextStream inputStream(&file);
    XmlInterface(inputStream).load(radioData);
  }
  else
#endif
  if (fileType==FILE_TYPE_HEX || fileType==FILE_TYPE_EEPE) { //read HEX file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  //reading HEX TEXT file
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Error opening file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }

    QDomDocument doc(ER9X_EEPROM_FILE_TYPE);
    bool xmlOK = doc.setContent(&file);
    if (xmlOK) {
      if (loadEEpromXml(radioData, doc)) {
        return true;
      }
    }
    file.reset();

    QTextStream inputStream(&file);
    if (fileType==FILE_TYPE_EEPE) {  // read EEPE file header
      QString hline = inputStream.readLine();
      if (hline != EEPE_EEPROM_FILE_HEADER) {
        return false;
      }
    }

    QByteArray eeprom(EESIZE_RLC_MAX, 0);
    int eeprom_size = HexInterface(inputStream).load((uint8_t *)eeprom.data(), EESIZE_RLC_MAX);
    if (!eeprom_size) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Invalid EEPROM File %1").arg(filename));
      return false;
    }
    if (!loadEEprom(radioData, (uint8_t *)eeprom.data(), eeprom_size)) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Invalid EEPROM File %1").arg(filename));
      return false;
    }
  }
  else if (fileType==FILE_TYPE_BIN) { //read binary
    int eeprom_size = file.size();
    if (!file.open(QFile::ReadOnly)) {  //reading binary file   - TODO HEX support
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Error opening file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
    QByteArray eeprom(eeprom_size, 0);
    long result = file.read(eeprom.data(), eeprom_size);
    if (result != eeprom_size) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Error reading file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
    if (!loadEEprom(radioData, (uint8_t *)eeprom.data(), eeprom_size) && !loadBackup(radioData, (uint8_t *)eeprom.data(), eeprom_size, 0)) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Invalid binary Models and Settings File %1").arg(filename));
      return false;
    }
  }

  return true;
}

bool saveEEprom(const QString &filename, const RadioData &radioData)
{
  QFile file(filename);
  int fileType = getFileType(filename);
  QByteArray eeprom(GetEepromInterface()->getEEpromSize(), 0);
  int eeprom_size = 0;

  if (fileType != FILE_TYPE_XML) {
    eeprom_size = GetEepromInterface()->save((uint8_t *)eeprom.data(), radioData, GetCurrentFirmware()->getVariantNumber(), 0/*last version*/);
    if (!eeprom_size) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
  }

  if (!file.open(fileType == FILE_TYPE_BIN ? QIODevice::WriteOnly : (QIODevice::WriteOnly | QIODevice::Text))) {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
    return false;
  }

  QTextStream outputStream(&file);

#if 0
  if (fileType==FILE_TYPE_XML) {
    if (!XmlInterface(outputStream).save(radioData)) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
  }
  else
#endif
  if (fileType==FILE_TYPE_HEX || fileType==FILE_TYPE_EEPE) { // write hex
    if (fileType==FILE_TYPE_EEPE)
      outputStream << EEPE_EEPROM_FILE_HEADER << "\n";

    if (!HexInterface(outputStream).save((const uint8_t *)eeprom.constData(), eeprom_size)) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Cannot write file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
  }
  else if (fileType==FILE_TYPE_BIN) { // write binary
    long result = file.write(eeprom.constData(), eeprom_size);
    if (result!=eeprom_size) {
      QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Error writing file %1:\n%2.").arg(filename).arg(file.errorString()));
      return false;
    }
  }
  else {
    QMessageBox::warning(NULL, QObject::tr("Error"), QObject::tr("Error writing file %1:\n%2.").arg(filename).arg("Unknown format"));
    return false;
  }

  return true;
}

bool isValidEEprom(const QString &filename)
{
  QSharedPointer<RadioData> radioData = QSharedPointer<RadioData>(new RadioData());
  return loadEEprom(filename, *radioData);
}

int getEEpromVersion(const QString &filename)
{
  QSharedPointer<RadioData> radioData = QSharedPointer<RadioData>(new RadioData());
  if (!loadEEprom(filename, *radioData))
    return -1;
  return radioData->generalSettings.version;
}

bool convertEEprom(const QString &sourceEEprom, const QString &destinationEEprom, const QString &firmwareFilename)
{
  Firmware *currentFirmware = GetCurrentFirmware();
  FirmwareInterface firmware(firmwareFilename);
  if (!firmware.isValid())
    return false;

  unsigned int version = firmware.getEEpromVersion();
  unsigned int variant = firmware.getEEpromVariant();

  QFile sourceFile(sourceEEprom);
  int eeprom_size = sourceFile.size();
  if (!eeprom_size)
    return false;

  if (!sourceFile.open(QIODevice::ReadOnly))
    return false;

  QByteArray eeprom(eeprom_size, 0);
  long result = sourceFile.read(eeprom.data(), eeprom_size);
  sourceFile.close();

  QSharedPointer<RadioData> radioData = QSharedPointer<RadioData>(new RadioData());
  if (!loadEEprom(*radioData, (uint8_t *)eeprom.data(), eeprom_size) || !currentFirmware->saveEEPROM((uint8_t *)eeprom.data(), *radioData, variant, version))
    return false;

  QFile destinationFile(destinationEEprom);
  if (!destinationFile.open(QIODevice::WriteOnly))
    return false;

  result = destinationFile.write(eeprom.constData(), eeprom_size);
  destinationFile.close();
  if (result != eeprom_size)
    return false;

  return true;
}
