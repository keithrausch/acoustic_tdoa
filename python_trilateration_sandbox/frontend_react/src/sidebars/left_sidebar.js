import React, { useState } from "react";
import { Box, TextField, Button, Tabs, Tab, Typography } from "@mui/material";
import { DataGrid } from "@mui/x-data-grid";
import { useWebSocket } from "../WebSocketContext"; // WebSocket context
import * as proto from "../commands"; // Import your generated proto file

const LeftSidebar = () => {
  const { sendCommand, createMessage } = useWebSocket();
  const [tabIndex, setTabIndex] = useState(0);
  const [nReceivers, setNReceivers] = useState(3);
  const [nEmitters, setNEmitters] = useState(3);

  // Handle number of receivers or emitters change
  const handleChangeCount = (e, type) => {
    const count = parseInt(e.target.value, 10);
    if (type === "receivers") {
      setNReceivers(count);
      setReceivers(randomPositions(count)); // Randomize when count changes
    } else if (type === "emitters") {
      setNEmitters(count);
      setEmitters(randomPositions(count)); // Randomize when count changes
    }
  };

  const randomPositions = (count) =>
    Array.from({ length: count }, () => ({
      x: parseFloat((Math.random() * 10- 5).toFixed(3)),
      y: parseFloat((Math.random() * 10- 5).toFixed(3)),
      z: parseFloat((Math.random() * 10- 5).toFixed(3)),
    }));

  const [receivers, setReceivers] = useState(randomPositions(nReceivers));
  const [emitters, setEmitters] = useState(randomPositions(nEmitters));

  // Handle randomizing xyz positions for receivers and emitters
  const randomizePositions = (type) => {
    if (type === "receivers") {
      setReceivers(randomPositions(nReceivers));
    } else if (type === "emitters") {
      setEmitters(randomPositions(nEmitters));
    }
  };

  // Package coordinates into protobuf message and send via WebSocket
  const startMockCalibration = (receivers, emitters) => {
    const rcv_xs = receivers.map((pnt) => pnt.x);
    const rcs_ys = receivers.map((pnt) => pnt.y);
    const rcs_zs = receivers.map((pnt) => pnt.z);

    const emt_xs = emitters.map((pnt) => pnt.x);
    const emt_ys = emitters.map((pnt) => pnt.y);
    const emt_zs = emitters.map((pnt) => pnt.z);

    const rcv_cloud = proto.commands.PointCloud.create({ x_coords: rcv_xs, y_coords: rcs_ys, z_coords: rcs_zs });
    const emt_cloud = proto.commands.PointCloud.create({ x_coords: emt_xs, y_coords: emt_ys, z_coords: emt_zs });

    const full_state_estimate = proto.commands.FullStateEstimate.create({true_receivers: rcv_cloud, true_emitters: emt_cloud});

    // const msg = proto.commands.MockSimulation.create({
    //   full_state_estimate: full_state_estimate,
    // });
    // msg.my_type_url = "MockSimulation"

    const msg = createMessage("commands.MockSimulation", proto.commands.MockSimulation, {
      full_state_estimate: full_state_estimate,
    });

    sendCommand("tdoa_client", "start_mock_simulation", msg)
      .then((response) => {
        console.log("Positions sent successfully:", response);
      })
      .catch((error) => {
        console.error("Error sending positions:", error);
      });
  };

  // Custom row className for alternating row shading
  const getRowClassName = (params) => {
    return params.index % 2 === 0 ? 'even-row' : 'odd-row';
  };

  return (
    <Box sx={{ width: 300, padding: 2, backgroundColor: "#f4f4f4" }}>
      <Tabs value={tabIndex} onChange={(e, newValue) => setTabIndex(newValue)}>
        <Tab label="Receivers" />
        <Tab label="Emitters" />
      </Tabs>

      <Box sx={{ marginTop: 2 }}>
        {tabIndex === 0 ? (
          <>
            <TextField
              label="Number of Receivers"
              type="number"
              value={nReceivers}
              onChange={(e) => handleChangeCount(e, "receivers")}
              fullWidth
              sx={{ marginBottom: 2 }}
            />
            <DataGrid
              rows={receivers.map((pos, idx) => ({
                id: idx,
                x: pos.x,
                y: pos.y,
                z: pos.z,
              }))}
              columns={[
                { field: "x", headerName: "X", flex: 1 },
                { field: "y", headerName: "Y", flex: 1 },
                { field: "z", headerName: "Z", flex: 1 },
              ]}
              pageSize={5}
              disableSelectionOnClick
              rowHeight={30} // Make rows more compact
              getRowClassName={getRowClassName} // Apply alternating row shading
              sx={{
                '& .even-row': {
                  backgroundColor: '#f9f9f9', // Light gray for even rows
                },
                '& .odd-row': {
                  backgroundColor: '#ffffff', // White for odd rows
                },
              }}
            />
            <Button
              fullWidth
              variant="contained"
              onClick={() => randomizePositions("receivers")}
              sx={{ marginTop: 2 }}
            >
              Randomize Receiver Positions
            </Button>

            <TextField
              label="Number of Emitters"
              type="number"
              value={nEmitters}
              onChange={(e) => handleChangeCount(e, "emitters")}
              fullWidth
              sx={{ marginBottom: 2, marginTop: 2 }}
            />
            <DataGrid
              rows={emitters.map((pos, idx) => ({
                id: idx,
                x: pos.x,
                y: pos.y,
                z: pos.z,
              }))}
              columns={[
                { field: "x", headerName: "X", flex: 1 },
                { field: "y", headerName: "Y", flex: 1 },
                { field: "z", headerName: "Z", flex: 1 },
              ]}
              pageSize={5}
              disableSelectionOnClick
              rowHeight={30} // Make rows more compact
              getRowClassName={getRowClassName} // Apply alternating row shading
              sx={{
                '& .even-row': {
                  backgroundColor: '#f9f9f9', // Light gray for even rows
                },
                '& .odd-row': {
                  backgroundColor: '#ffffff', // White for odd rows
                },
              }}
            />
            <Button
              fullWidth
              variant="contained"
              onClick={() => randomizePositions("emitters")}
              sx={{ marginTop: 2 }}
            >
              Randomize Emitter Positions
            </Button>

            <Button
              fullWidth
              variant="contained"
              sx={{ marginTop: 2 }}
              onClick={() => startMockCalibration(receivers, emitters)}
            >
              Start Mock Simulation
            </Button>
          </>
        ) : (
          <>
            {/* Additional functionality for Emitters can be added here */}
          </>
        )}
      </Box>
    </Box>
  );
};

export default LeftSidebar;