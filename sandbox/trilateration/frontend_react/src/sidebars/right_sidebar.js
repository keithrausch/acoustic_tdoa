import React from "react";
import { Box, Typography } from "@mui/material";

const RightSidebar = ({ selectedPoint }) => (
  <Box sx={{ width: 300, background: "#f4f4f4", padding: 2 }}>
    <Typography variant="h6">Selected Point Info</Typography>
    {selectedPoint ? (
      <>
        <Typography>
          <strong>Position:</strong> ({selectedPoint.position.x}, {selectedPoint.position.y}, {selectedPoint.position.z})
        </Typography>
        <Typography>
          <strong>Color:</strong> #{selectedPoint.color}
        </Typography>
      </>
    ) : (
      <Typography>Click on a point to see its details.</Typography>
    )}
  </Box>
);

export default RightSidebar;