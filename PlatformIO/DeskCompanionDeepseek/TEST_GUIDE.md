# DeepSeek API Test - Quick Setup Guide

## **What This Test Does**
- Connects to WiFi (DIGI-Gua4)
- Sends a test prompt to DeepSeek API
- Displays AI response on Serial Monitor

## **Before You Test**

### **1. Add Your DeepSeek API Key**
Edit `src/config.cpp` and replace:
```cpp
const char* DEEPSEEK_API_KEY = "sk-your-deepseek-api-key-here";
```
With your actual DeepSeek API key.

### **2. Upload to ESP32-CAM**
1. Connect ESP32-CAM to your programmer
2. Set GPIO 0 to GND (flash mode)
3. Upload this test firmware
4. Remove GPIO 0 from GND
5. Power cycle the board

### **3. Open Serial Monitor**
- Baud rate: **115200**
- You should see WiFi connection status
- Then API request and response

## **Expected Serial Output**

```
=== DeepSeek API Test ===
Connecting to WiFi.........
WiFi connected!
IP address: 192.168.1.100

=== Testing DeepSeek API ===
Sending request to DeepSeek...
Payload: {"model":"deepseek-chat","max_tokens":50,"temperature":0.7,"messages":[{"role":"system","content":"You are DeskMate..."},{"role":"user","content":"Tell me something interesting about space exploration in one short sentence."}]}
HTTP Response code: 200
Raw Response: {"id":"...","object":"chat.completion","created":...,"model":"deepseek-chat","choices":[{"index":0,"message":{"role":"assistant","content":"Space exploration reveals new planets in distant galaxies."},"finish_reason":"stop"}]}

=== AI Response ===
Space exploration reveals new planets in distant galaxies.
===================
```

## **Troubleshooting**

### **WiFi Connection Issues**
- Check network name: "DIGI-Gua4"
- Check password: "REEeD5sMad"
- Ensure router is broadcasting SSID

### **API Key Issues**
- Verify DeepSeek API key is valid
- Check for extra spaces or characters
- Ensure key starts with "sk-"

### **HTTP Errors**
- **200**: Success ✅
- **401**: Invalid API key
- **429**: Rate limit exceeded
- **500**: DeepSeek server error

### **JSON Parse Errors**
- Check internet connection
- Verify DeepSeek API is accessible
- Try running test again

## **Next Steps After Test**

Once this test works:
1. **Verify OLED display** works
2. **Test touch sensor** input
3. **Check Bluetooth audio** connection
4. **Combine all components** in full version

---

## **Ready to Test! 🚀**

The test firmware is compiled and ready to upload. It will verify:
- ✅ WiFi connection
- ✅ DeepSeek API integration  
- ✅ JSON parsing
- ✅ Serial output

**Good luck testing!** 🤖✨