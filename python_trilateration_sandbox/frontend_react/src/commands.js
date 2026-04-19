/*eslint-disable block-scoped-var, id-length, no-control-regex, no-magic-numbers, no-prototype-builtins, no-redeclare, no-shadow, no-var, sort-vars*/
"use strict";

var $protobuf = require("protobufjs/minimal");

// Common aliases
var $Reader = $protobuf.Reader, $Writer = $protobuf.Writer, $util = $protobuf.util;

// Exported root namespace
var $root = $protobuf.roots["default"] || ($protobuf.roots["default"] = {});

$root.commands = (function() {

    /**
     * Namespace commands.
     * @exports commands
     * @namespace
     */
    var commands = {};

    commands.PointCloud = (function() {

        /**
         * Properties of a PointCloud.
         * @memberof commands
         * @interface IPointCloud
         * @property {Array.<number>|null} [x_coords] PointCloud x_coords
         * @property {Array.<number>|null} [y_coords] PointCloud y_coords
         * @property {Array.<number>|null} [z_coords] PointCloud z_coords
         */

        /**
         * Constructs a new PointCloud.
         * @memberof commands
         * @classdesc Represents a PointCloud.
         * @implements IPointCloud
         * @constructor
         * @param {commands.IPointCloud=} [properties] Properties to set
         */
        function PointCloud(properties) {
            this.x_coords = [];
            this.y_coords = [];
            this.z_coords = [];
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * PointCloud x_coords.
         * @member {Array.<number>} x_coords
         * @memberof commands.PointCloud
         * @instance
         */
        PointCloud.prototype.x_coords = $util.emptyArray;

        /**
         * PointCloud y_coords.
         * @member {Array.<number>} y_coords
         * @memberof commands.PointCloud
         * @instance
         */
        PointCloud.prototype.y_coords = $util.emptyArray;

        /**
         * PointCloud z_coords.
         * @member {Array.<number>} z_coords
         * @memberof commands.PointCloud
         * @instance
         */
        PointCloud.prototype.z_coords = $util.emptyArray;

        /**
         * Creates a new PointCloud instance using the specified properties.
         * @function create
         * @memberof commands.PointCloud
         * @static
         * @param {commands.IPointCloud=} [properties] Properties to set
         * @returns {commands.PointCloud} PointCloud instance
         */
        PointCloud.create = function create(properties) {
            return new PointCloud(properties);
        };

        /**
         * Encodes the specified PointCloud message. Does not implicitly {@link commands.PointCloud.verify|verify} messages.
         * @function encode
         * @memberof commands.PointCloud
         * @static
         * @param {commands.IPointCloud} message PointCloud message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        PointCloud.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.x_coords != null && message.x_coords.length) {
                writer.uint32(/* id 1, wireType 2 =*/10).fork();
                for (var i = 0; i < message.x_coords.length; ++i)
                    writer.float(message.x_coords[i]);
                writer.ldelim();
            }
            if (message.y_coords != null && message.y_coords.length) {
                writer.uint32(/* id 2, wireType 2 =*/18).fork();
                for (var i = 0; i < message.y_coords.length; ++i)
                    writer.float(message.y_coords[i]);
                writer.ldelim();
            }
            if (message.z_coords != null && message.z_coords.length) {
                writer.uint32(/* id 3, wireType 2 =*/26).fork();
                for (var i = 0; i < message.z_coords.length; ++i)
                    writer.float(message.z_coords[i]);
                writer.ldelim();
            }
            return writer;
        };

        /**
         * Encodes the specified PointCloud message, length delimited. Does not implicitly {@link commands.PointCloud.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.PointCloud
         * @static
         * @param {commands.IPointCloud} message PointCloud message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        PointCloud.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a PointCloud message from the specified reader or buffer.
         * @function decode
         * @memberof commands.PointCloud
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.PointCloud} PointCloud
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        PointCloud.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.PointCloud();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        if (!(message.x_coords && message.x_coords.length))
                            message.x_coords = [];
                        if ((tag & 7) === 2) {
                            var end2 = reader.uint32() + reader.pos;
                            while (reader.pos < end2)
                                message.x_coords.push(reader.float());
                        } else
                            message.x_coords.push(reader.float());
                        break;
                    }
                case 2: {
                        if (!(message.y_coords && message.y_coords.length))
                            message.y_coords = [];
                        if ((tag & 7) === 2) {
                            var end2 = reader.uint32() + reader.pos;
                            while (reader.pos < end2)
                                message.y_coords.push(reader.float());
                        } else
                            message.y_coords.push(reader.float());
                        break;
                    }
                case 3: {
                        if (!(message.z_coords && message.z_coords.length))
                            message.z_coords = [];
                        if ((tag & 7) === 2) {
                            var end2 = reader.uint32() + reader.pos;
                            while (reader.pos < end2)
                                message.z_coords.push(reader.float());
                        } else
                            message.z_coords.push(reader.float());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a PointCloud message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.PointCloud
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.PointCloud} PointCloud
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        PointCloud.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a PointCloud message.
         * @function verify
         * @memberof commands.PointCloud
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        PointCloud.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.x_coords != null && message.hasOwnProperty("x_coords")) {
                if (!Array.isArray(message.x_coords))
                    return "x_coords: array expected";
                for (var i = 0; i < message.x_coords.length; ++i)
                    if (typeof message.x_coords[i] !== "number")
                        return "x_coords: number[] expected";
            }
            if (message.y_coords != null && message.hasOwnProperty("y_coords")) {
                if (!Array.isArray(message.y_coords))
                    return "y_coords: array expected";
                for (var i = 0; i < message.y_coords.length; ++i)
                    if (typeof message.y_coords[i] !== "number")
                        return "y_coords: number[] expected";
            }
            if (message.z_coords != null && message.hasOwnProperty("z_coords")) {
                if (!Array.isArray(message.z_coords))
                    return "z_coords: array expected";
                for (var i = 0; i < message.z_coords.length; ++i)
                    if (typeof message.z_coords[i] !== "number")
                        return "z_coords: number[] expected";
            }
            return null;
        };

        /**
         * Creates a PointCloud message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.PointCloud
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.PointCloud} PointCloud
         */
        PointCloud.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.PointCloud)
                return object;
            var message = new $root.commands.PointCloud();
            if (object.x_coords) {
                if (!Array.isArray(object.x_coords))
                    throw TypeError(".commands.PointCloud.x_coords: array expected");
                message.x_coords = [];
                for (var i = 0; i < object.x_coords.length; ++i)
                    message.x_coords[i] = Number(object.x_coords[i]);
            }
            if (object.y_coords) {
                if (!Array.isArray(object.y_coords))
                    throw TypeError(".commands.PointCloud.y_coords: array expected");
                message.y_coords = [];
                for (var i = 0; i < object.y_coords.length; ++i)
                    message.y_coords[i] = Number(object.y_coords[i]);
            }
            if (object.z_coords) {
                if (!Array.isArray(object.z_coords))
                    throw TypeError(".commands.PointCloud.z_coords: array expected");
                message.z_coords = [];
                for (var i = 0; i < object.z_coords.length; ++i)
                    message.z_coords[i] = Number(object.z_coords[i]);
            }
            return message;
        };

        /**
         * Creates a plain object from a PointCloud message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.PointCloud
         * @static
         * @param {commands.PointCloud} message PointCloud
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        PointCloud.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.arrays || options.defaults) {
                object.x_coords = [];
                object.y_coords = [];
                object.z_coords = [];
            }
            if (message.x_coords && message.x_coords.length) {
                object.x_coords = [];
                for (var j = 0; j < message.x_coords.length; ++j)
                    object.x_coords[j] = options.json && !isFinite(message.x_coords[j]) ? String(message.x_coords[j]) : message.x_coords[j];
            }
            if (message.y_coords && message.y_coords.length) {
                object.y_coords = [];
                for (var j = 0; j < message.y_coords.length; ++j)
                    object.y_coords[j] = options.json && !isFinite(message.y_coords[j]) ? String(message.y_coords[j]) : message.y_coords[j];
            }
            if (message.z_coords && message.z_coords.length) {
                object.z_coords = [];
                for (var j = 0; j < message.z_coords.length; ++j)
                    object.z_coords[j] = options.json && !isFinite(message.z_coords[j]) ? String(message.z_coords[j]) : message.z_coords[j];
            }
            return object;
        };

        /**
         * Converts this PointCloud to JSON.
         * @function toJSON
         * @memberof commands.PointCloud
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        PointCloud.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for PointCloud
         * @function getTypeUrl
         * @memberof commands.PointCloud
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        PointCloud.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.PointCloud";
        };

        return PointCloud;
    })();

    commands.AddTodoClass = (function() {

        /**
         * Properties of an AddTodoClass.
         * @memberof commands
         * @interface IAddTodoClass
         * @property {string|null} [text] AddTodoClass text
         */

        /**
         * Constructs a new AddTodoClass.
         * @memberof commands
         * @classdesc Represents an AddTodoClass.
         * @implements IAddTodoClass
         * @constructor
         * @param {commands.IAddTodoClass=} [properties] Properties to set
         */
        function AddTodoClass(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * AddTodoClass text.
         * @member {string} text
         * @memberof commands.AddTodoClass
         * @instance
         */
        AddTodoClass.prototype.text = "";

        /**
         * Creates a new AddTodoClass instance using the specified properties.
         * @function create
         * @memberof commands.AddTodoClass
         * @static
         * @param {commands.IAddTodoClass=} [properties] Properties to set
         * @returns {commands.AddTodoClass} AddTodoClass instance
         */
        AddTodoClass.create = function create(properties) {
            return new AddTodoClass(properties);
        };

        /**
         * Encodes the specified AddTodoClass message. Does not implicitly {@link commands.AddTodoClass.verify|verify} messages.
         * @function encode
         * @memberof commands.AddTodoClass
         * @static
         * @param {commands.IAddTodoClass} message AddTodoClass message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        AddTodoClass.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.text != null && Object.hasOwnProperty.call(message, "text"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.text);
            return writer;
        };

        /**
         * Encodes the specified AddTodoClass message, length delimited. Does not implicitly {@link commands.AddTodoClass.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.AddTodoClass
         * @static
         * @param {commands.IAddTodoClass} message AddTodoClass message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        AddTodoClass.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes an AddTodoClass message from the specified reader or buffer.
         * @function decode
         * @memberof commands.AddTodoClass
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.AddTodoClass} AddTodoClass
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        AddTodoClass.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.AddTodoClass();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.text = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes an AddTodoClass message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.AddTodoClass
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.AddTodoClass} AddTodoClass
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        AddTodoClass.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies an AddTodoClass message.
         * @function verify
         * @memberof commands.AddTodoClass
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        AddTodoClass.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.text != null && message.hasOwnProperty("text"))
                if (!$util.isString(message.text))
                    return "text: string expected";
            return null;
        };

        /**
         * Creates an AddTodoClass message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.AddTodoClass
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.AddTodoClass} AddTodoClass
         */
        AddTodoClass.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.AddTodoClass)
                return object;
            var message = new $root.commands.AddTodoClass();
            if (object.text != null)
                message.text = String(object.text);
            return message;
        };

        /**
         * Creates a plain object from an AddTodoClass message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.AddTodoClass
         * @static
         * @param {commands.AddTodoClass} message AddTodoClass
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        AddTodoClass.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults)
                object.text = "";
            if (message.text != null && message.hasOwnProperty("text"))
                object.text = message.text;
            return object;
        };

        /**
         * Converts this AddTodoClass to JSON.
         * @function toJSON
         * @memberof commands.AddTodoClass
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        AddTodoClass.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for AddTodoClass
         * @function getTypeUrl
         * @memberof commands.AddTodoClass
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        AddTodoClass.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.AddTodoClass";
        };

        return AddTodoClass;
    })();

    commands.PingClass = (function() {

        /**
         * Properties of a PingClass.
         * @memberof commands
         * @interface IPingClass
         */

        /**
         * Constructs a new PingClass.
         * @memberof commands
         * @classdesc Represents a PingClass.
         * @implements IPingClass
         * @constructor
         * @param {commands.IPingClass=} [properties] Properties to set
         */
        function PingClass(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * Creates a new PingClass instance using the specified properties.
         * @function create
         * @memberof commands.PingClass
         * @static
         * @param {commands.IPingClass=} [properties] Properties to set
         * @returns {commands.PingClass} PingClass instance
         */
        PingClass.create = function create(properties) {
            return new PingClass(properties);
        };

        /**
         * Encodes the specified PingClass message. Does not implicitly {@link commands.PingClass.verify|verify} messages.
         * @function encode
         * @memberof commands.PingClass
         * @static
         * @param {commands.IPingClass} message PingClass message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        PingClass.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            return writer;
        };

        /**
         * Encodes the specified PingClass message, length delimited. Does not implicitly {@link commands.PingClass.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.PingClass
         * @static
         * @param {commands.IPingClass} message PingClass message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        PingClass.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a PingClass message from the specified reader or buffer.
         * @function decode
         * @memberof commands.PingClass
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.PingClass} PingClass
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        PingClass.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.PingClass();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a PingClass message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.PingClass
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.PingClass} PingClass
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        PingClass.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a PingClass message.
         * @function verify
         * @memberof commands.PingClass
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        PingClass.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            return null;
        };

        /**
         * Creates a PingClass message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.PingClass
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.PingClass} PingClass
         */
        PingClass.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.PingClass)
                return object;
            return new $root.commands.PingClass();
        };

        /**
         * Creates a plain object from a PingClass message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.PingClass
         * @static
         * @param {commands.PingClass} message PingClass
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        PingClass.toObject = function toObject() {
            return {};
        };

        /**
         * Converts this PingClass to JSON.
         * @function toJSON
         * @memberof commands.PingClass
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        PingClass.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for PingClass
         * @function getTypeUrl
         * @memberof commands.PingClass
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        PingClass.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.PingClass";
        };

        return PingClass;
    })();

    commands.FullStateEstimate = (function() {

        /**
         * Properties of a FullStateEstimate.
         * @memberof commands
         * @interface IFullStateEstimate
         * @property {commands.IPointCloud|null} [true_receivers] FullStateEstimate true_receivers
         * @property {commands.IPointCloud|null} [true_emitters] FullStateEstimate true_emitters
         * @property {commands.IPointCloud|null} [est_receivers] FullStateEstimate est_receivers
         * @property {commands.IPointCloud|null} [est_emitters] FullStateEstimate est_emitters
         */

        /**
         * Constructs a new FullStateEstimate.
         * @memberof commands
         * @classdesc Represents a FullStateEstimate.
         * @implements IFullStateEstimate
         * @constructor
         * @param {commands.IFullStateEstimate=} [properties] Properties to set
         */
        function FullStateEstimate(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * FullStateEstimate true_receivers.
         * @member {commands.IPointCloud|null|undefined} true_receivers
         * @memberof commands.FullStateEstimate
         * @instance
         */
        FullStateEstimate.prototype.true_receivers = null;

        /**
         * FullStateEstimate true_emitters.
         * @member {commands.IPointCloud|null|undefined} true_emitters
         * @memberof commands.FullStateEstimate
         * @instance
         */
        FullStateEstimate.prototype.true_emitters = null;

        /**
         * FullStateEstimate est_receivers.
         * @member {commands.IPointCloud|null|undefined} est_receivers
         * @memberof commands.FullStateEstimate
         * @instance
         */
        FullStateEstimate.prototype.est_receivers = null;

        /**
         * FullStateEstimate est_emitters.
         * @member {commands.IPointCloud|null|undefined} est_emitters
         * @memberof commands.FullStateEstimate
         * @instance
         */
        FullStateEstimate.prototype.est_emitters = null;

        /**
         * Creates a new FullStateEstimate instance using the specified properties.
         * @function create
         * @memberof commands.FullStateEstimate
         * @static
         * @param {commands.IFullStateEstimate=} [properties] Properties to set
         * @returns {commands.FullStateEstimate} FullStateEstimate instance
         */
        FullStateEstimate.create = function create(properties) {
            return new FullStateEstimate(properties);
        };

        /**
         * Encodes the specified FullStateEstimate message. Does not implicitly {@link commands.FullStateEstimate.verify|verify} messages.
         * @function encode
         * @memberof commands.FullStateEstimate
         * @static
         * @param {commands.IFullStateEstimate} message FullStateEstimate message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        FullStateEstimate.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.true_receivers != null && Object.hasOwnProperty.call(message, "true_receivers"))
                $root.commands.PointCloud.encode(message.true_receivers, writer.uint32(/* id 1, wireType 2 =*/10).fork()).ldelim();
            if (message.true_emitters != null && Object.hasOwnProperty.call(message, "true_emitters"))
                $root.commands.PointCloud.encode(message.true_emitters, writer.uint32(/* id 2, wireType 2 =*/18).fork()).ldelim();
            if (message.est_receivers != null && Object.hasOwnProperty.call(message, "est_receivers"))
                $root.commands.PointCloud.encode(message.est_receivers, writer.uint32(/* id 3, wireType 2 =*/26).fork()).ldelim();
            if (message.est_emitters != null && Object.hasOwnProperty.call(message, "est_emitters"))
                $root.commands.PointCloud.encode(message.est_emitters, writer.uint32(/* id 4, wireType 2 =*/34).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified FullStateEstimate message, length delimited. Does not implicitly {@link commands.FullStateEstimate.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.FullStateEstimate
         * @static
         * @param {commands.IFullStateEstimate} message FullStateEstimate message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        FullStateEstimate.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a FullStateEstimate message from the specified reader or buffer.
         * @function decode
         * @memberof commands.FullStateEstimate
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.FullStateEstimate} FullStateEstimate
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        FullStateEstimate.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.FullStateEstimate();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.true_receivers = $root.commands.PointCloud.decode(reader, reader.uint32());
                        break;
                    }
                case 2: {
                        message.true_emitters = $root.commands.PointCloud.decode(reader, reader.uint32());
                        break;
                    }
                case 3: {
                        message.est_receivers = $root.commands.PointCloud.decode(reader, reader.uint32());
                        break;
                    }
                case 4: {
                        message.est_emitters = $root.commands.PointCloud.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a FullStateEstimate message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.FullStateEstimate
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.FullStateEstimate} FullStateEstimate
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        FullStateEstimate.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a FullStateEstimate message.
         * @function verify
         * @memberof commands.FullStateEstimate
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        FullStateEstimate.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.true_receivers != null && message.hasOwnProperty("true_receivers")) {
                var error = $root.commands.PointCloud.verify(message.true_receivers);
                if (error)
                    return "true_receivers." + error;
            }
            if (message.true_emitters != null && message.hasOwnProperty("true_emitters")) {
                var error = $root.commands.PointCloud.verify(message.true_emitters);
                if (error)
                    return "true_emitters." + error;
            }
            if (message.est_receivers != null && message.hasOwnProperty("est_receivers")) {
                var error = $root.commands.PointCloud.verify(message.est_receivers);
                if (error)
                    return "est_receivers." + error;
            }
            if (message.est_emitters != null && message.hasOwnProperty("est_emitters")) {
                var error = $root.commands.PointCloud.verify(message.est_emitters);
                if (error)
                    return "est_emitters." + error;
            }
            return null;
        };

        /**
         * Creates a FullStateEstimate message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.FullStateEstimate
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.FullStateEstimate} FullStateEstimate
         */
        FullStateEstimate.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.FullStateEstimate)
                return object;
            var message = new $root.commands.FullStateEstimate();
            if (object.true_receivers != null) {
                if (typeof object.true_receivers !== "object")
                    throw TypeError(".commands.FullStateEstimate.true_receivers: object expected");
                message.true_receivers = $root.commands.PointCloud.fromObject(object.true_receivers);
            }
            if (object.true_emitters != null) {
                if (typeof object.true_emitters !== "object")
                    throw TypeError(".commands.FullStateEstimate.true_emitters: object expected");
                message.true_emitters = $root.commands.PointCloud.fromObject(object.true_emitters);
            }
            if (object.est_receivers != null) {
                if (typeof object.est_receivers !== "object")
                    throw TypeError(".commands.FullStateEstimate.est_receivers: object expected");
                message.est_receivers = $root.commands.PointCloud.fromObject(object.est_receivers);
            }
            if (object.est_emitters != null) {
                if (typeof object.est_emitters !== "object")
                    throw TypeError(".commands.FullStateEstimate.est_emitters: object expected");
                message.est_emitters = $root.commands.PointCloud.fromObject(object.est_emitters);
            }
            return message;
        };

        /**
         * Creates a plain object from a FullStateEstimate message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.FullStateEstimate
         * @static
         * @param {commands.FullStateEstimate} message FullStateEstimate
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        FullStateEstimate.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.true_receivers = null;
                object.true_emitters = null;
                object.est_receivers = null;
                object.est_emitters = null;
            }
            if (message.true_receivers != null && message.hasOwnProperty("true_receivers"))
                object.true_receivers = $root.commands.PointCloud.toObject(message.true_receivers, options);
            if (message.true_emitters != null && message.hasOwnProperty("true_emitters"))
                object.true_emitters = $root.commands.PointCloud.toObject(message.true_emitters, options);
            if (message.est_receivers != null && message.hasOwnProperty("est_receivers"))
                object.est_receivers = $root.commands.PointCloud.toObject(message.est_receivers, options);
            if (message.est_emitters != null && message.hasOwnProperty("est_emitters"))
                object.est_emitters = $root.commands.PointCloud.toObject(message.est_emitters, options);
            return object;
        };

        /**
         * Converts this FullStateEstimate to JSON.
         * @function toJSON
         * @memberof commands.FullStateEstimate
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        FullStateEstimate.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for FullStateEstimate
         * @function getTypeUrl
         * @memberof commands.FullStateEstimate
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        FullStateEstimate.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.FullStateEstimate";
        };

        return FullStateEstimate;
    })();

    commands.MockSimulation = (function() {

        /**
         * Properties of a MockSimulation.
         * @memberof commands
         * @interface IMockSimulation
         * @property {commands.IFullStateEstimate|null} [full_state_estimate] MockSimulation full_state_estimate
         */

        /**
         * Constructs a new MockSimulation.
         * @memberof commands
         * @classdesc Represents a MockSimulation.
         * @implements IMockSimulation
         * @constructor
         * @param {commands.IMockSimulation=} [properties] Properties to set
         */
        function MockSimulation(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * MockSimulation full_state_estimate.
         * @member {commands.IFullStateEstimate|null|undefined} full_state_estimate
         * @memberof commands.MockSimulation
         * @instance
         */
        MockSimulation.prototype.full_state_estimate = null;

        /**
         * Creates a new MockSimulation instance using the specified properties.
         * @function create
         * @memberof commands.MockSimulation
         * @static
         * @param {commands.IMockSimulation=} [properties] Properties to set
         * @returns {commands.MockSimulation} MockSimulation instance
         */
        MockSimulation.create = function create(properties) {
            return new MockSimulation(properties);
        };

        /**
         * Encodes the specified MockSimulation message. Does not implicitly {@link commands.MockSimulation.verify|verify} messages.
         * @function encode
         * @memberof commands.MockSimulation
         * @static
         * @param {commands.IMockSimulation} message MockSimulation message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        MockSimulation.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.full_state_estimate != null && Object.hasOwnProperty.call(message, "full_state_estimate"))
                $root.commands.FullStateEstimate.encode(message.full_state_estimate, writer.uint32(/* id 1, wireType 2 =*/10).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified MockSimulation message, length delimited. Does not implicitly {@link commands.MockSimulation.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.MockSimulation
         * @static
         * @param {commands.IMockSimulation} message MockSimulation message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        MockSimulation.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a MockSimulation message from the specified reader or buffer.
         * @function decode
         * @memberof commands.MockSimulation
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.MockSimulation} MockSimulation
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        MockSimulation.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.MockSimulation();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.full_state_estimate = $root.commands.FullStateEstimate.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a MockSimulation message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.MockSimulation
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.MockSimulation} MockSimulation
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        MockSimulation.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a MockSimulation message.
         * @function verify
         * @memberof commands.MockSimulation
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        MockSimulation.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.full_state_estimate != null && message.hasOwnProperty("full_state_estimate")) {
                var error = $root.commands.FullStateEstimate.verify(message.full_state_estimate);
                if (error)
                    return "full_state_estimate." + error;
            }
            return null;
        };

        /**
         * Creates a MockSimulation message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.MockSimulation
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.MockSimulation} MockSimulation
         */
        MockSimulation.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.MockSimulation)
                return object;
            var message = new $root.commands.MockSimulation();
            if (object.full_state_estimate != null) {
                if (typeof object.full_state_estimate !== "object")
                    throw TypeError(".commands.MockSimulation.full_state_estimate: object expected");
                message.full_state_estimate = $root.commands.FullStateEstimate.fromObject(object.full_state_estimate);
            }
            return message;
        };

        /**
         * Creates a plain object from a MockSimulation message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.MockSimulation
         * @static
         * @param {commands.MockSimulation} message MockSimulation
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        MockSimulation.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults)
                object.full_state_estimate = null;
            if (message.full_state_estimate != null && message.hasOwnProperty("full_state_estimate"))
                object.full_state_estimate = $root.commands.FullStateEstimate.toObject(message.full_state_estimate, options);
            return object;
        };

        /**
         * Converts this MockSimulation to JSON.
         * @function toJSON
         * @memberof commands.MockSimulation
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        MockSimulation.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for MockSimulation
         * @function getTypeUrl
         * @memberof commands.MockSimulation
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        MockSimulation.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.MockSimulation";
        };

        return MockSimulation;
    })();

    commands.CMD_Header = (function() {

        /**
         * Properties of a CMD_Header.
         * @memberof commands
         * @interface ICMD_Header
         * @property {string|null} [request_id] CMD_Header request_id
         * @property {string|null} [obj_name] CMD_Header obj_name
         * @property {string|null} [cmd_name] CMD_Header cmd_name
         */

        /**
         * Constructs a new CMD_Header.
         * @memberof commands
         * @classdesc Represents a CMD_Header.
         * @implements ICMD_Header
         * @constructor
         * @param {commands.ICMD_Header=} [properties] Properties to set
         */
        function CMD_Header(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * CMD_Header request_id.
         * @member {string} request_id
         * @memberof commands.CMD_Header
         * @instance
         */
        CMD_Header.prototype.request_id = "";

        /**
         * CMD_Header obj_name.
         * @member {string} obj_name
         * @memberof commands.CMD_Header
         * @instance
         */
        CMD_Header.prototype.obj_name = "";

        /**
         * CMD_Header cmd_name.
         * @member {string} cmd_name
         * @memberof commands.CMD_Header
         * @instance
         */
        CMD_Header.prototype.cmd_name = "";

        /**
         * Creates a new CMD_Header instance using the specified properties.
         * @function create
         * @memberof commands.CMD_Header
         * @static
         * @param {commands.ICMD_Header=} [properties] Properties to set
         * @returns {commands.CMD_Header} CMD_Header instance
         */
        CMD_Header.create = function create(properties) {
            return new CMD_Header(properties);
        };

        /**
         * Encodes the specified CMD_Header message. Does not implicitly {@link commands.CMD_Header.verify|verify} messages.
         * @function encode
         * @memberof commands.CMD_Header
         * @static
         * @param {commands.ICMD_Header} message CMD_Header message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Header.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.request_id != null && Object.hasOwnProperty.call(message, "request_id"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.request_id);
            if (message.obj_name != null && Object.hasOwnProperty.call(message, "obj_name"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.obj_name);
            if (message.cmd_name != null && Object.hasOwnProperty.call(message, "cmd_name"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.cmd_name);
            return writer;
        };

        /**
         * Encodes the specified CMD_Header message, length delimited. Does not implicitly {@link commands.CMD_Header.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.CMD_Header
         * @static
         * @param {commands.ICMD_Header} message CMD_Header message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Header.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a CMD_Header message from the specified reader or buffer.
         * @function decode
         * @memberof commands.CMD_Header
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.CMD_Header} CMD_Header
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Header.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.CMD_Header();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.request_id = reader.string();
                        break;
                    }
                case 2: {
                        message.obj_name = reader.string();
                        break;
                    }
                case 3: {
                        message.cmd_name = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a CMD_Header message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.CMD_Header
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.CMD_Header} CMD_Header
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Header.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a CMD_Header message.
         * @function verify
         * @memberof commands.CMD_Header
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        CMD_Header.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.request_id != null && message.hasOwnProperty("request_id"))
                if (!$util.isString(message.request_id))
                    return "request_id: string expected";
            if (message.obj_name != null && message.hasOwnProperty("obj_name"))
                if (!$util.isString(message.obj_name))
                    return "obj_name: string expected";
            if (message.cmd_name != null && message.hasOwnProperty("cmd_name"))
                if (!$util.isString(message.cmd_name))
                    return "cmd_name: string expected";
            return null;
        };

        /**
         * Creates a CMD_Header message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.CMD_Header
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.CMD_Header} CMD_Header
         */
        CMD_Header.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.CMD_Header)
                return object;
            var message = new $root.commands.CMD_Header();
            if (object.request_id != null)
                message.request_id = String(object.request_id);
            if (object.obj_name != null)
                message.obj_name = String(object.obj_name);
            if (object.cmd_name != null)
                message.cmd_name = String(object.cmd_name);
            return message;
        };

        /**
         * Creates a plain object from a CMD_Header message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.CMD_Header
         * @static
         * @param {commands.CMD_Header} message CMD_Header
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        CMD_Header.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.request_id = "";
                object.obj_name = "";
                object.cmd_name = "";
            }
            if (message.request_id != null && message.hasOwnProperty("request_id"))
                object.request_id = message.request_id;
            if (message.obj_name != null && message.hasOwnProperty("obj_name"))
                object.obj_name = message.obj_name;
            if (message.cmd_name != null && message.hasOwnProperty("cmd_name"))
                object.cmd_name = message.cmd_name;
            return object;
        };

        /**
         * Converts this CMD_Header to JSON.
         * @function toJSON
         * @memberof commands.CMD_Header
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        CMD_Header.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for CMD_Header
         * @function getTypeUrl
         * @memberof commands.CMD_Header
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        CMD_Header.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.CMD_Header";
        };

        return CMD_Header;
    })();

    commands.CMD_Payload = (function() {

        /**
         * Properties of a CMD_Payload.
         * @memberof commands
         * @interface ICMD_Payload
         * @property {commands.IAddTodoClass|null} [add_todo_field] CMD_Payload add_todo_field
         * @property {commands.IPingClass|null} [ping_field] CMD_Payload ping_field
         * @property {commands.IMockSimulation|null} [start_mock_simulation] CMD_Payload start_mock_simulation
         */

        /**
         * Constructs a new CMD_Payload.
         * @memberof commands
         * @classdesc Represents a CMD_Payload.
         * @implements ICMD_Payload
         * @constructor
         * @param {commands.ICMD_Payload=} [properties] Properties to set
         */
        function CMD_Payload(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * CMD_Payload add_todo_field.
         * @member {commands.IAddTodoClass|null|undefined} add_todo_field
         * @memberof commands.CMD_Payload
         * @instance
         */
        CMD_Payload.prototype.add_todo_field = null;

        /**
         * CMD_Payload ping_field.
         * @member {commands.IPingClass|null|undefined} ping_field
         * @memberof commands.CMD_Payload
         * @instance
         */
        CMD_Payload.prototype.ping_field = null;

        /**
         * CMD_Payload start_mock_simulation.
         * @member {commands.IMockSimulation|null|undefined} start_mock_simulation
         * @memberof commands.CMD_Payload
         * @instance
         */
        CMD_Payload.prototype.start_mock_simulation = null;

        // OneOf field names bound to virtual getters and setters
        var $oneOfFields;

        /**
         * CMD_Payload variant.
         * @member {"add_todo_field"|"ping_field"|"start_mock_simulation"|undefined} variant
         * @memberof commands.CMD_Payload
         * @instance
         */
        Object.defineProperty(CMD_Payload.prototype, "variant", {
            get: $util.oneOfGetter($oneOfFields = ["add_todo_field", "ping_field", "start_mock_simulation"]),
            set: $util.oneOfSetter($oneOfFields)
        });

        /**
         * Creates a new CMD_Payload instance using the specified properties.
         * @function create
         * @memberof commands.CMD_Payload
         * @static
         * @param {commands.ICMD_Payload=} [properties] Properties to set
         * @returns {commands.CMD_Payload} CMD_Payload instance
         */
        CMD_Payload.create = function create(properties) {
            return new CMD_Payload(properties);
        };

        /**
         * Encodes the specified CMD_Payload message. Does not implicitly {@link commands.CMD_Payload.verify|verify} messages.
         * @function encode
         * @memberof commands.CMD_Payload
         * @static
         * @param {commands.ICMD_Payload} message CMD_Payload message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Payload.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.add_todo_field != null && Object.hasOwnProperty.call(message, "add_todo_field"))
                $root.commands.AddTodoClass.encode(message.add_todo_field, writer.uint32(/* id 3, wireType 2 =*/26).fork()).ldelim();
            if (message.ping_field != null && Object.hasOwnProperty.call(message, "ping_field"))
                $root.commands.PingClass.encode(message.ping_field, writer.uint32(/* id 4, wireType 2 =*/34).fork()).ldelim();
            if (message.start_mock_simulation != null && Object.hasOwnProperty.call(message, "start_mock_simulation"))
                $root.commands.MockSimulation.encode(message.start_mock_simulation, writer.uint32(/* id 5, wireType 2 =*/42).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified CMD_Payload message, length delimited. Does not implicitly {@link commands.CMD_Payload.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.CMD_Payload
         * @static
         * @param {commands.ICMD_Payload} message CMD_Payload message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Payload.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a CMD_Payload message from the specified reader or buffer.
         * @function decode
         * @memberof commands.CMD_Payload
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.CMD_Payload} CMD_Payload
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Payload.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.CMD_Payload();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 3: {
                        message.add_todo_field = $root.commands.AddTodoClass.decode(reader, reader.uint32());
                        break;
                    }
                case 4: {
                        message.ping_field = $root.commands.PingClass.decode(reader, reader.uint32());
                        break;
                    }
                case 5: {
                        message.start_mock_simulation = $root.commands.MockSimulation.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a CMD_Payload message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.CMD_Payload
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.CMD_Payload} CMD_Payload
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Payload.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a CMD_Payload message.
         * @function verify
         * @memberof commands.CMD_Payload
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        CMD_Payload.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            var properties = {};
            if (message.add_todo_field != null && message.hasOwnProperty("add_todo_field")) {
                properties.variant = 1;
                {
                    var error = $root.commands.AddTodoClass.verify(message.add_todo_field);
                    if (error)
                        return "add_todo_field." + error;
                }
            }
            if (message.ping_field != null && message.hasOwnProperty("ping_field")) {
                if (properties.variant === 1)
                    return "variant: multiple values";
                properties.variant = 1;
                {
                    var error = $root.commands.PingClass.verify(message.ping_field);
                    if (error)
                        return "ping_field." + error;
                }
            }
            if (message.start_mock_simulation != null && message.hasOwnProperty("start_mock_simulation")) {
                if (properties.variant === 1)
                    return "variant: multiple values";
                properties.variant = 1;
                {
                    var error = $root.commands.MockSimulation.verify(message.start_mock_simulation);
                    if (error)
                        return "start_mock_simulation." + error;
                }
            }
            return null;
        };

        /**
         * Creates a CMD_Payload message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.CMD_Payload
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.CMD_Payload} CMD_Payload
         */
        CMD_Payload.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.CMD_Payload)
                return object;
            var message = new $root.commands.CMD_Payload();
            if (object.add_todo_field != null) {
                if (typeof object.add_todo_field !== "object")
                    throw TypeError(".commands.CMD_Payload.add_todo_field: object expected");
                message.add_todo_field = $root.commands.AddTodoClass.fromObject(object.add_todo_field);
            }
            if (object.ping_field != null) {
                if (typeof object.ping_field !== "object")
                    throw TypeError(".commands.CMD_Payload.ping_field: object expected");
                message.ping_field = $root.commands.PingClass.fromObject(object.ping_field);
            }
            if (object.start_mock_simulation != null) {
                if (typeof object.start_mock_simulation !== "object")
                    throw TypeError(".commands.CMD_Payload.start_mock_simulation: object expected");
                message.start_mock_simulation = $root.commands.MockSimulation.fromObject(object.start_mock_simulation);
            }
            return message;
        };

        /**
         * Creates a plain object from a CMD_Payload message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.CMD_Payload
         * @static
         * @param {commands.CMD_Payload} message CMD_Payload
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        CMD_Payload.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (message.add_todo_field != null && message.hasOwnProperty("add_todo_field")) {
                object.add_todo_field = $root.commands.AddTodoClass.toObject(message.add_todo_field, options);
                if (options.oneofs)
                    object.variant = "add_todo_field";
            }
            if (message.ping_field != null && message.hasOwnProperty("ping_field")) {
                object.ping_field = $root.commands.PingClass.toObject(message.ping_field, options);
                if (options.oneofs)
                    object.variant = "ping_field";
            }
            if (message.start_mock_simulation != null && message.hasOwnProperty("start_mock_simulation")) {
                object.start_mock_simulation = $root.commands.MockSimulation.toObject(message.start_mock_simulation, options);
                if (options.oneofs)
                    object.variant = "start_mock_simulation";
            }
            return object;
        };

        /**
         * Converts this CMD_Payload to JSON.
         * @function toJSON
         * @memberof commands.CMD_Payload
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        CMD_Payload.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for CMD_Payload
         * @function getTypeUrl
         * @memberof commands.CMD_Payload
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        CMD_Payload.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.CMD_Payload";
        };

        return CMD_Payload;
    })();

    commands.CMD_Holder = (function() {

        /**
         * Properties of a CMD_Holder.
         * @memberof commands
         * @interface ICMD_Holder
         * @property {commands.ICMD_Header|null} [header] CMD_Holder header
         * @property {google.protobuf.IAny|null} [payload] CMD_Holder payload
         */

        /**
         * Constructs a new CMD_Holder.
         * @memberof commands
         * @classdesc Represents a CMD_Holder.
         * @implements ICMD_Holder
         * @constructor
         * @param {commands.ICMD_Holder=} [properties] Properties to set
         */
        function CMD_Holder(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * CMD_Holder header.
         * @member {commands.ICMD_Header|null|undefined} header
         * @memberof commands.CMD_Holder
         * @instance
         */
        CMD_Holder.prototype.header = null;

        /**
         * CMD_Holder payload.
         * @member {google.protobuf.IAny|null|undefined} payload
         * @memberof commands.CMD_Holder
         * @instance
         */
        CMD_Holder.prototype.payload = null;

        /**
         * Creates a new CMD_Holder instance using the specified properties.
         * @function create
         * @memberof commands.CMD_Holder
         * @static
         * @param {commands.ICMD_Holder=} [properties] Properties to set
         * @returns {commands.CMD_Holder} CMD_Holder instance
         */
        CMD_Holder.create = function create(properties) {
            return new CMD_Holder(properties);
        };

        /**
         * Encodes the specified CMD_Holder message. Does not implicitly {@link commands.CMD_Holder.verify|verify} messages.
         * @function encode
         * @memberof commands.CMD_Holder
         * @static
         * @param {commands.ICMD_Holder} message CMD_Holder message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Holder.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.header != null && Object.hasOwnProperty.call(message, "header"))
                $root.commands.CMD_Header.encode(message.header, writer.uint32(/* id 1, wireType 2 =*/10).fork()).ldelim();
            if (message.payload != null && Object.hasOwnProperty.call(message, "payload"))
                $root.google.protobuf.Any.encode(message.payload, writer.uint32(/* id 2, wireType 2 =*/18).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified CMD_Holder message, length delimited. Does not implicitly {@link commands.CMD_Holder.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.CMD_Holder
         * @static
         * @param {commands.ICMD_Holder} message CMD_Holder message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Holder.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a CMD_Holder message from the specified reader or buffer.
         * @function decode
         * @memberof commands.CMD_Holder
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.CMD_Holder} CMD_Holder
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Holder.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.CMD_Holder();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.header = $root.commands.CMD_Header.decode(reader, reader.uint32());
                        break;
                    }
                case 2: {
                        message.payload = $root.google.protobuf.Any.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a CMD_Holder message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.CMD_Holder
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.CMD_Holder} CMD_Holder
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Holder.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a CMD_Holder message.
         * @function verify
         * @memberof commands.CMD_Holder
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        CMD_Holder.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.header != null && message.hasOwnProperty("header")) {
                var error = $root.commands.CMD_Header.verify(message.header);
                if (error)
                    return "header." + error;
            }
            if (message.payload != null && message.hasOwnProperty("payload")) {
                var error = $root.google.protobuf.Any.verify(message.payload);
                if (error)
                    return "payload." + error;
            }
            return null;
        };

        /**
         * Creates a CMD_Holder message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.CMD_Holder
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.CMD_Holder} CMD_Holder
         */
        CMD_Holder.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.CMD_Holder)
                return object;
            var message = new $root.commands.CMD_Holder();
            if (object.header != null) {
                if (typeof object.header !== "object")
                    throw TypeError(".commands.CMD_Holder.header: object expected");
                message.header = $root.commands.CMD_Header.fromObject(object.header);
            }
            if (object.payload != null) {
                if (typeof object.payload !== "object")
                    throw TypeError(".commands.CMD_Holder.payload: object expected");
                message.payload = $root.google.protobuf.Any.fromObject(object.payload);
            }
            return message;
        };

        /**
         * Creates a plain object from a CMD_Holder message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.CMD_Holder
         * @static
         * @param {commands.CMD_Holder} message CMD_Holder
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        CMD_Holder.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.header = null;
                object.payload = null;
            }
            if (message.header != null && message.hasOwnProperty("header"))
                object.header = $root.commands.CMD_Header.toObject(message.header, options);
            if (message.payload != null && message.hasOwnProperty("payload"))
                object.payload = $root.google.protobuf.Any.toObject(message.payload, options);
            return object;
        };

        /**
         * Converts this CMD_Holder to JSON.
         * @function toJSON
         * @memberof commands.CMD_Holder
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        CMD_Holder.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for CMD_Holder
         * @function getTypeUrl
         * @memberof commands.CMD_Holder
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        CMD_Holder.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.CMD_Holder";
        };

        return CMD_Holder;
    })();

    commands.CMD_Response = (function() {

        /**
         * Properties of a CMD_Response.
         * @memberof commands
         * @interface ICMD_Response
         * @property {string|null} [request_id] CMD_Response request_id
         * @property {boolean|null} [success] CMD_Response success
         * @property {string|null} [message] CMD_Response message
         */

        /**
         * Constructs a new CMD_Response.
         * @memberof commands
         * @classdesc Represents a CMD_Response.
         * @implements ICMD_Response
         * @constructor
         * @param {commands.ICMD_Response=} [properties] Properties to set
         */
        function CMD_Response(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * CMD_Response request_id.
         * @member {string} request_id
         * @memberof commands.CMD_Response
         * @instance
         */
        CMD_Response.prototype.request_id = "";

        /**
         * CMD_Response success.
         * @member {boolean} success
         * @memberof commands.CMD_Response
         * @instance
         */
        CMD_Response.prototype.success = false;

        /**
         * CMD_Response message.
         * @member {string} message
         * @memberof commands.CMD_Response
         * @instance
         */
        CMD_Response.prototype.message = "";

        /**
         * Creates a new CMD_Response instance using the specified properties.
         * @function create
         * @memberof commands.CMD_Response
         * @static
         * @param {commands.ICMD_Response=} [properties] Properties to set
         * @returns {commands.CMD_Response} CMD_Response instance
         */
        CMD_Response.create = function create(properties) {
            return new CMD_Response(properties);
        };

        /**
         * Encodes the specified CMD_Response message. Does not implicitly {@link commands.CMD_Response.verify|verify} messages.
         * @function encode
         * @memberof commands.CMD_Response
         * @static
         * @param {commands.ICMD_Response} message CMD_Response message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Response.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.request_id != null && Object.hasOwnProperty.call(message, "request_id"))
                writer.uint32(/* id 1, wireType 2 =*/10).string(message.request_id);
            if (message.success != null && Object.hasOwnProperty.call(message, "success"))
                writer.uint32(/* id 2, wireType 0 =*/16).bool(message.success);
            if (message.message != null && Object.hasOwnProperty.call(message, "message"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.message);
            return writer;
        };

        /**
         * Encodes the specified CMD_Response message, length delimited. Does not implicitly {@link commands.CMD_Response.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.CMD_Response
         * @static
         * @param {commands.ICMD_Response} message CMD_Response message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        CMD_Response.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a CMD_Response message from the specified reader or buffer.
         * @function decode
         * @memberof commands.CMD_Response
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.CMD_Response} CMD_Response
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Response.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.CMD_Response();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.request_id = reader.string();
                        break;
                    }
                case 2: {
                        message.success = reader.bool();
                        break;
                    }
                case 3: {
                        message.message = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a CMD_Response message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.CMD_Response
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.CMD_Response} CMD_Response
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        CMD_Response.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a CMD_Response message.
         * @function verify
         * @memberof commands.CMD_Response
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        CMD_Response.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.request_id != null && message.hasOwnProperty("request_id"))
                if (!$util.isString(message.request_id))
                    return "request_id: string expected";
            if (message.success != null && message.hasOwnProperty("success"))
                if (typeof message.success !== "boolean")
                    return "success: boolean expected";
            if (message.message != null && message.hasOwnProperty("message"))
                if (!$util.isString(message.message))
                    return "message: string expected";
            return null;
        };

        /**
         * Creates a CMD_Response message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.CMD_Response
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.CMD_Response} CMD_Response
         */
        CMD_Response.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.CMD_Response)
                return object;
            var message = new $root.commands.CMD_Response();
            if (object.request_id != null)
                message.request_id = String(object.request_id);
            if (object.success != null)
                message.success = Boolean(object.success);
            if (object.message != null)
                message.message = String(object.message);
            return message;
        };

        /**
         * Creates a plain object from a CMD_Response message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.CMD_Response
         * @static
         * @param {commands.CMD_Response} message CMD_Response
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        CMD_Response.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.request_id = "";
                object.success = false;
                object.message = "";
            }
            if (message.request_id != null && message.hasOwnProperty("request_id"))
                object.request_id = message.request_id;
            if (message.success != null && message.hasOwnProperty("success"))
                object.success = message.success;
            if (message.message != null && message.hasOwnProperty("message"))
                object.message = message.message;
            return object;
        };

        /**
         * Converts this CMD_Response to JSON.
         * @function toJSON
         * @memberof commands.CMD_Response
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        CMD_Response.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for CMD_Response
         * @function getTypeUrl
         * @memberof commands.CMD_Response
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        CMD_Response.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.CMD_Response";
        };

        return CMD_Response;
    })();

    commands.TLM_HK = (function() {

        /**
         * Properties of a TLM_HK.
         * @memberof commands
         * @interface ITLM_HK
         * @property {number|Long|null} [n_clients] TLM_HK n_clients
         */

        /**
         * Constructs a new TLM_HK.
         * @memberof commands
         * @classdesc Represents a TLM_HK.
         * @implements ITLM_HK
         * @constructor
         * @param {commands.ITLM_HK=} [properties] Properties to set
         */
        function TLM_HK(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * TLM_HK n_clients.
         * @member {number|Long} n_clients
         * @memberof commands.TLM_HK
         * @instance
         */
        TLM_HK.prototype.n_clients = $util.Long ? $util.Long.fromBits(0,0,true) : 0;

        /**
         * Creates a new TLM_HK instance using the specified properties.
         * @function create
         * @memberof commands.TLM_HK
         * @static
         * @param {commands.ITLM_HK=} [properties] Properties to set
         * @returns {commands.TLM_HK} TLM_HK instance
         */
        TLM_HK.create = function create(properties) {
            return new TLM_HK(properties);
        };

        /**
         * Encodes the specified TLM_HK message. Does not implicitly {@link commands.TLM_HK.verify|verify} messages.
         * @function encode
         * @memberof commands.TLM_HK
         * @static
         * @param {commands.ITLM_HK} message TLM_HK message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_HK.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.n_clients != null && Object.hasOwnProperty.call(message, "n_clients"))
                writer.uint32(/* id 1, wireType 0 =*/8).uint64(message.n_clients);
            return writer;
        };

        /**
         * Encodes the specified TLM_HK message, length delimited. Does not implicitly {@link commands.TLM_HK.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.TLM_HK
         * @static
         * @param {commands.ITLM_HK} message TLM_HK message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_HK.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a TLM_HK message from the specified reader or buffer.
         * @function decode
         * @memberof commands.TLM_HK
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.TLM_HK} TLM_HK
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_HK.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.TLM_HK();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.n_clients = reader.uint64();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a TLM_HK message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.TLM_HK
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.TLM_HK} TLM_HK
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_HK.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a TLM_HK message.
         * @function verify
         * @memberof commands.TLM_HK
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        TLM_HK.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.n_clients != null && message.hasOwnProperty("n_clients"))
                if (!$util.isInteger(message.n_clients) && !(message.n_clients && $util.isInteger(message.n_clients.low) && $util.isInteger(message.n_clients.high)))
                    return "n_clients: integer|Long expected";
            return null;
        };

        /**
         * Creates a TLM_HK message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.TLM_HK
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.TLM_HK} TLM_HK
         */
        TLM_HK.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.TLM_HK)
                return object;
            var message = new $root.commands.TLM_HK();
            if (object.n_clients != null)
                if ($util.Long)
                    (message.n_clients = $util.Long.fromValue(object.n_clients)).unsigned = true;
                else if (typeof object.n_clients === "string")
                    message.n_clients = parseInt(object.n_clients, 10);
                else if (typeof object.n_clients === "number")
                    message.n_clients = object.n_clients;
                else if (typeof object.n_clients === "object")
                    message.n_clients = new $util.LongBits(object.n_clients.low >>> 0, object.n_clients.high >>> 0).toNumber(true);
            return message;
        };

        /**
         * Creates a plain object from a TLM_HK message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.TLM_HK
         * @static
         * @param {commands.TLM_HK} message TLM_HK
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        TLM_HK.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults)
                if ($util.Long) {
                    var long = new $util.Long(0, 0, true);
                    object.n_clients = options.longs === String ? long.toString() : options.longs === Number ? long.toNumber() : long;
                } else
                    object.n_clients = options.longs === String ? "0" : 0;
            if (message.n_clients != null && message.hasOwnProperty("n_clients"))
                if (typeof message.n_clients === "number")
                    object.n_clients = options.longs === String ? String(message.n_clients) : message.n_clients;
                else
                    object.n_clients = options.longs === String ? $util.Long.prototype.toString.call(message.n_clients) : options.longs === Number ? new $util.LongBits(message.n_clients.low >>> 0, message.n_clients.high >>> 0).toNumber(true) : message.n_clients;
            return object;
        };

        /**
         * Converts this TLM_HK to JSON.
         * @function toJSON
         * @memberof commands.TLM_HK
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        TLM_HK.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for TLM_HK
         * @function getTypeUrl
         * @memberof commands.TLM_HK
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        TLM_HK.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.TLM_HK";
        };

        return TLM_HK;
    })();

    commands.TLM_Header = (function() {

        /**
         * Properties of a TLM_Header.
         * @memberof commands
         * @interface ITLM_Header
         * @property {number|Long|null} [time_ns] TLM_Header time_ns
         * @property {string|null} [topic_type] TLM_Header topic_type
         * @property {string|null} [topic_name] TLM_Header topic_name
         */

        /**
         * Constructs a new TLM_Header.
         * @memberof commands
         * @classdesc Represents a TLM_Header.
         * @implements ITLM_Header
         * @constructor
         * @param {commands.ITLM_Header=} [properties] Properties to set
         */
        function TLM_Header(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * TLM_Header time_ns.
         * @member {number|Long} time_ns
         * @memberof commands.TLM_Header
         * @instance
         */
        TLM_Header.prototype.time_ns = $util.Long ? $util.Long.fromBits(0,0,true) : 0;

        /**
         * TLM_Header topic_type.
         * @member {string} topic_type
         * @memberof commands.TLM_Header
         * @instance
         */
        TLM_Header.prototype.topic_type = "";

        /**
         * TLM_Header topic_name.
         * @member {string} topic_name
         * @memberof commands.TLM_Header
         * @instance
         */
        TLM_Header.prototype.topic_name = "";

        /**
         * Creates a new TLM_Header instance using the specified properties.
         * @function create
         * @memberof commands.TLM_Header
         * @static
         * @param {commands.ITLM_Header=} [properties] Properties to set
         * @returns {commands.TLM_Header} TLM_Header instance
         */
        TLM_Header.create = function create(properties) {
            return new TLM_Header(properties);
        };

        /**
         * Encodes the specified TLM_Header message. Does not implicitly {@link commands.TLM_Header.verify|verify} messages.
         * @function encode
         * @memberof commands.TLM_Header
         * @static
         * @param {commands.ITLM_Header} message TLM_Header message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Header.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.time_ns != null && Object.hasOwnProperty.call(message, "time_ns"))
                writer.uint32(/* id 1, wireType 0 =*/8).uint64(message.time_ns);
            if (message.topic_type != null && Object.hasOwnProperty.call(message, "topic_type"))
                writer.uint32(/* id 2, wireType 2 =*/18).string(message.topic_type);
            if (message.topic_name != null && Object.hasOwnProperty.call(message, "topic_name"))
                writer.uint32(/* id 3, wireType 2 =*/26).string(message.topic_name);
            return writer;
        };

        /**
         * Encodes the specified TLM_Header message, length delimited. Does not implicitly {@link commands.TLM_Header.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.TLM_Header
         * @static
         * @param {commands.ITLM_Header} message TLM_Header message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Header.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a TLM_Header message from the specified reader or buffer.
         * @function decode
         * @memberof commands.TLM_Header
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.TLM_Header} TLM_Header
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Header.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.TLM_Header();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.time_ns = reader.uint64();
                        break;
                    }
                case 2: {
                        message.topic_type = reader.string();
                        break;
                    }
                case 3: {
                        message.topic_name = reader.string();
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a TLM_Header message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.TLM_Header
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.TLM_Header} TLM_Header
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Header.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a TLM_Header message.
         * @function verify
         * @memberof commands.TLM_Header
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        TLM_Header.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.time_ns != null && message.hasOwnProperty("time_ns"))
                if (!$util.isInteger(message.time_ns) && !(message.time_ns && $util.isInteger(message.time_ns.low) && $util.isInteger(message.time_ns.high)))
                    return "time_ns: integer|Long expected";
            if (message.topic_type != null && message.hasOwnProperty("topic_type"))
                if (!$util.isString(message.topic_type))
                    return "topic_type: string expected";
            if (message.topic_name != null && message.hasOwnProperty("topic_name"))
                if (!$util.isString(message.topic_name))
                    return "topic_name: string expected";
            return null;
        };

        /**
         * Creates a TLM_Header message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.TLM_Header
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.TLM_Header} TLM_Header
         */
        TLM_Header.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.TLM_Header)
                return object;
            var message = new $root.commands.TLM_Header();
            if (object.time_ns != null)
                if ($util.Long)
                    (message.time_ns = $util.Long.fromValue(object.time_ns)).unsigned = true;
                else if (typeof object.time_ns === "string")
                    message.time_ns = parseInt(object.time_ns, 10);
                else if (typeof object.time_ns === "number")
                    message.time_ns = object.time_ns;
                else if (typeof object.time_ns === "object")
                    message.time_ns = new $util.LongBits(object.time_ns.low >>> 0, object.time_ns.high >>> 0).toNumber(true);
            if (object.topic_type != null)
                message.topic_type = String(object.topic_type);
            if (object.topic_name != null)
                message.topic_name = String(object.topic_name);
            return message;
        };

        /**
         * Creates a plain object from a TLM_Header message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.TLM_Header
         * @static
         * @param {commands.TLM_Header} message TLM_Header
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        TLM_Header.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                if ($util.Long) {
                    var long = new $util.Long(0, 0, true);
                    object.time_ns = options.longs === String ? long.toString() : options.longs === Number ? long.toNumber() : long;
                } else
                    object.time_ns = options.longs === String ? "0" : 0;
                object.topic_type = "";
                object.topic_name = "";
            }
            if (message.time_ns != null && message.hasOwnProperty("time_ns"))
                if (typeof message.time_ns === "number")
                    object.time_ns = options.longs === String ? String(message.time_ns) : message.time_ns;
                else
                    object.time_ns = options.longs === String ? $util.Long.prototype.toString.call(message.time_ns) : options.longs === Number ? new $util.LongBits(message.time_ns.low >>> 0, message.time_ns.high >>> 0).toNumber(true) : message.time_ns;
            if (message.topic_type != null && message.hasOwnProperty("topic_type"))
                object.topic_type = message.topic_type;
            if (message.topic_name != null && message.hasOwnProperty("topic_name"))
                object.topic_name = message.topic_name;
            return object;
        };

        /**
         * Converts this TLM_Header to JSON.
         * @function toJSON
         * @memberof commands.TLM_Header
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        TLM_Header.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for TLM_Header
         * @function getTypeUrl
         * @memberof commands.TLM_Header
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        TLM_Header.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.TLM_Header";
        };

        return TLM_Header;
    })();

    commands.TLM_Payload = (function() {

        /**
         * Properties of a TLM_Payload.
         * @memberof commands
         * @interface ITLM_Payload
         * @property {commands.ICMD_Response|null} [cmd_response_payload] TLM_Payload cmd_response_payload
         * @property {commands.ITLM_HK|null} [hk_payload] TLM_Payload hk_payload
         * @property {commands.IFullStateEstimate|null} [full_state_estimate] TLM_Payload full_state_estimate
         */

        /**
         * Constructs a new TLM_Payload.
         * @memberof commands
         * @classdesc Represents a TLM_Payload.
         * @implements ITLM_Payload
         * @constructor
         * @param {commands.ITLM_Payload=} [properties] Properties to set
         */
        function TLM_Payload(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * TLM_Payload cmd_response_payload.
         * @member {commands.ICMD_Response|null|undefined} cmd_response_payload
         * @memberof commands.TLM_Payload
         * @instance
         */
        TLM_Payload.prototype.cmd_response_payload = null;

        /**
         * TLM_Payload hk_payload.
         * @member {commands.ITLM_HK|null|undefined} hk_payload
         * @memberof commands.TLM_Payload
         * @instance
         */
        TLM_Payload.prototype.hk_payload = null;

        /**
         * TLM_Payload full_state_estimate.
         * @member {commands.IFullStateEstimate|null|undefined} full_state_estimate
         * @memberof commands.TLM_Payload
         * @instance
         */
        TLM_Payload.prototype.full_state_estimate = null;

        // OneOf field names bound to virtual getters and setters
        var $oneOfFields;

        /**
         * TLM_Payload variant.
         * @member {"cmd_response_payload"|"hk_payload"|"full_state_estimate"|undefined} variant
         * @memberof commands.TLM_Payload
         * @instance
         */
        Object.defineProperty(TLM_Payload.prototype, "variant", {
            get: $util.oneOfGetter($oneOfFields = ["cmd_response_payload", "hk_payload", "full_state_estimate"]),
            set: $util.oneOfSetter($oneOfFields)
        });

        /**
         * Creates a new TLM_Payload instance using the specified properties.
         * @function create
         * @memberof commands.TLM_Payload
         * @static
         * @param {commands.ITLM_Payload=} [properties] Properties to set
         * @returns {commands.TLM_Payload} TLM_Payload instance
         */
        TLM_Payload.create = function create(properties) {
            return new TLM_Payload(properties);
        };

        /**
         * Encodes the specified TLM_Payload message. Does not implicitly {@link commands.TLM_Payload.verify|verify} messages.
         * @function encode
         * @memberof commands.TLM_Payload
         * @static
         * @param {commands.ITLM_Payload} message TLM_Payload message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Payload.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.cmd_response_payload != null && Object.hasOwnProperty.call(message, "cmd_response_payload"))
                $root.commands.CMD_Response.encode(message.cmd_response_payload, writer.uint32(/* id 1, wireType 2 =*/10).fork()).ldelim();
            if (message.hk_payload != null && Object.hasOwnProperty.call(message, "hk_payload"))
                $root.commands.TLM_HK.encode(message.hk_payload, writer.uint32(/* id 2, wireType 2 =*/18).fork()).ldelim();
            if (message.full_state_estimate != null && Object.hasOwnProperty.call(message, "full_state_estimate"))
                $root.commands.FullStateEstimate.encode(message.full_state_estimate, writer.uint32(/* id 3, wireType 2 =*/26).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified TLM_Payload message, length delimited. Does not implicitly {@link commands.TLM_Payload.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.TLM_Payload
         * @static
         * @param {commands.ITLM_Payload} message TLM_Payload message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Payload.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a TLM_Payload message from the specified reader or buffer.
         * @function decode
         * @memberof commands.TLM_Payload
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.TLM_Payload} TLM_Payload
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Payload.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.TLM_Payload();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.cmd_response_payload = $root.commands.CMD_Response.decode(reader, reader.uint32());
                        break;
                    }
                case 2: {
                        message.hk_payload = $root.commands.TLM_HK.decode(reader, reader.uint32());
                        break;
                    }
                case 3: {
                        message.full_state_estimate = $root.commands.FullStateEstimate.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a TLM_Payload message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.TLM_Payload
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.TLM_Payload} TLM_Payload
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Payload.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a TLM_Payload message.
         * @function verify
         * @memberof commands.TLM_Payload
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        TLM_Payload.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            var properties = {};
            if (message.cmd_response_payload != null && message.hasOwnProperty("cmd_response_payload")) {
                properties.variant = 1;
                {
                    var error = $root.commands.CMD_Response.verify(message.cmd_response_payload);
                    if (error)
                        return "cmd_response_payload." + error;
                }
            }
            if (message.hk_payload != null && message.hasOwnProperty("hk_payload")) {
                if (properties.variant === 1)
                    return "variant: multiple values";
                properties.variant = 1;
                {
                    var error = $root.commands.TLM_HK.verify(message.hk_payload);
                    if (error)
                        return "hk_payload." + error;
                }
            }
            if (message.full_state_estimate != null && message.hasOwnProperty("full_state_estimate")) {
                if (properties.variant === 1)
                    return "variant: multiple values";
                properties.variant = 1;
                {
                    var error = $root.commands.FullStateEstimate.verify(message.full_state_estimate);
                    if (error)
                        return "full_state_estimate." + error;
                }
            }
            return null;
        };

        /**
         * Creates a TLM_Payload message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.TLM_Payload
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.TLM_Payload} TLM_Payload
         */
        TLM_Payload.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.TLM_Payload)
                return object;
            var message = new $root.commands.TLM_Payload();
            if (object.cmd_response_payload != null) {
                if (typeof object.cmd_response_payload !== "object")
                    throw TypeError(".commands.TLM_Payload.cmd_response_payload: object expected");
                message.cmd_response_payload = $root.commands.CMD_Response.fromObject(object.cmd_response_payload);
            }
            if (object.hk_payload != null) {
                if (typeof object.hk_payload !== "object")
                    throw TypeError(".commands.TLM_Payload.hk_payload: object expected");
                message.hk_payload = $root.commands.TLM_HK.fromObject(object.hk_payload);
            }
            if (object.full_state_estimate != null) {
                if (typeof object.full_state_estimate !== "object")
                    throw TypeError(".commands.TLM_Payload.full_state_estimate: object expected");
                message.full_state_estimate = $root.commands.FullStateEstimate.fromObject(object.full_state_estimate);
            }
            return message;
        };

        /**
         * Creates a plain object from a TLM_Payload message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.TLM_Payload
         * @static
         * @param {commands.TLM_Payload} message TLM_Payload
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        TLM_Payload.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (message.cmd_response_payload != null && message.hasOwnProperty("cmd_response_payload")) {
                object.cmd_response_payload = $root.commands.CMD_Response.toObject(message.cmd_response_payload, options);
                if (options.oneofs)
                    object.variant = "cmd_response_payload";
            }
            if (message.hk_payload != null && message.hasOwnProperty("hk_payload")) {
                object.hk_payload = $root.commands.TLM_HK.toObject(message.hk_payload, options);
                if (options.oneofs)
                    object.variant = "hk_payload";
            }
            if (message.full_state_estimate != null && message.hasOwnProperty("full_state_estimate")) {
                object.full_state_estimate = $root.commands.FullStateEstimate.toObject(message.full_state_estimate, options);
                if (options.oneofs)
                    object.variant = "full_state_estimate";
            }
            return object;
        };

        /**
         * Converts this TLM_Payload to JSON.
         * @function toJSON
         * @memberof commands.TLM_Payload
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        TLM_Payload.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for TLM_Payload
         * @function getTypeUrl
         * @memberof commands.TLM_Payload
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        TLM_Payload.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.TLM_Payload";
        };

        return TLM_Payload;
    })();

    commands.TLM_Holder = (function() {

        /**
         * Properties of a TLM_Holder.
         * @memberof commands
         * @interface ITLM_Holder
         * @property {commands.ITLM_Header|null} [header] TLM_Holder header
         * @property {google.protobuf.IAny|null} [payload] TLM_Holder payload
         */

        /**
         * Constructs a new TLM_Holder.
         * @memberof commands
         * @classdesc Represents a TLM_Holder.
         * @implements ITLM_Holder
         * @constructor
         * @param {commands.ITLM_Holder=} [properties] Properties to set
         */
        function TLM_Holder(properties) {
            if (properties)
                for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                    if (properties[keys[i]] != null)
                        this[keys[i]] = properties[keys[i]];
        }

        /**
         * TLM_Holder header.
         * @member {commands.ITLM_Header|null|undefined} header
         * @memberof commands.TLM_Holder
         * @instance
         */
        TLM_Holder.prototype.header = null;

        /**
         * TLM_Holder payload.
         * @member {google.protobuf.IAny|null|undefined} payload
         * @memberof commands.TLM_Holder
         * @instance
         */
        TLM_Holder.prototype.payload = null;

        /**
         * Creates a new TLM_Holder instance using the specified properties.
         * @function create
         * @memberof commands.TLM_Holder
         * @static
         * @param {commands.ITLM_Holder=} [properties] Properties to set
         * @returns {commands.TLM_Holder} TLM_Holder instance
         */
        TLM_Holder.create = function create(properties) {
            return new TLM_Holder(properties);
        };

        /**
         * Encodes the specified TLM_Holder message. Does not implicitly {@link commands.TLM_Holder.verify|verify} messages.
         * @function encode
         * @memberof commands.TLM_Holder
         * @static
         * @param {commands.ITLM_Holder} message TLM_Holder message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Holder.encode = function encode(message, writer) {
            if (!writer)
                writer = $Writer.create();
            if (message.header != null && Object.hasOwnProperty.call(message, "header"))
                $root.commands.TLM_Header.encode(message.header, writer.uint32(/* id 1, wireType 2 =*/10).fork()).ldelim();
            if (message.payload != null && Object.hasOwnProperty.call(message, "payload"))
                $root.google.protobuf.Any.encode(message.payload, writer.uint32(/* id 2, wireType 2 =*/18).fork()).ldelim();
            return writer;
        };

        /**
         * Encodes the specified TLM_Holder message, length delimited. Does not implicitly {@link commands.TLM_Holder.verify|verify} messages.
         * @function encodeDelimited
         * @memberof commands.TLM_Holder
         * @static
         * @param {commands.ITLM_Holder} message TLM_Holder message or plain object to encode
         * @param {$protobuf.Writer} [writer] Writer to encode to
         * @returns {$protobuf.Writer} Writer
         */
        TLM_Holder.encodeDelimited = function encodeDelimited(message, writer) {
            return this.encode(message, writer).ldelim();
        };

        /**
         * Decodes a TLM_Holder message from the specified reader or buffer.
         * @function decode
         * @memberof commands.TLM_Holder
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @param {number} [length] Message length if known beforehand
         * @returns {commands.TLM_Holder} TLM_Holder
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Holder.decode = function decode(reader, length, error) {
            if (!(reader instanceof $Reader))
                reader = $Reader.create(reader);
            var end = length === undefined ? reader.len : reader.pos + length, message = new $root.commands.TLM_Holder();
            while (reader.pos < end) {
                var tag = reader.uint32();
                if (tag === error)
                    break;
                switch (tag >>> 3) {
                case 1: {
                        message.header = $root.commands.TLM_Header.decode(reader, reader.uint32());
                        break;
                    }
                case 2: {
                        message.payload = $root.google.protobuf.Any.decode(reader, reader.uint32());
                        break;
                    }
                default:
                    reader.skipType(tag & 7);
                    break;
                }
            }
            return message;
        };

        /**
         * Decodes a TLM_Holder message from the specified reader or buffer, length delimited.
         * @function decodeDelimited
         * @memberof commands.TLM_Holder
         * @static
         * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
         * @returns {commands.TLM_Holder} TLM_Holder
         * @throws {Error} If the payload is not a reader or valid buffer
         * @throws {$protobuf.util.ProtocolError} If required fields are missing
         */
        TLM_Holder.decodeDelimited = function decodeDelimited(reader) {
            if (!(reader instanceof $Reader))
                reader = new $Reader(reader);
            return this.decode(reader, reader.uint32());
        };

        /**
         * Verifies a TLM_Holder message.
         * @function verify
         * @memberof commands.TLM_Holder
         * @static
         * @param {Object.<string,*>} message Plain object to verify
         * @returns {string|null} `null` if valid, otherwise the reason why it is not
         */
        TLM_Holder.verify = function verify(message) {
            if (typeof message !== "object" || message === null)
                return "object expected";
            if (message.header != null && message.hasOwnProperty("header")) {
                var error = $root.commands.TLM_Header.verify(message.header);
                if (error)
                    return "header." + error;
            }
            if (message.payload != null && message.hasOwnProperty("payload")) {
                var error = $root.google.protobuf.Any.verify(message.payload);
                if (error)
                    return "payload." + error;
            }
            return null;
        };

        /**
         * Creates a TLM_Holder message from a plain object. Also converts values to their respective internal types.
         * @function fromObject
         * @memberof commands.TLM_Holder
         * @static
         * @param {Object.<string,*>} object Plain object
         * @returns {commands.TLM_Holder} TLM_Holder
         */
        TLM_Holder.fromObject = function fromObject(object) {
            if (object instanceof $root.commands.TLM_Holder)
                return object;
            var message = new $root.commands.TLM_Holder();
            if (object.header != null) {
                if (typeof object.header !== "object")
                    throw TypeError(".commands.TLM_Holder.header: object expected");
                message.header = $root.commands.TLM_Header.fromObject(object.header);
            }
            if (object.payload != null) {
                if (typeof object.payload !== "object")
                    throw TypeError(".commands.TLM_Holder.payload: object expected");
                message.payload = $root.google.protobuf.Any.fromObject(object.payload);
            }
            return message;
        };

        /**
         * Creates a plain object from a TLM_Holder message. Also converts values to other types if specified.
         * @function toObject
         * @memberof commands.TLM_Holder
         * @static
         * @param {commands.TLM_Holder} message TLM_Holder
         * @param {$protobuf.IConversionOptions} [options] Conversion options
         * @returns {Object.<string,*>} Plain object
         */
        TLM_Holder.toObject = function toObject(message, options) {
            if (!options)
                options = {};
            var object = {};
            if (options.defaults) {
                object.header = null;
                object.payload = null;
            }
            if (message.header != null && message.hasOwnProperty("header"))
                object.header = $root.commands.TLM_Header.toObject(message.header, options);
            if (message.payload != null && message.hasOwnProperty("payload"))
                object.payload = $root.google.protobuf.Any.toObject(message.payload, options);
            return object;
        };

        /**
         * Converts this TLM_Holder to JSON.
         * @function toJSON
         * @memberof commands.TLM_Holder
         * @instance
         * @returns {Object.<string,*>} JSON object
         */
        TLM_Holder.prototype.toJSON = function toJSON() {
            return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
        };

        /**
         * Gets the default type url for TLM_Holder
         * @function getTypeUrl
         * @memberof commands.TLM_Holder
         * @static
         * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
         * @returns {string} The default type url
         */
        TLM_Holder.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
            if (typeUrlPrefix === undefined) {
                typeUrlPrefix = "type.googleapis.com";
            }
            return typeUrlPrefix + "/commands.TLM_Holder";
        };

        return TLM_Holder;
    })();

    return commands;
})();

