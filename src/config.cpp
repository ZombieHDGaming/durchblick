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

#include "config.hpp"
#include "ui/durchblick.hpp"
#include "ui/durchblick_dock.hpp"
#include "ui/new_multiview_dialog.hpp"
#include "ui/manage_multiviews_dialog.hpp"
#include "util/util.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>

#if !defined(_WIN32) && !defined(__APPLE__)
#    include <obs-nix-platform.h>
#endif
namespace Config {

Durchblick* db = nullptr;
DurchblickDock* dbdock = nullptr;
QMap<QString, MultiviewInstance*> multiviews;
QMenu* toolsMenu = nullptr;
bool isLoading = false;

QJsonObject Cfg;

MultiviewInstance::MultiviewInstance(const QString& name, const QString& id, bool persistent)
    : name(name)
    , id(id)
    , window(nullptr)
    , isPersistent(persistent)
{
    window = new Durchblick();
}

MultiviewInstance::~MultiviewInstance()
{
    if (window) {
        delete window;
        window = nullptr;
    }
}

QJsonObject LoadLayoutsForCurrentSceneCollection()
{
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    BPtr<char> folder = obs_module_config_path("");

    if (os_mkdirs(folder) == MKDIR_ERROR) {
        berr("Failed to change directory from '%s'. Cannot save/load layouts.", folder.Get());
        return {};
    }

    QFile f(utf8_to_qt(path.Get()));

    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc;
            doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            Cfg = doc.object();
            auto scKey = utf8_to_qt(sc.Get());
            auto scData = Cfg[scKey];

            // Check if old format (array) or new format (object)
            if (scData.isArray()) {
                // Migrate old format to new format
                binfo("Migrating old layout format to new format for scene collection: %s", sc.Get());
                auto layouts = scData.toArray();
                QJsonObject newFormat;

                // Migrate dock layout (index 1 in old format)
                if (layouts.size() > 1)
                    newFormat["dock"] = layouts[1];

                // Migrate default window (index 0 in old format)
                QJsonObject multiviewsObj;
                if (layouts.size() > 0) {
                    QJsonObject defaultMV;
                    defaultMV["name"] = "Main Window";
                    defaultMV["persistent"] = true;
                    defaultMV["visible"] = false;
                    defaultMV["layout"] = layouts[0];
                    multiviewsObj["default"] = defaultMV;
                }
                newFormat["multiviews"] = multiviewsObj;

                Cfg[scKey] = newFormat;
                return newFormat;
            } else if (scData.isObject()) {
                return scData.toObject();
            } else {
                berr("No layouts found");
                return {};
            }
        }
    }
    return {};
}

static void save_callback(obs_data_t*, bool, void*)
{
    // Refresh this flag because if the user changed the "Hide OBS window from display capture setting"
    // durchblick would otherwise suddenly show up again
    if (db)
        db->SetHideFromDisplayCapture(db->GetHideFromDisplayCapture());
}

static void event_callback(enum obs_frontend_event event, void*)
{
    if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        Load();
    } else if (event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN) {
        // I couldn't find another event that was on exit and
        // before source/scene data was cleared
        Cleanup();
    } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) {
        // Clear all multiview layouts before scene collection changes
        for (auto it = multiviews.begin(); it != multiviews.end(); ++it) {
            if (it.value() && it.value()->window)
                it.value()->window->GetLayout()->Clear();
        }
        if (dbdock && dbdock->GetDurchblick())
            dbdock->GetDurchblick()->GetLayout()->Clear();
    } else if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED) {
        Load();
    }
}

void RegisterCallbacks()
{
    obs_frontend_add_save_callback(save_callback, nullptr);
    obs_frontend_add_event_callback(event_callback, nullptr);
}

void RemoveCallbacks()
{
    obs_frontend_remove_save_callback(save_callback, nullptr);
    obs_frontend_remove_event_callback(event_callback, nullptr);
}

void Load()
{
    blog(LOG_INFO, "[durchblick] Config::Load() called");
    isLoading = true;
    auto cfg = LoadLayoutsForCurrentSceneCollection();

    // Clear existing multiviews
    for (auto it = multiviews.begin(); it != multiviews.end(); ++it) {
        delete it.value();
    }
    multiviews.clear();

    // Reset db pointer as it may have pointed to a deleted multiview
    db = nullptr;

    // Load multiviews
    auto multiviewsObj = cfg["multiviews"].toObject();
    for (auto it = multiviewsObj.begin(); it != multiviewsObj.end(); ++it) {
        auto mvData = it.value().toObject();
        auto id = it.key();
        auto name = mvData["name"].toString();
        bool persistent = mvData["persistent"].toBool(true);
        bool visible = mvData["visible"].toBool(false);

        auto* mv = new MultiviewInstance(name, id, persistent);
        mv->window->setWindowTitle(name);
        // Force display creation even if window will be hidden
        // This ensures rendering callbacks are properly connected
        mv->window->CreateDisplay(true);
        mv->window->Load(mvData["layout"].toObject());
        mv->window->setVisible(visible);

        multiviews[id] = mv;

        // Keep backward compatibility: set db to the first/default multiview
        if (id == "default" && !db) {
            db = mv->window;
        }
    }

    // If no multiviews exist, create default one
    if (multiviews.isEmpty()) {
        if (!db)
            db = new Durchblick;
        db->setVisible(false);
        db->GetLayout()->CreateDefaultLayout();
    }

#if !defined(_WIN32) && !defined(__APPLE__)
    if (obs_get_nix_platform() > OBS_NIX_PLATFORM_X11_EGL)
#endif
    {
        if (!dbdock) {
            const auto main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
            obs_frontend_push_ui_translation(obs_module_get_string);
            dbdock = new DurchblickDock((QWidget*)main_window);
            obs_frontend_add_dock_by_id("durchblick", "Durchblick", Config::dbdock);
            obs_frontend_pop_ui_translation();
        }

        auto dockLayout = cfg["dock"].toObject();
        if (!dockLayout.isEmpty()) {
            dbdock->GetDurchblick()->Load(dockLayout);
        } else {
            dbdock->setVisible(false);
            dbdock->GetDurchblick()->GetLayout()->CreateDefaultLayout();
        }
    }
    isLoading = false;
    blog(LOG_INFO, "[durchblick] Config::Load() finished");
}

