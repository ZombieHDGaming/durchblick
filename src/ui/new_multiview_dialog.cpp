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

#include "new_multiview_dialog.hpp"
#include "../config.hpp"
#include "durchblick.hpp"
#include "durchblick_dock.hpp"
#include "../util/util.h"
#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>

void NewMultiviewDialog::OKClicked()
{
    QString name = m_name_edit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, T_DIALOG_NEW_MULTIVIEW, "Please enter a name for the multiview window.");
        return;
    }

    bool persistent = m_persistent_checkbox->isChecked();
    bool docked = m_docked_checkbox->isChecked();

    // Create the new multiview
    auto* mv = Config::CreateMultiview(name, persistent, docked);
    if (mv) {
        if (docked && mv->dock) {
            mv->dock->show();
        } else if (!docked && mv->window) {
            mv->window->show();
            mv->window->raise();
            mv->window->activateWindow();
        }
    }

    Config::Save();
    accept();
}

void NewMultiviewDialog::CancelClicked()
{
    reject();
}

NewMultiviewDialog::NewMultiviewDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(T_DIALOG_NEW_MULTIVIEW);

    auto* layout = new QVBoxLayout(this);

    // Name input
    layout->addWidget(new QLabel(T_LABEL_WINDOW_NAME, this));
    m_name_edit = new QLineEdit(this);
    m_name_edit->setPlaceholderText("e.g., Recording Monitor");
    layout->addWidget(m_name_edit);

    // Persistent checkbox
    m_persistent_checkbox = new QCheckBox(T_LABEL_PERSISTENT, this);
    m_persistent_checkbox->setChecked(true);
    m_persistent_checkbox->setToolTip("If checked, this window will be saved with the scene collection and restored on startup.");
    layout->addWidget(m_persistent_checkbox);

    // Docked checkbox
    m_docked_checkbox = new QCheckBox("Show as dock", this);
    m_docked_checkbox->setChecked(false);
    m_docked_checkbox->setToolTip("If checked, this multiview will be shown as a dockable panel instead of a standalone window.");
    layout->addWidget(m_docked_checkbox);

    // Button box
    m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(m_button_box);

    setLayout(layout);

    connect(m_button_box->button(QDialogButtonBox::Ok), &QPushButton::pressed, this, &NewMultiviewDialog::OKClicked);
    connect(m_button_box->button(QDialogButtonBox::Cancel), &QPushButton::pressed, this, &NewMultiviewDialog::CancelClicked);

    resize(400, 150);

    // Center dialog on primary screen
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2 + screenGeometry.x();
        int y = (screenGeometry.height() - height()) / 2 + screenGeometry.y();
        move(x, y);
    }
}
