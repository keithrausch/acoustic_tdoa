import time
import numpy as np
import websockets
# from tdoa import tdoa_algos
from backend import ServerBase
import asyncio

class TDOAClient():
    def __init__(self, clients, commands):
        self._clients = clients
        self._commands = commands
        self._ws_to_embedded = None

    @classmethod
    def create(cls, clients, commands, embedded_address):
        instance = cls(clients, commands)
        asyncio.create_task(instance._connect_to_embedded(embedded_address))
        return instance

    async def _connect_to_embedded(self, uri):
        # Using 'async for' on connect() enables automatic reconnection logic
        while True:
            self._ws_to_embedded = None
            try:
                print(f'attempting to connect to {uri}')
                async with websockets.connect(uri) as websocket:
                    print('connected!')
                    self._ws_to_embedded = websocket
                    try:
                        # Process messages from the connection
                        async for msg in websocket:
                            # print(f"Received: {msg}")
                            for client in await self._clients.snap():
                                await client.send_bytes(msg)
                    except websockets.ConnectionClosed:
                        print("Connection lost, reconnecting...")
                    except Exception as e:
                        print(f"Unexpected error: {e}")
                    self._ws_to_embedded = None
            except (websockets.exceptions.InvalidStatus) as e:
                print(e)
            except ConnectionRefusedError as e:
                print(f"Connection refused: {e}")
            except Exception as e:
                print(f"Unexpected error: {e}")
            
            time.sleep(1)

    @ServerBase.cmd('my_ping')
    def ping(cmd_header, cmd_payload, response_out):
        print('client ping!')
        print(f'header: {cmd_header}')
        print(f'payload: {cmd_payload}')
        response_out.success = True


    @ServerBase.cmd('start_mock_simulation')
    async def handle_start_mock_simulation(self, ws, cmd_header, cmd_payload, response_out):
        if self._ws_to_embedded:
            print('self._ws_to_embedded forwarding')
            response_out.success = True
            await ServerBase.server.forward_cmd(ws=self._ws_to_embedded, 
                                                cmd_header=cmd_header, 
                                                cmd_payload=cmd_payload,
                                                obj_name_override='tdoa_embedded')
        else:
            print('self._ws_to_embedded not connected')
            response_out.success = False

    async def broadcast_housekeeping(self):
        pass
        # print('house keeping')
        # for client in await self._clients.snap():
        #     await self.send_full_state(client)
