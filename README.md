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

- `collections`: Use `deque` to save the latest heart rate for moving average calculation.
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

```python
app = Flask(__name__)
socketio = SocketIO(app)
MOVING_AVERAGE_WINDOW_SIZE = 5
```
