#include "main_window.h"
#include "widget/line_edit_plus.h"
#include "widget/check_box_plus.h"

#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QCloseEvent>

#define PRESETS_ROW 12
static QString presets[PRESETS_ROW][2] =
{
    "localhost ipv4 all", "outbound and loopback",
    "localhost ipv4 tcp", "tcp and outbound and loopback",
    "localhost ipv4 udp", "udp and outbound and loopback",
    "all sending packets", "outbound",
    "all receiving packets", "inbound",
    "all ipv4 against specific ip", "ip.DstAddr == 198.51.100.1 or ip.SrcAddr == 198.51.100.1",
    "tcp ipv4 against specific ip", "tcp and (ip.DstAddr == 198.51.100.1 or ip.SrcAddr == 198.51.100.1)",
    "udp ipv4 against specific ip", "udp and (ip.DstAddr == 198.51.100.1 or ip.SrcAddr == 198.51.100.1)",
    "all ipv4 against port", "ip.DstPort == 12354 or ip.SrcPort == 12354",
    "tcp ipv4 against port", "tcp and (tcp.DstPort == 12354 or tcp.SrcPort == 12354)",
    "udp ipv4 against port", "udp and (udp.DstPort == 12354 or udp.SrcPort == 12354)",
    "ipv6 all", "ipv6"
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_isRunning = false;

    m_worker = new DivertWorker;
    connect(this, &MainWindow::startStateChanged, m_worker, &DivertWorker::startStateChangedSlot);
    connect(m_worker, &DivertWorker::resultFeedback, this, &MainWindow::resultFeedbackSlot);

    setMinimumWidth(CENTRAL_WIDGET_WIDTH);

    m_inbound = new Bundle(&DivertWorker::inboundVariables);
    m_inbound->name = tr("Inbound");
    m_outbound = new Bundle(&DivertWorker::outboundVariables);
    m_outbound->name = tr("Outbound");

    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignLeft);
    statusBar()->addWidget(m_statusLabel);

    setCentralWidget(createCentralWidget());

    setMenuBar(nullptr);
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_isRunning)
    {
        m_isRunning = !m_isRunning;
        emit startStateChanged(DivertState::end, m_filterLineEdit->text());
        m_worker->deleteLater();
    }

    e->accept();
}

QWidget *MainWindow::createCentralWidget()
{
    initFilterWidget();
    initFunctionsWidget();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_filterGroupBox);
    mainLayout->addWidget(m_functionsGroupBox);

    QWidget *w = new QWidget;
    w->setLayout(mainLayout);

    return w;
}

void MainWindow::initFilterWidget()
{
    m_filterGroupBox = new QGroupBox;
    m_filterLineEdit = new QLineEdit;
    m_filterGroupBox->setTitle(tr("Filter"));
    m_startPushButton = new QPushButton(tr("Start"));
    connect(m_startPushButton, &QPushButton::clicked, this, &MainWindow::startBtnClicked);
    m_presetsComboBox = new QComboBox;
    m_presetsComboBox->setFixedWidth(PRESETS_COMBOBOX_WIDTH);
    connect(m_presetsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::presetsComboBoxIndexChanged);
    for (int i = 0; i < PRESETS_ROW; ++i)
        m_presetsComboBox->addItem(presets[i][0]);
    QLabel *lbl1 = new QLabel(tr("Presets:"));

    QHBoxLayout *hLayout1 = new QHBoxLayout;
    hLayout1->addWidget(m_startPushButton);
    hLayout1->addStretch();
    hLayout1->addWidget(lbl1);
    hLayout1->addWidget(m_presetsComboBox);

    QVBoxLayout *filterLayout = new QVBoxLayout;
    filterLayout->addWidget(m_filterLineEdit);
    filterLayout->addLayout(hLayout1);
    m_filterGroupBox->setLayout(filterLayout);
}

void MainWindow::initFunctionsWidget()
{
    m_functionsGroupBox = new QGroupBox;
    m_functionsGroupBox->setTitle(tr("Functions"));
    QVBoxLayout *functionsLayout = new QVBoxLayout;
    functionsLayout->addWidget(createWidgetFromBundle(m_inbound));
    functionsLayout->addWidget(createWidgetFromBundle(m_outbound));
    m_functionsGroupBox->setLayout(functionsLayout);
}

