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

BLE Bluetooth device pushes data → 
 callback function is triggered → 
 data parsing → 
 sliding average → 
 determines validity → 
 sent to front-end page via WebSocket



`heart_rate = data[1]`
`spo2 = data[2] `
This is the "unpacking" of the raw BLE data.
- Byte 0: Flag bit 
- Byte 1: Heart rate value 
- Byte 2: SpO2 value 

Add the currently read heart_rate to the deque cache we defined earlier. The purpose of the deque is to hold the last 5 heart rate values for smoothing purposes. Call the `moving_average()` function defined earlier to calculate a smoothed value for the current heart rate using the sliding average algorithm. 
Sensors send back heart rate = 0 when they don't detect a signal (e.g. unsteady hand, abnormal red light reflection). Filter out such invalid values here to prevent the front-end from displaying a 0 or drawing an error line.

`socketio.emit()` will take this dictionary {'heart_rate': ... , 'spo2': ...} to all connected clients on the frontend
The front-end JS listens to the `update_data` event and receives this JSON data to update the chart.

```python
def notification_handler(sender, data):

    heart_rate = data[1]
    spo2 = data[2]

    heart_rate_buffer.append(heart_rate)
    smoothed_heart_rate = moving_average(heart_rate_buffer, MOVING_AVERAGE_WINDOW_SIZE)

    display_heart_rate = smoothed_heart_rate if heart_rate != 0 else None

    print(f"Received data: Heart Rate: {heart_rate}, Smoothed Heart Rate: {smoothed_heart_rate},spo2 : {spo2}, Raw Data: {data}")
    socketio.emit('update_data', {'heart_rate': display_heart_rate, 'spo2': spo2}, namespace='/')
    print("Data emitted to frontend")
```
## BLE device connection
The async def defines an asynchronous function that must be used with await.

An asynchronous function means that it can be hung without blocking the main thread, and is intended to run BLE's asynchronous I/O operations.

**BLE is an event-driven communication (notify), unlike traditional request-response. Can't write a dead loop to poll BLE, you have to wait for an event to occur with await.**

This function handles the asynchronous connection to the BLE device using BleakClient. Inside the async with block, I initiate a notification subscription to the heart rate characteristic (UUID 0x2A37) using `await start_notify(...)`.

Every time new data arrives, the registered notification_handler is triggered automatically.

The while True: `await asyncio.sleep(10)` loop is simply to keep the coroutine alive without blocking since BLE notifications are event-driven, we don’t actively poll, just stay connected.



```python
async def run_ble_client():
    async with BleakClient(DEVICE_ADDRESS) as client:
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        while True:
            await asyncio.sleep(10)
```
## Launch
`asyncio.set_event_loop(loop) `
- sets the current thread's event loop to loop
- must be set explicitly, because asyncio's event loop is thread-independent.
- If you don't set it, running the loop in a sub-thread will result in an error: RuntimeError: There is no current event loop in thread

`loop.run_until_complete(run_ble_client()) `
- starts the asynchronous concatenation run_ble_client() and keeps it running in this thread (until manually interrupted)

```python
def start_ble_loop(loop):
    asyncio.set_event_loop(loop)
    loop.run_until_complete(run_ble_client())

@app.route('/')
def index():
    return render_template('index.html')
```
## main
- Create an `asyncio` event loop `loop`
- Start the BLE listener task with a new thread `start_ble_loop`
- Must use threads to run asynchronous code for BLE. Running Flask in the main thread and BLE in a sub-thread enables concurrent communication + front-end access. `host='0.0.0.0'` Receive requests from any IP (can be accessed by the same LAN)
- Starting Web Services
if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    t = threading.Thread(target=start_ble_loop, args=(loop,))
    t.start()
    socketio.run(app, debug=True, host='0.0.0.0', port=5002)
