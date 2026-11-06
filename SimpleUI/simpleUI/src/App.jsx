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
    BASE_TOPIC + "tkosatkina4510/AcquisitionDevice/Temperature"                               //Tatiana's endpoint
];

export default function App() {
    // --- Use states for the status messages
    const [status, setStatus] = useState({ text: 'Connecting...', class: 'connecting' });
    const [messages, setMessages] = useState({
        DeviceController: { payload: 'No data received yet', time: '--:--:--' },
        endpoint2: { payload: 'No data received yet', time: '--:--:--' },
        endpoint3: { payload: 'No data received yet', time: '--:--:--' },
    });

    // NEW STATES: Running list of historical logs & target checkbox filter settings
    const [logs, setLogs] = useState([]);
    const [activeFilters, setActiveFilters] = useState({
        DeviceController: true,
        endpoint2: true,
        endpoint3: true
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

        client.on('close', () => {  //Handle closing of server
            setStatus({ text: 'Disconnected', class: 'disconnected' });
        });

        client.on('message', (topic, message) => {  //Route incoming messages
            const payloadStr = message.toString();
            const timeStr = new Date().toLocaleTimeString();
            console.log(`[${topic}]: ${payloadStr}`);

            let targetKey = '';
            let shortName = ''; // Plain text descriptor for log entries

            if (topic === ENDPOINTS[0]) { targetKey = 'DeviceController'; shortName = "Aly's Endpoint"; }
            else if (topic === ENDPOINTS[1]) { targetKey = 'endpoint2'; shortName = "Kyle's Endpoint"; }
            else if (topic === ENDPOINTS[2]) { targetKey = 'endpoint3'; shortName = "Tatiana's Endpoint"; }

            if (targetKey) {
                // Keep the exact card functionality active
                setMessages(prev => ({
                    ...prev,
                    [targetKey]: { payload: payloadStr, time: timeStr }
                }));

                // Append the incoming event details to the top of our log array
                setLogs(prevLogs => [
                    { endpointKey: targetKey, name: shortName, payload: payloadStr, time: timeStr },
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
                    <h3>Endpoint 1</h3>
                    <div className="topic-string">{ENDPOINTS[0]}</div>
                    <div className="payload">{messages.DeviceController.payload}</div>
                    <div className="timestamp">{messages.DeviceController.time}</div>
                </div>

                <div className="card">
                    <h3>Endpoint 2</h3>
                    <div className="topic-string">{ENDPOINTS[1]}</div>
                    <div className="payload">{messages.endpoint2.payload}</div>
                    <div className="timestamp">{messages.endpoint2.time}</div>
                </div>

                <div className="card">
                    <h3>Endpoint 3</h3>
                    <div className="topic-string">{ENDPOINTS[2]}</div>
                    <div className="payload">{messages.endpoint3.payload}</div>
                    <div className="timestamp">{messages.endpoint3.time}</div>
                </div>
            </div>

            {/* Checklist Filters & Historical Table Container */}
            <div className="log-section">
                <h3>Historical Command Feed</h3>
                
                <div className="filter-checklist">
                    <label>
                        <input type="checkbox" checked={activeFilters.DeviceController} onChange={() => handleCheckboxChange('DeviceController')} />
                        Aly's Endpoint
                    </label>
                    <label>
                        <input type="checkbox" checked={activeFilters.endpoint2} onChange={() => handleCheckboxChange('endpoint2')} />
                        Kyle's Endpoint
                    </label>
                    <label>
                        <input type="checkbox" checked={activeFilters.endpoint3} onChange={() => handleCheckboxChange('endpoint3')} />
                        Tatiana's Endpoint
                    </label>
                </div>

                <div className="log-table-container">
                    <table className="log-table">
                        <thead>
                            <tr>
                                <th>Source</th>
                                <th>Message</th>
                                <th>Timestamp</th>
                            </tr>
                        </thead>
                        <tbody>
                            {filteredLogs.length === 0 ? (
                                <tr>
                                    <td colSpan="3" className="empty-log">No history found matching checked criteria...</td>
                                </tr>
                            ) : (
                                filteredLogs.map((log, idx) => (
                                    <tr key={idx} className={`row-${log.endpointKey}`}>
                                        <td className="log-name">{log.name}</td>
                                        <td className="log-payload">{log.payload}</td>
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
