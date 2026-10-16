#include "sensorhub.h"

#include <QDateTime>

SensorHub::SensorHub(QObject *parent)
    : QObject(parent)
{
    m_client.setHostname(QStringLiteral("localhost"));
    m_client.setPort(1883);

    m_channels = {
        new SensorChannel(QStringLiteral("temperature"), tr("Temperature"), QStringLiteral("°C"),
                          {QStringLiteral("temperature")}, SensorChannel::DefaultCapacity, this),
        new SensorChannel(QStringLiteral("humidity"), tr("Humidity"), QStringLiteral("%"),
                          {QStringLiteral("humidity")}, SensorChannel::DefaultCapacity, this),
        new SensorChannel(QStringLiteral("gyro"), tr("Gyro"), QStringLiteral("°/s"),
                          {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z")},
                          SensorChannel::DefaultCapacity, this),
    };
    for (SensorChannel *channel : std::as_const(m_channels))
        m_channelList.append(QVariant::fromValue(channel));
    applyTopics();

    connect(&m_client, &QMqttClient::stateChanged, this, [this](QMqttClient::ClientState state) {
        switch (state) {
        case QMqttClient::Disconnected:
            m_subscriptions.clear();
            setStatus(tr("Disconnected"));
            break;
        case QMqttClient::Connecting:
            setStatus(tr("Connecting..."));
            break;
        case QMqttClient::Connected:
            setStatus(tr("Connected"));
            subscribeAll();
            break;
        }
        emit connectedChanged();
    });

    connect(&m_client, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        if (error != QMqttClient::NoError)
            setStatus(tr("Error: %1").arg(static_cast<int>(error)));
    });

    connect(&m_client, &QMqttClient::messageReceived, this, &SensorHub::handleMessage);
}

void SensorHub::setHostname(const QString &hostname)
{
    if (hostname == m_client.hostname())
        return;
    m_client.setHostname(hostname);
    emit hostnameChanged();
}

void SensorHub::setPort(int port)
{
    if (port == m_client.port() || port < 1 || port > 65535)
        return;
    m_client.setPort(static_cast<quint16>(port));
    emit portChanged();
}

void SensorHub::setBaseTopic(const QString &baseTopic)
{
    const QString cleaned = baseTopic.trimmed();
    if (cleaned == m_baseTopic || cleaned.isEmpty())
        return;
    m_baseTopic = cleaned;
    emit baseTopicChanged();

    if (isConnected())
        unsubscribeAll();
    applyTopics();
    if (isConnected())
        subscribeAll();
}

void SensorHub::applyTopics()
{
    for (SensorChannel *channel : std::as_const(m_channels))
        channel->setTopic(QStringLiteral("%1/%2").arg(m_baseTopic, channel->id()));
}

void SensorHub::connectToBroker()
{
    if (m_client.state() == QMqttClient::Disconnected)
        m_client.connectToHost();
}

void SensorHub::disconnectFromBroker()
{
    m_client.disconnectFromHost();
}

void SensorHub::subscribeAll()
{
    for (SensorChannel *channel : std::as_const(m_channels)) {
        QMqttSubscription *sub = m_client.subscribe(QMqttTopicFilter(channel->topic()), 0);
        if (!sub) {
            setStatus(tr("Subscribe failed: %1").arg(channel->topic()));
            continue;
        }
        m_subscriptions.append(sub);
    }
}

void SensorHub::unsubscribeAll()
{
    for (QMqttSubscription *sub : std::as_const(m_subscriptions))
        m_client.unsubscribe(sub->topic());
    m_subscriptions.clear();
}

void SensorHub::handleMessage(const QByteArray &payload, const QMqttTopicName &topic)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (SensorChannel *channel : std::as_const(m_channels)) {
        if (QMqttTopicFilter(channel->topic()).match(topic))
            channel->addPayload(payload, now);
    }
}

void SensorHub::setStatus(const QString &status)
{
    if (status == m_status)
        return;
    m_status = status;
    emit statusChanged();
}
