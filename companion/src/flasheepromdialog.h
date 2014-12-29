#ifndef FLASHEEPROMDIALOG_H_
#define FLASHEEPROMDIALOG_H_

#include <QDialog>

namespace Ui
{
  class FlashEEpromDialog;
}

class RadioData;

class FlashEEpromDialog : public QDialog
{
  Q_OBJECT

public:
  explicit FlashEEpromDialog(QWidget *parent, const QString &fileName="");
  ~FlashEEpromDialog();

private slots:
  void on_eepromLoad_clicked();
  void on_burnButton_clicked();
  void on_cancelButton_clicked();
  void shrink();

protected:
  void updateUI();
  bool patchCalibration();
  bool patchHardwareSettings();

private:
  Ui::FlashEEpromDialog *ui;
  QString eepromFilename;
  RadioData *radioData;
};

#endif // FLASHEEPROMDIALOG_H_
