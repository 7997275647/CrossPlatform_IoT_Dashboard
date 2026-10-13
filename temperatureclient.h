#pragma once

#include <QObject>
#include <QtMqtt/QMqttClient>
#include <QtQml/qqmlregistration.h>

class TemperatureClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString hostname READ hostname WRITE setHostname NOTIFY hostnameChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString topic READ topic WRITE setTopic NOTIFY topicChanged)
    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(bool hasValue READ hasValue NOTIFY temperatureChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit TemperatureClient(QObject *parent = nullptr);

    QString hostname() const { return m_client.hostname(); }
    void setHostname(const QString &hostname);

    int port() const { return m_client.port(); }
    void setPort(int port);

    QString topic() const { return m_topic; }
    void setTopic(const QString &topic);

    double temperature() const { return m_temperature; }
    bool hasValue() const { return m_hasValue; }
    bool isConnected() const { return m_client.state() == QMqttClient::Connected; }
    QString status() const { return m_status; }

    Q_INVOKABLE void connectToBroker();
    Q_INVOKABLE void disconnectFromBroker();

signals:
    void hostnameChanged();
    void portChanged();
    void topicChanged();
    void temperatureChanged();
    void connectedChanged();
    void statusChanged();

private:
    void subscribe();
    void handleMessage(const QByteArray &payload);
    void setStatus(const QString &status);

    QMqttClient m_client;
    QMqttSubscription *m_subscription = nullptr;
    QString m_topic = QStringLiteral("sensors/temperature");
    double m_temperature = 0.0;
    bool m_hasValue = false;
    QString m_status = QStringLiteral("Disconnected");
};