void Save()
{
    if (isLoading) {
        blog(LOG_WARNING, "[durchblick] Config::Save() called during load, ignoring to prevent overwriting loaded data");
        return;
    }
    blog(LOG_INFO, "[durchblick] Config::Save() called");
    QJsonObject sceneCollectionData {};
    BPtr<char> path = obs_module_config_path("layout.json");
    BPtr<char> sc = obs_frontend_get_current_scene_collection();
    QFile f(utf8_to_qt(path.Get()));

    // Save multiviews
    QJsonObject multiviewsObj {};
    for (auto it = multiviews.begin(); it != multiviews.end(); ++it) {
        auto* mv = it.value();
        if (!mv->isPersistent)
            continue; // Don't save non-persistent multiviews

        QJsonObject mvData {};
        mvData["name"] = mv->name;
        mvData["persistent"] = mv->isPersistent;
        mvData["visible"] = mv->window->isVisible();

        QJsonObject layout {};
        mv->window->Save(layout);
        mvData["layout"] = layout;

        multiviewsObj[mv->id] = mvData;
    }
    sceneCollectionData["multiviews"] = multiviewsObj;

    // Save dock layout
    if (dbdock) {
        QJsonObject dockLayout {};
        dbdock->GetDurchblick()->Save(dockLayout);
        sceneCollectionData["dock"] = dockLayout;
    }

    Cfg[utf8_to_qt(sc.Get())] = sceneCollectionData;

    if (f.open(QIODevice::WriteOnly)) {
        QJsonDocument doc;
        doc.setObject(Cfg);
        auto data = doc.toJson();
        auto wrote = f.write(data);

        if (data.length() != wrote) {
            berr("Couldn't write config file to %s, only"
                 "wrote %lli bytes out of %i",
                path.Get(), wrote, int(data.length()));
        }
        f.close();
    } else {
        berr("Couldn't write config to %s", path.Get());
    }
}

void Cleanup()
{
    // Clean up multiviews
    for (auto it = multiviews.begin(); it != multiviews.end(); ++it) {
        delete it.value();
    }
    multiviews.clear();

    // Note: db might point to a multiview window, so don't delete it here
    // It will be deleted when multiviews are cleaned up
    db = nullptr;

    if (dbdock) {
        delete dbdock;
        dbdock = nullptr;
    }
}

MultiviewInstance* CreateMultiview(const QString& name, bool persistent)
{
    // Generate unique ID
    QString id = name.toLower().replace(" ", "_");
    int counter = 1;
    QString baseId = id;
    while (multiviews.contains(id)) {
        id = baseId + "_" + QString::number(counter++);
    }

    auto* mv = new MultiviewInstance(name, id, persistent);
    mv->window->setWindowTitle(name);
    mv->window->CreateDisplay(true);
    mv->window->GetLayout()->CreateDefaultLayout();

    multiviews[id] = mv;

    return mv;
}

void RemoveMultiview(const QString& id)
{
    if (multiviews.contains(id)) {
        auto* mv = multiviews[id];

        // Don't delete if this is the legacy db pointer
        if (mv->window == db) {
            db = nullptr;
        }

        multiviews.remove(id);
        delete mv;
    }
}

MultiviewInstance* GetMultiview(const QString& id)
{
    return multiviews.value(id, nullptr);
}

QList<QString> GetMultiviewIds()
{
    return multiviews.keys();
}

void UpdateToolsMenu()
{
    if (!toolsMenu)
        return;

    // Clear all existing menu items
    toolsMenu->clear();

    // "New Multiview Window..." action
    QAction* newWindowAction = toolsMenu->addAction(T_MENU_NEW_WINDOW);
    QObject::connect(newWindowAction, &QAction::triggered, [] {
        ShowNewMultiviewDialog();
    });

    // "Manage Windows..." action
    QAction* manageAction = toolsMenu->addAction(T_MENU_MANAGE);
    QObject::connect(manageAction, &QAction::triggered, [] {
        ShowManageMultiviewsDialog();
    });

    // Separator for dynamic multiview list
    toolsMenu->addSeparator();

    // Add actions for each multiview
    for (auto it = multiviews.begin(); it != multiviews.end(); ++it) {
        auto* mv = it.value();
        QAction* action = toolsMenu->addAction(mv->name);
        QString capturedId = mv->id; // Capture by value for lambda
        QObject::connect(action, &QAction::triggered, [capturedId] {
            auto* mv = GetMultiview(capturedId);
            if (mv && mv->window) {
                mv->window->show();
                mv->window->raise();
                mv->window->activateWindow();
            }
        });
    }
}

void ShowNewMultiviewDialog()
{
    auto* dialog = new NewMultiviewDialog(static_cast<QWidget*>(obs_frontend_get_main_window()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void ShowManageMultiviewsDialog()
{
    auto* dialog = new ManageMultiviewsDialog(static_cast<QWidget*>(obs_frontend_get_main_window()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

}
