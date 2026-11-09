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
  BASE_TOPIC + "apalmer2583/DeviceController/",     //Aly's endpoint - Device Controller
  BASE_TOPIC + "kmurawsky0383/DisplayDevice/",      //Kyle's endpoint - Display Device
  BASE_TOPIC + "tkosatkina4510/AcquisitionDevice/"  //Tatiana's endpoint - Acquisition Device
];

export default function App() {
  //--- USE STATES
  const [status, setStatus] = useState({ text: 'Connecting...', class: 'connecting' }); //Connection status
  const [messages, setMessages] = useState({                                            //Latest data status
    DeviceController: { payload: 'No data received yet', time: '--:--:--' },
    DisplayDevice: { payload: 'No data received yet', time: '--:--:--' },
    AcquisitionDevice: { payload: 'No data received yet', time: '--:--:--' },
  });
  const [logs, setLogs] = useState([]);                 //Use state for log list
  const [activeFilters, setActiveFilters] = useState({  //Use state for log list filters
    DeviceController: true,
    DisplayDevice: true,
    AcquisitionDevice: true
  });

  useEffect(() => {
    //--- Connect to the MQTT broker
    console.log("Connecting to EMQX Broker via WebSockets...");
    const client = mqtt.connect(OPTIONS); // Connect to the MQTT broker

    client.on('connect', () => {                            //Log/Update text once connected
      setStatus({ text: 'Connected', class: 'connected' }); //Update the connection use state
      console.log("Successfully connected!");

      ENDPOINTS.forEach(topic => {
        client.subscribe(topic, (err) => {          //Subscribe to topics
          if (!err) {                               //Check for error
            console.log(`Subscribed to: ${topic}`); //Log successful connection
          }
        });
      });
    });

    client.on('error', (err) => {                           //Handle connection errors
      console.error('MQTT Error:', err);                    //Log error to console
      setStatus({ text: 'Error', class: 'disconnected' });  //Update connection status use state
    });

    client.on('close', () => {                                        //When connection closes
      if (client.reconnecting) {                                      //Check if client is attempting to reconnect
        setStatus({ text: 'Reconnecting...', class: 'connecting' });  //Update connection status use case
        console.warn("[MQTT UI Sync] Lost heartbeat packet. Auto-reconnecting background sockets...");
      } else {
        setStatus({ text: 'Disconnected', class: 'disconnected' }); //Update status to disconnected if client is not attempting to reconnect
      }
    });

    client.on('reconnect', () => {  //When connection is re-established
      console.log("[MQTT UI Sync] Socket reconnected successfully!");
      setStatus({ text: 'Connected', class: 'connected' }); //Update status to connected
    });

    client.on('message', (topic, message) => {          //Route incoming messages
      const payloadStr = message.toString();            //Map the incoming message to a string
      const timeStr = new Date().toLocaleTimeString();  //Log current timestamp
      console.log(`[${topic}]: ${payloadStr}`);

      //Variables to hold information regarding endpoints
      let targetKey = '';
      let shortName = '';

      //Set above variables depending on the topic name and endpoints map
      if (topic === ENDPOINTS[0]) { targetKey = 'DeviceController'; shortName = "Device Controller"; }
      else if (topic === ENDPOINTS[1]) { targetKey = 'DisplayDevice'; shortName = "Display Device"; }
      else if (topic === ENDPOINTS[2]) { targetKey = 'AcquisitionDevice'; shortName = "Acquisition Device"; }

      if (targetKey) {
        setMessages(prev => ({
          ...prev,  // Copy the state of other cards (the UI element holding most recent command) so that they don't disappear when updating data
          [targetKey]: { payload: payloadStr, time: timeStr }
        }));

        // --- Checks which endpoint message came from. Display device doesn't need to be parsed since it only sends status messages
        const isParsable = ['DeviceController', 'AcquisitionDevice'].includes(targetKey);
        let parsedValue = "--"
        if (isParsable){
          parsedValue = parseMessage(payloadStr)
        }

        // Update the logs
        setLogs(prevLogs => [
          {
            endpointKey: targetKey,
            name: shortName,
            payload: payloadStr,
            parsed: parsedValue,
            time: timeStr
          },
          ...prevLogs //Copies old rows in the logs so they are not overwritten
        ]);
      }
    });

    return () => {
      if (client) client.end(); //shut off the client
    };
  }, []);

  const handleCheckboxChange = (key) => {
    setActiveFilters(prev => ({ ...prev, [key]: !prev[key] })); //Flips filter
  };

  const filteredLogs = logs.filter(log => activeFilters[log.endpointKey]);  //Filters visible logs

  //--- HTML
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
          <div className="payload">{parseMessage(messages.DeviceController.payload)}</div>
          <div className="timestamp">{messages.DeviceController.time}</div>
        </div>

        <div className="card">
          <h3>Display Device</h3>
          <div className="topic-string">{ENDPOINTS[1]}</div>
          <div className="payload">{messages.DisplayDevice.payload}</div>
          <div className="timestamp">{messages.DisplayDevice.time}</div>
        </div>

        <div className="card">
          <h3>Acquisition Device</h3>
          <div className="topic-string">{ENDPOINTS[2]}</div>
          <div className="payload">{parseMessage(messages.AcquisitionDevice.payload)}</div>
          <div className="timestamp">{messages.AcquisitionDevice.time}</div>
        </div>
      </div>

      <div className="log-section">
        <h3>Historical Command Feed</h3>

        <div className="filter-checklist">
          <label>
            <input type="checkbox" checked={activeFilters.DeviceController} onChange={() => handleCheckboxChange('DeviceController')} />
            Device Controller
          </label>
          <label>
            <input type="checkbox" checked={activeFilters.DisplayDevice} onChange={() => handleCheckboxChange('DisplayDevice')} />
            Display Device
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

// Parse incoming messages from Device Controller & Acquisition Device
function parseMessage(rawMessage) {
  if (!rawMessage || typeof rawMessage !== 'string') return 'No Data';  //If no data or data in the wrong format is sent

  //Clean and standardize to upper
  const cleanStr = rawMessage.trim().toUpperCase();
  if (cleanStr.length < 2) return rawMessage;

  const controllerDict = {
    'A_pos1': 'Acquisition',  //pos1/2 maps to where in the string the letter appears
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

  // Parsing for Device Controller
  if (cleanStr.length === 3) {
    // Handle character 1
    let key1 = "--"
    if (cleanStr[0] === 'A'){ //If the first character is A map to A_pos1 so that the right A is set
      key1 = 'A_pos1';
    }
    else{
      key1 = cleanStr[0]; //Else - Map to whichever character is there
    }
    const d1 = controllerDict[key1] || cleanStr[0]; //Map to the right word

    // Handle character 2
    let key2 = "--"
    if (cleanStr[1] === 'A'){
      key2 = 'A_pos2';  //If second character is A, map to the correct A
    }
    else{
      key2 = cleanStr[1]; //Map to whichever character is there
    }
    const d2 = controllerDict[key2] || cleanStr[1]; //Map to the right word

    // Handle the third letter - if T, set directly to 'active' to avoid the issue we have with a
    let d3 = "--"
    if (cleanStr[2] === 'T'){
      d3 = 'Active';
    }
    else{
      d3 = controllerDict[cleanStr[2]] || cleanStr[2];
    }

    return `${d1} device | ${d2} | ${d3}`;  //Return the parsed strings
  }

  // Handle message from Acquisition device
  const firstChar = cleanStr.charAt(0);                   //Grab the first character
  
  //Ensure second character is numerical - prevent mapping status requests as Acquisition device messages
  if (isNaN(cleanStr.charAt(1)) && cleanStr.charAt(1) !== '-'){
    return rawMessage;  //If second character is not a number, return the raw message, likely a status response
  }
  if (['T', 'L', 'H', 'A', 'B'].includes(firstChar)) {    //Check if first character is a known command
    const sensorType = acquisitionDict[firstChar];        //Map to the dict
    const numericValue = rawMessage.trim().substring(1);  //Parse the numerical value
    return `${sensorType} | ${numericValue}`;             //Return the parsed sensor and value
  }

  return rawMessage; // Return the raw message if it cannot be parsed
}