QWidget *MainWindow::createWidgetFromBundle(Bundle *bundle)
{
    // lag function
    QHBoxLayout *lagLayout = new QHBoxLayout;
    lagLayout->addWidget(bundle->lagCheckBox);
    lagLayout->addStretch();
    lagLayout->addWidget(bundle->lagLineEdit);
    connect(bundle->lagCheckBox, &QCheckBox::clicked, this, &MainWindow::setLagEnabled);

    // drop dunction
    QHBoxLayout *dropLayout = new QHBoxLayout;
    dropLayout->addWidget(bundle->dropCheckBox);
    dropLayout->addStretch();
    dropLayout->addWidget(bundle->dropLineEdit);
    connect(bundle->dropCheckBox, &QCheckBox::clicked, this, &MainWindow::setDropEnabled);

    // throttle function
    QHBoxLayout *hLayout1 = new QHBoxLayout;
    hLayout1->addWidget(bundle->throttleTimeFrameLineEdit);
    hLayout1->addWidget(bundle->throttleLineEdit);
    hLayout1->setSpacing(SPACING_OF_LINEEDITS);
    QHBoxLayout *throttleLayout = new QHBoxLayout;
    throttleLayout->addWidget(bundle->throttleCheckBox);
    throttleLayout->addStretch();
    throttleLayout->addLayout(hLayout1);
    connect(bundle->throttleCheckBox, &QCheckBox::clicked, this, &MainWindow::setThrottleEnabled);

    // out of order function
    QHBoxLayout *outoforderLayout = new QHBoxLayout;
    outoforderLayout->addWidget(bundle->outoforderCheckBox);
    outoforderLayout->addStretch();
    outoforderLayout->addWidget(bundle->outoforderLineEdit);
    connect(bundle->outoforderCheckBox, &QCheckBox::clicked, this, &MainWindow::setOutoforderEnabled);

    // duplicate function
    QHBoxLayout *hLayout2 = new QHBoxLayout;
    hLayout2->addWidget(bundle->duplicateCountLineEdit);
    hLayout2->addWidget(bundle->duplicateLineEdit);
    hLayout2->setSpacing(SPACING_OF_LINEEDITS);
    QHBoxLayout *duplicateLayout = new QHBoxLayout;
    duplicateLayout->addWidget(bundle->duplicateCheckBox);
    duplicateLayout->addStretch();
    duplicateLayout->addLayout(hLayout2);
    connect(bundle->duplicateCheckBox, &QCheckBox::clicked, this, &MainWindow::setDuplicateEnabled);

    // tamper function
    QHBoxLayout *hLayout3 = new QHBoxLayout;
    hLayout3->addStretch();
    hLayout3->addWidget(bundle->tamperChecksumCheckBox);
    hLayout3->addWidget(bundle->tamperLineEdit);
    hLayout3->setSpacing(SPACING_OF_LINEEDITS);
    QHBoxLayout *tamperLayout = new QHBoxLayout;
    tamperLayout->addWidget(bundle->tamperCheckBox);
    tamperLayout->addStretch();
    tamperLayout->addLayout(hLayout3);
    connect(bundle->tamperCheckBox, &QCheckBox::clicked, this, &MainWindow::setTamperEnabled);

    // groupbox
    QGroupBox *groupBox = new QGroupBox(bundle->name);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(lagLayout);
    vLayout->addLayout(dropLayout);
    vLayout->addLayout(throttleLayout);
    vLayout->addLayout(outoforderLayout);
    vLayout->addLayout(duplicateLayout);
    vLayout->addLayout(tamperLayout);
    groupBox->setLayout(vLayout);

    return groupBox;
}

