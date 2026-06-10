#pragma once

#include <QMainWindow>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QDate>
#include <QDoubleSpinBox>
#include <QChartView>
#include <QCheckBox>
#include <QDateEdit>

class AppCore;

class QLabel;
class QPushButton;
class QToolButton;
class QStackedWidget;
class QCalendarWidget;
class QDateTimeEdit;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QSpinBox;
class QWidget;
class QVBoxLayout;
class QGridLayout;
class QScrollArea;
class QFrame;
class QLayout;
class QEvent;
class QResizeEvent;
class QGraphicsBlurEffect;
class QListWidget;
class QListWidgetItem;
class QTextBrowser;
class QPlainTextEdit;
class QTimer;
class QDateEdit;
class QSystemTrayIcon;
class ExamAttachmentDropArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AppCore* core, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    AppCore* m_core = nullptr;

    bool m_sidebarCollapsed = false;

    // в private секцию, в начало (после других переменных)
    struct FileAttachmentInfo {
        QString id;
        QString storagePath;
        QString fileName;
    };
    QList<FileAttachmentInfo> m_editExamAttachments;  // для хранения существующих файлов при редактировании

    QWidget* m_central = nullptr;
    QFrame* m_sidebar = nullptr;
    QWidget* m_content = nullptr;
    QLabel* m_title = nullptr;
    QStackedWidget* m_stack = nullptr;

    QWidget* m_brandRow = nullptr;
    QLabel* m_brandTrackly = nullptr;
    QLabel* m_brandDesktop = nullptr;

    QToolButton* m_sidebarToggle = nullptr;
    QToolButton* m_helpButton = nullptr;

    QPushButton* m_navHome = nullptr;
    QPushButton* m_navSeizures = nullptr;
    QPushButton* m_navMeds = nullptr;
    QPushButton* m_navExams = nullptr;
    QPushButton* m_navProfile = nullptr;
    QPushButton* m_navAnalytics = nullptr;
    QPushButton* m_navTriggers = nullptr;
    QPushButton* m_themeBtn = nullptr;
    QPushButton* m_languageBtn = nullptr;

    QStringList m_pendingExamFiles;
    QJsonArray m_cachedAttachments;
    ExamAttachmentDropArea* m_examDropArea = nullptr;

    QTabWidget*  m_medsTab = nullptr;
    QWidget*     m_todayIntakesHost = nullptr;
    QVBoxLayout* m_todayIntakesLayout = nullptr;
    QDateEdit*   m_intakesDateEdit = nullptr;

    QJsonArray   m_cachedMedicationItems;

    QLabel* m_summary1 = nullptr;
    QLabel* m_summary2 = nullptr;
    QLabel* m_summary3 = nullptr;
    QLabel* m_summary4 = nullptr;
    QPushButton* m_dashboardReportBtn = nullptr;

    QCalendarWidget* m_dashboardCalendar = nullptr;
    QWidget* m_dashboardExamHost = nullptr;
    QVBoxLayout* m_dashboardExamLayout = nullptr;
    QMap<QDate, QStringList> m_dashboardEvents;
    QSet<QDate> m_datesWithSeizures;
    QSet<QDate> m_datesWithMedication;
    QSet<QDate> m_datesWithExams;

    QDateTimeEdit* m_seizureDate = nullptr;
    QComboBox* m_seizurePlace = nullptr;
    QComboBox* m_seizureType = nullptr;
    QLineEdit* m_seizureTriggers = nullptr;
    QLineEdit* m_seizurePost = nullptr;
    QSpinBox* m_seizureSeverity = nullptr;
    QTextEdit* m_seizureNotes = nullptr;
    QPushButton* m_saveSeizureBtn = nullptr;
    QString m_editSeizureId;
    QJsonArray m_cachedSeizureItems;

    QCalendarWidget* m_medCalendar = nullptr;
    QWidget* m_dayMedicationHost = nullptr;
    QVBoxLayout* m_dayMedicationLayout = nullptr;
    QLineEdit* m_medTitle = nullptr;
    QComboBox* m_medTherapy = nullptr;
    QLineEdit* m_medDose = nullptr;
    QSpinBox* m_medTimesPerDay = nullptr;
    QLineEdit* m_medTimes = nullptr;
    QSpinBox* m_medStock = nullptr;
    QWidget* m_medCardsHost = nullptr;
    QGridLayout* m_medCardsLayout = nullptr;
    QPushButton* m_saveMedicationBtn = nullptr;
    QString m_editMedicationId;

    // новые
    QMap<QString, QString> m_medicationNamesById;   // medication_id -> title
    QMap<QString, QString> m_scheduleTitlesById;   // schedule_id -> human title
    QMenu* m_dayIntakeContextMenu = nullptr;
    QAction* m_cancelIntakeAction = nullptr;
    QString m_contextMenuLogId;
    QLineEdit* m_medInn = nullptr;          // МНН
    QDoubleSpinBox* m_medDoseTablets = nullptr; // разовая доза в таблетках
    QDoubleSpinBox* m_medStockTablets = nullptr; // остаток в таблетках
    QSystemTrayIcon* m_trayIcon = nullptr;

    QString m_contextMenuScheduleId;
    QString m_contextMenuMedId;
    QString m_contextMenuCurrentStatus;

    // в private секции
    QStringList m_pendingExamFilesForNewExam;  // временное хранилище для файлов нового обследования


    // id лога для отмены

    QJsonArray m_cachedMedications;
    QJsonArray m_cachedMedicationLogs;
    QJsonArray m_cachedExaminations;

    QComboBox* m_examType = nullptr;
    QComboBox* m_examSubtype = nullptr;
    QDateTimeEdit* m_examDate = nullptr;
    QTextEdit* m_examNotes = nullptr;
    QString m_selectedFile;
    QWidget* m_pastExamHost = nullptr;
    QVBoxLayout* m_pastExamLayout = nullptr;
    QWidget* m_plannedExamHost = nullptr;
    QVBoxLayout* m_plannedExamLayout = nullptr;
    QPushButton* m_saveExamBtn = nullptr;
    QString m_editExaminationId;
    QJsonArray m_cachedExaminationItems;
    // в private секцию
    QString m_pendingExamType;
    QString m_pendingExamSubtype;
    QDateTime m_pendingExamDate;

    QLabel* m_avatarPreview = nullptr;
    QLabel* m_sidebarAvatar = nullptr;
    QLabel* m_sidebarEmail = nullptr;
    QLineEdit* m_profileFullName = nullptr;
    QLineEdit* m_profileBirthDate = nullptr;
    QComboBox* m_profileGender = nullptr;
    QLineEdit* m_profilePhone = nullptr;
    QLineEdit* m_profileEmail = nullptr;
    QLineEdit* m_profileAddress = nullptr;
    QLineEdit* m_profileEmergency = nullptr;
    QLineEdit* m_profileDoctor = nullptr;
    QTextEdit* m_profileDiagnosis = nullptr;
    QTimer* m_medReminderTimer = nullptr;
    QTimer* m_wellbeingReminderTimer = nullptr;
    QJsonObject m_cachedProfile;

    QWidget* m_analyticsHost = nullptr;
    QVBoxLayout* m_analyticsLayout = nullptr;
    QPushButton* m_exportAnalyticsPdfBtn = nullptr;

    QComboBox* m_triggerKind = nullptr;
    QDateTimeEdit* m_triggerDate = nullptr;
    QComboBox* m_moodScore = nullptr;
    QDoubleSpinBox* m_sleepHours = nullptr;
    QComboBox* m_sleepQuality = nullptr;
    QLineEdit* m_triggerTitle = nullptr;
    QLineEdit* m_triggerTags = nullptr;
    QTextEdit* m_triggerNote = nullptr;
    QPushButton* m_saveTriggerBtn = nullptr;
    QWidget* m_triggerListHost = nullptr;
    QVBoxLayout* m_triggerListLayout = nullptr;
    QJsonArray m_cachedTriggers;

    QMenu* m_medContextMenu = nullptr;
    QString m_contextMenuMedicationId;

    QWidget* m_helpOverlay = nullptr;
    QFrame* m_helpCard = nullptr;
    QLabel* m_helpTextLabel = nullptr;
    QPushButton* m_helpCallBtn = nullptr;
    QPushButton* m_helpSmsBtn = nullptr;
    QPushButton* m_helpMailBtn = nullptr;

    QPushButton* m_openMedicationDialogBtn = nullptr;

    QWidget* mauthOverlay = nullptr;
    QFrame* mauthCard = nullptr;
    QLabel* mauthTitle = nullptr;
    QLabel* mauthSubtitle = nullptr;
    QLineEdit* mauthNameEdit = nullptr;
    QLineEdit* mauthEmailEdit = nullptr;
    QLineEdit* mauthPasswordEdit = nullptr;
    QPushButton* mauthSubmit = nullptr;
    QPushButton* mauthSwitch = nullptr;
    QToolButton* mauthLangBtn = nullptr;
    QCheckBox* mauthAgreeCheckbox = nullptr;
    QLabel* mauthStatusLabel = nullptr;
    bool mauthRegisterMode = false;
    bool mauthBusy = false;




    QLabel* mauthLogoLabel = nullptr;

    QCheckBox* mauthPolicyCheckbox = nullptr;
    QCheckBox* mauthPersonalDataCheckbox = nullptr;

    QLabel* mauthPolicyLinksLabel = nullptr;
    QLabel* mauthPersonalDataLinkLabel = nullptr;

    QGraphicsBlurEffect* m_blurSidebar = nullptr;
    QGraphicsBlurEffect* m_blurContent = nullptr;

