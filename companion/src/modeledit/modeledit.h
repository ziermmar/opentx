#ifndef MODELEDIT_H
#define MODELEDIT_H

#include "genericpanel.h"
#include <QDialog>
#include <QDockWidget>

class RadioData;

class ModelPanel : public GenericPanel
{
  public:
    ModelPanel(QWidget *parent, ModelData & model, GeneralSettings & generalSettings, Firmware * firmware):
      GenericPanel(parent, &model, generalSettings, firmware)
    {
    }
};

class ModelEdit : public QObject // QDialog
{
    Q_OBJECT

  public:
    ModelEdit(QWidget * parent, RadioData & radioData, int modelId, Firmware * firmware);
  
  signals:
    void modified();

  protected slots:
    void onTabModified();

  private:
    int modelId;
    ModelData & model;
    GeneralSettings & generalSettings;
    Firmware * firmware;
    QVector<GenericPanel *> panels;
    QDockWidget *addTab(GenericPanel *panel, const QString &name, QDockWidget *dock=NULL);

};

#endif // MODELEDIT_H
