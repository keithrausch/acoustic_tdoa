import time
import numpy as np
from tdoa import tdoa_algos
from backend import ServerBase

class TDOAEmbedded():
    def __init__(self, clients, commands):
        self._clients = clients
        self._commands = commands
        self._tdoa = tdoa_algos.TDOASystemSVD

        self.true_receivers = np.zeros((3,1))
        self.true_emitters = np.zeros((3,1))
        self.est_receivers = np.zeros((3,1))
        self.est_emitters = np.zeros((3,1))

    @classmethod
    def create(cls, clients, commands):
        # create_task?
        return cls(clients, commands)

    @ServerBase.cmd('my_ping')
    def ping(cmd_header, cmd_payload, response_out):
        print('embedded ping!')
        print(f'header: {cmd_header}')
        print(f'payload: {cmd_payload}')
        response_out.success = True


    @ServerBase.cmd('start_mock_simulation')
    async def handle_start_mock_simulation(self, ws, cmd_header, cmd_payload, response_out):
        # print(cmd_payload)

        state = cmd_payload.full_state_estimate
        self.true_receivers = np.stack([state.true_receivers.x_coords, 
                                        state.true_receivers.y_coords, 
                                        state.true_receivers.z_coords], axis=0)
        self.true_emitters = np.stack([state.true_emitters.x_coords, 
                                        state.true_emitters.y_coords, 
                                        state.true_emitters.z_coords], axis=0)
        self.est_receivers = np.stack([state.est_receivers.x_coords, 
                                        state.est_receivers.y_coords, 
                                        state.est_receivers.z_coords], axis=0)
        self.est_emitters = np.stack([state.est_emitters.x_coords, 
                                        state.est_emitters.y_coords, 
                                        state.est_emitters.z_coords], axis=0)
        
        await self.send_full_state(ws)
        response_out.success = True

    async def send_full_state(self, ws):
        true_rcv_pts = ServerBase.commands_pb2.PointCloud()
        true_rcv_pts.x_coords.extend(self.true_receivers[0,:])  # Use extend to add elements to the repeated field
        true_rcv_pts.y_coords.extend(self.true_receivers[1,:])
        true_rcv_pts.z_coords.extend(self.true_receivers[2,:])

        true_emt_pts = ServerBase.commands_pb2.PointCloud()
        true_emt_pts.x_coords.extend(self.true_emitters[0,:])  # Use extend to add elements to the repeated field
        true_emt_pts.y_coords.extend(self.true_emitters[1,:])
        true_emt_pts.z_coords.extend(self.true_emitters[2,:])

        est_rcv_pts = ServerBase.commands_pb2.PointCloud()
        est_rcv_pts.x_coords.extend(self.est_receivers[0,:])  # Use extend to add elements to the repeated field
        est_rcv_pts.y_coords.extend(self.est_receivers[1,:])
        est_rcv_pts.z_coords.extend(self.est_receivers[2,:])

        est_emt_pts = ServerBase.commands_pb2.PointCloud()
        est_emt_pts.x_coords.extend(self.est_emitters[0,:])  # Use extend to add elements to the repeated field
        est_emt_pts.y_coords.extend(self.est_emitters[1,:])
        est_emt_pts.z_coords.extend(self.est_emitters[2,:])


        packet = ServerBase.commands_pb2.FullStateEstimate(true_receivers = true_rcv_pts, 
                                                           true_emitters = true_emt_pts, 
                                                           est_receivers = est_rcv_pts, 
                                                           est_emitters = est_emt_pts)

        await ServerBase.server.send_tlm(ws=ws, topic_name='lololol', tlm_payload=packet)
        
    async def broadcast_housekeeping(self):
        for ws in await self._clients.snap():
            await self.send_full_state(ws)
