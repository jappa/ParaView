/*=========================================================================

   Program: ParaView
   Module:  pqPreviewMenuManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqPreviewMenuManager.h"
#include "ui_pqCustomResolutionDialog.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"

#include <QDialog>
#include <QIntValidator>
#include <QMenu>
#include <QRegExp>
#include <QStringList>

#define SETUP_ACTION(actn)                                                                         \
  if (QAction* tmp = actn)                                                                         \
  {                                                                                                \
    tmp->setCheckable(true);                                                                       \
    this->connect(tmp, SIGNAL(triggered(bool)), SLOT(lockResolution(bool)));                       \
  }

namespace
{
QString generateText(int dx, int dy, const QString& label)
{
  return label.isEmpty() ? QString("%1 x %2").arg(dx).arg(dy)
                         : QString("%1 x %2 (%3)").arg(dx).arg(dy).arg(label);
}

QString extractLabel(const QString& txt)
{
  QRegExp re("^(\\d+) x (\\d+) \\((.*)\\)$");
  if (re.indexIn(txt) != -1)
  {
    return re.cap(3);
  }
  return QString();
}

QSize extractSize(const QString& txt)
{
  QRegExp re("^(\\d+) x (\\d+)");
  if (re.indexIn(txt) != -1)
  {
    return QSize(re.cap(1).toInt(), re.cap(2).toInt());
  }
  return QSize();
}
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::pqPreviewMenuManager(QMenu* menu)
  : Superclass(menu)
  , FirstCustomAction(nullptr)
{
  QStringList defaultItems;
  defaultItems << "1280 x 800 (WXGA)"
               << "1280 x 1024 (SXGA)"
               << "1600 x 900 (HD+)"
               << "1920 x 1080 (FHD)"
               << "3840 x 2160 (4K UHD)";
  this->init(defaultItems, menu);
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::pqPreviewMenuManager(const QStringList& defaultItems, QMenu* menu)
  : Superclass(menu)
  , FirstCustomAction(nullptr)
{
  this->init(defaultItems, menu);
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::init(const QStringList& defaultItems, QMenu* menu)
{
  if (qobject_cast<pqTabbedMultiViewWidget*>(
        pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET")))
  {
    menu->setEnabled(true);
    foreach (const QString& txt, defaultItems)
    {
      SETUP_ACTION(menu->addAction(txt));
    }
    if (defaultItems.size() > 0)
    {
      menu->addSeparator();
    }
    menu->addAction("Custom ...", this, SLOT(addCustom()));
    this->updateCustomActions();
  }
  else
  {
    menu->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
pqPreviewMenuManager::~pqPreviewMenuManager()
{
}

//-----------------------------------------------------------------------------
QMenu* pqPreviewMenuManager::parentMenu() const
{
  return qobject_cast<QMenu*>(this->parent());
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::updateCustomActions()
{
  QMenu* menu = this->parentMenu();

  // remove old custom actions.
  QList<QAction*> actions = menu->actions();
  for (int index = 0; index < actions.size(); ++index)
  {
    if (actions[index]->data().toBool() == true)
    {
      menu->removeAction(actions[index]);
    }
  }

  this->FirstCustomAction = nullptr;

  // add custom actions.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList resolutions = settings->value("PreviewResolutions").toStringList();
  foreach (const QString& res, resolutions)
  {
    QAction* actn = menu->addAction(res);
    SETUP_ACTION(actn);
    actn->setData(true); // flag custom actions.

    // save for later.
    if (this->FirstCustomAction == nullptr)
    {
      this->FirstCustomAction = actn;
    }
  }
}

//-----------------------------------------------------------------------------
bool pqPreviewMenuManager::prependCustomResolution(int dx, int dy, const QString& label)
{
  if (dx >= 1 && dy >= 1)
  {
    pqSettings* settings = pqApplicationCore::instance()->settings();
    QStringList resolutions = settings->value("PreviewResolutions").toStringList();

    QString txt = generateText(dx, dy, label);
    // find and remove duplicate, if any.
    resolutions.removeOne(txt);
    resolutions.push_front(txt);
    while (resolutions.size() > 5)
    {
      resolutions.pop_back();
    }
    settings->setValue("PreviewResolutions", resolutions);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::addCustom()
{
  QDialog dialog(pqCoreUtilities::mainWidget());
  Ui::CustomResolutionDialog ui;
  ui.setupUi(&dialog);
  QIntValidator* validator = new QIntValidator(&dialog);
  validator->setBottom(1);
  ui.resolutionX->setValidator(validator);
  ui.resolutionY->setValidator(validator);
  if (dialog.exec() == QDialog::Accepted)
  {
    const int dx = ui.resolutionX->text().toInt();
    const int dy = ui.resolutionY->text().toInt();
    const QString label = ui.resolutionLabel->text();
    if (this->prependCustomResolution(dx, dy, label))
    {
      this->updateCustomActions();
      this->lockResolution(dx, dy);
    }
  }
}

//-----------------------------------------------------------------------------
QAction* pqPreviewMenuManager::findAction(int dx, int dy)
{
  QString prefix = QString("%1 x %2").arg(dx).arg(dy);

  foreach (QAction* actn, this->parentMenu()->actions())
  {
    if (actn->text().startsWith(prefix))
    {
      return actn;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(bool lock)
{
  if (QAction* actn = qobject_cast<QAction*>(this->sender()))
  {
    if (lock)
    {
      QSize size = extractSize(actn->text());
      if (size.isEmpty() == false)
      {
        this->lockResolution(size.width(), size.height(), actn);
      }
    }
    else
    {
      // unlock.
      this->unlock();
    }
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(int dx, int dy)
{
  if (QSize(dx, dy).isEmpty())
  {
    this->unlock();
  }
  else
  {
    this->lockResolution(dx, dy, nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::lockResolution(int dx, int dy, QAction* target)
{
  Q_ASSERT(dx >= 1 && dy >= 1);

  if (QAction* actn = (target ? target : this->findAction(dx, dy)))
  {
    actn->setChecked(true);

    foreach (QAction* other, this->parentMenu()->actions())
    {
      if (other != actn && other->isChecked())
      {
        other->setChecked(false);
      }
    }

    // if `actn` is a custom action, let's sort the custom resolutions list to
    // have the most recently used item at the top of the list.
    if (actn->data().toBool() == true)
    {
      this->prependCustomResolution(dx, dy, extractLabel(actn->text()));

      // move actn to the top of the list in the menu as well.
      if (this->FirstCustomAction != nullptr && this->FirstCustomAction != actn)
      {
        this->parentMenu()->removeAction(actn);
        this->parentMenu()->insertAction(this->FirstCustomAction, actn);
        this->FirstCustomAction = actn;
      }
    }
  }

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  Q_ASSERT(viewManager);
  if (viewManager->preview(QSize(dx, dy)) == false)
  {
    pqCoreUtilities::promptUser("pqPreviewMenuManager/LockResolutionPrompt",
      QMessageBox::Information, "Requested resolution too big for window",
      "The resolution requested is too big for the current window. Fitting to aspect ratio "
      "instead.",
      QMessageBox::Ok | QMessageBox::Save, pqCoreUtilities::mainWidget());
  }
}

//-----------------------------------------------------------------------------
void pqPreviewMenuManager::unlock()
{
  foreach (QAction* other, this->parentMenu()->actions())
  {
    if (other->isChecked())
    {
      other->setChecked(false);
    }
  }

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  Q_ASSERT(viewManager);
  viewManager->preview(QSize());
}
