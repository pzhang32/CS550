# P2Pal Testing Guide

## Test Cases

### 1. Basic Communication Test
- **Steps:**
  1. Launch two instances of P2Pal
  2. Wait for peer discovery (about 5 seconds)
  3. Send messages from Instance 1 to Instance 2
  4. Send messages from Instance 2 to Instance 1
- **Expected Results:**
  - Both instances should discover each other
  - Messages should appear in both chat windows
  - Message sequence numbers should be consistent

### 2. Peer Discovery Test
- **Steps:**
  1. Launch first instance
  2. Wait 10 seconds
  3. Launch second instance
  4. Launch third instance
- **Expected Results:**
  - All instances should discover each other
  - Each instance should display peer discovery messages

### 3. Message Ordering Test
- **Steps:**
  1. Launch two instances
  2. Rapidly send multiple messages from both instances
- **Expected Results:**
  - Messages should maintain sequence ordering
  - All messages should be received
  - Sequence numbers should be consistent

### 4. Anti-Entropy Test
- **Steps:**
  1. Launch three instances
  2. Send messages from Instance 1
  3. Close Instance 2 temporarily
  4. Continue sending messages
  5. Relaunch Instance 2
- **Expected Results:**
  - Instance 2 should receive all missed messages upon reconnection

### 5. Multiple Instance Test
- **Steps:**
  1. Use run_tests.bat to launch multiple instances
  2. Send messages between all instances
- **Expected Results:**
  - All instances should communicate properly
  - Messages should propagate to all peers

## Running Tests
1. Use the provided `run_tests.bat` script
2. Or manually test following the test cases above
3. Document any issues or unexpected behavior