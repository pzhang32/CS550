#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QKeyEvent>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , seqNumber(0)  // Initialize the serial number
{
    ui->setupUi(this);
    setupUi();
    setupNetwork();

    // Set the timer
    rumorTimer = new QTimer(this);
    connect(rumorTimer, &QTimer::timeout, this, &MainWindow::resendMessages);
    rumorTimer->start(1000);

    // Anti-Entropy Timer
    antiEntropyTimer = new QTimer(this);
    connect(antiEntropyTimer, &QTimer::timeout, this, &MainWindow::sendAntiEntropyMessage);
    antiEntropyTimer->start(2000);

    // Discovery Timer
    discoveryTimer = new QTimer(this);
    connect(discoveryTimer, &QTimer::timeout, this, &MainWindow::sendDiscoveryMessage);
    discoveryTimer->start(5000);

    // Peer Check Timer
    peerCheckTimer = new QTimer(this);
    connect(peerCheckTimer, &QTimer::timeout, this, &MainWindow::checkPeerStatus);
    peerCheckTimer->start(10000);
}

void MainWindow::setupNetwork()
{
    // Create a socket
    socket = new QUdpSocket(this);

    // Let the system automatically assign a port
    //if(socket->bind(QHostAddress::LocalHost, 0)) {
        //localPort = socket->localPort();
        //setWindowTitle(QString("P2Pal - Port: %1").arg(localPort));

        // Connect the readyRead signal to the receiving slot
        //connect(socket, &QUdpSocket::readyRead, this, &MainWindow::handleReceiveMessage);
        //ui->textEdit->append(QString("Listening on port: %1").arg(localPort));

        //sendDiscoveryMessage();
    //} else {
        //ui->textEdit->append("Failed to bind socket");
    //}

    // Temporarily set the peer port, which will be obtained later through peer discovery
    //peerPort = (localPort == 0) ? 0 : localPort + 1;

    bool bound = false;
    for(quint16 port = 51000; port < 51100; port++) {
        if(socket->bind(QHostAddress::LocalHost, port)) {
            localPort = port;
            bound = true;
            break;
        }
    }

    if(bound) {
        setWindowTitle(QString("P2Pal - Port: %1").arg(localPort));
        connect(socket, &QUdpSocket::readyRead, this, &MainWindow::handleReceiveMessage);
        ui->textEdit->append(QString("Listening on port: %1").arg(localPort));
        sendDiscoveryMessage();
    } else {
        ui->textEdit->append("Failed to bind socket - no available ports in range");
    }
}

void MainWindow::sendDiscoveryMessage()
{
    QVariantMap message;
    message["type"] = "discovery";
    message["port"] = localPort;

    QJsonObject jsonObj = QJsonObject::fromVariantMap(message);
    QJsonDocument doc(jsonObj);
    QByteArray datagram = doc.toJson();

    for(quint16 port = 51000; port < 51100; port++) {
        if(port != localPort) {
            socket->writeDatagram(datagram, QHostAddress::LocalHost, port);
        }
    }
}

void MainWindow::processDiscoveryMessage(quint16 senderPort)
{
    addPeer(senderPort);

    QVariantMap response;
    response["type"] = "discovery_ack";
    response["port"] = localPort;

    QJsonObject jsonObj = QJsonObject::fromVariantMap(response);
    QJsonDocument doc(jsonObj);
    QByteArray datagram = doc.toJson();

    socket->writeDatagram(datagram, QHostAddress::LocalHost, senderPort);
}

void MainWindow::addPeer(quint16 port)
{
    if (!peers.contains(port)) {
        peers.insert(port);
        lastSeen[port] = QDateTime::currentSecsSinceEpoch();
        peerPort = port;

        ui->textEdit->append(QString("New peer discovered: %1").arg(port));
        ui->textEdit->append(QString("Current peers: %1").arg(QString::number(peers.size())));
    } else {
        lastSeen[port] = QDateTime::currentSecsSinceEpoch();
    }
}

void MainWindow::removePeer(quint16 port)
{
    if (peers.remove(port)) {
        lastSeen.remove(port);
        ui->textEdit->append(QString("Peer disconnected: %1").arg(port));
    }
}

void MainWindow::checkPeerStatus()
{
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    QSet<quint16> peersToRemove;

    // Check the last response time of each peer
    for (auto it = lastSeen.begin(); it != lastSeen.end(); ++it) {
        if (currentTime - it.value() > 30) {
            peersToRemove.insert(it.key());
        }
    }

    for (quint16 port : peersToRemove) {
        removePeer(port);
    }
}

void MainWindow::broadcastMessage(const QByteArray &datagram)
{
    for (quint16 peerPort : peers) {
        socket->writeDatagram(datagram, QHostAddress::LocalHost, peerPort);
    }
}

void MainWindow::sendStatusMessage()
{
    QVariantMap status;
    status["type"] = "status";
    QVariantMap wants;

    // Convert the vector clock to a format that can be sent
    for (auto it = vectorClock.begin(); it != vectorClock.end(); ++it) {
        wants[it.key()] = it.value();
    }
    status["wants"] = wants;

    QJsonObject jsonObj = QJsonObject::fromVariantMap(status);
    QJsonDocument doc(jsonObj);
    QByteArray datagram = doc.toJson();

    //socket->writeDatagram(datagram, QHostAddress::LocalHost, peerPort);
    broadcastMessage(datagram);
}

void MainWindow::sendAntiEntropyMessage()
{
    sendStatusMessage();
}