$root.google = (function() {

    /**
     * Namespace google.
     * @exports google
     * @namespace
     */
    var google = {};

    google.protobuf = (function() {

        /**
         * Namespace protobuf.
         * @memberof google
         * @namespace
         */
        var protobuf = {};

        protobuf.Any = (function() {

            /**
             * Properties of an Any.
             * @memberof google.protobuf
             * @interface IAny
             * @property {string|null} [type_url] Any type_url
             * @property {Uint8Array|null} [value] Any value
             */

            /**
             * Constructs a new Any.
             * @memberof google.protobuf
             * @classdesc Represents an Any.
             * @implements IAny
             * @constructor
             * @param {google.protobuf.IAny=} [properties] Properties to set
             */
            function Any(properties) {
                if (properties)
                    for (var keys = Object.keys(properties), i = 0; i < keys.length; ++i)
                        if (properties[keys[i]] != null)
                            this[keys[i]] = properties[keys[i]];
            }

            /**
             * Any type_url.
             * @member {string} type_url
             * @memberof google.protobuf.Any
             * @instance
             */
            Any.prototype.type_url = "";

            /**
             * Any value.
             * @member {Uint8Array} value
             * @memberof google.protobuf.Any
             * @instance
             */
            Any.prototype.value = $util.newBuffer([]);

            /**
             * Creates a new Any instance using the specified properties.
             * @function create
             * @memberof google.protobuf.Any
             * @static
             * @param {google.protobuf.IAny=} [properties] Properties to set
             * @returns {google.protobuf.Any} Any instance
             */
            Any.create = function create(properties) {
                return new Any(properties);
            };

            /**
             * Encodes the specified Any message. Does not implicitly {@link google.protobuf.Any.verify|verify} messages.
             * @function encode
             * @memberof google.protobuf.Any
             * @static
             * @param {google.protobuf.IAny} message Any message or plain object to encode
             * @param {$protobuf.Writer} [writer] Writer to encode to
             * @returns {$protobuf.Writer} Writer
             */
            Any.encode = function encode(message, writer) {
                if (!writer)
                    writer = $Writer.create();
                if (message.type_url != null && Object.hasOwnProperty.call(message, "type_url"))
                    writer.uint32(/* id 1, wireType 2 =*/10).string(message.type_url);
                if (message.value != null && Object.hasOwnProperty.call(message, "value"))
                    writer.uint32(/* id 2, wireType 2 =*/18).bytes(message.value);
                return writer;
            };

            /**
             * Encodes the specified Any message, length delimited. Does not implicitly {@link google.protobuf.Any.verify|verify} messages.
             * @function encodeDelimited
             * @memberof google.protobuf.Any
             * @static
             * @param {google.protobuf.IAny} message Any message or plain object to encode
             * @param {$protobuf.Writer} [writer] Writer to encode to
             * @returns {$protobuf.Writer} Writer
             */
            Any.encodeDelimited = function encodeDelimited(message, writer) {
                return this.encode(message, writer).ldelim();
            };

            /**
             * Decodes an Any message from the specified reader or buffer.
             * @function decode
             * @memberof google.protobuf.Any
             * @static
             * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
             * @param {number} [length] Message length if known beforehand
             * @returns {google.protobuf.Any} Any
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            Any.decode = function decode(reader, length, error) {
                if (!(reader instanceof $Reader))
                    reader = $Reader.create(reader);
                var end = length === undefined ? reader.len : reader.pos + length, message = new $root.google.protobuf.Any();
                while (reader.pos < end) {
                    var tag = reader.uint32();
                    if (tag === error)
                        break;
                    switch (tag >>> 3) {
                    case 1: {
                            message.type_url = reader.string();
                            break;
                        }
                    case 2: {
                            message.value = reader.bytes();
                            break;
                        }
                    default:
                        reader.skipType(tag & 7);
                        break;
                    }
                }
                return message;
            };

            /**
             * Decodes an Any message from the specified reader or buffer, length delimited.
             * @function decodeDelimited
             * @memberof google.protobuf.Any
             * @static
             * @param {$protobuf.Reader|Uint8Array} reader Reader or buffer to decode from
             * @returns {google.protobuf.Any} Any
             * @throws {Error} If the payload is not a reader or valid buffer
             * @throws {$protobuf.util.ProtocolError} If required fields are missing
             */
            Any.decodeDelimited = function decodeDelimited(reader) {
                if (!(reader instanceof $Reader))
                    reader = new $Reader(reader);
                return this.decode(reader, reader.uint32());
            };

            /**
             * Verifies an Any message.
             * @function verify
             * @memberof google.protobuf.Any
             * @static
             * @param {Object.<string,*>} message Plain object to verify
             * @returns {string|null} `null` if valid, otherwise the reason why it is not
             */
            Any.verify = function verify(message) {
                if (typeof message !== "object" || message === null)
                    return "object expected";
                if (message.type_url != null && message.hasOwnProperty("type_url"))
                    if (!$util.isString(message.type_url))
                        return "type_url: string expected";
                if (message.value != null && message.hasOwnProperty("value"))
                    if (!(message.value && typeof message.value.length === "number" || $util.isString(message.value)))
                        return "value: buffer expected";
                return null;
            };

            /**
             * Creates an Any message from a plain object. Also converts values to their respective internal types.
             * @function fromObject
             * @memberof google.protobuf.Any
             * @static
             * @param {Object.<string,*>} object Plain object
             * @returns {google.protobuf.Any} Any
             */
            Any.fromObject = function fromObject(object) {
                if (object instanceof $root.google.protobuf.Any)
                    return object;
                var message = new $root.google.protobuf.Any();
                if (object.type_url != null)
                    message.type_url = String(object.type_url);
                if (object.value != null)
                    if (typeof object.value === "string")
                        $util.base64.decode(object.value, message.value = $util.newBuffer($util.base64.length(object.value)), 0);
                    else if (object.value.length >= 0)
                        message.value = object.value;
                return message;
            };

            /**
             * Creates a plain object from an Any message. Also converts values to other types if specified.
             * @function toObject
             * @memberof google.protobuf.Any
             * @static
             * @param {google.protobuf.Any} message Any
             * @param {$protobuf.IConversionOptions} [options] Conversion options
             * @returns {Object.<string,*>} Plain object
             */
            Any.toObject = function toObject(message, options) {
                if (!options)
                    options = {};
                var object = {};
                if (options.defaults) {
                    object.type_url = "";
                    if (options.bytes === String)
                        object.value = "";
                    else {
                        object.value = [];
                        if (options.bytes !== Array)
                            object.value = $util.newBuffer(object.value);
                    }
                }
                if (message.type_url != null && message.hasOwnProperty("type_url"))
                    object.type_url = message.type_url;
                if (message.value != null && message.hasOwnProperty("value"))
                    object.value = options.bytes === String ? $util.base64.encode(message.value, 0, message.value.length) : options.bytes === Array ? Array.prototype.slice.call(message.value) : message.value;
                return object;
            };

            /**
             * Converts this Any to JSON.
             * @function toJSON
             * @memberof google.protobuf.Any
             * @instance
             * @returns {Object.<string,*>} JSON object
             */
            Any.prototype.toJSON = function toJSON() {
                return this.constructor.toObject(this, $protobuf.util.toJSONOptions);
            };

            /**
             * Gets the default type url for Any
             * @function getTypeUrl
             * @memberof google.protobuf.Any
             * @static
             * @param {string} [typeUrlPrefix] your custom typeUrlPrefix(default "type.googleapis.com")
             * @returns {string} The default type url
             */
            Any.getTypeUrl = function getTypeUrl(typeUrlPrefix) {
                if (typeUrlPrefix === undefined) {
                    typeUrlPrefix = "type.googleapis.com";
                }
                return typeUrlPrefix + "/google.protobuf.Any";
            };

            return Any;
        })();

        return protobuf;
    })();

    return google;
})();

module.exports = $root;
