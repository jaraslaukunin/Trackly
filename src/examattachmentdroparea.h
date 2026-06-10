#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

class ExamAttachmentDropArea : public QFrame
{
    Q_OBJECT
public:
    explicit ExamAttachmentDropArea(QWidget* parent = nullptr);

    void setFiles(const QStringList& paths, const QMap<QString, QString>& fileIds = QMap<QString, QString>());
    QStringList files() const;

signals:
    void filesChanged(const QStringList& paths);
    void fileRemoved(const QString& fileId);  // 🔥 НОВЫЙ СИГНАЛ

protected:
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragLeaveEvent(QDragLeaveEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    void rebuildList();

    QLabel* m_hintLabel = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QStringList m_files;
    QMap<QString, QString> m_fileIds;
};
