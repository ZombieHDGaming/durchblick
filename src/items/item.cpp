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

#include "item.hpp"
#include "../config.hpp"

LayoutItem::LayoutItem(Layout* parent, int x, int y, int w, int h)
    : QObject((QObject*)parent)
    , m_layout(parent)
{
    m_cell = { x, y, w, h };
    m_toggle_stretch = new QAction(T_WIDGET_STRETCH, this);
    m_toggle_stretch->setCheckable(true);

    // Connect toggle action to trigger save when changed
    connect(m_toggle_stretch, &QAction::toggled, [] { Config::Save(); });
}

LayoutItem::~LayoutItem()
{
}
