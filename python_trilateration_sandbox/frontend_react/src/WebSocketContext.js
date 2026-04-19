// WebSocketContext.js
import React, { createContext, useContext, useState, useEffect, useRef } from "react";
import * as proto from "./commands"; // Import your generated proto file
import protobuf from "protobufjs"; // Import protobufjs
import commandsJson from "./commands.json";

let pendingResponses = new Map();
let messageHandlers = new Map();  // Store message handlers based on message type

// WebSocket context definition
const WebSocketContext = createContext(null);

// WebSocketProvider component
export const WebSocketProvider = ({ children }) => {
  const [connected, setConnected] = useState(false);
  const [ws, setWs] = useState(null);
  const socketRef = useRef(null);
  const reconnectAttempts = useRef(0);
  const maxReconnectDelay = 1000;
  const manualDisconnect = useRef(false);

  const connectWebSocket = () => {
    const socket = new WebSocket("ws://localhost:8080/ws");
    socket.binaryType = "arraybuffer";

    socket.onopen = () => {
      console.log("Connected to WebSocket");
      setConnected(true);
      reconnectAttempts.current = 0; // reset attempts on successful connection
      manualDisconnect.current = false; // clear flag if reconnecting succeeds
    };

    socket.onmessage = (event) => {
      if (event.data instanceof ArrayBuffer || event.data instanceof Blob) {
        const bytes = new Uint8Array(event.data);
        const msg = proto.commands.TLM_Holder.decode(bytes);
        const tlm_header = msg.header;
        const tlm_payload_any = msg.payload;

        const typeUrl = tlm_payload_any.type_url;
        const msg_type = typeUrl.split('/').pop().split('.').pop();
        const messageClass = proto.commands[msg_type];

        let tlm_payload;
        if (messageClass) {
          try {
            tlm_payload = messageClass.decode(tlm_payload_any.value);
          } catch (err) {
            console.error("Error unpacking message:", err);
          }
        } else {
          console.error(`Unknown message type: ${msg_type}`);
        }

        if (msg_type === proto.commands.CMD_Response.name) {
          const request_id = tlm_payload.request_id;
          if (pendingResponses.has(request_id)) {
            pendingResponses.get(request_id)(tlm_payload); // resolve promise
            pendingResponses.delete(request_id);
          } else {
            console.log('No matching request for', request_id);
          }
        }

        const topic_type = tlm_header.topic_type;
        const topic_name = tlm_header.topic_name;

        if (messageHandlers.has(topic_type)) {
          const pairs = messageHandlers.get(topic_type);
          pairs.forEach(([handler, topic_names]) => {
            if (topic_names.length === 0 || topic_names.includes(topic_name)) {
              handler(tlm_payload, topic_name);
            }
          });
        }

      } else if (typeof event.data === "string") {
        console.log("Text message received:", event.data);
      } else {
        console.log("Unknown message type");
      }
    };

    socket.onclose = (event) => {
      console.log(`WebSocket closed (code: ${event.code}). Attempting to reconnect...`);
      setConnected(false);

      // Only auto-reconnect if not manually disconnected
      if (!manualDisconnect.current) {
        attemptReconnect();
      } else {
        // Reset the flag for future connections
        manualDisconnect.current = false;
      }
    };

    socket.onerror = (err) => {
      console.error("WebSocket error:", err);
      socket.close();
      setConnected(false);
    };

    socketRef.current = socket;
    setWs(socket);
  };

  const disconnectWebSocket = () => {
  if (socketRef.current) {
    manualDisconnect.current = true;  // mark as intentional
    socketRef.current.close(1000, "Manual disconnect");
    socketRef.current = null;
    setConnected(false);
  }
};

  const attemptReconnect = () => {
    reconnectAttempts.current += 1;
    const delay = Math.min(1000 * 2 ** reconnectAttempts.current, maxReconnectDelay);
    console.log(`Reconnecting in ${delay / 1000}s...`);
    setTimeout(() => {
      connectWebSocket();
    }, delay);
  };

  useEffect(() => {
    connectWebSocket();

    return () => {
      if (socketRef.current) {
        socketRef.current.close(1000, "Provider unmounted");
      }
    };
  }, []);

  const getTypeUrl = (msg) => {
    if (!msg?.$type) throw new Error("Message is not a protobufjs type");
    return `types.googleapis.com/${msg.$type.fullName.slice(1)}`;
  };

  const createMessage = (typeName, Type, payload) => {
    // const root = protobuf.Root.fromJSON(commandsJson);
    // const Type = root.lookupType(typeName);
    // const msg = Type.create(payload);
    // const msg = Type.fromObject(payload);
    const msg = Type.create(payload);
    msg.my_type_url = typeName;
    return msg;
  };

  const sendCommand = (obj_name, cmd_name, cmd_payload) => {
    if (!socketRef.current || socketRef.current.readyState !== WebSocket.OPEN) {
      console.error("WebSocket is not connected. Cannot send command.");
      return Promise.reject(new Error("WebSocket not connected"));
    }

    const request_id = crypto.randomUUID();

    const cmd_header = proto.commands.CMD_Header.create({
      request_id,
      obj_name,
      cmd_name
    });

    const serializedPayload = cmd_payload.constructor.encode(cmd_payload).finish();
    // const typeUrl = `types.googleapis.com/${cmd_payload.constructor.name}`;
    // const typeUrl = `types.googleapis.com/${cmd_payload.$type.fullName.slice(1)}`;
    const typeUrl = cmd_payload.my_type_url;
    const anyPayload = proto.google.protobuf.Any.create({
      type_url: typeUrl,
      value: serializedPayload
    });

    const cmd_holder = proto.commands.CMD_Holder.create({
      header: cmd_header,
      payload: anyPayload
    });

    const buffer = proto.commands.CMD_Holder.encode(cmd_holder).finish();
    socketRef.current.send(buffer);

    return new Promise((resolve) => {
      pendingResponses.set(request_id, resolve);
    });
  };

  const registerMessageHandler = (handler, pb_class, topic_names = []) => {
    const topic_type = pb_class.name;
    if (!messageHandlers.has(topic_type)) {
      messageHandlers.set(topic_type, []);
    }
    messageHandlers.get(topic_type).push([handler, topic_names]);
  };

  const unregisterMessageHandler = (handler) => {
    messageHandlers.forEach((list, key) => {
      messageHandlers.set(key, list.filter(pair => pair[0] !== handler));
    });
  };

  return (
    <WebSocketContext.Provider value={{ 
      connected,
      connectWebSocket,
      disconnectWebSocket,
      sendCommand,
      createMessage,
      registerMessageHandler,
      unregisterMessageHandler
      }}>
      {children}
    </WebSocketContext.Provider>
  );
};

export const useWebSocket = () => {
  const context = useContext(WebSocketContext);
  if (!context) throw new Error("useWebSocket must be used within a WebSocketProvider");
  return context;
};