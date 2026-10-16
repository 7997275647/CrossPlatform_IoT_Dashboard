#pragma once

#include "sensorchannel.h"

#include <QObject>
#include <QVariantList>
#include <QtMqtt/QMqttClient>
#include <QtQml/qqmlregistration.h>

// Connects to an MQTT broker, subscribes to <baseTopic>/temperature, /humidity and /gyro
// and feeds the received data into one SensorChannel (last 1024 samples) per sensor.
class SensorHub : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString hostname READ hostname WRITE setHostname NOTIFY hostnameChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString baseTopic READ baseTopic WRITE setBaseTopic NOTIFY baseTopicChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList channels READ channels CONSTANT)

public:
    explicit SensorHub(QObject *parent = nullptr);

    QString hostname() const { return m_client.hostname(); }
    void setHostname(const QString &hostname);

    int port() const { return m_client.port(); }
    void setPort(int port);

    QString baseTopic() const { return m_baseTopic; }
    void setBaseTopic(const QString &baseTopic);

    bool isConnected() const { return m_client.state() == QMqttClient::Connected; }
    QString status() const { return m_status; }
    QVariantList channels() const { return m_channelList; }

    Q_INVOKABLE void connectToBroker();
    Q_INVOKABLE void disconnectFromBroker();

signals:
    void hostnameChanged();
    void portChanged();
    void baseTopicChanged();
    void connectedChanged();
    void statusChanged();

private:
    void applyTopics();
    void subscribeAll();
    void unsubscribeAll();
    void handleMessage(const QByteArray &payload, const QMqttTopicName &topic);
    void setStatus(const QString &status);

    QMqttClient m_client;
    QList<SensorChannel *> m_channels;
    QVariantList m_channelList;
    QList<QMqttSubscription *> m_subscriptions;
    QString m_baseTopic = QStringLiteral("sensors");
    QString m_status = QStringLiteral("Disconnected");
};
