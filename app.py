from flask import Flask, request, render_template, jsonify, url_for
from datetime import datetime

app = Flask(__name__)

# Store latest sensor data
latest_data = {"soil": "--", "temp": "--", "humidity": "--", "ph": "--"}
pump_status = "OFF"
control_mode = "auto"  # "auto" or "manual"

@app.route('/')
def dashboard():
    return render_template("index.html")

@app.route('/data', methods=['POST'])
def receive_data():
    global latest_data, pump_status, control_mode
    try:
        data = request.json
        print("📥 Received from ESP32:", data)

        # Update sensor data
        latest_data["temp"] = data.get("temp", "--")
        latest_data["humidity"] = data.get("humidity", "--")
        latest_data["soil"] = data.get("soil", "--")
        latest_data["ph"] = data.get("ph", "--")

        # Auto pump control
        if control_mode == "auto":
            soil = float(data.get("soil", 0))
            pump_status = "ON" if soil < 30.0 else "OFF"

        return jsonify({
            "status": "OK",
            "pump": pump_status,
            "control_mode": control_mode,
            "manual": "manual" if control_mode == "manual" else "auto"
        }), 200

    except Exception as e:
        print("❌ Error:", e)
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/get_data')
def get_data():
    return jsonify({
        "temp": latest_data["temp"],
        "humidity": latest_data["humidity"],
        "soil": latest_data["soil"],
        "ph": latest_data["ph"],
        "pump": pump_status,
        "mode": control_mode.upper(),
        "datetime": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    })

@app.route('/toggle_pump', methods=['POST'])
def toggle_pump():
    global pump_status
    if control_mode == "manual":  # Only allow toggling in manual mode
        pump_status = "ON" if pump_status == "OFF" else "OFF"
        print(f"🔧 Manual pump toggle: {pump_status}")
    return jsonify({"status": "OK", "pump": pump_status})

@app.route('/switch_mode', methods=['POST'])
def switch_mode():
    global control_mode
    control_mode = "manual" if control_mode == "auto" else "auto"
    print(f"🔄 Mode switched to: {control_mode.upper()}")
    return jsonify({"status": "OK", "mode": control_mode})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)