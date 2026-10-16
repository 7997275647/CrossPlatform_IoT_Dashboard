#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QPointF>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <deque>
#include <vector>

// One sensor type (e.g. temperature, or the 3-axis gyro) holding the most recent
// `capacity` samples. Also a table model for the newest-first sample table.
class SensorChannel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by SensorHub")

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString unit READ unit CONSTANT)
    Q_PROPERTY(QStringList seriesNames READ seriesNames CONSTANT)
    Q_PROPERTY(QStringList columnLabels READ columnLabels CONSTANT)
    Q_PROPERTY(QString topic READ topic NOTIFY topicChanged)
    Q_PROPERTY(int capacity READ capacity CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY samplesChanged)
    Q_PROPERTY(bool hasValue READ hasValue NOTIFY samplesChanged)
    Q_PROPERTY(QDateTime lastUpdate READ lastUpdate NOTIFY samplesChanged)
    Q_PROPERTY(QVariantList latest READ latest NOTIFY samplesChanged)
    Q_PROPERTY(QVariantList minimum READ minimum NOTIFY samplesChanged)
    Q_PROPERTY(QVariantList maximum READ maximum NOTIFY samplesChanged)
    Q_PROPERTY(QVariantList average READ average NOTIFY samplesChanged)

public:
    static constexpr int DefaultCapacity = 1024;

    SensorChannel(const QString &id, const QString &title, const QString &unit,
                  const QStringList &seriesNames, int capacity = DefaultCapacity,
                  QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString title() const { return m_title; }
    QString unit() const { return m_unit; }
    QStringList seriesNames() const { return m_seriesNames; }
    QStringList columnLabels() const { return QStringList{tr("Time")} + m_seriesNames; }
    QString topic() const { return m_topic; }
    void setTopic(const QString &topic);
    int capacity() const { return m_capacity; }
    int count() const { return static_cast<int>(m_samples.size()); }
    bool hasValue() const { return !m_samples.empty(); }
    QDateTime lastUpdate() const;
    QVariantList latest() const;
    QVariantList minimum() const { return toVariants(m_min); }
    QVariantList maximum() const { return toVariants(m_max); }
    QVariantList average() const;

    // Parses a plain number, CSV, JSON object or JSON array. Returns false if unusable.
    bool addPayload(const QByteArray &payload, qint64 timestampMs);
    void addSample(qint64 timestampMs, const std::vector<double> &values);

    // Points (x = epoch ms, y = value) of one series, oldest first, at most maxSamples.
    Q_INVOKABLE QVariantList samples(int series, int maxSamples = DefaultCapacity) const;
    Q_INVOKABLE void clear();
    // Writes all stored samples to a CSV file in the Documents folder; returns its path (empty on failure).
    Q_INVOKABLE QString exportCsv() const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

signals:
    void topicChanged();
    void samplesChanged();

private:
    struct Sample {
        qint64 timestampMs;
        std::vector<double> values;
    };

    bool parseValues(const QByteArray &payload, std::vector<double> &out) const;
    void recomputeStats();
    static QVariantList toVariants(const std::vector<double> &v);

    QString m_id;
    QString m_title;
    QString m_unit;
    QStringList m_seriesNames;
    QString m_topic;
    int m_capacity;
    std::deque<Sample> m_samples; // front = newest
    std::vector<double> m_min, m_max, m_sum;
};