private:
    void buildUi();
    void wire();
    void retranslateUi();

    QWidget* buildSidebar();
    QWidget* buildTopbar();
    QWidget* createDashboardPage();
    QWidget* createSeizuresPage();
    QWidget* createMedicinesPage();
    QWidget* createExamsPage();
    QWidget* createProfilePage();
    QWidget* createTriggersPage();

    QWidget* createAnalyticsPage();
    void refreshAnalyticsPage();

    void createMedicationSchedulesFor(const QJsonObject& med);
    void createIntakeItemWidget(const QDate& date,
                                const QJsonObject& med,
                                const QJsonObject& schedule,
                                const QJsonObject& existingLog,
                                bool isTaken);
    void showDayIntakeContextMenu(const QPoint& pos);
    void handleIntakeStatusChange(const QString& clicked);

    void startMedicationReminderLoop();
    void checkMedicationReminders();
    void startWellbeingReminderLoop();
    void checkWellbeingReminders();


    void handleIntakeToggle(QWidget* cell, const QString& targetStatus);
    void handleIntakeDelete(QWidget* cell);

    void handleAttachmentsReady(const QJsonArray& attachments);
    QWidget* makeExamCard(const QJsonObject& exam);

    void refreshTodayIntakesList();

    void markMedicationTakenFromSchedule(const QJsonObject& med,
                                         const QJsonObject& schedule,
                                         const QDate& date);
    void showIntakesForMedicationToday(const QString& medId);

    QFrame* makeAnalyticsStatCard(const QString& title,
                                  const QString& value,
                                  const QString& hint);

    QChartView* makeMoodChart(const QList<QPair<QDate, double>>& points);
    QChartView* makeBarChart(const QString& chartTitle,
                             const QList<QPair<QString, int>>& items,
                             const QColor& color,
                             int maxItems = 6);

    QWidget* makeAnalyticsInsightsCard(const QStringList& insights);
    QWidget* makeAnalyticsEmptyState(const QString& text);

    QWidget* makeSummaryCard(const QString& title, const QString& body, QLabel** valueLabel);
    QPushButton* makeNavButton(const QString& text, const QString& iconPath);
    QWidget* makeSection(const QString& title, QWidget* inner, const QString& objectName = "SectionCard");

    void navigateTo(int index, const QString& title);
    void setActiveNav(QPushButton* active);
    void updateSidebarUi();
    void updateThemeIcon();
    void updateIcons();
    bool isDarkTheme() const;
    QIcon makeTintedIcon(const QString& path, const QColor& color, const QSize& logicalSize = QSize(20, 20));

    void updateDashboard(const QJsonObject& obj);
    void fillSeizures(const QJsonArray& arr);
    void fillMedications(const QJsonArray& arr);
    void fillMedicationLogs(const QJsonArray& arr);
    void fillExaminations(const QJsonArray& arr);
    void fillProfile(const QJsonObject& obj);
    void refreshDayMedicationSummary();

    void showMedicationContextMenu(const QPoint& pos);
    void cancelMedicationIntake();

    void handleEditIntakeTime();
    void handleDeletePrnIntake();
    void handlePrnIntakeFromMedication();

    void showMedicationMenuFromCard(const QPoint& pos);
    void handleEditMedicationFromCard();
    void handleDeleteMedicationFromCard();




    void editMedicationFromCard();
    void toggleMedicationArchive();

    void submitSeizureForm();
    void submitMedicationForm();
    void submitExaminationForm();

    void updateAuthLogo();
    void openMarkIntakeDialog(const QJsonObject& med,
                              const QString& plannedTime);
    void openPrnIntakeDialog();
    void updateMedicationStockOnIntake(const QString& medId, double deltaTablets);

    void openPrnIntakeDialog(const QJsonObject& med);
    void openMedicationDialog(const QJsonObject& existingMed);


    void resetSeizureForm();
    void resetMedicationForm();
    void resetExaminationForm();

    void deleteSeizure(const QJsonObject& seizure);
    void deleteMedication(const QJsonObject& med);
    void deleteExamination(const QJsonObject& exam);
    void submitTriggerWithDetails(const QStringList& symptoms,
                                  const QStringList& triggers);

    void showSeizureMenu(const QJsonObject& seizure, QWidget* anchor);
    void showMedicationMenu(const QJsonObject& med, QWidget* anchor);
    void showExaminationMenu(const QJsonObject& exam, QWidget* anchor);

    void editSeizure(const QJsonObject& seizure);
    void editMedication(const QJsonObject& med);
    void editExamination(const QJsonObject& exam);

    void markMedicationTaken(const QJsonObject& med, const QDate& date, const QString& time = QString());

    void populateExamSubtypeOptions();
    void pickAvatar();
    void pickExamFile();

    void buildHelpOverlay();
    void showHelpOverlay();
    void hideHelpOverlay();

    void buildAuthOverlay();
    void showAuthOverlay(bool forceLogin = false);
    void hideAuthOverlay();
    void setAuthRegisterMode(bool enabled);
    void setAuthBusy(bool busy);
    void applyBlur(bool enabled);

    void updateDashboardCalendarDecorations();
    void refreshDashboardDayDetails();
    void fillTriggers(const QJsonArray& arr);
    void refreshTriggersList();
    void submitTrigger();
    void deleteTrigger(const QJsonObject& trigger);


    void createAnalyticsPdfReport();
    QFrame* makeMedicationCard(const QJsonObject& med);
    void rebuildMedicationCards();

    void openEmergencyCall();
    void openEmergencySms();
    void openEmergencyEmail();
    void createPdfReport();
     void onMedicationsReady(const QJsonArray& meds);

    static QString formatTs(const QString& iso);
    static void clearLayout(QLayout* layout);
};
