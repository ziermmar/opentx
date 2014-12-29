#ifndef FLASH_FIRMWARE_DIALOG_H
#define FLASH_FIRMWARE_DIALOG_H

#include <QtGui>
#include <QDialog>
#include "eeprominterface.h"
#include "firmwareinterface.h"
#include "xmlinterface.h"

namespace Ui
{
  class FlashFirmwareDialog;
}

class FlashFirmwareDialog : public QDialog
{
  Q_OBJECT

public:
  FlashFirmwareDialog(QWidget *parent = 0);
  ~FlashFirmwareDialog();

private slots:
  void on_firmwareLoad_clicked();
  void on_burnButton_clicked();
  void on_cancelButton_clicked();
  void on_useProfileSplash_clicked();
  void on_useFirmwareSplash_clicked();
  void on_useLibrarySplash_clicked();
  void on_useExternalSplash_clicked();
  void shrink();

protected:
  void updateUI();
  void startFlash(const QString &filename);

private:
  Ui::FlashFirmwareDialog *ui;
  QString fwName;
  RadioData radioData;
  enum ImageSource {FIRMWARE, PROFILE, LIBRARY, EXTERNAL};
  ImageSource imageSource;
  QString imageFile;
};

#endif // FLASH_FIRMWARE_DIALOG_H
