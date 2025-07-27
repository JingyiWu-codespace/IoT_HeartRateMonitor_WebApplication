# Web application 
## import libraries
```python
import collections
import time
import random
from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import asyncio
import threading
from bleak import BleakClient
```

- `collections`: Use `deque` to save the latest heart rate for moving average calculation. `deque`-Fixed-length double-ended queue
- `time`: For sleep
- `random`: For test without hardware/sensor
- `flask`: For building a new web app instance.
- `render_template()`:render html
- `flask_socketio`: a specific plugin that supports Flask can do WebSocket communication. WebSocket is used to build a long connection for develier message actively.
- `SocketIO(app)`: Initiate Websocket functionality and wraps the Flask application instance.
- `emit()`: push real time data from back to front end.
- `asyncio`: Python's standard library for asynchronous programming for async/await support. Need it to run BLE Bluetooth communication, and BleakClient is the async API.
- `threading`: Multi-threaded library that allows to run BLE communication loops concurrently outside of the main thread. Key solution to the problem of both Flask and BLE taking up the main thread.
- `bleak`: Bleak is the main Bluetooth Low Energy (BLE) communication library in Python. `BleakClient` is a client that connects to BLE devices and supports connecting, subscribing to features, receiving data, and so on.

## Initiate applications
- Create a Flask application instance. Register routes through it later (e.g. / to access the index page).
- Use Flask-SocketIO to mount the WebSocket functionality to your Flask application.
- After that you can use socketio.emit() to send data to the frontend.
- Define a constant: the window size of the sliding average is 5. Meaning: each time the heart rate is updated, take the 5 most recent data and do an average. This is to solve the jitter problem of the sensor signal and to improve the data smoothness.

```python
app = Flask(__name__)
socketio = SocketIO(app)
MOVING_AVERAGE_WINDOW_SIZE = 5

# Device MAC Address PSOC6 (Can be scanned by LightBlue) 
DEVICE_ADDRESS = "378B4DFC-AB5F-F241-22F4-AB0C216932A1"

# Heart Rate Character, also have spo2
CHARACTERISTIC_UUID = "2A37"  


def moving_average(data, window_size):
    if len(data)==0:
        return 0
    elif len(data) < window_size:
        return sum(data) / len(data)
    else:
        return sum(data) / len(data)
```

### Data processing
`heart_rate = data[1] \n spo2 = data[2] `
This is the "unpacking" of the raw BLE data.
Byte 0: Flag bit 
Byte 1: Heart rate value (BPM) 
Byte 2: SpO2 value (% oxygen)


```python
def notification_handler(sender, data):
    # 根据您的设备数据格式进行解析
    heart_rate = data[1]
    spo2 = data[2]

    heart_rate_buffer.append(heart_rate)
    smoothed_heart_rate = moving_average(heart_rate_buffer, MOVING_AVERAGE_WINDOW_SIZE)

    # 只有在心率不为0时才更新到前端
    display_heart_rate = smoothed_heart_rate if heart_rate != 0 else None

    print(f"Received data: Heart Rate: {heart_rate}, Smoothed Heart Rate: {smoothed_heart_rate},spo2 : {spo2}, Raw Data: {data}")
    socketio.emit('update_data', {'heart_rate': display_heart_rate, 'spo2': spo2}, namespace='/')
    print("Data emitted to frontend")
```