void MainWindow::setLagEnabled()
{
    bool inChecked = m_inbound->lagCheckBox->isChecked();
    if (!(inChecked && m_inbound->lagLineEdit->isEnabled()))
    {
        m_inbound->lagLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->lagCheckBox->isChecked();
    if (!(outChecked && m_outbound->lagLineEdit->isEnabled()))
    {
        m_outbound->lagLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::setDropEnabled()
{
    bool inChecked = m_inbound->dropCheckBox->isChecked();
    if (!(inChecked && m_inbound->dropLineEdit->isEnabled()))
    {
        m_inbound->dropLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->dropCheckBox->isChecked();
    if (!(outChecked && m_outbound->dropLineEdit->isEnabled()))
    {
        m_outbound->dropLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::setThrottleEnabled()
{
    bool inChecked = m_inbound->throttleCheckBox->isChecked();
    if (!(inChecked && m_inbound->throttleLineEdit->isEnabled()))
    {
        m_inbound->throttleTimeFrameLineEdit->setEnabled(inChecked);
        m_inbound->throttleLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->throttleCheckBox->isChecked();
    if (!(outChecked && m_outbound->throttleLineEdit->isEnabled()))
    {
        m_outbound->throttleTimeFrameLineEdit->setEnabled(outChecked);
        m_outbound->throttleLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::setOutoforderEnabled()
{
    bool inChecked = m_inbound->outoforderCheckBox->isChecked();
    if (!(inChecked && m_inbound->outoforderLineEdit->isEnabled()))
    {
        m_inbound->outoforderLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->outoforderCheckBox->isChecked();
    if (!(outChecked && m_outbound->outoforderLineEdit->isEnabled()))
    {
        m_outbound->outoforderLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::setDuplicateEnabled()
{
    bool inChecked = m_inbound->duplicateCheckBox->isChecked();
    if (!(inChecked && m_inbound->duplicateLineEdit->isEnabled()))
    {
        m_inbound->duplicateCountLineEdit->setEnabled(inChecked);
        m_inbound->duplicateLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->duplicateCheckBox->isChecked();
    if (!(outChecked && m_outbound->duplicateLineEdit->isEnabled()))
    {
        m_outbound->duplicateCountLineEdit->setEnabled(outChecked);
        m_outbound->duplicateLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::setTamperEnabled()
{
    bool inChecked = m_inbound->tamperCheckBox->isChecked();
    if (!(inChecked && m_inbound->tamperLineEdit->isEnabled()))
    {
        m_inbound->tamperChecksumCheckBox->setEnabled(inChecked);
        m_inbound->tamperLineEdit->setEnabled(inChecked);
    }

    bool outChecked = m_outbound->tamperCheckBox->isChecked();
    if (!(outChecked && m_outbound->tamperLineEdit->isEnabled()))
    {
        m_outbound->tamperChecksumCheckBox->setEnabled(outChecked);
        m_outbound->tamperLineEdit->setEnabled(outChecked);
    }
}

void MainWindow::startBtnClicked(bool clicked)
{
    Q_UNUSED(clicked);

    DivertState state = DivertState::end;
    if (m_isRunning)
    {
        m_startPushButton->setText(tr("Start"));
        m_filterLineEdit->setEnabled(true);
        m_presetsComboBox->setEnabled(true);
    }
    else
    {
        state = DivertState::start;
        m_startPushButton->setText(tr("End"));
        m_filterLineEdit->setEnabled(false);
        m_presetsComboBox->setEnabled(false);
    }

    m_isRunning = !m_isRunning;

    emit startStateChanged(state, m_filterLineEdit->text());
}

void MainWindow::presetsComboBoxIndexChanged(int index)
{
    if (index < PRESETS_ROW)
    {
        m_filterLineEdit->setText(presets[index][1]);
    }
}

void MainWindow::resultFeedbackSlot(ResultCode code, const QString &info)
{
    if (code == ResultCode::error)
    {
        m_startPushButton->setText(tr("Start"));
        m_filterLineEdit->setEnabled(true);
        m_presetsComboBox->setEnabled(true);
    }

    m_statusLabel->setText(info);
}

MainWindow::Bundle::Bundle(VariableBundle *syncedVariableBundle)
{
    assert(syncedVariableBundle);

    lagCheckBox = new CheckBoxPlus(tr("Lag"), &syncedVariableBundle->lagEnabled);
    lagLineEdit = new LineEditPlus(tr("Delay(ms):"), "50", &syncedVariableBundle->lagTime);
    lagLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    lagLineEdit->setEnabled(false);

    dropCheckBox = new CheckBoxPlus(tr("Drop"), &syncedVariableBundle->dropEnabled);
    dropLineEdit = new LineEditPlus(tr("Chance(%):"), "10", &syncedVariableBundle->dropChance);
    dropLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    dropLineEdit->setEnabled(false);

    throttleCheckBox = new CheckBoxPlus(tr("Throttle"), &syncedVariableBundle->throttleEnabled);
    throttleTimeFrameLineEdit = new LineEditPlus(tr("Timeframe(ms):"), "30", &syncedVariableBundle->throttleTimeFrame);
    throttleTimeFrameLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    throttleTimeFrameLineEdit->setEnabled(false);
    throttleTimeFrameLineEdit->setChangeable(false);
    throttleLineEdit = new LineEditPlus(tr("Chance(%):"), "10", &syncedVariableBundle->throttleChance);
    throttleLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    throttleLineEdit->setEnabled(false);

    outoforderCheckBox = new CheckBoxPlus(tr("Out of order"), &syncedVariableBundle->outoforderEnabled);
    outoforderLineEdit = new LineEditPlus(tr("Chance(%):"), "10", &syncedVariableBundle->outoforderChance);
    outoforderLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    outoforderLineEdit->setEnabled(false);

    duplicateCheckBox = new CheckBoxPlus(tr("Duplicate"), &syncedVariableBundle->duplicateEnabled);
    duplicateCountLineEdit = new LineEditPlus(tr("Count:"), "2", &syncedVariableBundle->duplicateCount);
    duplicateCountLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    duplicateCountLineEdit->setEnabled(false);
    duplicateLineEdit = new LineEditPlus(tr("Chance(%):"), "10", &syncedVariableBundle->duplicateChance);
    duplicateLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    duplicateLineEdit->setEnabled(false);

    tamperCheckBox = new CheckBoxPlus(tr("Tamper"), &syncedVariableBundle->tamperEnabled);
    tamperChecksumCheckBox = new CheckBoxPlus(tr("Redo Checksum"), &syncedVariableBundle->tamperRedoChecksum);
    tamperChecksumCheckBox->setEnabled(false);
    tamperLineEdit = new LineEditPlus(tr("Chance(%):"), "10", &syncedVariableBundle->tamperChance);
    tamperLineEdit->setInputRegular("^([0-9]*)(±)?([0-9]*)?$");
    tamperLineEdit->setEnabled(false);
}
