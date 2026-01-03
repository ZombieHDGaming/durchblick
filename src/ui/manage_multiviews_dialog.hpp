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

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class ManageMultiviewsDialog : public QDialog {
    Q_OBJECT

    QListWidget* m_multiview_list;
    QPushButton* m_show_button;
    QPushButton* m_rename_button;
    QPushButton* m_delete_button;
    QPushButton* m_duplicate_button;
    QPushButton* m_close_button;

private slots:
    void OnShowWindow();
    void OnRenameWindow();
    void OnDeleteWindow();
    void OnDuplicateWindow();
    void OnSelectionChanged();
    void RefreshList();

public:
    ManageMultiviewsDialog(QWidget* parent = nullptr);
};
