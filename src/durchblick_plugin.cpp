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
#include "items/registry.hpp"
#include "ui/durchblick.hpp"
#include "util/util.h"
#include <QAction>
#include <QMenu>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <thread>
#include <util/util.hpp>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("durchblick", "en-US")

bool obs_module_load()
{
    binfo("Loading v%s-%s (%s) build time %s", PLUGIN_VERSION, GIT_BRANCH, GIT_COMMIT_HASH, BUILD_TIME);

    Registry::RegisterCustomWidgetProcedure();

    // Create Durchblick submenu
    Config::toolsMenu = new QMenu(T_MENU_DURCHBLICK);

    // "New Multiview Window..." action
    QAction* newWindowAction = Config::toolsMenu->addAction(T_MENU_NEW_WINDOW);
    QAction::connect(newWindowAction, &QAction::triggered, [] {
        Config::ShowNewMultiviewDialog();
    });

    // "Manage Windows..." action
    QAction* manageAction = Config::toolsMenu->addAction(T_MENU_MANAGE);
    QAction::connect(manageAction, &QAction::triggered, [] {
        Config::ShowManageMultiviewsDialog();
    });

    // Separator for dynamic multiview list
    Config::toolsMenu->addSeparator();

    // Add submenu to OBS Tools menu
    obs_frontend_add_tools_menu(Config::toolsMenu);

    return true;
}

void obs_module_post_load()
{

    // Speeds up loading
    std::thread reg([] { Registry::RegisterDefaults(); });

    reg.detach();
    Config::RegisterCallbacks();
    Config::Load();
}

void obs_module_unload()
{
    Registry::Free();
}
