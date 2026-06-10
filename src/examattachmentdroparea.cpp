#include "examattachmentdroparea.h"

#include <QStyle>
#include <QHBoxLayout>
#include <QToolButton>
#include <QFileInfo>

ExamAttachmentDropArea::ExamAttachmentDropArea(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("ExamDropArea");
    setAcceptDrops(true);
    setFrameStyle(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(72);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(12, 10, 12, 10);
    outer->setSpacing(6);

    m_hintLabel = new QLabel(tr("Перетащи сюда файлы или нажми «Выбрать файл»"), this);
    m_hintLabel->setObjectName("MutedLabel");
    m_hintLabel->setWordWrap(true);
    outer->addWidget(m_hintLabel);

    m_listLayout = new QVBoxLayout;
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(4);
    outer->addLayout(m_listLayout);
}

void ExamAttachmentDropArea::setFiles(const QStringList& paths, const QMap<QString, QString>& fileIds)
{
    m_files = paths;
    m_fileIds = fileIds;
    rebuildList();
    emit filesChanged(m_files);
}

QStringList ExamAttachmentDropArea::files() const
{
    return m_files;
}

void ExamAttachmentDropArea::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
        setProperty("dragActive", true);
        style()->unpolish(this);
        style()->polish(this);
    } else {
        e->ignore();
    }
}

void ExamAttachmentDropArea::dragLeaveEvent(QDragLeaveEvent* e)
{
    Q_UNUSED(e);
    setProperty("dragActive", false);
    style()->unpolish(this);
    style()->polish(this);
}

void ExamAttachmentDropArea::dropEvent(QDropEvent* e)
{
    setProperty("dragActive", false);
    style()->unpolish(this);
    style()->polish(this);

    QStringList paths = m_files;
    for (const QUrl& url : e->mimeData()->urls()) {
        if (url.isLocalFile()) {
            const QString p = url.toLocalFile();
            if (!paths.contains(p))
                paths << p;
        }
    }
    setFiles(paths);

    e->acceptProposedAction();
}

void ExamAttachmentDropArea::rebuildList()
{
    QLayoutItem* child;
    while ((child = m_listLayout->takeAt(0)) != nullptr) {
        if (auto* w = child->widget())
            w->deleteLater();
        delete child;
    }

    if (m_files.isEmpty()) {
        m_hintLabel->setVisible(true);
        return;
    }

    m_hintLabel->setVisible(false);

    for (int i = 0; i < m_files.size(); ++i) {
        const QString path = m_files[i];
        const QString fileName = QFileInfo(path).fileName();
        const QString fileId = m_fileIds.value(path, QString());

        auto* row = new QWidget(this);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);

        auto* label = new QLabel(fileName, row);
        label->setObjectName("MutedLabel");
        label->setToolTip(path);

        // Показываем бейдж "уже в базе" для существующих файлов
        if (!fileId.isEmpty()) {
            auto* badge = new QLabel(tr("(загружен)"), row);
            badge->setObjectName("BadgeLabel");
            badge->setStyleSheet("padding: 2px 6px; font-size: 10px;");
            h->addWidget(badge);
        }

        auto* removeBtn = new QToolButton(row);
        removeBtn->setObjectName("IconOnlyButton");
        removeBtn->setText(QStringLiteral("✕"));
        removeBtn->setToolTip(tr("Удалить файл"));
        removeBtn->setProperty("filePath", path);
        removeBtn->setProperty("fileId", fileId);

        h->addWidget(label);
        h->addStretch();
        h->addWidget(removeBtn);

        connect(removeBtn, &QToolButton::clicked, this, [this, path, fileId]() {
            if (!fileId.isEmpty()) {
                emit fileRemoved(fileId);  // сигнал для удаления из БД
            }

            QStringList newList = m_files;
            newList.removeAll(path);
            QMap<QString, QString> newIds = m_fileIds;
            newIds.remove(path);
            setFiles(newList, newIds);
        });

        m_listLayout->addWidget(row);
    }
}
