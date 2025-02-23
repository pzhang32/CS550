# P2Pal - Peer-to-Peer Chat Application

A peer-to-peer chat application implementing the Gossip protocol for reliable message delivery.

## Features
- UDP-based P2P communication
- Automatic peer discovery
- Message ordering with sequence numbers
- Rumor mongering for message propagation
- Anti-entropy mechanism for reliability
- Support for multiple concurrent instances

## Build Requirements
- Qt 6.8.2 or higher
- CMake 3.16 or higher
- C++ compiler supporting C++17

## Build Instructions

1. Install Qt 6.8.2 or higher from [Qt's official website](https://www.qt.io/download)

2. Building with Qt Creator:
   - Open the project's CMakeLists.txt in Qt Creator
   - Configure the project with appropriate kit
   - Click Build button or press Ctrl+B

3. Building from command line:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build .

## Running the Application

- Use the provided run_tests.bat to launch multiple instances automatically
- Or run the executable manually multiple times for testing

## Usage

- Launch multiple instances of P2Pal
- Each instance will automatically discover other peers
- Type messages in the input field
- Click Send button or press Enter to send messages
- Messages will be propagated to all connected peers

## Implementation Details

- Uses UDP sockets for communication
- Implements Gossip protocol with:

   - Rumor mongering for message propagation
   - Anti-entropy for ensuring message delivery
   - Vector clocks for message ordering


- Automatic peer discovery on local network
