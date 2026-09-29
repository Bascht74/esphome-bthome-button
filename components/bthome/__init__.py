import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID
from esphome.core import CORE

CODEOWNERS = ["@Bascht74"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["esp32_ble"]

CONF_BURST_DURATION = "burst_duration"
CONF_MANUFACTURER_ID = "manufacturer_id"
CONF_MANUFACTURER_DATA = "manufacturer_data"
CONF_EVENT = "event"
CONF_INDEX = "index"

bthome_ns = cg.esphome_ns.namespace("bthome")
BTHome = bthome_ns.class_("BTHome", cg.Component)
TransmitAction = bthome_ns.class_("TransmitAction", automation.Action)

BUTTON_EVENTS = {
    "press": 0x01,
    "double_press": 0x02,
    "triple_press": 0x03,
    "long_press": 0x04,
    "long_double_press": 0x05,
    "long_triple_press": 0x06,
    "hold": 0x80,
}


def _hex_bytes(value):
    if isinstance(value, list):
        return value
    text = str(value).replace(" ", "").replace(":", "").replace("-", "")
    if text == "" or len(text) % 2 or re.fullmatch(r"[0-9a-fA-F]+", text) is None:
        raise cv.Invalid("expected an even number of hex digits")
    return [int(text[i : i + 2], 16) for i in range(0, len(text), 2)]


def _fnv1a(text):
    hash_value = 2166136261
    for char in text:
        hash_value ^= ord(char)
        hash_value = (hash_value * 16777619) & 0xFFFFFFFF
    return hash_value


def _validate(config):
    has_id = CONF_MANUFACTURER_ID in config
    has_data = CONF_MANUFACTURER_DATA in config
    if has_id != has_data:
        raise cv.Invalid(f"{CONF_MANUFACTURER_ID} and {CONF_MANUFACTURER_DATA} must both be set, or neither")
    if has_data and len(config[CONF_MANUFACTURER_DATA]) > 15:
        raise cv.Invalid(f"{CONF_MANUFACTURER_DATA} is longer than 15 bytes")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BTHome),
            cv.Optional(CONF_BURST_DURATION, default="1500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MANUFACTURER_ID): cv.uint16_t,
            cv.Optional(CONF_MANUFACTURER_DATA): _hex_bytes,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_burst_duration(config[CONF_BURST_DURATION]))
    cg.add(var.set_pref_hash(_fnv1a(str(config[CONF_ID]))))
    if CONF_MANUFACTURER_ID in config:
        company = config[CONF_MANUFACTURER_ID]
        payload = [company & 0xFF, (company >> 8) & 0xFF]
        payload.extend(config[CONF_MANUFACTURER_DATA])
        cg.add(var.set_manufacturer_data(payload))
    if CORE.is_esp32:
        cg.add_define("USE_ESP32_BLE_ADVERTISING")


@automation.register_action(
    "bthome.transmit",
    TransmitAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(BTHome),
            cv.Required(CONF_EVENT): cv.enum(BUTTON_EVENTS, lower=True),
            cv.Optional(CONF_INDEX, default=1): cv.int_range(min=1, max=8),
        }
    ),
)
async def bthome_transmit_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_event(config[CONF_EVENT]))
    cg.add(var.set_index(config[CONF_INDEX]))
    return var
