#include "sensorchannel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>
#include <cmath>

SensorChannel::SensorChannel(const QString &id, const QString &title, const QString &unit,
                             const QStringList &seriesNames, int capacity, QObject *parent)
    : QAbstractTableModel(parent)
    , m_id(id)
    , m_title(title)
    , m_unit(unit)
    , m_seriesNames(seriesNames)
    , m_capacity(qMax(1, capacity))
    , m_min(seriesNames.size(), 0.0)
    , m_max(seriesNames.size(), 0.0)
    , m_sum(seriesNames.size(), 0.0)
{
}

void SensorChannel::setTopic(const QString &topic)
{
    if (topic == m_topic)
        return;
    m_topic = topic;
    emit topicChanged();
}

QDateTime SensorChannel::lastUpdate() const
{
    if (m_samples.empty())
        return {};
    return QDateTime::fromMSecsSinceEpoch(m_samples.front().timestampMs);
}

QVariantList SensorChannel::latest() const
{
    if (m_samples.empty())
        return {};
    return toVariants(m_samples.front().values);
}

QVariantList SensorChannel::average() const
{
    QVariantList list;
    const double n = static_cast<double>(m_samples.size());
    for (double s : m_sum)
        list.append(n > 0 ? s / n : 0.0);
    return list;
}

QVariantList SensorChannel::toVariants(const std::vector<double> &v)
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(v.size()));
    for (double d : v)
        list.append(d);
    return list;
}

bool SensorChannel::parseValues(const QByteArray &payload, std::vector<double> &out) const
{
    const QByteArray trimmed = payload.trimmed();
    const qsizetype n = m_seriesNames.size();
    out.clear();
    if (trimmed.isEmpty())
        return false;

    if (trimmed.startsWith('{')) {
        const QJsonObject obj = QJsonDocument::fromJson(trimmed).object();
        for (const QString &name : std::as_const(m_seriesNames)) {
            QStringList keys{name, QStringLiteral("g") + name};
            if (n == 1)
                keys << m_id << QStringLiteral("value");
            bool found = false;
            for (const QString &key : std::as_const(keys)) {
                const QJsonValue v = obj.value(key);
                if (v.isDouble()) {
                    out.push_back(v.toDouble());
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
    } else if (trimmed.startsWith('[')) {
        const QJsonArray arr = QJsonDocument::fromJson(trimmed).array();
        if (arr.size() < n)
            return false;
        for (qsizetype i = 0; i < n; ++i) {
            if (!arr.at(i).isDouble())
                return false;
            out.push_back(arr.at(i).toDouble());
        }
    } else {
        const QList<QString> parts = QString::fromUtf8(trimmed)
                                         .split(QRegularExpression(QStringLiteral("[,;\\s]+")),
                                                Qt::SkipEmptyParts);
        if (parts.size() < n)
            return false;
        for (qsizetype i = 0; i < n; ++i) {
            bool ok = false;
            const double d = parts.at(i).toDouble(&ok);
            if (!ok)
                return false;
            out.push_back(d);
        }
    }

    return std::all_of(out.begin(), out.end(), [](double d) { return std::isfinite(d); });
}

bool SensorChannel::addPayload(const QByteArray &payload, qint64 timestampMs)
{
    std::vector<double> values;
    if (!parseValues(payload, values))
        return false;
    addSample(timestampMs, values);
    return true;
}

void SensorChannel::addSample(qint64 timestampMs, const std::vector<double> &values)
{
    if (values.size() != static_cast<size_t>(m_seriesNames.size()))
        return;

    if (static_cast<int>(m_samples.size()) >= m_capacity) {
        const int last = static_cast<int>(m_samples.size()) - 1;
        beginRemoveRows({}, last, last);
        m_samples.pop_back();
        endRemoveRows();
    }

    beginInsertRows({}, 0, 0);
    m_samples.push_front({timestampMs, values});
    endInsertRows();

    recomputeStats();
    emit samplesChanged();
}

void SensorChannel::recomputeStats()
{
    const size_t n = static_cast<size_t>(m_seriesNames.size());
    m_min.assign(n, 0.0);
    m_max.assign(n, 0.0);
    m_sum.assign(n, 0.0);
    bool first = true;
    for (const Sample &s : m_samples) {
        for (size_t i = 0; i < n; ++i) {
            const double v = s.values[i];
            m_min[i] = first ? v : std::min(m_min[i], v);
            m_max[i] = first ? v : std::max(m_max[i], v);
            m_sum[i] += v;
        }
        first = false;
    }
}

QVariantList SensorChannel::samples(int series, int maxSamples) const
{
    QVariantList list;
    if (series < 0 || series >= m_seriesNames.size())
        return list;
    const int n = qMin(maxSamples, static_cast<int>(m_samples.size()));
    list.reserve(n);
    for (int i = n - 1; i >= 0; --i) {
        const Sample &s = m_samples[static_cast<size_t>(i)];
        list.append(QPointF(static_cast<double>(s.timestampMs), s.values[static_cast<size_t>(series)]));
    }
    return list;
}

void SensorChannel::clear()
{
    if (m_samples.empty())
        return;
    beginResetModel();
    m_samples.clear();
    endResetModel();
    recomputeStats();
    emit samplesChanged();
}

QString SensorChannel::exportCsv() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QStringLiteral("%1/%2_%3.csv")
                             .arg(dir, m_id, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return {};

    QTextStream out(&file);
    QStringList header{QStringLiteral("timestamp")};
    for (const QString &name : m_seriesNames)
        header << (m_unit.isEmpty() ? name : QStringLiteral("%1 [%2]").arg(name, m_unit));
    out << header.join(QLatin1Char(',')) << '\n';

    for (auto it = m_samples.rbegin(); it != m_samples.rend(); ++it) {
        out << QDateTime::fromMSecsSinceEpoch(it->timestampMs).toString(Qt::ISODateWithMs);
        for (double v : it->values)
            out << ',' << v;
        out << '\n';
    }
    return path;
}

int SensorChannel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_samples.size());
}

int SensorChannel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_seriesNames.size()) + 1;
}

QVariant SensorChannel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || index.row() >= static_cast<int>(m_samples.size()))
        return {};
    const Sample &s = m_samples[static_cast<size_t>(index.row())];
    if (index.column() == 0)
        return QDateTime::fromMSecsSinceEpoch(s.timestampMs).toString(QStringLiteral("HH:mm:ss.zzz"));
    return QString::number(s.values[static_cast<size_t>(index.column() - 1)], 'f', 2);
}

QVariant SensorChannel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    if (section == 0)
        return tr("Time");
    return m_seriesNames.value(section - 1);
}
