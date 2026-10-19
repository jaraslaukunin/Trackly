#pragma once

#include <QObject>

#ifdef Q_OS_MACOS
void sendMacNotification(const QString& title, const QString& body);
#endif
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QList>
#include <QPair>
#include <functional>
#include <QNetworkRequest>

class QNetworkAccessManager;
class QNetworkReply;
class QTranslator;

class AppCore : public QObject
{
    Q_OBJECT

public:
    enum class ThemeMode {
        Light,
        Dark,
        Auto
    };

    explicit AppCore(QObject* parent = nullptr);
    ~AppCore() override;

    void setSupabaseConfig(const QString& url, const QString& anonKey);

    // в public секцию
    QString generateNormalizedFileName(const QString& examType,
                                       const QString& examSubtype,
                                       const QDateTime& examDate,
                                       int fileIndex);

    void applyTheme(ThemeMode mode);
    ThemeMode themeMode() const;
    void updateApplicationIcon(bool dark);

    void applyLanguage(const QString& languageCode);
    QString language() const;

    bool hasSavedSession() const;
    bool isAuthenticated() const;

    QString accessToken() const;
    QString refreshToken() const;
    QString currentUserId() const;
    QString currentEmail() const;

    QJsonArray medicationsJson() const;

    // в public секцию
    QString generateNormalizedFileName(int fileIndex);

    void updateMedicationStock(const QString& id, int stock);

    void signIn(const QString& email, const QString& password);
    void signUp(const QString& email, const QString& password, const QString& name);
    void signOut();

    void fetchDashboard();
    void fetchSeizures();
    void fetchMedications();
    void fetchMedicationLogs();
    void fetchExaminations();
    void fetchProfile();
    void fetchTriggers();

    void deleteAttachment(const QString& attachmentId);

    QJsonArray medicationLogsForDay(const QDate& date) const;
    void cancelMedicationLog(const QString& logId);

    // Медикаменты: расписания и приёмы
    void fetchMedicationSchedules();
    void createMedicationSchedule(const QJsonObject& obj);
    void markMedicationIntake(const QString& medicationId,
                              const QString& scheduleId,
                              const QString& status); // "taken" или "missed"

    void fetchAttachments();
    QString makePublicAttachmentUrl(const QString& storagePath) const;

    // Экстренные контакты
    void fetchEmergencyContacts();
    void createEmergencyContact(const QJsonObject& obj);
    void deleteEmergencyContact(const QString& id);

    void markMedicationIntakePrn(const QString& medicationId,
                                 const QDateTime& takenAt,
                                 const QString& status);

    void setMedicationIntakeStatus(const QString& medicationId,
                                   const QString& scheduleId,
                                   const QDate& date,
                                   const QString& status);

    void updateMedicationLogTime(const QString& logId,
                                 const QDateTime& takenAt);

    void deleteMedicationLog(const QString& id);
    const QJsonArray& medicationLogsJson() const { return m_cachedMedicationLogs; }

    void createSeizure(const QJsonObject& obj);
    void createMedication(const QJsonObject& obj);
    void createMedicationLog(const QJsonObject& obj);
    void createExamination(const QJsonObject& obj);
    void createTrigger(const QJsonObject& obj);

    void updateSeizure(const QString& id, const QJsonObject& obj);
    void updateMedication(const QString& id, const QJsonObject& obj);
    void updateExamination(const QString& id, const QJsonObject& obj);

    void deleteSeizure(const QString& id);
    void deleteMedication(const QString& id);
    void deleteExamination(const QString& id);
    void deleteTrigger(const QString& id);

    void updateProfile(const QJsonObject& obj);

    void uploadAttachment(const QString& localPath,
                          const QString& storagePath,
                          const QString& examinationId);
    void sendEmergencyEmail(const QString& to,
                            const QString& patientName,
                            const QString& subject = QString(),
                            const QString& text = QString());

    // Верификация через Resend
    void sendVerificationCode(const QString& email, const QString& name);
    void verifyCode(const QString& email, const QString& code);

    // AI-аналитика через proxyAPI
    void setProxyApiKey(const QString& key);
    bool hasProxyApiKey() const;
    void requestAiAnalytics(const QJsonObject& analyticsData);

signals:
    void authSucceeded();
    void authFailed(const QString& error);
    void signedOut();

    void dashboardReady(const QJsonObject& obj);
    void seizuresReady(const QJsonArray& arr);
    void medicationsReady(const QJsonArray& arr);
    void medicationLogsReady(const QJsonArray& arr);
    void examinationsReady(const QJsonArray& arr);
    void profileReady(const QJsonObject& obj);
    void triggersReady(const QJsonArray& arr);
    void medicationSchedulesLoaded(const QJsonArray& items);
    void medicationLogCreated(const QString& id);

    void emergencyContactsLoaded(const QJsonArray& items);
    void emergencyContactCreated(const QString& id);
    void emergencyContactDeleted(const QString& id);

    void saveSucceeded(const QString& entityKey);
    void networkError(const QString& error);

    void medicationLogCancelled(const QString& id);
    void medicationCreated(const QJsonObject& med);

    void examinationCreated(const QJsonObject& exam);
    void attachmentsReady(const QJsonArray& attachments);
    void aiAnalyticsReady(const QString& summary);
    void aiAnalyticsError(const QString& error);

    void verificationCodeSent();
    void verificationCodeError(const QString& error);
    void codeVerified();
    void codeVerifyError(const QString& error);

private:
    void loadSession();
    void saveSession();
    void clearSession();

    bool ensureConfigured() const;
    bool ensureHasSession(const QString& actionName);

    QList<QPair<QByteArray, QByteArray>> authHeaders(bool json = true) const;
    QNetworkRequest makeRequest(
        const QString& path,
        const QList<QPair<QByteArray, QByteArray>>& extraHeaders = {});

    bool isAccessTokenExpired(int skewSeconds = 30) const;
    void refreshSession(std::function<void(bool)> callback);
    void withValidSession(const QString& actionName, std::function<void()> onReady);
    void rebuildDashboard();
    void upsertProfileAfterSignup(const QString& name);


private:
    QNetworkAccessManager* m_net = nullptr;

    QString m_supabaseUrl;
    QString m_anonKey;

    QJsonArray m_cachedAttachments;

    QString m_proxyApiKey;


    ThemeMode m_themeMode = ThemeMode::Auto;
    QString m_language = "ru";
    QTranslator* m_translator = nullptr;

    QString m_accessToken;
    QString m_refreshToken;
    QString m_userId;
    QString m_email;
    QDateTime m_expiresAt;
    bool m_authenticated = false;

    bool m_refreshInProgress = false;
    QList<std::function<void(bool)>> m_pendingSessionCallbacks;

    QJsonArray m_cachedSeizures;
    QJsonArray m_cachedMedications;
    QJsonArray m_cachedMedicationLogs;
    QJsonArray m_cachedExaminations;
    QJsonArray m_cachedTriggers;
    QJsonObject m_cachedProfile;
};
