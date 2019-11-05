#ifndef TIMEULARSETUPDIALOG_H
#define TIMEULARSETUPDIALOG_H

#include <QDialog>
#include <QScopedPointer>

#include "TimeularAdaptor.h"

class QLabel;
namespace Ui {
class TimeularSetupDialog;
}

class TimeularSetupDialog : public QDialog
{
public:
    TimeularSetupDialog(QVector<TimeularAdaptor::FaceMapping> mappings, TimeularManager *manager,
                        QWidget *parent = nullptr);
    ~TimeularSetupDialog();

    QVector<TimeularAdaptor::FaceMapping> mappings();

private:
    void selectTask();
    void clearFace();
    void faceChanged(TimeularManager::Orientation orientation);
    void discoveredDevicesChanged(QStringList dl);
    void deviceSelectionChanged();
    void setPairedDevice();
    void connectToDevice();

    QScopedPointer<Ui::TimeularSetupDialog> m_ui;
    QVector<TimeularAdaptor::FaceMapping> m_mappings;
    TimeularManager *m_manager;
    QVector<QLabel *> m_labels;
};

#endif // TIMEULARSETUPDIALOG_H
