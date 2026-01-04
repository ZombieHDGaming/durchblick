#include "durchblick_dock.hpp"
#include "../config.hpp"
#include "durchblick.hpp"
#include <QShowEvent>
#include <QVBoxLayout>

void DurchblickDock::closeEvent(QCloseEvent* e)
{
    e->accept();

    // For multiview docks, lifecycle is managed by MultiviewInstance
    // Don't perform cleanup operations during close
    if (m_is_multiview_dock) {
        hide();
        return;
    }

    // Legacy dock behavior
    db->OnClose();
    Config::Save();
    db->GetLayout()->DeleteLayout();
    db->DeleteDisplay();
    hide();
}

void DurchblickDock::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);

    // For multiview docks, the display and layout are managed by MultiviewInstance
    // Just ensure the embedded widget is visible
    if (m_is_multiview_dock) {
        if (db && !db->isVisible()) {
            db->show();
        }
        return;
    }

    // Legacy single dock behavior
    if (!e->spontaneous()) {
        auto cfg = Config::LoadLayoutsForCurrentSceneCollection();
        db->GetLayout()->DeleteLayout();
        db->CreateDisplay(true);

        auto dockLayout = cfg["dock"].toObject();
        if (!dockLayout.isEmpty()) {
            db->Load(dockLayout);
        } else {
            db->GetLayout()->CreateDefaultLayout();
        }
        // Forces a grid refresh
        // just refreshing the grid doesn't seem to work
        resize(size().grownBy({ 1, 0, 0, 0 }));
    }
}

DurchblickDock::DurchblickDock(QWidget* parent, bool isMultiviewDock)
    : QWidget(parent)
    , db(new Durchblick(this, Qt::Widget))
    , m_is_multiview_dock(isMultiviewDock)
{
    setWindowTitle("Durchblick");
    setObjectName("DurchblickDock");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(db);
    setLayout(mainLayout);

    // For embedded widgets, explicitly show to ensure proper initialization
    // Similar to obs-source-dock approach
    db->show();
}

DurchblickDock::~DurchblickDock()
{
}
