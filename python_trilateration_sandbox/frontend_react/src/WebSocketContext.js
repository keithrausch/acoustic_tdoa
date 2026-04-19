import React, { createContext, useContext, useState, useEffect, useRef } from "react";
import * as proto from "./commands_pb";
import { Any } from "google-protobuf/google/protobuf/any_pb";
import { getMessageClass, getTypeUrl } from "./protobufRegistry";

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
        const msg = proto.TLM_Holder.deserializeBinary(bytes);
        const tlm_header = msg.getHeader();
        const tlm_payload_any = msg.getPayload();

        const typeUrl = tlm_payload_any.getTypeUrl();
        const messageClass = getMessageClass(typeUrl);

        let tlm_payload = null;
        if (messageClass) {
          try {
            tlm_payload = messageClass.deserializeBinary(tlm_payload_any.getValue());
          } catch (err) {
            console.error("Error unpacking message:", err);
          }
        } else {
          console.error(`Unknown message type: ${typeUrl}`);
        }

        if (messageClass === proto.CMD_Response) {
          const request_id = tlm_payload.getRequestId?.();
          if (pendingResponses.has(request_id)) {
            pendingResponses.get(request_id)(tlm_payload); // resolve promise
            pendingResponses.delete(request_id);
          } else {
            console.log('No matching request for', request_id);
          }
        }

        // const topic_type = tlm_header.getTopicType();
        const topic_type = messageClass.constructor;
        const topic_name = tlm_header.getTopicName();

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
      socketRef.current?.close(1000, "Provider unmounted");
    };
  }, []);

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

    const cmd_header = new proto.CMD_Header();
    cmd_header.setRequestId(request_id);
    cmd_header.setObjName(obj_name);
    cmd_header.setCmdName(cmd_name);

    const anyPayload = new Any();

    const payloadBytes = cmd_payload.serializeBinary();

    anyPayload.setValue(payloadBytes);
    anyPayload.setTypeUrl(getTypeUrl(cmd_payload.constructor));

    const cmd_holder = new proto.CMD_Holder();
    cmd_holder.setHeader(cmd_header);
    cmd_holder.setPayload(anyPayload);

    socketRef.current.send(cmd_holder.serializeBinary());

    return new Promise((resolve) => {
      pendingResponses.set(request_id, resolve);
    });
  };

  const registerMessageHandler = (handler, pb_class, topic_names = []) => {
    const topic_type = pb_class.constructor;
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