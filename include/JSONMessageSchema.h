#pragma once

const char* configMessageSchema = R"({
  "type": "object",
  "properties": {
    "messageType": {
      "type": "string",
      "description": "Type of config message being sent."
    },
    "port": {
      "type": "integer",
      "description": "Port to communicate with",
      "minimum": 1,
      "maximum": 65535
    },
    "isDynamic": {
      "type": "boolean",
      "description": "Whether the offset pointers are dynamic or not"
    },
    "nickname": {
      "type": "string",
      "description": "Nickname for this data. Sent back with the data."
    },
    "offsets": {
      "type": "array",
      "description": "Array of memory offsets to follow when resolving this address",
      "minItems": 1,
      "items": {
        "type": "integer"
      }
    },
    "dataType": {
      "type": "string",
      "description": "The data-type/size of each value."
    },
    "dataCount": {
      "type": "integer",
      "description": "The number of values to read",
      "minimum": 0
    },
    "dataArray": {
      "type": "array",
      "description": "Array of values to be placed (poked) into memory",
      "minItems": 0,
      "items": {
        "type": "number"
      }
    },
    "nsInterval": {
      "type": "integer",
      "description": "How often to read and send this block of memory",
      "minimum": 0
    }
  },
  "required": [
    "messageType",
    "port"
  ]
})";
