#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QVariantMap>
#include <QTimer>
#include <QHash>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct Message {
    QString chatText;
    QString origin;
    quint32 seqNumber;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slot function that handles sending messages
    void handleSendMessage();
    void on_sendButton_clicked();
    void handleReceiveMessage();
    void resendMessages();
    void sendAntiEntropyMessage();
    void sendDiscoveryMessage();
    void checkPeerStatus();

private:
    Ui::MainWindow *ui;
    QUdpSocket *socket;
    QTimer *rumorTimer;
    QTimer *antiEntropyTimer;
    QTimer *discoveryTimer;
    QTimer *peerCheckTimer;

    void setupUi();  // Set other UI properties
    void setupNetwork();

    int localPort;
    int peerPort;
    quint32 seqNumber;   // Serial Number
    QSet<quint16> peers;
    QHash<quint16, qint64> lastSeen;

    // Message storage
    QHash<QString, QHash<quint32, Message>> messageStore;  // origin -> (seqNumber -> Message)
    QSet<QPair<QString, quint32>> unconfirmedMessages;
    QHash<QString, quint32> vectorClock;

    // Serialization and Deserialization Functions
    QByteArray serializeMessage(const QString &text);
    void processReceivedMessage(const QVariantMap &message, quint16 senderPort);
    void storeMessage(const Message &msg);
    void sendRumorMessage(const Message &msg);

    // Anti-Entropy related functions
    void sendStatusMessage();
    void processStatusMessage(const QVariantMap &status, quint16 senderPort);
    void handleMissingMessages(const QString &origin, quint32 maxSeq, quint16 senderPort);

    // Peer Discovery related functions
    void processDiscoveryMessage(quint16 senderPort);
    void addPeer(quint16 port);
    void removePeer(quint16 port);
    void broadcastMessage(const QByteArray &datagram);
};
#endif // MAINWINDOW_H
