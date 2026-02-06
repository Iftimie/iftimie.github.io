# ESP32-CAM AI Companion "DeskMate" Implementation Plan

## **Project Overview**
Build an AI companion using ESP32-CAM with OLED display, touch sensor, Bluetooth audio, and DeepSeek API integration.

## **Hardware Requirements**
- ESP32-CAM (AI Thinker model)
- 0.96" OLED Display (SSD1306, 128x64, I2C)
- External touch sensor module
- SD card with out.raw audio file
- G435 Bluetooth Gaming Headset

## **GPIO Pin Assignment**

| Component | Pin(s) | Status | Notes |
|-----------|---------|---------|--------|
| **OLED Display** | GPIO 3 (SDA - U0RXD), GPIO 1 (SCL - U0TXD) | ✅ Safe | I2C on UART pins (no-solder)
| **Touch Sensor** | GPIO 4 | OK | Acceptable |
| **Bluetooth** | Internal | ✅ Safe | A2DP audio |
| **SD Card** | Built-in SPI | ⚠️ Shared | Conflicts with touch |

**Critical GPIO Constraints Resolved:**
- Default I2C pins (21/22) are camera-reserved → **Use software I2C on 13/16**
- Touch sensor conflicts with SD card → **Accepted conflict**
- Only safe GPIOs used: 13, 14, 16, 4

## **Software Architecture**

### **File Structure**
```
src/
├── main.cpp                    # Main program entry point
├── config.h                    # Configuration constants
├── oled_display.h              # OLED display management
├── deepseek_client.h           # LLM API integration
├── bluetooth_audio.h           # Audio playback system
├── touch_input.h              # Touch sensor handling
└── conversation_manager.h      # Conversation flow management
```

### **Library Dependencies (platformio.ini)**
```ini
lib_deps =
  bblanchon/ArduinoJson@^6
  adafruit/Adafruit_SSD1306@^2.5.7
  adafruit/Adafruit_GFX@^1.11.9
  ESP32-a2dpSource@^0.1.0
```

## **Implementation Phases**

### **Phase 1: Foundation Setup**
1. Update `platformio.ini` with required libraries
2. Create `config.h` with WiFi credentials and API key
3. Set up basic `main.cpp` structure

### **Phase 2: Core Components**
4. Implement OLED display with software I2C (pins 5/16)
5. Set up WiFi connection to DIGI-Gua4
6. Implement DeepSeek API client
7. Add touch sensor input handling

### **Phase 3: Advanced Features**
8. Implement Bluetooth A2DP audio system
9. Add SD card audio file loading
10. Create conversation flow state machine
11. Implement subject management system

## **Technical Specifications**

### **DeepSeek API Integration**
- **Endpoint**: `https://api.deepseek.com/v1/chat/completions`
- **Model**: `deepseek-chat`
- **Max Tokens**: 100 (for OLED display)
- **Temperature**: 0.7
- **Headers**: Authorization Bearer, Content-Type: application/json

-### **OLED Display Configuration (no-solder option)**
- **Type**: SSD1306, 128x64 resolution
- **Interface**: I2C on UART pins
- **Pins**: GPIO 3 (SDA / U0RXD), GPIO 1 (SCL / U0TXD)
- **Address**: 0x3C
- **Notes**: Boot logs and `Serial` output will be unreliable while the display is connected. Disconnect SDA/SCL to flash or to get serial logs.
- **Features**: Text wrapping, scrolling for long content

### **Audio System**
- **Format**: RAW PCM (16-bit, 44.1kHz)
- **File**: `/out2.raw` on SD card
- **Output**: Bluetooth A2DP to G435 headset
- **Trigger**: Play on new text appearance

### **Touch Sensor**
- **Pin**: GPIO 33
- **Response**: Hardcoded "Yes"
- **Debouncing**: Simple software implementation
- **Conflict**: Pauses during SD card access

## **Conversation Flow Design**

### **State Machine**
```
INITIAL_SUBJECT → WAITING_RESPONSE → MORE_DETAILS → WAITING_CONTINUE → NEW_SUBJECT
```

### **Prompt Engineering**
System prompt defines DeskMate persona:
- Short, clear sentences (8-12 words)
- Random subject selection with short facts
- Yes/No interaction pattern
- Accessible, engaging content

### **Response Management**
- Parse LLM JSON responses
- Format for OLED display
- Handle conversation state transitions
- Play notification audio on updates

## **Memory & Performance**

### **Optimization Strategies**
- Limited API response size (100 tokens)
- Efficient JSON parsing
- Minimal string allocations
- Static buffers where possible
- Software I2C timing optimization

### **Error Handling**
- WiFi reconnection logic
- API timeout and retry
- SD card access conflicts
- Bluetooth connection management
- Touch sensor debouncing

## **Testing Strategy**

### **Component Testing**
1. **OLED Display**: Text rendering, scrolling
2. **WiFi Connection**: Network stability
3. **DeepSeek API**: Request/response handling
4. **Touch Sensor**: Input detection and debouncing
5. **Audio System**: SD card loading, Bluetooth playback
6. **Integration**: Full conversation flow

### **Validation Criteria**
- OLED displays text correctly
- WiFi connects reliably
- LLM provides appropriate responses
- Touch input works consistently
- Audio plays on new text
- Conversation flows logically

## **Risk Assessment**

### **Technical Risks**
- **Memory constraints**: Mitigated with small responses
- **GPIO conflicts**: Resolved with software I2C
- **SD card conflicts**: Accepted with touch sensor
- **API limits**: Implemented retry logic
- **Bluetooth connectivity**: Connection state management

### **Hardware Constraints**
- Limited available GPIOs
- Shared SD card pins
- Camera pin restrictions
- Power considerations for BT+WiFi

## **Deployment Considerations**

### **Power Management**
- 5V power recommended for ESP32-CAM
- Current draw for Bluetooth + OLED + SD card
- Battery life considerations (if portable)

### **Physical Layout**
- Compact wiring for OLED and touch sensor
- SD card access availability
- Bluetooth antenna positioning
- Camera integration (future features)

## **Future Enhancements**

### **Potential Additions**
- Camera integration for visual input
- Additional touch sensors (GPIO expander)
- Voice input via microphone
- More complex conversation flows
- Offline capabilities with local LLM

### **Scalability**
- Modular code structure for easy expansion
- Configuration-driven behavior
- Plugin architecture for new features

---

## **Implementation Checklist**

- [ ] Update platformio.ini with libraries
- [ ] Create config.h with credentials
- [ ] Implement software I2C OLED driver
- [ ] Set up WiFi connection handling
- [ ] Create DeepSeek API client
- [ ] Implement touch sensor reading
- [ ] Add Bluetooth A2DP audio system
- [ ] Create conversation state machine
- [ ] Implement subject management
- [ ] Add error handling and retry logic
- [ ] Test all components individually
- [ ] Integration testing
- [ ] Performance optimization
- [ ] Documentation and comments

---

**Ready for implementation! This plan addresses all GPIO constraints, integrates all required components, and provides a systematic approach to building the ESP32-CAM AI companion.**