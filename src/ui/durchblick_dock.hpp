#pragma once
#include <QDockWidget>
#include <QTextEdit>
#include <obs-frontend-api.h>

class Durchblick;

class DurchblickDock : public QWidget {
    Q_OBJECT

private:
    Durchblick* db;
    bool m_is_multiview_dock;  // True if this dock is managed by a multiview

protected:
    void closeEvent(QCloseEvent*) override;
    void showEvent(QShowEvent*) override;

public:
    DurchblickDock(QWidget* parent = nullptr, bool isMultiviewDock = false);
    ~DurchblickDock();

    Durchblick* GetDurchblick() { return db; }
};