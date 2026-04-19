import inspect
import asyncio

def cmd(cmd_name):
    def decorator(func):
        def wrapper(*args, **kwargs):
            result = func(*args, **kwargs)
            return result
        wrapper._is_cmd = True  # create tags
        wrapper._cmd_name = cmd_name  # create tags
        wrapper._original_signature = inspect.signature(func)
        return wrapper
    return decorator

class Commands():
    def __init__(self):
        self._commands = {} # todo type hints
        self._lock = asyncio.Lock()

    async def registered(self):
        async with self._lock:
            return self._commands

    @staticmethod
    def _is_string_sanitary(txt, forbidden=[' ', '-', '.', '@']):
        return not any(char in txt for char in forbidden)

    async def register(self, obj, obj_name):
        assert(self._is_string_sanitary(obj_name))
        for name in dir(obj):
            attr = getattr(obj, name)
            # Check if it's a method and has your tag
            is_cmd =getattr(attr, "_is_cmd", False)
            cmd_name = getattr(attr, "_cmd_name", "")
            if callable(attr) and is_cmd and cmd_name:
                assert(self._is_string_sanitary(cmd_name))

                # check the function takes the required named arguments
                sig_params = attr._original_signature.parameters
                def check_if_func_takes_named_argument(arg_name):
                    takes_cmd_header = any(p.name == arg_name for p in sig_params.values())
                    if not takes_cmd_header:
                        raise Exception(f'object by the name of \"{obj_name}\" is trying to register a command by the name \"{cmd_name}\" which does not take an argument with the name \"{arg_name}\"')
                check_if_func_takes_named_argument('cmd_header')
                check_if_func_takes_named_argument('cmd_payload')
                check_if_func_takes_named_argument('response_out')

                print(f"Registering command {obj_name}.{cmd_name}")

                async with self._lock:
                    if obj_name not in self._commands:
                        self._commands[obj_name] = {}
                    self._commands[obj_name][cmd_name] = attr

    async def get_handle(self, obj_name, cmd_name):
        async with self._lock:
            if obj_name not in self._commands:
                return None
            if cmd_name not in self._commands[obj_name]:
                return None
            return self._commands[obj_name][cmd_name]

