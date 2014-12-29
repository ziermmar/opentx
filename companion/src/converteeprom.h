#ifndef CONVERTEEPROM_H_
#define CONVERTEEPROM_H_
// TODO change file name

#include <QString>
#include "eeprominterface.h"

#define ER9X_EEPROM_FILE_TYPE          "ER9X_EEPROM_FILE"
#define EEPE_EEPROM_FILE_HEADER        "EEPE EEPROM FILE"
#define EEPE_MODEL_FILE_HEADER         "EEPE MODEL FILE"
#define HEX_FILES_FILTER               "HEX files (*.hex);;"
#define BIN_FILES_FILTER               "BIN files (*.bin);;"
#define EXTERNAL_EEPROM_FILES_FILTER   "EEPROM files (*.bin *.hex);;" BIN_FILES_FILTER HEX_FILES_FILTER
#define EEPE_FILES_FILTER              "EEPE EEPROM files (*.eepe);;"
#define EEPROM_FILES_FILTER            "EEPE files (*.eepe *.bin *.hex);;" EEPE_FILES_FILTER BIN_FILES_FILTER HEX_FILES_FILTER
#define FLASH_FILES_FILTER             "FLASH files (*.bin *.hex);;" BIN_FILES_FILTER HEX_FILES_FILTER

bool loadEEprom(const QString &filename, RadioData &radioData);
bool saveEEprom(const QString &filename, const RadioData &radioData);

int getEEpromVersion(const QString &filename);
bool isValidEEprom(const QString &filename);

bool convertEEprom(const QString &sourceEEprom, const QString &destinationEEprom, const QString &firmware);

#endif /* CONVERTEEPROM_H_ */
