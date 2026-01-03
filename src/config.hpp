/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QMenu>

class Durchblick;
class DurchblickDock;

namespace Config {

struct MultiviewInstance {
    QString name;
    QString id;
    Durchblick* window;
    bool isPersistent;

    MultiviewInstance(const QString& name, const QString& id, bool persistent = true);
    ~MultiviewInstance();
};

extern QJsonObject LoadLayoutsForCurrentSceneCollection();

extern Durchblick* db; // Legacy default window - kept for backward compatibility
extern DurchblickDock* dbdock;
extern QMap<QString, MultiviewInstance*> multiviews;
extern QMenu* toolsMenu;

extern void RegisterCallbacks();
extern void RemoveCallbacks();

extern void Load();

extern void Save();

extern void Cleanup();

// Multiview management functions
extern MultiviewInstance* CreateMultiview(const QString& name, bool persistent = true);
extern void RemoveMultiview(const QString& id);
extern MultiviewInstance* GetMultiview(const QString& id);
extern QList<QString> GetMultiviewIds();
extern void UpdateToolsMenu();
extern void ShowNewMultiviewDialog();
extern void ShowManageMultiviewsDialog();

}
