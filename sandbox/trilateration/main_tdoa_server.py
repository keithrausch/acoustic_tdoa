import asyncio
from backend.ServerBase import Server, Clients, Commands
from backend.specializations.tdoa_client import TDOAClient
from backend.specializations.tdoa_embedded import TDOAEmbedded

async def main():

    website_port = 8080
    embedded_port = 8081
    embedded_address = f"ws://127.0.0.1:{embedded_port}/ws"

    servers = []
    algos_to_poll = []

    # the main execution server
    web_clients = Clients()
    web_commands = Commands()
    web_server = Server(clients=web_clients, commands=web_commands)
    algos_to_poll.append(await web_server.register_algo(TDOAClient.create(clients=web_clients, 
                                                                          commands=web_commands, 
                                                                          embedded_address=embedded_address), 
                                                        'tdoa_client'))
    await web_server.start(port=website_port, root='frontend_react/build')
    servers.append(web_server)

    # the hardware device runs its own server. lets spoof it here
    embedded_clients = Clients()
    embedded_commands = Commands()
    embedded_server = Server(clients=embedded_clients, commands=embedded_commands)
    algos_to_poll.append(await embedded_server.register_algo(TDOAEmbedded.create(clients=embedded_clients, 
                                                                          commands=embedded_commands), 
                                                        'tdoa_embedded'))
    await embedded_server.start(port=embedded_port)
    servers.append(embedded_server)

    try:
        while True:
            await asyncio.sleep(1)
            for algo in algos_to_poll:
                await algo.broadcast_housekeeping()
    except asyncio.CancelledError:
        pass
    finally:
        for server in servers:
            await server.cleanup()

if __name__ == "__main__":

    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass