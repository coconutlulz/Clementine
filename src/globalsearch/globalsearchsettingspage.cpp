/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "globalsearch.h"
#include "globalsearchsettingspage.h"
#include "core/logging.h"
#include "ui/iconloader.h"
#include "ui/settingsdialog.h"
#include "ui_globalsearchsettingspage.h"

#include <QSettings>

GlobalSearchSettingsPage::GlobalSearchSettingsPage(SettingsDialog* dialog)
  : SettingsPage(dialog),
    ui_(new Ui::GlobalSearchSettingsPage)
{
  ui_->setupUi(this);
  setWindowIcon(IconLoader::Load("edit-find"));

  ui_->sources->header()->setResizeMode(0, QHeaderView::Stretch);
  ui_->sources->header()->setResizeMode(1, QHeaderView::ResizeToContents);

  warning_icon_ = IconLoader::Load("dialog-warning");

  connect(ui_->up, SIGNAL(clicked()), SLOT(MoveUp()));
  connect(ui_->down, SIGNAL(clicked()), SLOT(MoveDown()));
  connect(ui_->configure, SIGNAL(clicked()), SLOT(ConfigureProvider()));
  connect(ui_->sources, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          SLOT(CurrentProviderChanged(QTreeWidgetItem*)));
}

GlobalSearchSettingsPage::~GlobalSearchSettingsPage() {
}

static bool CompareProviderId(SearchProvider* left, SearchProvider* right) {
  return left->id() < right->id();
}

void GlobalSearchSettingsPage::Load() {
  QSettings s;
  s.beginGroup(GlobalSearch::kSettingsGroup);

  GlobalSearch* engine = dialog()->global_search();
  QList<SearchProvider*> providers = engine->providers();

  // Sort the list of providers alphabetically (by id) initially, so any that
  // aren't in the provider_order list will take this order.
  qSort(providers.begin(), providers.end(), CompareProviderId);

  // Add the ones in the configured list first
  ui_->sources->clear();
  foreach (const QString& id, s.value("provider_order", QStringList() << "library").toStringList()) {
    // Find a matching provider for this id
    for (QList<SearchProvider*>::iterator it = providers.begin();
         it != providers.end() ; ++it) {
      if ((*it)->id() == id) {
        AddProviderItem(engine, *it);
        providers.erase(it);
        break;
      }
    }
  }

  // Now add any others that are remaining
  foreach (SearchProvider* provider, providers) {
    AddProviderItem(engine, provider);
  }

  ui_->show_globalsearch->setChecked(s.value("show_globalsearch", true).toBool());
  ui_->hide_others->setChecked(s.value("hide_others", false).toBool());
  ui_->combine->setChecked(s.value("combine_identical_results", true).toBool());
  ui_->tooltip->setChecked(s.value("tooltip", true).toBool());
  ui_->tooltip_help->setChecked(s.value("tooltip_help", true).toBool());
}

void GlobalSearchSettingsPage::AddProviderItem(GlobalSearch* engine,
                                               SearchProvider* provider) {
  QTreeWidgetItem* item = new QTreeWidgetItem;
  item->setText(0, provider->name());
  item->setIcon(0, provider->icon());

  const bool enabled = engine->is_provider_enabled(provider);
  const bool logged_in = provider->IsLoggedIn();

  if (!logged_in) {
    item->setData(0, Qt::CheckStateRole, Qt::Unchecked);
    item->setIcon(1, warning_icon_);
    item->setText(1, tr("Not logged in"));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  } else if (enabled) {
    item->setData(0, Qt::CheckStateRole, Qt::Checked);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
  } else {
    item->setData(0, Qt::CheckStateRole, Qt::Unchecked);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
  }

  item->setData(0, Qt::UserRole, QVariant::fromValue(provider));

  ui_->sources->invisibleRootItem()->addChild(item);
}

void GlobalSearchSettingsPage::Save() {

}

void GlobalSearchSettingsPage::MoveUp() {
  MoveCurrentItem(-1);
}

void GlobalSearchSettingsPage::MoveDown() {
  MoveCurrentItem(+1);
}

void GlobalSearchSettingsPage::MoveCurrentItem(int d) {
  QTreeWidgetItem* item = ui_->sources->currentItem();
  if (!item)
    return;

  QTreeWidgetItem* root = ui_->sources->invisibleRootItem();

  const int row = root->indexOfChild(item);
  const int new_row = qBound(0, row + d, root->childCount());

  if (row == new_row)
    return;

  root->removeChild(item);
  root->insertChild(new_row, item);

  ui_->sources->setCurrentItem(item);
}

void GlobalSearchSettingsPage::ConfigureProvider() {
  QTreeWidgetItem* item = ui_->sources->currentItem();
  if (!item)
    return;

  SearchProvider* provider = item->data(0, Qt::UserRole).value<SearchProvider*>();
  provider->ShowConfig();
}

void GlobalSearchSettingsPage::CurrentProviderChanged(QTreeWidgetItem* item) {
  if (!item)
    return;

  QTreeWidgetItem* root = ui_->sources->invisibleRootItem();
  SearchProvider* provider = item->data(0, Qt::UserRole).value<SearchProvider*>();
  const int row = root->indexOfChild(item);

  ui_->up->setEnabled(row != 0);
  ui_->down->setEnabled(row != root->childCount() - 1);
  ui_->configure->setEnabled(provider->CanShowConfig());
}