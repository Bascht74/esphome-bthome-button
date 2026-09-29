import esphome.codegen as cg
from esphome.components import binary_sensor, ble_device_base
import esphome.config_validation as cv
from esphome.const import CONF_MAC_ADDRESS

from . import BUTTON_EVENTS, CONF_EVENT, CONF_INDEX

AUTO_LOAD = ["ble_device_base"]

CONF_PULSE_LENGTH = "pulse_length"
CONF_TYPE = "type"

bthome_ns = cg.esphome_ns.namespace("bthome")
BTHomeButtonBinarySensor = bthome_ns.class_(
    "BTHomeButtonBinarySensor",
    binary_sensor.BinarySensor,
    cg.Component,
    ble_device_base.ESPBTDeviceListener,
)

CONFIG_SCHEMA = cv.All(
    ble_device_base.rename_legacy_hub_id("bthome"),
    binary_sensor.binary_sensor_schema(BTHomeButtonBinarySensor)
    .extend(
        {
            cv.Required(CONF_MAC_ADDRESS): cv.mac_address,
            cv.Required(CONF_TYPE): cv.one_of("button", lower=True),
            cv.Required(CONF_EVENT): cv.enum(BUTTON_EVENTS, lower=True),
            cv.Optional(CONF_INDEX, default=1): cv.int_range(min=1, max=8),
            cv.Optional(CONF_PULSE_LENGTH, default="200ms"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ble_device_base.BLE_DEVICE_SCHEMA),
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    await ble_device_base.register_ble_device(var, config)
    cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))
    cg.add(var.set_index(config[CONF_INDEX]))
    cg.add(var.set_event(config[CONF_EVENT]))
    cg.add(var.set_pulse_length(config[CONF_PULSE_LENGTH]))
