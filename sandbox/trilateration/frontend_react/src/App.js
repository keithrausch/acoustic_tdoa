// App.js
import React, { useState } from "react";
import { WebSocketProvider, useWebSocket } from "./WebSocketContext";
import WebSocket3DPlot from "./3dplot";
import TopBanner from "./TopBanner";

import { ThemeProvider, createTheme } from "@mui/material/styles";
import { Button, Typography, Box, Container } from "@mui/material";

// Create a MUI theme
const theme = createTheme({
  palette: {
    mode: "light", // you can switch to "dark" if you like
    primary: { main: "#1976d2" },
    secondary: { main: "#ac145a" },
  },
  typography: {
    h1: { fontSize: "2rem", fontWeight: 700 },
  },
});

function App() {
  const [connected, setConnected] = useState(false);
  const [mode, setMode] = useState("Live");

  const toggleConnect = () => setConnected(prev => !prev);
  const toggleMode = () => setMode(prev => (prev === "Live" ? "Replay" : "Live"));
  return (
    <ThemeProvider theme={theme}>
        <WebSocketProvider>
        <TopBanner
          connected={connected}
          onConnectToggle={toggleConnect}
          mode={mode}
          onModeToggle={toggleMode}
        />
        <div style={{ padding: "20px" }}>
          {/* <MainApp /> */}
          <WebSocket3DPlot />
        </div>
        </WebSocketProvider>
    </ThemeProvider>
  );
}

function MainApp() {
  const { sendCommand } = useWebSocket();

  const handlePing = async () => {
    const res = await sendCommand("ping_field", {});
    alert(res.message);
  };

  const handleAddTodo = async () => {
    const text = prompt("Enter todo text:");
    if (!text) return;

    const res = await sendCommand("add_todo_field", { text });
    alert(res.message);
  };

//   const handle_todo = (msg) => {
//       console.log("received todo", msg.add_todo_field);
//     };
// 
//   registerMessageHandler("add_todo_field", handle_cmd_response);

  return (
    <Container sx={{ paddingY: 4 }}>
      <Typography variant="h1" gutterBottom>
        Command Demo
      </Typography>

      <Box sx={{ display: "flex", gap: 2, flexDirection: "column", maxWidth: 300 }}>
        <Button variant="contained" color="primary" onClick={handlePing}>
          Ping Server
        </Button>

        <Button variant="outlined" color="secondary" onClick={handleAddTodo}>
          Add Todo
        </Button>
      </Box>
    </Container>
  );
}

export default App;