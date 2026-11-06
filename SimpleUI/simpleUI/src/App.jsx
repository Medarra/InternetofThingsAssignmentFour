// IMPORTS
import React, { useState, useEffect } from 'react';
import mqtt from 'mqtt';
import './App.css';

// CONSTANTS
const BROKER_HOST = 'p07da41d.ala.us-east-1.emqxsl.com';  //The base URL
const OPTIONS = {
  host: BROKER_HOST,                                                          //The above base URL
  port: 8084,                                                                 //The port the broker connects through
  protocol: 'wss',                                                            //The protocol used for the connection
  path: '/mqtt',                                                              //The standard mqtt path
  username: 'ogingerich5354',                                                 //Hardcoded username yay!
  password: '5354',                                                           //Hardcoded password not yay
  clientId: 'react_dashboard_' + Math.random().toString(16).substring(2, 10), //Create a unique client ID
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
};
const BASE_TOPIC = "SENG3030/Wednesday/"; //Base topic string
const ENDPOINTS = [
  BASE_TOPIC + "apalmer2583/DeviceController/", //Aly's endpoint
  BASE_TOPIC + "",                              //Kyle's endpoint
  BASE_TOPIC + "tkosatkina4510/AcquisitionDevice/"                               //Tatiana's endpoint
];

