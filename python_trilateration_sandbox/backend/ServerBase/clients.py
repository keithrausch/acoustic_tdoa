import asyncio

class Clients():
    def __init__(self):
        self._clients = set()
        self._lock = asyncio.Lock()

    async def register(self, client):
        async with self._lock:
            self._clients.add(client)

    async def unregister(self, client):
        async with self._lock:
            self._clients.remove(client)

    async def snap(self):
        async with self._lock:
            return self._clients.copy()