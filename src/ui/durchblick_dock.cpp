#include "durchblick_dock.hpp"
#include "../config.hpp"
#include "durchblick.hpp"
#include <QShowEvent>
#include <QVBoxLayout>

void DurchblickDock::closeEvent(QCloseEvent* e)
{
    e->accept();
    db->OnClose();
    Config::Save();
    db->GetLayout()->DeleteLayout();
    db->DeleteDisplay();
    hide();
}

void DurchblickDock::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);

    // For multiview docks, the display and layout are already managed by MultiviewInstance
    // Only handle legacy single dock behavior
    if (m_is_multiview_dock)
        return;

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
    mainLayout->addWidget(db);

    auto* dockWidgetContents = new QWidget(this);
    dockWidgetContents->setLayout(mainLayout);
}

DurchblickDock::~DurchblickDock()
{
}
