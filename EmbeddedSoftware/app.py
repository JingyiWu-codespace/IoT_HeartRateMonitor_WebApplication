import collections
import time
import random


from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import asyncio
import threading
from bleak import BleakClient

app = Flask(__name__)
socketio = SocketIO(app)
MOVING_AVERAGE_WINDOW_SIZE = 5

# 存储最近的心率数据
heart_rate_buffer = collections.deque(maxlen=MOVING_AVERAGE_WINDOW_SIZE)
def moving_average(data, window_size):
    if len(data)==0:
        return 0
    elif len(data) < window_size:
        return sum(data) / len(data)
    else:
        return sum(data) / len(data)

# 使用您在 LightBlue 中看到的设备地址
DEVICE_ADDRESS = "378B4DFC-AB5F-F241-22F4-AB0C216932A1"
# 替换为您的特性 UUID
CHARACTERISTIC_UUID = "2A37"  # 这是一个常见的心率测量特性 UUID

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

# 模拟数据发送函数
def generate_fake_data():
    while True:
        heart_rate = random.randint(60, 100)  # 随机心率数据
        spo2 = random.randint(90, 100)  # 随机 SpO2 数据
        print(f"Generated fake data: Heart Rate: {heart_rate}, SpO2: {spo2}")
        socketio.emit('update_data', {'heart_rate': heart_rate, 'spo2': spo2})
        time.sleep(1)  # 每秒发送一次数据

async def run_ble_client():
    async with BleakClient(DEVICE_ADDRESS) as client:
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        while True:
            await asyncio.sleep(10)

def start_ble_loop(loop):
    asyncio.set_event_loop(loop)
    loop.run_until_complete(run_ble_client())

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    t = threading.Thread(target=start_ble_loop, args=(loop,))
    t.start()
    # fake_data_thread = threading.Thread(target=generate_fake_data)
    # fake_data_thread.daemon = True
    # fake_data_thread.start()

    ## 真实的时候把fake开头的注释掉
    socketio.run(app, debug=True, host='0.0.0.0', port=5002)