void MainWindow::processStatusMessage(const QVariantMap &status, quint16 senderPort)
{
    QVariantMap wants = status["wants"].toMap();

    // Check the messages for each origin
    for (auto it = messageStore.begin(); it != messageStore.end(); ++it) {
        QString origin = it.key();
        quint32 maxLocalSeq = 0;

        // Find the maximum sequence number of this origin
        for (auto msgIt = it.value().begin(); msgIt != it.value().end(); ++msgIt) {
            maxLocalSeq = qMax(maxLocalSeq, msgIt.key());
        }

        // If the other party does not have the message of this origin
        // or the sequence number is smaller than ours
        quint32 peerSeq = wants.contains(origin) ? wants[origin].toUInt() : 0;
        if (peerSeq < maxLocalSeq) {
            handleMissingMessages(origin, maxLocalSeq, senderPort);
        }
    }
}

void MainWindow::handleMissingMessages(const QString &origin, quint32 maxSeq, quint16 senderPort)
{
    // Send the missing message
    for (quint32 seq = 1; seq <= maxSeq; seq++) {
        if (messageStore[origin].contains(seq)) {
            sendRumorMessage(messageStore[origin][seq]);
        }
    }
}

void MainWindow::handleReceiveMessage()
{
    while (socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (senderPort == 0 || senderPort == localPort) {
            continue;
        }

        //ui->textEdit->append(QString("Received message from port: %1").arg(senderPort));

        QJsonDocument doc = QJsonDocument::fromJson(datagram);
        if (!doc.isNull()) {
            QVariantMap message = doc.object().toVariantMap();

            if (message.contains("type")) {
                QString type = message["type"].toString();
                //ui->textEdit->append(QString("Message type: %1").arg(type));

                if (type == "discovery") {
                    processDiscoveryMessage(senderPort);
                } else if (type == "discovery_ack") {
                    addPeer(senderPort);
                } else if (type == "status") {
                    processStatusMessage(message, senderPort);
                    lastSeen[senderPort] = QDateTime::currentSecsSinceEpoch();
                }
            } else {
                processReceivedMessage(message, senderPort);
                lastSeen[senderPort] = QDateTime::currentSecsSinceEpoch();
            }
        }
    }
}

QByteArray MainWindow::serializeMessage(const QString &text)
{
    QVariantMap message;
    message["chatText"] = text;
    message["origin"] = QString::number(localPort);
    message["seqNumber"] = ++seqNumber;

    // Convert QVariantMap to JSON and then to ByteArray
    QJsonObject jsonObj = QJsonObject::fromVariantMap(message);
    QJsonDocument doc(jsonObj);
    return doc.toJson();
}

void MainWindow::storeMessage(const Message &msg)
{
    messageStore[msg.origin][msg.seqNumber] = msg;
    if (msg.origin == QString::number(localPort)) {
        // If it is a message sent by yourself, add it to the unconfirmed collection
        unconfirmedMessages.insert({msg.origin, msg.seqNumber});
    }
}

void MainWindow::sendRumorMessage(const Message &msg)
{
    QVariantMap message;
    message["chatText"] = msg.chatText;
    message["origin"] = msg.origin;
    message["seqNumber"] = msg.seqNumber;

    QJsonObject jsonObj = QJsonObject::fromVariantMap(message);
    QJsonDocument doc(jsonObj);
    QByteArray datagram = doc.toJson();

    //ui->textEdit->append(QString("Sending message to peers. Peer count: %1").arg(peers.size()));

    if (!peers.isEmpty()) {
        for (quint16 peerPort : peers) {
            socket->writeDatagram(datagram, QHostAddress::LocalHost, peerPort);
            //ui->textEdit->append(QString("Message sent to peer: %1").arg(peerPort));
        }
    } else {
        socket->writeDatagram(datagram, QHostAddress::LocalHost, peerPort);
        //ui->textEdit->append("Waiting for peers to be discovered...");
    }
}

void MainWindow::resendMessages()
{
    // Resend unconfirmed messages
    for (const auto &pair : unconfirmedMessages) {
        const Message &msg = messageStore[pair.first][pair.second];
        sendRumorMessage(msg);
    }
}

void MainWindow::handleSendMessage()
{
    QString text = ui->textEdit_2->toPlainText();
    if (!text.isEmpty()) {
        Message msg;
        msg.chatText = text;
        msg.origin = QString::number(localPort);
        msg.seqNumber = ++seqNumber;

        // Store and send messages
        storeMessage(msg);
        sendRumorMessage(msg);

        // Show in your chat history
        QString displayText = QString("Me [%1]: %2")
                                  .arg(msg.seqNumber)
                                  .arg(msg.chatText);
        ui->textEdit->append(displayText);

        ui->textEdit_2->clear();
        ui->textEdit_2->setFocus();
    }
}

void MainWindow::processReceivedMessage(const QVariantMap &message, quint16 senderPort)
{
    Message msg;
    msg.chatText = message["chatText"].toString();
    msg.origin = message["origin"].toString();
    msg.seqNumber = message["seqNumber"].toUInt();

    if (messageStore[msg.origin].contains(msg.seqNumber)) {
        return;
    }

    // Storing Messages
    storeMessage(msg);

    // Display Message
    QString displayText = QString("Peer(%1) [%2]: %3")
                              .arg(msg.origin)
                              .arg(msg.seqNumber)
                              .arg(msg.chatText);
    ui->textEdit->append(displayText);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (socket) {
        socket->close();
        delete socket;
    }
}

void MainWindow::setupUi()
{
    // Set the window title
    setWindowTitle("P2Pal");

    // Automatically focus on the input box
    ui->textEdit_2->setFocus();  // Assume the input box is textEdit_2

    // Set the default window size
    resize(400, 600);
}



void MainWindow::on_sendButton_clicked()
{
    handleSendMessage();
}
