#include "temperatureclient.h"

#include <QJsonDocument>
#include <QJsonObject>

TemperatureClient::TemperatureClient(QObject *parent)
    : QObject(parent)
{
    m_client.setHostname(QStringLiteral("localhost"));
    m_client.setPort(1883);

    connect(&m_client, &QMqttClient::stateChanged, this, [this](QMqttClient::ClientState state) {
        switch (state) {
        case QMqttClient::Disconnected:
            m_subscription = nullptr;
            setStatus(QStringLiteral("Disconnected"));
            break;
        case QMqttClient::Connecting:
            setStatus(QStringLiteral("Connecting..."));
            break;
        case QMqttClient::Connected:
            setStatus(QStringLiteral("Connected"));
            subscribe();
            break;
        }
        emit connectedChanged();
    });

    connect(&m_client, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
        if (error != QMqttClient::NoError)
            setStatus(QStringLiteral("Error: %1").arg(static_cast<int>(error)));
    });
}

void TemperatureClient::setHostname(const QString &hostname)
{
    if (hostname == m_client.hostname())
        return;
    m_client.setHostname(hostname);
    emit hostnameChanged();
}

void TemperatureClient::setPort(int port)
{
    if (port == m_client.port() || port < 0 || port > 65535)
        return;
    m_client.setPort(static_cast<quint16>(port));
    emit portChanged();
}

void TemperatureClient::setTopic(const QString &topic)
{
    if (topic == m_topic)
        return;
    m_topic = topic;
    emit topicChanged();

    if (isConnected()) {
        if (m_subscription)
            m_subscription->unsubscribe();
        subscribe();
    }
}

void TemperatureClient::connectToBroker()
{
    if (m_client.state() == QMqttClient::Disconnected)
        m_client.connectToHost();
}

void TemperatureClient::disconnectFromBroker()
{
    m_client.disconnectFromHost();
}

void TemperatureClient::subscribe()
{
    m_subscription = m_client.subscribe(QMqttTopicFilter(m_topic), 0);
    if (!m_subscription) {
        setStatus(QStringLiteral("Subscribe failed"));
        return;
    }
    connect(m_subscription, &QMqttSubscription::messageReceived, this,
            [this](const QMqttMessage &msg) { handleMessage(msg.payload()); });
}

// Accepts either a plain number ("23.5") or JSON ({"temperature": 23.5}).
void TemperatureClient::handleMessage(const QByteArray &payload)
{
    bool ok = false;
    double value = payload.trimmed().toDouble(&ok);

    if (!ok) {
        const QJsonDocument doc = QJsonDocument::fromJson(payload);
        const QJsonValue v = doc.object().value(QStringLiteral("temperature"));
        if (v.isDouble()) {
            value = v.toDouble();
            ok = true;
        }
    }

    if (!ok)
        return;

    m_temperature = value;
    m_hasValue = true;
    emit temperatureChanged();
}

void TemperatureClient::setStatus(const QString &status)
{
    if (status == m_status)
        return;
    m_status = status;
    emit statusChanged();
}
