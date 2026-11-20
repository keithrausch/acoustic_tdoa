import * as proto from "./commands_pb";

// const TYPE_REGISTRY = new Map();

/**
 * Build registry automatically from generated protobuf namespace
 */
function buildRegistry(prefix = "type.googleapis.com") {
  const map = new Map();

  function walk(namespace, path = "") {
    Object.entries(namespace).forEach(([key, value]) => {
      if (!value) return;

      // protobuf message classes have this method
      if (typeof value === "function" && value.prototype?.serializeBinary) {
        const fullName = path + key;
        map.set(`${prefix}/${fullName}`, value);
      }

      // recurse into namespaces
      if (typeof value === "object") {
        walk(value, `${path}${key}.`);
      }
    });
  }

  walk(proto, "commands.");

  return map;
}

// build once at startup
const TYPE_REGISTRY = buildRegistry();
console.log(TYPE_REGISTRY)

export function getMessageClass(typeUrl) {
  return TYPE_REGISTRY.get(typeUrl);
}

export function getTypeUrl(protoClass) {
  // Find the key in TYPE_REGISTRY that matches this class
  for (const [fullName, cls] of TYPE_REGISTRY.entries()) {
    // console.log(cls)
    // console.log(protoClass)
    if (cls === protoClass) return fullName;
  }
  throw new Error('Class not registered in TYPE_REGISTRY');
}