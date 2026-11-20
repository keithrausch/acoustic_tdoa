
import React from "react";
import { useWebSocket } from "./WebSocketContext";
import { FaCircle, FaPlay, FaHistory } from "react-icons/fa";

const TopBanner = ({ mode, onModeToggle }) => {
  const { connected, connectWebSocket, disconnectWebSocket } = useWebSocket();

  const toggleConnect = () => {
    if (connected) disconnectWebSocket();
    else connectWebSocket();
  };

  return (
    <div style={styles.banner}>
      <div style={styles.leftSection}>
        <FaCircle style={{ color: connected ? "limegreen" : "red", marginRight: 8 }} />
        <span>{connected ? "Connected" : "Disconnected"}</span>
      </div>

      <div style={styles.rightSection}>
        <button onClick={onModeToggle} style={styles.modeButton}>
          {mode === "Live" ? <FaPlay /> : <FaHistory />} {mode}
        </button>

        <button onClick={toggleConnect} style={styles.connectButton}>
          {connected ? "Disconnect" : "Connect"}
        </button>
      </div>
    </div>
  );
};

const styles = {
  banner: {
    display: "flex",
    justifyContent: "space-between",
    alignItems: "center",
    padding: "10px 20px",
    backgroundColor: "#1f2937",
    color: "#f9fafb",
    borderBottom: "2px solid #3b82f6",
    position: "sticky",
    top: 0,
    zIndex: 1000,
    boxShadow: "0 2px 5px rgba(0,0,0,0.2)",
    fontFamily: "Arial, sans-serif",
  },
  leftSection: {
    display: "flex",
    alignItems: "center",
    fontWeight: "bold",
  },
  rightSection: {
    display: "flex",
    alignItems: "center",
    gap: "12px",
  },
  connectButton: {
    padding: "6px 16px",
    backgroundColor: "#3b82f6",
    border: "none",
    borderRadius: 4,
    color: "#fff",
    fontWeight: "bold",
    cursor: "pointer",
    transition: "background 0.2s",
  },
  modeButton: {
    display: "flex",
    alignItems: "center",
    gap: 6,
    padding: "6px 12px",
    backgroundColor: "#10b981",
    border: "none",
    borderRadius: 4,
    color: "#fff",
    fontWeight: "bold",
    cursor: "pointer",
    transition: "background 0.2s",
  },
};

export default TopBanner;