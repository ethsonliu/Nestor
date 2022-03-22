#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include "divert_worker.h"

#include <QString>
#include <QMainWindow>

class QGroupBox;
class QLineEdit;
class QPushButton;
class QComboBox;
class CheckBoxPlus;
class QLabel;
class LineEditPlus;
class VariableBundle;
class QLabel;
class QCloseEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    struct Bundle
    {
        QString name;

        CheckBoxPlus *lagCheckBox;
        LineEditPlus *lagLineEdit;

        CheckBoxPlus *dropCheckBox;
        LineEditPlus *dropLineEdit;

        CheckBoxPlus *throttleCheckBox;
        LineEditPlus *throttleTimeFrameLineEdit;
        LineEditPlus *throttleLineEdit;

        CheckBoxPlus *outoforderCheckBox;
        LineEditPlus *outoforderLineEdit;

        CheckBoxPlus *duplicateCheckBox;
        LineEditPlus *duplicateCountLineEdit;
        LineEditPlus *duplicateLineEdit;

        CheckBoxPlus *tamperCheckBox;
        CheckBoxPlus *tamperChecksumCheckBox;
        LineEditPlus *tamperLineEdit;

        Bundle(VariableBundle *syncedVariableBundle);
    } *m_inbound, *m_outbound;

private:
    void initFilterWidget();
    void initFunctionsWidget();
    QWidget *createWidgetFromBundle(Bundle *bundle);
    QWidget *createCentralWidget();

    void setLagEnabled();
    void setDropEnabled();
    void setThrottleEnabled();
    void setOutoforderEnabled();
    void setDuplicateEnabled();
    void setTamperEnabled();

    void startBtnClicked(bool clicked);

private slots:
    void presetsComboBoxIndexChanged(int index);
    void resultFeedbackSlot(ResultCode code, const QString &info);

signals:
    void startStateChanged(DivertState state, const QString &filter);

private:
    QGroupBox *m_filterGroupBox;
    QGroupBox *m_functionsGroupBox;
    QLineEdit *m_filterLineEdit;
    QPushButton *m_startPushButton;
    QComboBox *m_presetsComboBox;
    bool m_isRunning;
    DivertWorker *m_worker;
    QLabel *m_statusLabel;
};
#endif // MAINWINDOW_H
