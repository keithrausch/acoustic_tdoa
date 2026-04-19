import React, { useState, useEffect, useRef } from "react";
import { Canvas, useThree } from "@react-three/fiber";
import { OrbitControls } from "@react-three/drei";
import * as THREE from "three";
import * as proto from "./commands"; // Import your generated proto file
import { useWebSocket } from "./WebSocketContext"; // WebSocket logic
import LeftSidebar from "./sidebars/left_sidebar";  // Updated Left Sidebar with Tabs, Inputs, etc.
import RightSidebar from "./sidebars/right_sidebar";  // Right Sidebar for selected point info

const PointCloud = ({points, color, setSelectedPoint }) => {
  const { camera, size } = useThree(); // Get the camera from the Canvas context
  const pointsRef = useRef([]);

  // Handle mouse clicks and raycasting to detect which point was clicked
  const handleClick = (event) => {
    const clicked = event.intersections[0]; // R3F already did raycasting
    if (clicked) {
      const clickedPoint = clicked.object;
      setSelectedPoint({
        position: clickedPoint.position,
        color: clickedPoint.material.color.getHexString(),
      });
    } else {
      setSelectedPoint(null);
    }
  };

  const createPointMeshes = () => {
    return points.map((point, index) => (
      <mesh key={index} position={point} onClick={handleClick}>
        <sphereGeometry args={[0.1, 8, 8]} />
        <meshBasicMaterial color={color} />
      </mesh>
    ));
  };

  return <>{createPointMeshes()}</>;
};

const WebSocket3DPlot = () => {
  const { registerMessageHandler, unregisterMessageHandler } = useWebSocket(); // WebSocket hooks
  const [receiverPoints, setReceiverPoints] = useState([]); // Store the 3D points
  const [emitterPoints, setEmitterPoints] = useState([]); // Store the 3D points
  const [selectedPoint, setSelectedPoint] = useState(null); // State for the selected point

  // Handle incoming WebSocket messages
  useEffect(() => {
    const handleWebSocketMessage = (tlm_payload, topic_name) => {

      // Check if message is the type we expect, e.g., "point_cloud_payload"
      // if (msg.tlm === "full_state_estimate") {
        const truth_rcv_x = tlm_payload.true_receivers.x_coords;
        const truth_rcv_y = tlm_payload.true_receivers.y_coords;
        const truth_rcv_z = tlm_payload.true_receivers.z_coords;
        const newReceiverPoints = truth_rcv_x.map((xValue, index) => [xValue, truth_rcv_y[index], truth_rcv_z[index]]);
        setReceiverPoints(newReceiverPoints); // Update the state with new points

        const truth_emt_x = tlm_payload.true_emitters.x_coords;
        const truth_emt_y = tlm_payload.true_emitters.y_coords;
        const truth_emt_z = tlm_payload.true_emitters.z_coords;
        const newEmitterPoints = truth_emt_x.map((xValue, index) => [xValue, truth_emt_y[index], truth_emt_z[index]]);
        setEmitterPoints(newEmitterPoints); // Update the state with new points
      // }
    };

    // Register WebSocket message handler
    registerMessageHandler(handleWebSocketMessage, "commands.FullStateEstimate", proto.commands.FullStateEstimate);

    // Cleanup the WebSocket handler when the component is unmounted
    return () => {
      unregisterMessageHandler(handleWebSocketMessage);
    };
  }, [registerMessageHandler, unregisterMessageHandler]);

  return (
    <div style={{ display: "flex", height: "100vh" }}>
      {/* Left Sidebar */}
      <LeftSidebar />

      {/* 3D Canvas */}
      <div style={{ flex: 1, display: "flex", flexDirection: "column" }}>
        <Canvas style={{ flex: 1 }}>
          <OrbitControls />
          <ambientLight intensity={0.5} />
          <pointLight position={[10, 10, 10]} intensity={1} />
          <gridHelper args={[10, 10]} /> {/* 10x10 grid with each square of size 1 unit */}

          {/* Points */}
          <PointCloud points={receiverPoints} color={new THREE.Color("#1E90FF")} setSelectedPoint={setSelectedPoint} />
          <PointCloud points={emitterPoints} color={new THREE.Color("#ff1ea9")} setSelectedPoint={setSelectedPoint} />
        </Canvas>
      </div>

      {/* Right Sidebar */}
      <RightSidebar selectedPoint={selectedPoint} />
    </div>
  );
};

export default WebSocket3DPlot;