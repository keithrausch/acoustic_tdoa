import asyncio
from aiohttp import web
import time
import numpy as np
from google.protobuf.any_pb2 import Any
from ..generated.proto.shared import commands_pb2
from .clients import Clients
from .commands import Commands

async def send_tlm(ws, topic_name, tlm_payload, topic_type = None, time_ns = None):
    if time_ns is None:
        time_ns = time.time_ns()
    
    if topic_type is None:
        topic_type = tlm_payload.DESCRIPTOR.name

    # stuff results into a telemetry message and send
    tlm_header = commands_pb2.TLM_Header(time_ns = time_ns, topic_type = topic_type, topic_name = topic_name)
    
    msg = commands_pb2.TLM_Holder(header = tlm_header)
    msg.payload.Pack(tlm_payload)
    await ws.send_bytes(msg.SerializeToString())

async def forward_cmd(ws, cmd_header, cmd_payload, obj_name_override=None):
    msg = commands_pb2.CMD_Holder(header = cmd_header)
    if obj_name_override is not None:
        msg.header.obj_name = obj_name_override
    msg.payload.Pack(cmd_payload)
    await ws.send(msg.SerializeToString()) # not send_bytes???

class Server():
    def __init__(self, clients = Clients(), commands = Commands()):
        self._algos = {}
        self._runner = None
        self._clients = clients
        self._commands = commands
        pass

    async def register_algo(self, obj, obj_name):
        if obj_name in self._algos:
            raise Exception(f'object by the name of {obj_name} already exists')
        self._algos[obj_name] = obj
        await self._commands.register(obj, obj_name)
        return obj

    async def start(self, host = '0.0.0.0', port = 8080, root : str = None):

        app = web.Application()
        app.router.add_get("/ws", self._websocket_handler)

        if root:
            # app.add_routes([web.static('static', root)])

            async def index_handler(request):
                return web.FileResponse(root + 'index.html')
            app.router.add_get('/', index_handler)
            app.router.add_static('/', path=root, show_index=True)
            # app.add_routes([web.get('/', root)])

        # Attach startup and cleanup tasks
        # app.on_startup.append(on_startup)
        # app.on_cleanup.append(on_cleanup)
        # web.run_app(app, port=port)
        # Set up aiohttp using AppRunner (non-blocking)

        self._runner = web.AppRunner(app)
        await self._runner.setup()
        print(f'starting on port {port}')
        site = web.TCPSite(self._runner, host, port)
        await site.start()

    async def cleanup(self):
        self._runner.cleanup()

    async def _dispatch(self, ws, cmd_header, cmd_payload_any, response_out):

        # Get the typeUrl from the Any field (e.g., "types.googleapis.com/commands.AddTodoClass")
        type_url = cmd_payload_any.type_url

        # Extract the message name from the typeUrl (after the last '/')
        message_type_name = type_url.split('/')[-1]
        message_type_name = message_type_name.split('.')[-1]
        print(f"type_url: {type_url}, message_type_name: {message_type_name}, value: {cmd_payload_any.value}")

        # Dynamically fetch the message class based on the message type name
        # Find the message class in the generated code (commands_pb2 in this case)
        if not hasattr(commands_pb2, message_type_name):
            print(f'NO ATTRIBUTE: {message_type_name}')
            response_out.success = False
            response_out.message = f"Unknown message type: {message_type_name}"
            return
        
        message_class = getattr(commands_pb2, message_type_name)
        print(f'got message type: {message_type_name}')

        # Unpack the Any field into the message class
        cmd_payload = message_class()
        # success = cmd_payload_any.Unpack(cmd_payload)
        success = cmd_payload.ParseFromString(cmd_payload_any.value)

        if not success:
            print("FAIL")
        else:
            print("GOOD")


        # print(envelope)
        # print(which)
        handler = await self._commands.get_handle(cmd_header.obj_name, cmd_header.cmd_name)
        if handler:
            await handler(ws, cmd_header=cmd_header, cmd_payload=cmd_payload, response_out=response_out)
        else:
            response_out.success = False
            response_out.message = f"Unknown command - header: {cmd_header}, payload: {cmd_payload}"

    # --- WebSocket handler ---
    async def _websocket_handler(self, request):
        ws = web.WebSocketResponse()
        await ws.prepare(request)
        await self._clients.register(ws)

        async for msg in ws:
            time_ns = time.time_ns()
            if msg.type == web.WSMsgType.BINARY:
                envelope = commands_pb2.CMD_Holder()
                envelope.ParseFromString(msg.data)

                cmd_header = envelope.header
                cmd_payload = envelope.payload

                # set default response fields
                response_out = commands_pb2.CMD_Response(request_id=cmd_header.request_id,
                                                         success=False, 
                                                         message="")

                await self._dispatch(ws, cmd_header, cmd_payload, response_out)

                # stuff results into a telemetry message and send
                # tlm_header = commands_pb2.TLM_Header(time_ns = time_ns, topic_type = "", topic_name = "server")
                # tlm_payload = commands_pb2.TLM_Payload(cmd_response_payload = response_out)
                # response = commands_pb2.TLM_Holder(header = tlm_header, payload = tlm_payload)
                # await ws.send_bytes(response.SerializeToString())
                await send_tlm(ws,
                               topic_name="server", 
                               tlm_payload=response_out, 
                               time_ns=time_ns)

        await self._clients.unregister(ws)
        return ws