export default function App() {
  // --- Use states for the status messages
  const [status, setStatus] = useState({ text: 'Connecting...', class: 'connecting' });
  const [messages, setMessages] = useState({
    DeviceController: { payload: 'No data received yet', time: '--:--:--' },
    endpoint2: { payload: 'No data received yet', time: '--:--:--' },
    AcquisitionDevice: { payload: 'No data received yet', time: '--:--:--' },
  });

  // NEW STATES: Running list of historical logs & target checkbox filter settings
  const [logs, setLogs] = useState([]);
  const [activeFilters, setActiveFilters] = useState({
    DeviceController: true,
    endpoint2: true,
    AcquisitionDevice: true
  });

  useEffect(() => {
    console.log("Connecting to EMQX Broker via WebSockets...");
    const client = mqtt.connect(OPTIONS); // Connect to the MQTT broker

    client.on('connect', () => {  //Log/Update text once connected
      setStatus({ text: 'Connected', class: 'connected' });
      console.log("Successfully connected!");

      ENDPOINTS.forEach(topic => {
        client.subscribe(topic, (err) => {  //Subscribe to topics
          if (!err) console.log(`Subscribed to: ${topic}`);
        });
      });
    });

    client.on('error', (err) => { //Handle connection errors
      console.error('MQTT Error:', err);
      setStatus({ text: 'Error', class: 'disconnected' });
    });

    client.on('close', () => {
      // Check if the client engine is already spinning up an auto-reconnect sequence
      if (client.reconnecting) {
        setStatus({ text: 'Reconnecting...', class: 'connecting' });
        console.warn("[MQTT UI Sync] Lost heartbeat packet. Auto-reconnecting background sockets...");
      } else {
        setStatus({ text: 'Disconnected', class: 'disconnected' });
      }
    });

    // NEW: This forces the badge BACK to green the exact millisecond the stream recovers
    client.on('reconnect', () => {
      console.log("[MQTT UI Sync] Socket reconnected successfully!");
      setStatus({ text: 'Connected', class: 'connected' });
    });

    client.on('message', (topic, message) => {  //Route incoming messages
      const payloadStr = message.toString();
      const timeStr = new Date().toLocaleTimeString();
      console.log(`[${topic}]: ${payloadStr}`);

      let targetKey = '';
      let shortName = ''; // Plain text descriptor for log entries

      if (topic === ENDPOINTS[0]) { targetKey = 'DeviceController'; shortName = "Device Controller"; }
      else if (topic === ENDPOINTS[1]) { targetKey = 'endpoint2'; shortName = "Kyle's Endpoint"; }
      else if (topic === ENDPOINTS[2]) { targetKey = 'AcquisitionDevice'; shortName = "Acquisition Device"; }

      if (targetKey) {
        // Keep the exact card functionality active
        setMessages(prev => ({
          ...prev,
          [targetKey]: { payload: payloadStr, time: timeStr }
        }));

        const isParsable = ['DeviceController', 'AcquisitionDevice'].includes(targetKey);
        const parsedValue = isParsable ? parseMessage(payloadStr) : '--';

        setLogs(prevLogs => [
          {
            endpointKey: targetKey,
            name: shortName,
            payload: payloadStr,   // Keeps original raw packet (e.g. "[A][T][F]")
            parsed: parsedValue,   // NEW VALUE: Holds human translation (e.g. "Acquisition | Temperature | Inactive")
            time: timeStr
          },
          ...prevLogs
        ]);
      }
    });

    return () => {
      if (client) client.end();
    };
  }, []);

  // Filter checkbox interaction handler
  const handleCheckboxChange = (key) => {
    setActiveFilters(prev => ({ ...prev, [key]: !prev[key] }));
  };

  // Computes which historical logs should show up inside our table markup
  const filteredLogs = logs.filter(log => activeFilters[log.endpointKey]);

  return (
    <div className="container">
      <div className="header">
        <h2>MQTT Command Monitor</h2>
        <div className={`status ${status.class}`}>{status.text}</div>
      </div>

      <div className="grid">
        <div className="card">
          <h3>Device Controller</h3>
          <div className="topic-string">{ENDPOINTS[0]}</div>
          {/* CALL UNIFIED PARSER HERE */}
          <div className="payload">{parseMessage(messages.DeviceController.payload)}</div>
          <div className="timestamp">{messages.DeviceController.time}</div>
        </div>

        <div className="card">
          <h3>Endpoint 2</h3>
          <div className="topic-string">{ENDPOINTS[1]}</div>
          <div className="payload">{messages.endpoint2.payload}</div>
          <div className="timestamp">{messages.endpoint2.time}</div>
        </div>

        <div className="card">
          <h3>Acquisition Device</h3>
          <div className="topic-string">{ENDPOINTS[2]}</div>
          {/* CALL UNIFIED PARSER HERE */}
          <div className="payload">{parseMessage(messages.AcquisitionDevice.payload)}</div>
          <div className="timestamp">{messages.AcquisitionDevice.time}</div>
        </div>
      </div>

      {/* Checklist Filters & Historical Table Container */}
      <div className="log-section">
        <h3>Historical Command Feed</h3>

        <div className="filter-checklist">
          <label>
            <input type="checkbox" checked={activeFilters.DeviceController} onChange={() => handleCheckboxChange('DeviceController')} />
            Device Controller
          </label>
          <label>
            <input type="checkbox" checked={activeFilters.endpoint2} onChange={() => handleCheckboxChange('endpoint2')} />
            Kyle's Endpoint
          </label>
          <label>
            <input type="checkbox" checked={activeFilters.AcquisitionDevice} onChange={() => handleCheckboxChange('AcquisitionDevice')} />
            Acquisition Device
          </label>
        </div>

        <div className="log-table-container">
          <table className="log-table">
            <thead>
              <tr>
                <th>Source</th>
                <th>Message</th>
                <th>Parsed Message</th>
                <th>Timestamp</th>
              </tr>
            </thead>
            <tbody>
              {filteredLogs.length === 0 ? (
                <tr>
                  <td colSpan="4" className="empty-log">No history found matching checked criteria...</td>
                </tr>
              ) : (
                filteredLogs.map((log, idx) => (
                  <tr key={idx} className={`row-${log.endpointKey}`}>
                    <td className="log-name">{log.name}</td>
                    <td className="log-payload">{log.payload}</td>
                    <td className="log-parsed">{log.parsed}</td>
                    <td className="log-time">{log.time}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

function parseMessage(rawMessage) {
  if (!rawMessage || typeof rawMessage !== 'string') return 'No Data';  //If no data or data in the wrong format is sent

  //Clean and standardice to upper
  const cleanStr = rawMessage.trim().toUpperCase();
  if (cleanStr.length < 2) return rawMessage;

  // Contextual lookup tables
  const controllerDict = {
    'A_pos1': 'Acquisition', // Explicitly mapped by position index
    'A_pos2': 'Accelerometer',
    'D': 'Display',
    'T': 'Temperature',
    'H': 'Humidity',
    'L': 'Altitude',
    'B': 'Battery',
    'F': 'Inactive'
  };

  const acquisitionDict = {
    'T': 'Temperature',
    'H': 'Humidity',
    'A': 'Accelerometer',
    'L': 'Altitude',
    'B': 'Battery'
  };

  // Protocol 1: Device Controller (Exactly 3 letters like "AAF")
  if (cleanStr.length === 3) {
    // 1st Letter: If it's 'A', use 'A_pos1' (Acquisition)
    const key1 = cleanStr[0] === 'A' ? 'A_pos1' : cleanStr[0];
    const d1 = controllerDict[key1] || cleanStr[0];

    // 2nd Letter: If it's 'A', use 'A_pos2' (Accelerometer)
    const key2 = cleanStr[1] === 'A' ? 'A_pos2' : cleanStr[1];
    const d2 = controllerDict[key2] || cleanStr[1];

    // 3rd Letter: Handle 'T' position swap (Active on position 3, Temperature everywhere else)
    const d3 = cleanStr[2] === 'T' ? 'Active' : (controllerDict[cleanStr[2]] || cleanStr[2]);

    return `${d1} device | ${d2} | ${d3}`;
  }

  // Protocol 2: Acquisition Device (Starts with T, L, or H followed immediately by a number)
  const firstChar = cleanStr.charAt(0);
  if (['T', 'L', 'H', 'A', 'B'].includes(firstChar)) {
    const sensorType = acquisitionDict[firstChar];
    const numericValue = rawMessage.trim().substring(1); // Keeps decimal precision exactly as sent
    return `${sensorType} | ${numericValue}`;
  }

  return rawMessage; // Fallback for raw text if it doesn't fit either rule
}