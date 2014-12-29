#include "modeledit.h"
#include "setup.h"
#include "heli.h"
#include "flightmodes.h"
#include "inputs.h"
#include "mixes.h"
#include "channels.h"
#include "curves.h"
#include "logicalswitches.h"
#include "customfunctions.h"
#include "telemetry.h"
#include "verticalscrollarea.h"
#include <QDockWidget>

ModelEdit::ModelEdit(QWidget * parent, RadioData & radioData, int modelId, Firmware * firmware) :
  modelId(modelId),
  model(radioData.models[modelId]),
  generalSettings(radioData.generalSettings),
  firmware(firmware)
{
  SetupPanel * setupPanel = new SetupPanel(parent, model, generalSettings, firmware);
  QDockWidget *dock = addTab(setupPanel, tr("Setup"));
  if (firmware->getCapability(Heli))
    addTab(new HeliPanel(parent, model, generalSettings, firmware), tr("Heli"), dock);
  addTab(new FlightModesPanel(parent, model, generalSettings, firmware), tr("Flight Modes"), dock);
  addTab(new InputsPanel(parent, model, generalSettings, firmware), tr("Inputs"), dock);
  addTab(new MixesPanel(parent, model, generalSettings, firmware), tr("Mixes"), dock);
  Channels * chnPanel = new Channels(parent, model, generalSettings, firmware);
  addTab(chnPanel, tr("Servos"), dock);
  addTab(new Curves(parent, model, generalSettings, firmware), tr("Curves"), dock);
  addTab(new LogicalSwitchesPanel(parent, model, generalSettings, firmware), tr("Logical Switches"), dock);
  addTab(new CustomFunctionsPanel(parent, &model, generalSettings, firmware), tr("Special Functions"), dock);
  if (firmware->getCapability(Telemetry) & TM_HASTELEMETRY)
    addTab(new TelemetryPanel(parent, model, generalSettings, firmware), tr("Telemetry"), dock);
    
  connect(setupPanel, SIGNAL(extendedLimitsToggled()), chnPanel, SLOT(refreshExtendedLimits()));
}

QDockWidget *ModelEdit::addTab(GenericPanel *panel, const QString &name, QDockWidget *tab)
{
  panels << panel;

  QMainWindow *parent = (QMainWindow *)panel->parent();

  VerticalScrollArea *area = new VerticalScrollArea(parent, panel);
  area->setFrameShape(QFrame::NoFrame);
  panel->update();
  connect(panel, SIGNAL(modified()), this, SLOT(onTabModified()));

  //QDockWidget *result = mainWindow->addDockWindow(name, area, dock, Qt::RightDockWidgetArea);


  QDockWidget *dock = new QDockWidget(name, parent);
  dock->setObjectName(name);
  dock->setWidget(area);

  if (tab)
    parent->tabifyDockWidget(tab, dock);
  else
    parent->addDockWidget(Qt::RightDockWidgetArea, dock);

  return dock;
}

void ModelEdit::onTabModified()
{
  GenericPanel *origin = dynamic_cast<GenericPanel *>(sender());
  foreach(GenericPanel *panel, panels) {
    if (panel != origin)
      panel->update();
  }
  emit modified();
}
