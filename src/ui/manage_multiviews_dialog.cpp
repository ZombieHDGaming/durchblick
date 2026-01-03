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

#include "manage_multiviews_dialog.hpp"
#include "../config.hpp"
#include "durchblick.hpp"
#include "../util/util.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>

void ManageMultiviewsDialog::OnShowWindow()
{
    auto* item = m_multiview_list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto* mv = Config::GetMultiview(id);
    if (mv && mv->window) {
        mv->window->show();
        mv->window->raise();
        mv->window->activateWindow();
    }
}

void ManageMultiviewsDialog::OnRenameWindow()
{
    auto* item = m_multiview_list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto* mv = Config::GetMultiview(id);
    if (!mv)
        return;

    bool ok;
    QString newName = QInputDialog::getText(this, T_BUTTON_RENAME,
        "Enter new name:", QLineEdit::Normal,
        mv->name, &ok);

    if (ok && !newName.trimmed().isEmpty()) {
        mv->name = newName.trimmed();
        if (mv->window) {
            mv->window->setWindowTitle(newName.trimmed());
        }
        Config::UpdateToolsMenu();
        RefreshList();
    }
}

void ManageMultiviewsDialog::OnDeleteWindow()
{
    auto* item = m_multiview_list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto* mv = Config::GetMultiview(id);
    if (!mv)
        return;

    auto result = QMessageBox::question(this, T_BUTTON_DELETE,
        QString("Are you sure you want to delete '%1'?").arg(mv->name),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        Config::RemoveMultiview(id);
        RefreshList();
    }
}

void ManageMultiviewsDialog::OnDuplicateWindow()
{
    auto* item = m_multiview_list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto* mv = Config::GetMultiview(id);
    if (!mv)
        return;

    bool ok;
    QString newName = QInputDialog::getText(this, T_BUTTON_DUPLICATE,
        "Enter name for duplicate:", QLineEdit::Normal,
        mv->name + " (Copy)", &ok);

    if (ok && !newName.trimmed().isEmpty()) {
        // Create new multiview
        auto* newMv = Config::CreateMultiview(newName.trimmed(), mv->isPersistent);

        // Copy layout from source
        if (newMv && newMv->window && mv->window) {
            QJsonObject sourceLayout;
            mv->window->Save(sourceLayout);
            newMv->window->Load(sourceLayout);
        }

        RefreshList();
    }
}

void ManageMultiviewsDialog::OnSelectionChanged()
{
    bool hasSelection = m_multiview_list->currentItem() != nullptr;
    m_show_button->setEnabled(hasSelection);
    m_rename_button->setEnabled(hasSelection);
    m_delete_button->setEnabled(hasSelection);
    m_duplicate_button->setEnabled(hasSelection);
}

void ManageMultiviewsDialog::RefreshList()
{
    m_multiview_list->clear();

    auto ids = Config::GetMultiviewIds();
    for (const auto& id : ids) {
        auto* mv = Config::GetMultiview(id);
        if (!mv)
            continue;

        QString displayText = mv->name;
        if (!mv->isPersistent) {
            displayText += " (Temporary)";
        }

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, id);
        m_multiview_list->addItem(item);
    }

    OnSelectionChanged();
}

ManageMultiviewsDialog::ManageMultiviewsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(T_DIALOG_MANAGE_MULTIVIEWS);

    auto* mainLayout = new QVBoxLayout(this);

    // List widget
    m_multiview_list = new QListWidget(this);
    mainLayout->addWidget(m_multiview_list);

    // Button layout
    auto* buttonLayout = new QHBoxLayout();

    m_show_button = new QPushButton(T_BUTTON_SHOW, this);
    m_rename_button = new QPushButton(T_BUTTON_RENAME, this);
    m_delete_button = new QPushButton(T_BUTTON_DELETE, this);
    m_duplicate_button = new QPushButton(T_BUTTON_DUPLICATE, this);

    buttonLayout->addWidget(m_show_button);
    buttonLayout->addWidget(m_rename_button);
    buttonLayout->addWidget(m_delete_button);
    buttonLayout->addWidget(m_duplicate_button);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // Close button
    m_close_button = new QPushButton("Close", this);
    mainLayout->addWidget(m_close_button);

    setLayout(mainLayout);

    // Connect signals
    connect(m_show_button, &QPushButton::pressed, this, &ManageMultiviewsDialog::OnShowWindow);
    connect(m_rename_button, &QPushButton::pressed, this, &ManageMultiviewsDialog::OnRenameWindow);
    connect(m_delete_button, &QPushButton::pressed, this, &ManageMultiviewsDialog::OnDeleteWindow);
    connect(m_duplicate_button, &QPushButton::pressed, this, &ManageMultiviewsDialog::OnDuplicateWindow);
    connect(m_close_button, &QPushButton::pressed, this, &ManageMultiviewsDialog::accept);
    connect(m_multiview_list, &QListWidget::itemSelectionChanged, this, &ManageMultiviewsDialog::OnSelectionChanged);
    connect(m_multiview_list, &QListWidget::itemDoubleClicked, this, &ManageMultiviewsDialog::OnShowWindow);

    resize(500, 400);

    RefreshList();

    // Center dialog on primary screen
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2 + screenGeometry.x();
        int y = (screenGeometry.height() - height()) / 2 + screenGeometry.y();
        move(x, y);
    }
}
