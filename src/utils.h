#pragma once

#include <QString>
#include <QLayout>

QString formatIsoToLocal(const QString& iso);
void clearLayout(QLayout* layout);
QString prettifyError(const QString& raw);
QString successMessageFor(const QString& key);
